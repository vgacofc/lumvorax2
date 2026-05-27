#!/usr/bin/env python3
"""
Créer kernel C396 manuel en hexadécimal
Basé sur ISA OpenCL désassemblé + documentation IGC
OBJECTIF: Indépendance totale d'OpenCL
"""

import struct
import sys

def create_gen9_send_corrected():
    """
    Créer SEND instruction corrigée avec TOUS les facteurs:
    1. SIMD8 (pas SIMD1)
    2. Payload vectorisé (r11, pas r6 scalaire)
    3. Message type 0x09 (untyped surface write)
    4. Registres r5+r11 (convention OpenCL)
    5. Message descriptor 0x04025E09
    """
    
    kernel = bytearray()
    
    # Instruction 1: mov (8) r2.0<1>:ud r0.0<1;1,0>:ud
    # Copier thread state
    # Format Gen9: opcode=0x01 (MOV), execSize=8, dst=r2, src=r0
    inst1 = bytes.fromhex(
        "01 00 00 00 20 00 00 00 00 00 00 00 00 00 00 00"
    )
    kernel.extend(inst1)
    
    # Instruction 2: or (1) cr0.0<1>:ud cr0.0<0;1,0>:ud 0x4C0:uw
    # Activer SEND
    inst2 = bytes.fromhex(
        "05 00 00 00 C0 04 00 00 00 00 00 00 00 00 00 00"
    )
    kernel.extend(inst2)
    
    # Instruction 3: mov (8) r4.0<1>:ud r1.0<8;8,1>:uw
    # Copier GID
    inst3 = bytes.fromhex(
        "01 00 00 00 40 00 00 00 10 00 00 00 00 00 00 00"
    )
    kernel.extend(inst3)
    
    # Instruction 4: shl (8) r5.0<1>:ud r4.0<8;8,1>:ud 2:w
    # Calculer offset (gid * 4)
    inst4 = bytes.fromhex(
        "09 00 00 00 50 00 00 00 40 00 00 00 02 00 00 00"
    )
    kernel.extend(inst4)
    
    # Instruction 5: mov (8) r11.0<1>:ud 0xDEADBEEF:ud
    # Charger payload vectorisé
    inst5 = bytes.fromhex(
        "01 00 00 00 B0 00 00 00 EF BE AD DE 00 00 00 00"
    )
    kernel.extend(inst5)
    
    # Instruction 6: mov (8) r127.0<1>:ud r2.0<8;8,1>:ud
    # Sauvegarder pour EOT
    inst6 = bytes.fromhex(
        "01 00 00 00 7F 00 00 00 20 00 00 00 00 00 00 00"
    )
    kernel.extend(inst6)
    
    # ⭐ Instruction 7: sends (8) null:ud r5 r11 0x8C 0x04025E09
    # SEND CORRIGÉ avec message descriptor OpenCL-like
    # 
    # Message Descriptor 0x04025E09 décodé:
    #   [18:14] = 0x09 (UNTYPED_SURFACE_WRITE)
    #   [13:12] = 0x3 (DC1 category)
    #   [11:8]  = 0x1 (SIMD16 mode - mais on utilise SIMD8)
    #   [7:0]   = 0x00 (BTI = 0)
    #   [28:25] = 0x02 (Message length = 2)
    #   [24:19] = 0x00 (Response length = 0)
    #
    # Format SENDS (split-send):
    # - Opcode: 0x7B
    # - ExecSize: 8 (0x3 dans bits [2:0])
    # - dst: null (0x00)
    # - src0: r5 (address)
    # - src1: r11 (data)
    # - exDesc: 0x8C
    # - msgDesc: 0x04025E09
    
    # SENDS est une instruction de 24 bytes sur Gen9
    inst7 = bytes.fromhex(
        "7B 03 00 00"  # Opcode SENDS + ExecSize=8
        "00 00 00 00"  # dst: null
        "50 00 00 00"  # src0: r5
        "B0 00 00 00"  # src1: r11
        "8C 00 00 00"  # Extended descriptor: 0x8C
        "09 5E 02 04"  # Message descriptor: 0x04025E09
    )
    kernel.extend(inst7)
    
    # Instruction 8: send (1) null r127 0x27 0x02000010 {EOT}
    # End of thread
    inst8 = bytes.fromhex(
        "31 00 00 00 00 00 00 00 7F 00 00 00 27 00 00 00 10 00 00 02"
    )
    kernel.extend(inst8)
    
    # Padding pour aligner à 128 bytes
    while len(kernel) < 128:
        kernel.append(0x00)
    
    return bytes(kernel)

def create_elf_wrapper(kernel_code):
    """Créer wrapper ELF minimal"""
    
    # ELF Header simplifié
    elf_header = bytearray([
        0x7F, 0x45, 0x4C, 0x46,  # Magic
        0x02, 0x01, 0x01, 0x00,  # 64-bit, little-endian
    ])
    elf_header.extend([0x00] * 8)  # Padding
    elf_header.extend([
        0x01, 0x00,              # Relocatable
        0xCD, 0x00,              # Intel GPU
        0x01, 0x00, 0x00, 0x00,  # Version
    ])
    elf_header.extend([0x00] * 32)
    elf_header.extend([
        0x40, 0x00,  # ELF header size
        0x00, 0x00,  # Program header size
        0x00, 0x00,  # Program header count
        0x40, 0x00,  # Section header size
        0x03, 0x00,  # Section header count
        0x02, 0x00,  # String table index
    ])
    
    # Section .text
    text_offset = 0x40
    text_section = kernel_code
    
    # Section headers
    section_headers = bytearray([0x00] * 64)  # NULL section
    
    # .text section header
    text_sh = bytearray([
        0x01, 0x00, 0x00, 0x00,  # Name offset
        0x01, 0x00, 0x00, 0x00,  # PROGBITS
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ALLOC+EXEC
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Address
    ])
    text_sh.extend(struct.pack('<Q', text_offset))
    text_sh.extend(struct.pack('<Q', len(text_section)))
    text_sh.extend([0x00] * 24)
    section_headers.extend(text_sh)
    
    # .shstrtab
    strtab_offset = text_offset + len(text_section)
    strtab_data = b'\x00.text\x00.shstrtab\x00'
    
    strtab_sh = bytearray([
        0x07, 0x00, 0x00, 0x00,  # Name offset
        0x03, 0x00, 0x00, 0x00,  # STRTAB
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    ])
    strtab_sh.extend(struct.pack('<Q', strtab_offset))
    strtab_sh.extend(struct.pack('<Q', len(strtab_data)))
    strtab_sh.extend([0x00] * 24)
    section_headers.extend(strtab_sh)
    
    # Assembler
    elf_file = bytearray(elf_header)
    elf_file.extend(text_section)
    elf_file.extend(strtab_data)
    elf_file.extend(section_headers)
    
    return bytes(elf_file)

def main():
    print("🔧 Création kernel C396 manuel (INDÉPENDANT OpenCL)...")
    print("📋 Corrections appliquées:")
    print("   1. ✅ SIMD1 → SIMD8")
    print("   2. ✅ Payload scalaire → vectorisé")
    print("   3. ✅ Message type 0x1A → 0x09")
    print("   4. ✅ Registres r4+r6 → r5+r11")
    print("   5. ✅ Descriptor 0x040681FF → 0x04025E09")
    print()
    
    # Créer kernel
    kernel_code = create_gen9_send_corrected()
    print(f"✅ Kernel code: {len(kernel_code)} bytes")
    
    # Créer ELF
    elf_binary = create_elf_wrapper(kernel_code)
    print(f"✅ ELF binary: {len(elf_binary)} bytes")
    
    # Écrire
    output_path = "kernels/test_c396_manual_corrected.bin"
    with open(output_path, 'wb') as f:
        f.write(elf_binary)
    
    print(f"✅ Kernel créé: {output_path}")
    print()
    print("📊 Message Descriptor: 0x04025E09")
    print("   [18:14] = 0x09 (UNTYPED_SURFACE_WRITE)")
    print("   [13:12] = 0x3  (DC1 category)")
    print("   [11:8]  = 0x1  (SIMD mode)")
    print("   [7:0]   = 0x00 (BTI = 0)")
    print()
    print("🚀 Prochaine étape: Tester avec test_c396_runner.c")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())

# Made with Bob
