#!/usr/bin/env python3
"""
TEST C546 - ANALYSE BATCH OPENCL vs NOTRE BATCH
Objectif: Décoder et comparer byte-par-byte les deux batches
"""

import struct
import sys

def decode_command(dwords, offset):
    """Décode une commande GPU à partir d'un offset"""
    cmd = dwords[offset]
    
    # Extraire opcode et length selon le format Intel
    # Format: bits [31:29] = client, [28:16] = opcode, [15:0] = length
    client = (cmd >> 29) & 0x7
    opcode = (cmd >> 16) & 0x1FFF
    length_field = cmd & 0xFFFF
    
    # Table des commandes connues (client:opcode)
    commands = {
        (3, 0x104): ("PIPELINE_SELECT", 1),
        (3, 0x101): ("STATE_BASE_ADDRESS", 10),
        (0, 0x100): ("3DSTATE_BINDING_TABLE_POOL_ALLOC", 3),
        (3, 0x000): ("STATE_SIP", 2),
        (2, 0x000): ("MEDIA_VFE_STATE", 8),
        (2, 0x002): ("MEDIA_INTERFACE_DESCRIPTOR_LOAD", 3),
        (2, 0x105): ("GPGPU_WALKER", 11),
        (0, 0x000): ("MI_BATCH_BUFFER_END", 1),
    }
    
    key = (client, opcode)
    if key in commands:
        name, fixed_len = commands[key]
        return name, fixed_len
    
    # Sinon utiliser le champ length
    return f"UNKNOWN_C{client}_OP{opcode:04x}", (length_field & 0xFF) + 2

def analyze_batch(filename, label):
    """Analyse un batch buffer"""
    print(f"\n{'='*80}")
    print(f"ANALYSE: {label}")
    print(f"{'='*80}\n")
    
    with open(filename, 'rb') as f:
        data = f.read()
    
    # Convertir en DWords
    dwords = struct.unpack(f'<{len(data)//4}I', data)
    
    offset = 0
    cmd_num = 0
    
    while offset < len(dwords):
        if dwords[offset] == 0:
            break
            
        cmd_name, cmd_len = decode_command(dwords, offset)
        
        print(f"[{cmd_num:02d}] Offset 0x{offset*4:04x}: {cmd_name} ({cmd_len} DWords)")
        
        # Afficher les DWords de la commande
        for i in range(cmd_len):
            if offset + i < len(dwords):
                dw = dwords[offset + i]
                print(f"  DW{i:02d}: 0x{dw:08x}", end="")
                
                # Annotations spéciales
                if cmd_name == "STATE_BASE_ADDRESS":
                    if i == 1:
                        print(" <- General State Base Address", end="")
                    elif i == 3:
                        print(" <- Surface State Base Address", end="")
                    elif i == 5:
                        print(" <- Dynamic State Base Address", end="")
                    elif i == 7:
                        print(" <- Indirect Object Base Address", end="")
                    elif i == 9:
                        print(" <- Instruction Base Address", end="")
                
                elif cmd_name == "MEDIA_VFE_STATE":
                    if i == 1:
                        print(" <- Scratch Space Base", end="")
                    elif i == 3:
                        print(" <- Max Threads, URB Entries", end="")
                    elif i == 4:
                        print(" <- URB Entry Size, CURBE Size", end="")
                
                elif cmd_name == "MEDIA_INTERFACE_DESCRIPTOR_LOAD":
                    if i == 1:
                        print(" <- Interface Descriptor Total Length", end="")
                    elif i == 2:
                        print(" <- Interface Descriptor Data Start Address", end="")
                
                elif cmd_name == "GPGPU_WALKER":
                    if i == 1:
                        print(" <- Interface Descriptor Offset", end="")
                    elif i == 2:
                        print(" <- Indirect Data Length", end="")
                    elif i == 3:
                        print(" <- Thread Group ID Starting X", end="")
                    elif i == 7:
                        print(" <- Thread Group ID X Dimension", end="")
                
                print()
        
        offset += cmd_len
        cmd_num += 1
        print()
    
    print(f"Total: {cmd_num} commandes, {offset*4} bytes\n")
    return dwords

def compare_batches(opencl_dwords, our_dwords):
    """Compare deux batches DWord par DWord"""
    print(f"\n{'='*80}")
    print("COMPARAISON DÉTAILLÉE")
    print(f"{'='*80}\n")
    
    max_len = max(len(opencl_dwords), len(our_dwords))
    differences = []
    
    for i in range(max_len):
        opencl_val = opencl_dwords[i] if i < len(opencl_dwords) else 0
        our_val = our_dwords[i] if i < len(our_dwords) else 0
        
        if opencl_val != our_val:
            differences.append((i, opencl_val, our_val))
            print(f"❌ DW{i:03d} @ 0x{i*4:04x}: OpenCL=0x{opencl_val:08x} vs Notre=0x{our_val:08x}")
    
    if not differences:
        print("✅ Les batches sont IDENTIQUES!")
    else:
        print(f"\n❌ {len(differences)} différences trouvées")
        
        # Analyse des patterns de différences
        print("\n=== ANALYSE DES DIFFÉRENCES ===")
        for i, opencl_val, our_val in differences[:10]:  # Premiers 10
            diff = opencl_val ^ our_val
            print(f"\nDW{i:03d}:")
            print(f"  OpenCL: 0x{opencl_val:08x} = {opencl_val:032b}")
            print(f"  Notre:  0x{our_val:08x} = {our_val:032b}")
            print(f"  XOR:    0x{diff:08x} = {diff:032b}")

if __name__ == "__main__":
    # Analyser le batch OpenCL
    opencl_dwords = analyze_batch("logs/opencl_batch_1.bin", "BATCH OPENCL (QUI FONCTIONNE)")
    
    # Analyser notre batch (si fourni)
    if len(sys.argv) > 1:
        our_dwords = analyze_batch(sys.argv[1], "NOTRE BATCH")
        compare_batches(opencl_dwords, our_dwords)
    else:
        print("\nPour comparer avec notre batch:")
        print("  python3 tests/test_c546_analyze_opencl_batch.py logs/notre_batch.bin")

# Made with Bob
