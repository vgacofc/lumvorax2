#!/usr/bin/env python3
"""
Outil d'Analyse Forensique .lum — Phase 15X
Analyse le fichier .lum pour comprendre errno=5 au dispatch 4

Usage:
    python3 analyze_lum_forensic.py <fichier.lum>

Auteur: LumVorax Team
Date: 2026-05-11
"""

import sys
import struct
from dataclasses import dataclass
from typing import List, Optional
from enum import IntEnum

# ============================================================================
# CONSTANTES FORMAT .LUM
# ============================================================================

LUM_MAGIC = 0x4C554D00  # "LUM\0"
LUM_VERSION = 1
LUM_HEADER_SIZE = 64

class EventType(IntEnum):
    """Types d'événements .lum"""
    GEM_CREATE = 0
    GEM_CLOSE = 1
    GPU_MMAP = 2
    GPU_MUNMAP = 3
    BATCH_WRITE = 4
    EXECBUFFER2 = 5
    GPU_WAIT = 6
    CTX_CREATE = 7
    CTX_DESTROY = 8
    RAM_ALLOC = 9
    RAM_FREE = 10
    IOCTL_CALL = 11
    IOCTL_RESULT = 12
    ERROR = 13

EVENT_NAMES = {
    0: "GEM_CREATE",
    1: "GEM_CLOSE",
    2: "GPU_MMAP",
    3: "GPU_MUNMAP",
    4: "BATCH_WRITE",
    5: "EXECBUFFER2",
    6: "GPU_WAIT",
    7: "CTX_CREATE",
    8: "CTX_DESTROY",
    9: "RAM_ALLOC",
    10: "RAM_FREE",
    11: "IOCTL_CALL",
    12: "IOCTL_RESULT",
    13: "ERROR"
}

# ============================================================================
# STRUCTURES
# ============================================================================

@dataclass
class LumHeader:
    """Header .lum (64 bytes)"""
    magic: int
    version: int
    timestamp_start: int
    timestamp_end: int
    module: str
    arch: str
    event_count: int

@dataclass
class EventHeader:
    """Header événement (16 bytes)"""
    event_type: int
    timestamp_ns: int
    size: int

@dataclass
class Event:
    """Événement complet"""
    header: EventHeader
    data: dict
    
# ============================================================================
# PARSING
# ============================================================================

def parse_header(data: bytes) -> Optional[LumHeader]:
    """Parse le header .lum"""
    if len(data) < LUM_HEADER_SIZE:
        return None
    
    # Unpack header (little-endian)
    magic, version, ts_start, ts_end = struct.unpack('<IIQQ', data[0:24])
    
    if magic != LUM_MAGIC:
        print(f"❌ Magic invalide: 0x{magic:08X} (attendu: 0x{LUM_MAGIC:08X})")
        return None
    
    if version != LUM_VERSION:
        print(f"⚠️  Version: {version} (attendu: {LUM_VERSION})")
    
    # Module et arch (16 bytes chacun, null-terminated)
    module = data[24:40].decode('utf-8').rstrip('\x00')
    arch = data[40:56].decode('utf-8').rstrip('\x00')
    
    # Event count
    event_count = struct.unpack('<Q', data[56:64])[0]
    
    return LumHeader(
        magic=magic,
        version=version,
        timestamp_start=ts_start,
        timestamp_end=ts_end,
        module=module,
        arch=arch,
        event_count=event_count
    )

def parse_event_header(data: bytes, offset: int) -> Optional[EventHeader]:
    """Parse un header d'événement"""
    if offset + 16 > len(data):
        return None
    
    event_type, timestamp_ns, size = struct.unpack('<IQI', data[offset:offset+16])
    
    return EventHeader(
        event_type=event_type,
        timestamp_ns=timestamp_ns,
        size=size
    )

def parse_event_data(event_type: int, data: bytes, offset: int, size: int) -> dict:
    """Parse les données d'un événement selon son type"""
    event_data = {}
    
    if event_type == EventType.GEM_CREATE:
        handle, gem_size, flags = struct.unpack('<IQI', data[offset:offset+16])
        event_data = {'handle': handle, 'size': gem_size, 'flags': flags}
    
    elif event_type == EventType.GEM_CLOSE:
        handle = struct.unpack('<I', data[offset:offset+4])[0]
        event_data = {'handle': handle}
    
    elif event_type == EventType.GPU_MMAP:
        handle, addr, mmap_size = struct.unpack('<IQQ', data[offset:offset+20])
        event_data = {'handle': handle, 'addr': f'0x{addr:016X}', 'size': mmap_size}
    
    elif event_type == EventType.EXECBUFFER2:
        ctx_id, handle, exec_offset, exec_len = struct.unpack('<IIQI', data[offset:offset+20])
        event_data = {'ctx_id': ctx_id, 'handle': handle, 'offset': exec_offset, 'len': exec_len}
    
    elif event_type == EventType.GPU_WAIT:
        ctx_id, timeout, result = struct.unpack('<IQi', data[offset:offset+16])
        event_data = {'ctx_id': ctx_id, 'timeout_ns': timeout, 'result': result}
    
    elif event_type == EventType.CTX_CREATE:
        ctx_id = struct.unpack('<I', data[offset:offset+4])[0]
        event_data = {'ctx_id': ctx_id}
    
    elif event_type == EventType.CTX_DESTROY:
        ctx_id = struct.unpack('<I', data[offset:offset+4])[0]
        event_data = {'ctx_id': ctx_id}
    
    elif event_type == EventType.IOCTL_CALL:
        ioctl_nr, fd = struct.unpack('<Ii', data[offset:offset+8])
        event_data = {'ioctl': f'0x{ioctl_nr:08X}', 'fd': fd}
    
    elif event_type == EventType.IOCTL_RESULT:
        ioctl_nr, result, err = struct.unpack('<Iii', data[offset:offset+12])
        event_data = {'ioctl': f'0x{ioctl_nr:08X}', 'result': result, 'errno': err}
    
    elif event_type == EventType.ERROR:
        err_code = struct.unpack('<i', data[offset:offset+4])[0]
        # Message (reste des données)
        msg_len = size - 4
        if msg_len > 0:
            msg = data[offset+4:offset+4+msg_len].decode('utf-8', errors='ignore').rstrip('\x00')
            event_data = {'errno': err_code, 'message': msg}
        else:
            event_data = {'errno': err_code}
    
    return event_data

def parse_lum_file(filepath: str) -> tuple[Optional[LumHeader], List[Event]]:
    """Parse un fichier .lum complet"""
    try:
        with open(filepath, 'rb') as f:
            data = f.read()
    except Exception as e:
        print(f"❌ Erreur lecture fichier: {e}")
        return None, []
    
    # Parse header
    header = parse_header(data)
    if not header:
        return None, []
    
    # Parse événements
    events = []
    offset = LUM_HEADER_SIZE
    
    for i in range(header.event_count):
        # Parse event header
        ev_header = parse_event_header(data, offset)
        if not ev_header:
            print(f"⚠️  Événement {i+1}/{header.event_count} incomplet")
            break
        
        offset += 16
        
        # Parse event data
        ev_data = parse_event_data(ev_header.event_type, data, offset, ev_header.size)
        offset += ev_header.size
        
        events.append(Event(header=ev_header, data=ev_data))
    
    return header, events

# ============================================================================
# ANALYSE
# ============================================================================

def analyze_dispatches(events: List[Event]) -> dict:
    """Analyse les dispatches GPU"""
    dispatches = []
    current_dispatch = None
    
    for event in events:
        if event.header.event_type == EventType.EXECBUFFER2:
            if current_dispatch:
                dispatches.append(current_dispatch)
            current_dispatch = {
                'execbuffer': event,
                'wait': None,
                'error': None,
                'success': False
            }
        
        elif event.header.event_type == EventType.GPU_WAIT and current_dispatch:
            current_dispatch['wait'] = event
            if event.data.get('result', -1) == 0:
                current_dispatch['success'] = True
        
        elif event.header.event_type == EventType.ERROR and current_dispatch:
            current_dispatch['error'] = event
            current_dispatch['success'] = False
    
    if current_dispatch:
        dispatches.append(current_dispatch)
    
    return {'dispatches': dispatches, 'count': len(dispatches)}

def print_dispatch_analysis(analysis: dict):
    """Affiche l'analyse des dispatches"""
    print("\n" + "="*70)
    print("  ANALYSE DISPATCHES GPU")
    print("="*70)
    
    dispatches = analysis['dispatches']
    
    for i, dispatch in enumerate(dispatches, 1):
        exec_ev = dispatch['execbuffer']
        wait_ev = dispatch['wait']
        error_ev = dispatch['error']
        
        status = "✅ OK" if dispatch['success'] else "❌ ÉCHEC"
        
        print(f"\n[Dispatch {i}] {status}")
        print(f"  EXECBUFFER2:")
        print(f"    ctx_id:  {exec_ev.data['ctx_id']}")
        print(f"    handle:  {exec_ev.data['handle']}")
        print(f"    offset:  {exec_ev.data['offset']}")
        print(f"    len:     {exec_ev.data['len']}")
        print(f"    time:    {exec_ev.header.timestamp_ns} ns")
        
        if wait_ev:
            print(f"  GPU_WAIT:")
            print(f"    result:  {wait_ev.data['result']}")
            print(f"    timeout: {wait_ev.data['timeout_ns']} ns")
            print(f"    time:    {wait_ev.header.timestamp_ns} ns")
            
            # Calcul latence
            latency_ns = wait_ev.header.timestamp_ns - exec_ev.header.timestamp_ns
            latency_ms = latency_ns / 1_000_000
            print(f"    latency: {latency_ms:.3f} ms")
        
        if error_ev:
            print(f"  ERROR:")
            print(f"    errno:   {error_ev.data['errno']}")
            if 'message' in error_ev.data:
                print(f"    message: {error_ev.data['message']}")

def compare_dispatches(analysis: dict):
    """Compare les dispatches pour identifier les différences"""
    print("\n" + "="*70)
    print("  COMPARAISON DISPATCHES (Recherche Pattern Crash)")
    print("="*70)
    
    dispatches = analysis['dispatches']
    
    if len(dispatches) < 2:
        print("\n⚠️  Pas assez de dispatches pour comparaison")
        return
    
    # Comparer dispatches réussis vs échec
    success_dispatches = [d for d in dispatches if d['success']]
    failed_dispatches = [d for d in dispatches if not d['success']]
    
    print(f"\n✅ Dispatches réussis: {len(success_dispatches)}")
    print(f"❌ Dispatches échoués: {len(failed_dispatches)}")
    
    if failed_dispatches:
        print("\n🔍 ANALYSE DISPATCH ÉCHOUÉ:")
        failed = failed_dispatches[0]
        exec_ev = failed['execbuffer']
        
        print(f"\n  Contexte: ctx_id={exec_ev.data['ctx_id']}")
        print(f"  Handle:   {exec_ev.data['handle']}")
        print(f"  Offset:   {exec_ev.data['offset']}")
        print(f"  Len:      {exec_ev.data['len']}")
        
        # Comparer avec dernier succès
        if success_dispatches:
            last_success = success_dispatches[-1]
            last_exec = last_success['execbuffer']
            
            print(f"\n  Comparaison avec dernier succès (dispatch {len(success_dispatches)}):")
            
            if exec_ev.data['ctx_id'] != last_exec.data['ctx_id']:
                print(f"    ⚠️  ctx_id différent: {last_exec.data['ctx_id']} → {exec_ev.data['ctx_id']}")
            else:
                print(f"    ✓ ctx_id identique: {exec_ev.data['ctx_id']}")
            
            if exec_ev.data['handle'] != last_exec.data['handle']:
                print(f"    ⚠️  handle différent: {last_exec.data['handle']} → {exec_ev.data['handle']}")
            else:
                print(f"    ✓ handle identique: {exec_ev.data['handle']}")
            
            if exec_ev.data['len'] != last_exec.data['len']:
                print(f"    ⚠️  len différent: {last_exec.data['len']} → {exec_ev.data['len']}")
            else:
                print(f"    ✓ len identique: {exec_ev.data['len']}")

# ============================================================================
# MAIN
# ============================================================================

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_lum_forensic.py <fichier.lum>")
        sys.exit(1)
    
    filepath = sys.argv[1]
    
    print("="*70)
    print("  ANALYSE FORENSIQUE .LUM — Phase 15X")
    print("="*70)
    print(f"\nFichier: {filepath}\n")
    
    # Parse fichier
    header, events = parse_lum_file(filepath)
    
    if not header:
        print("❌ Échec parsing header")
        sys.exit(1)
    
    # Afficher header
    print("HEADER:")
    print(f"  Magic:          0x{header.magic:08X}")
    print(f"  Version:        {header.version}")
    print(f"  Module:         {header.module}")
    print(f"  Arch:           {header.arch}")
    print(f"  Event count:    {header.event_count}")
    print(f"  Timestamp start: {header.timestamp_start} ns")
    print(f"  Timestamp end:   {header.timestamp_end} ns")
    
    duration_ns = header.timestamp_end - header.timestamp_start
    duration_sec = duration_ns / 1_000_000_000
    print(f"  Duration:       {duration_sec:.6f} sec")
    
    print(f"\n✅ {len(events)} événements parsés")
    
    # Analyser dispatches
    analysis = analyze_dispatches(events)
    print_dispatch_analysis(analysis)
    compare_dispatches(analysis)
    
    print("\n" + "="*70)
    print("  FIN ANALYSE")
    print("="*70)

if __name__ == '__main__':
    main()

# Made with Bob
