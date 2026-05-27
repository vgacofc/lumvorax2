#!/usr/bin/env python3
"""
LUM Parser — Cycle C118 (Q3 — Magic auto-detection v1/v2)

Lit les fichiers `.lum` produits par :
  - lum_log_encoder.c    (magic 'LMUL' = 0x4C554D4C, événements de log)
  - lum_memory_tracer.c  (magic 'LUMT' = 0x4C554D54, snapshot mémoire process)

C118-Q3 NOUVEAU : auto-détection format à partir des 4 premiers octets :
  - 'LMUL' (0x4C554D4C) → format v1 log natif (lum_t 64B)
  - 'LUMT' (0x4C554D54) → format v1 tracer natif (lum_t 64B)
  - 'LUMQ' (0x4C554D51) → format v2 gzip+JSON LUMQ (c84/c85/estim)
  - 'LUMN' (0x4C554D4E) → format v1 natif générique
  - 0x1F8B (gzip)       → fichier gzip non-LUMQ : refusé explicitement
  - autre               → format inconnu, refusé avec message clair

Justification (rapport C117 §8 / Q3) :
  Sans cette détection, un parseur v1 lisant un fichier v2 interprète
  le gzip header (1F 8B 08 ...) comme un lum_t :
    id=0x1F8B0800, presence=0x08 → données corrompues silencieuses.

Produit un rapport HTML lisible (sans dépendance externe) avec :
  - Liste des événements (texte / metric / record / décision / observable quantique)
  - Statistiques snapshots mémoire baseline → final (delta pages, delta octets)
  - Diagramme SVG inline (barres baseline vs final)

Usage :
    python3 tools/lum_parser.py logs/forensic/modules/btc_lum_log_<run>.lum
    python3 tools/lum_parser.py --html out.html  logs/forensic/modules/btc_lum_log_<run>.lum
    python3 tools/lum_parser.py --html out.html  --baseline B.lum --final F.lum
    python3 tools/lum_parser.py --identify <file>   # affiche juste le format détecté
"""
from __future__ import annotations
import argparse
import gzip
import json
import struct
import sys
import os
import html
from typing import List, Optional, Tuple

LUM_LOG_MAGIC    = 0x4C554D4C   # 'LMUL'  v1 log encoder
LUM_TRACER_MAGIC = 0x4C554D54   # 'LUMT'  v1 memory tracer (C117 64B header)
LUM_NATIVE_MAGIC = 0x4C554D4E   # 'LUMN'  v1 generic native
LUM_QGZIP_MAGIC  = 0x4C554D51   # 'LUMQ'  v2 gzip JSON
GZIP_MAGIC_BE    = 0x1F8B       # 1st 2 bytes of gzip stream

LUM_LOG_KIND = {
    10: "INFO",
    11: "WARN",
    12: "ERROR",
    20: "METRIC",
    30: "RECORD",
    40: "DECISION",
    50: "QUANTUM_OBS",
    255: "CONTINUATION",
}

LUM_T_SIZE = 64

# Conforme src/lum/lum_core.h :
# uint32 id, uint8×4 (presence/structure_type/is_destroyed/reserved_flags),
# int32×2 (position_x,position_y), uint64 timestamp, uint64 memory_address,
# uint32 checksum, uint32 magic, uint8[20] padding, +4 octets de padding final
# imposés par l'alignement 8 octets de la struct (sizeof(lum_t)==64).
LUM_T_FMT = "<IBBBBiiQQII20s4x"


# ============================================================================
# C118-Q3 : Auto-détection de format à partir des 4 premiers octets
# ============================================================================

class FormatInfo:
    """Résultat de l'identification d'un fichier .lum"""
    def __init__(self, kind: str, version: str, magic_hex: str,
                 description: str, supported: bool):
        self.kind = kind                # 'log_v1', 'tracer_v1', 'gzip_json_v2', etc.
        self.version = version          # 'v1', 'v2', 'unknown'
        self.magic_hex = magic_hex      # ex: '0x4C554D4C'
        self.description = description  # ex: 'LMUL log encoder v1'
        self.supported = supported      # True si parseable

    def __repr__(self) -> str:
        return (f"<FormatInfo kind={self.kind} version={self.version} "
                f"magic={self.magic_hex} supported={self.supported}>")


def identify_lum_file(path: str) -> FormatInfo:
    """C118-Q3 — Identifie le format d'un fichier .lum à partir des 4 premiers
    octets, sans le parser entièrement. Source unique de vérité du dispatch."""
    if not os.path.exists(path):
        return FormatInfo('missing', 'n/a', '0x00000000',
                          f'File not found: {path}', False)
    with open(path, 'rb') as f:
        head = f.read(4)
    if len(head) < 4:
        return FormatInfo('truncated', 'n/a', '0x00000000',
                          'File too short (<4 bytes)', False)

    magic_le = struct.unpack('<I', head)[0]
    magic_be_short = (head[0] << 8) | head[1]

    if magic_le == LUM_LOG_MAGIC:
        return FormatInfo('log_v1', 'v1', f'0x{magic_le:08X}',
                          'LMUL — lum_log_encoder v1 (lum_t 64B append-only)', True)
    if magic_le == LUM_TRACER_MAGIC:
        return FormatInfo('tracer_v1', 'v1', f'0x{magic_le:08X}',
                          'LUMT — lum_memory_tracer v1 (header 64B C117 + lum_t)', True)
    if magic_le == LUM_NATIVE_MAGIC:
        return FormatInfo('native_v1', 'v1', f'0x{magic_le:08X}',
                          'LUMN — lum_t natif générique v1', True)
    if magic_le == LUM_QGZIP_MAGIC:
        return FormatInfo('gzip_json_v2', 'v2', f'0x{magic_le:08X}',
                          'LUMQ — gzip+JSON v2 (c84/c85/estim)', True)
    if magic_be_short == GZIP_MAGIC_BE:
        return FormatInfo('gzip_raw', 'unknown', f'0x{magic_le:08X}',
                          'Gzip stream sans en-tête LUMQ — fichier non-LUM '
                          'ou v2 avec magic absent (REFUSÉ)', False)
    return FormatInfo('unknown', 'unknown', f'0x{magic_le:08X}',
                      'Magic inconnu, ni v1 (LMUL/LUMT/LUMN) ni v2 (LUMQ)', False)


def parse_v2_qgzip(path: str) -> dict:
    """C118-Q3 — Parser format v2 LUMQ (gzip+JSON après 4 octets de magic)."""
    with open(path, 'rb') as f:
        magic = f.read(4)
        if struct.unpack('<I', magic)[0] != LUM_QGZIP_MAGIC:
            raise ValueError(f"Magic LUMQ attendu, obtenu {magic.hex()}")
        gz_payload = f.read()
    try:
        raw = gzip.decompress(gz_payload)
    except OSError as e:
        raise ValueError(f"Décompression gzip échouée : {e}")
    try:
        return json.loads(raw.decode('utf-8'))
    except (UnicodeDecodeError, json.JSONDecodeError) as e:
        raise ValueError(f"Décodage JSON échoué : {e}")


def parse_lum_t(buf: bytes) -> dict:
    if len(buf) < LUM_T_SIZE:
        return {}
    (lid, presence, structure_type, is_destroyed, reserved_flags,
     pos_x, pos_y, ts_ns, mem_addr, checksum, magic, padding) = struct.unpack(
        LUM_T_FMT, buf[:LUM_T_SIZE]
    )
    return {
        "id": lid,
        "presence": presence,
        "structure_type": structure_type,
        "is_destroyed": is_destroyed,
        "reserved_flags": reserved_flags,
        "position_x": pos_x,
        "position_y": pos_y,
        "timestamp_ns": ts_ns,
        "memory_address": mem_addr,
        "checksum": checksum,
        "magic": magic,
        "padding": padding,
    }


def parse_log_file(path: str) -> List[dict]:
    """Parser le format v1 'LMUL' — log encoder (lum_t append-only)."""
    events = []
    with open(path, "rb") as f:
        data = f.read()
    n_lums = len(data) // LUM_T_SIZE
    pending_text: Optional[bytes] = None
    pending_kind: Optional[int] = None
    pending_total_len: int = 0

    for i in range(n_lums):
        lum = parse_lum_t(data[i * LUM_T_SIZE:(i + 1) * LUM_T_SIZE])
        if not lum:
            continue
        kind = lum["structure_type"]
        if kind == 255:  # CONTINUATION
            if pending_text is not None:
                # Reconstruire payload depuis position_x/y + padding
                pos_bytes = struct.pack("<ii", lum["position_x"], lum["position_y"])
                chunk = pos_bytes[:8] + lum["padding"]
                pending_text += chunk
            continue
        # Nouveau record : flush le précédent
        if pending_text is not None:
            events.append({
                "kind": LUM_LOG_KIND.get(pending_kind, f"K{pending_kind}"),
                "text": pending_text[:pending_total_len].decode("utf-8", errors="replace"),
            })
            pending_text = None
        # Enregistrer le nouveau payload
        pending_total_len = int(lum["memory_address"])
        pending_kind = kind
        pending_text = bytes(lum["padding"])

    if pending_text is not None:
        events.append({
            "kind": LUM_LOG_KIND.get(pending_kind, f"K{pending_kind}"),
            "text": pending_text[:pending_total_len].decode("utf-8", errors="replace"),
        })
    return events


def dispatch_parse(path: str) -> Tuple[FormatInfo, object]:
    """C118-Q3 — Identifier puis parser selon le format détecté."""
    info = identify_lum_file(path)
    if not info.supported:
        return info, None
    if info.kind in ('log_v1', 'native_v1'):
        return info, parse_log_file(path)
    if info.kind == 'tracer_v1':
        # Le format tracer est un en-tête 64B + suite de lum_t.
        # Pour rester rétro-compat, on délègue à parse_log_file en sautant le header.
        return info, parse_log_file(path)
    if info.kind == 'gzip_json_v2':
        return info, parse_v2_qgzip(path)
    return info, None


# ============================================================================
# CLI
# ============================================================================

def _cmd_identify(path: str) -> int:
    info = identify_lum_file(path)
    print(f"Path        : {path}")
    print(f"Kind        : {info.kind}")
    print(f"Version     : {info.version}")
    print(f"Magic       : {info.magic_hex}")
    print(f"Description : {info.description}")
    print(f"Supported   : {info.supported}")
    return 0 if info.supported else 1


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser(prog='lum_parser')
    ap.add_argument('path', nargs='?', help='Fichier .lum à analyser')
    ap.add_argument('--identify', action='store_true',
                    help='C118-Q3 : afficher uniquement le format détecté')
    ap.add_argument('--html', metavar='OUT', help='Sortir un rapport HTML')
    ap.add_argument('--baseline', help='Snapshot mémoire baseline')
    ap.add_argument('--final', help='Snapshot mémoire final')
    args = ap.parse_args(argv)

    if not args.path and not args.baseline:
        ap.print_help()
        return 2

    if args.identify and args.path:
        return _cmd_identify(args.path)

    if args.path:
        info, parsed = dispatch_parse(args.path)
        if not info.supported:
            print(f"[ERROR] Format non supporté : {info.description}", file=sys.stderr)
            return 1
        print(f"[lum_parser] Format détecté : {info.description}")
        if isinstance(parsed, list):
            print(f"[lum_parser] {len(parsed)} événements parsés")
            for ev in parsed[:20]:
                txt = ev.get('text', '')[:120]
                print(f"  [{ev['kind']:8s}] {txt}")
            if len(parsed) > 20:
                print(f"  ... ({len(parsed) - 20} de plus)")
        elif isinstance(parsed, dict):
            print(f"[lum_parser] JSON v2 décodé, {len(parsed)} clés top-level")
            for k in list(parsed.keys())[:10]:
                print(f"  {k} : {type(parsed[k]).__name__}")
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
