#!/usr/bin/env python3
"""
TEST C549 - ANALYSE STRUCTURES OPENCL CAPTURÉES
Objectif: Décoder IDRT, Surface State, Binding Table, Kernel ISA
"""

import struct
import sys

def analyze_idrt(data):
    """Analyse Interface Descriptor Table"""
    print("\n" + "="*80)
    print("INTERFACE DESCRIPTOR (IDRT) - 32 bytes")
    print("="*80)
    
    dwords = struct.unpack('<8I', data[:32])
    
    print(f"\nDW0: 0x{dwords[0]:08x}")
    kernel_offset = dwords[0] & 0xFFFFFFC0
    print(f"  Kernel Start Pointer: 0x{kernel_offset:08x} (offset relatif)")
    
    print(f"\nDW1: 0x{dwords[1]:08x}")
    print(f"  Reserved")
    
    print(f"\nDW2: 0x{dwords[2]:08x}")
    sampler_count = (dwords[2] >> 2) & 0x7
    sampler_ptr = dwords[2] & 0xFFFFFFE0
    print(f"  Sampler Count: {sampler_count}")
    print(f"  Sampler State Pointer: 0x{sampler_ptr:08x}")
    
    print(f"\nDW3: 0x{dwords[3]:08x}")
    binding_table_entry_count = dwords[3] & 0x1F
    binding_table_ptr = dwords[3] & 0xFFFFFFE0
    print(f"  Binding Table Entry Count: {binding_table_entry_count}")
    print(f"  Binding Table Pointer: 0x{binding_table_ptr:08x}")
    
    print(f"\nDW4: 0x{dwords[4]:08x}")
    curbe_read_offset = (dwords[4] >> 16) & 0xFFFF
    curbe_read_length = dwords[4] & 0xFFFF
    print(f"  CURBE Read Offset: 0x{curbe_read_offset:04x} ({curbe_read_offset} bytes)")
    print(f"  CURBE Read Length: 0x{curbe_read_length:04x} ({curbe_read_length} bytes)")
    
    print(f"\nDW5: 0x{dwords[5]:08x}")
    barrier_enable = (dwords[5] >> 21) & 0x1
    slm_size = (dwords[5] >> 16) & 0x1F
    num_threads = dwords[5] & 0x3FF
    print(f"  Barrier Enable: {barrier_enable}")
    print(f"  Shared Local Memory Size: {slm_size}")
    print(f"  Number of Threads in GPGPU Thread Group: {num_threads}")
    
    print(f"\nDW6: 0x{dwords[6]:08x}")
    print(f"  Cross-Thread Constant Data Read Length: {dwords[6] & 0xFFFF}")
    
    print(f"\nDW7: 0x{dwords[7]:08x}")
    print(f"  Reserved")

def analyze_surface_state(data):
    """Analyse Surface State"""
    print("\n" + "="*80)
    print("SURFACE STATE - 64 bytes")
    print("="*80)
    
    dwords = struct.unpack('<16I', data[:64])
    
    print(f"\nDW0: 0x{dwords[0]:08x}")
    surface_type = dwords[0] & 0x7
    surface_format = (dwords[0] >> 18) & 0x1FF
    print(f"  Surface Type: {surface_type} (0=1D, 1=2D, 2=3D, 3=CUBE, 4=BUFFER)")
    print(f"  Surface Format: 0x{surface_format:03x}")
    
    print(f"\nDW1-2: Base Address")
    base_addr = dwords[1] | (dwords[2] << 32)
    print(f"  0x{base_addr:016x}")
    
    print(f"\nDW3: 0x{dwords[3]:08x}")
    width = (dwords[3] & 0x3FFF) + 1
    print(f"  Width: {width}")
    
    print(f"\nDW4: 0x{dwords[4]:08x}")
    height = (dwords[4] & 0x3FFF) + 1
    print(f"  Height: {height}")
    
    print(f"\nDW5: 0x{dwords[5]:08x}")
    surface_pitch = (dwords[5] & 0x3FFFF) + 1
    print(f"  Surface Pitch: {surface_pitch} bytes")

def analyze_binding_table(data):
    """Analyse Binding Table"""
    print("\n" + "="*80)
    print("BINDING TABLE")
    print("="*80)
    
    # Chaque entrée = 1 DWord (offset vers Surface State)
    for i in range(8):  # Max 8 entrées
        offset = struct.unpack('<I', data[i*4:(i+1)*4])[0]
        if offset != 0:
            print(f"\nEntry[{i}]: 0x{offset:08x}")
            print(f"  → Surface State @ offset 0x{offset:08x}")

def analyze_kernel_isa(data):
    """Analyse Kernel ISA (premiers 256 bytes)"""
    print("\n" + "="*80)
    print("KERNEL ISA (premiers 256 bytes)")
    print("="*80)
    
    dwords = struct.unpack('<64I', data[:256])
    
    for i in range(0, 64, 4):
        print(f"\n0x{i*4:04x}: ", end="")
        for j in range(4):
            if i+j < 64:
                print(f"{dwords[i+j]:08x} ", end="")
        print()

def main():
    print("="*80)
    print("ANALYSE STRUCTURES OPENCL CAPTURÉES - C549")
    print("="*80)
    
    # Analyser buffer[8] - IDRT + structures
    print("\n### BUFFER[8] - IDRT + STRUCTURES GPU ###")
    with open("logs/opencl_buffer_1_8.bin", "rb") as f:
        buffer8 = f.read()
    
    # IDRT semble être au début
    print("\n--- IDRT @ offset 0x0000 ---")
    analyze_idrt(buffer8[0x00:])
    
    # Chercher Surface State (pattern: type=4 pour BUFFER)
    print("\n--- Recherche Surface State ---")
    for offset in [0x40, 0x80, 0x100, 0x200]:
        dw0 = struct.unpack('<I', buffer8[offset:offset+4])[0]
        if (dw0 & 0x7) == 4:  # Type = BUFFER
            print(f"\n✓ Surface State trouvé @ offset 0x{offset:04x}")
            analyze_surface_state(buffer8[offset:])
            break
    
    # Analyser buffer[9] - Batch
    print("\n\n### BUFFER[9] - BATCH BUFFER ###")
    with open("logs/opencl_buffer_1_9.bin", "rb") as f:
        batch = f.read()
    
    print(f"\nTaille: {len(batch)} bytes")
    print("Premiers 128 bytes:")
    dwords = struct.unpack(f'<{len(batch)//4}I', batch)
    for i in range(0, min(32, len(dwords)), 8):
        print(f"{i*4:04x}: ", end="")
        for j in range(8):
            if i+j < len(dwords):
                print(f"{dwords[i+j]:08x} ", end="")
        print()
    
    # Analyser buffer[2] - Peut contenir CURBE ou autres structures
    print("\n\n### BUFFER[2] - STRUCTURES ADDITIONNELLES ###")
    with open("logs/opencl_buffer_1_2.bin", "rb") as f:
        buffer2 = f.read()
    
    print(f"\nTaille: {len(buffer2)} bytes")
    print("Premiers 128 bytes:")
    dwords = struct.unpack('<32I', buffer2[:128])
    for i in range(0, 32, 8):
        print(f"{i*4:04x}: ", end="")
        for j in range(8):
            print(f"{dwords[i+j]:08x} ", end="")
        print()

if __name__ == "__main__":
    main()

# Made with Bob
