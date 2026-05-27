#!/usr/bin/env python3
"""
Créer kernel C396 avec message descriptor OpenCL correct
Basé sur analyse comparative C395: OpenCL vs C391
"""

import struct
import sys

def create_gen9_instruction(opcode, exec_size, dst, src0, src1=None, imm=None):
    """Créer une instruction Gen9 (format simplifié)"""
    # Format basique Gen9: 128 bits (16 bytes) par instruction
    # Ceci est une simplification - le vrai format est plus complexe
    inst = bytearray(16)
    
    # Opcode dans les premiers bits
    inst[0] = opcode & 0xFF
    inst[1] = (opcode >> 8) & 0xFF
    
    return bytes(inst)

def create_c396_kernel():
    """
    Créer kernel C396 avec descriptor OpenCL 0x04025E00
    
    Basé sur désassemblage OpenCL:
    sends (16|M0) null:w r5 r11 0x8C 0x04025E00
    
    Notre version simplifiée:
    sends (1|M0) null:ud r4 r6 0x8C 0x04025E00
    """
    
    kernel = bytearray()
    
    # Instruction 1: mov (8) r2.0<1>:ud r0.0<1;1,0>:ud
    # Copier thread state
    # Opcode MOV = 0x01, format Align1
    inst1 = bytes([
        0x01, 0x00, 0x00, 0x00,  # Opcode + flags
        0x20, 0x00, 0x00, 0x00,  # dst: r2.0
        0x00, 0x00, 0x00, 0x00,  # src0: r0.0
        0x00, 0x00, 0x00, 0x00   # padding
    ])
    kernel.extend(inst1)
    
    # Instruction 2: or (1) cr0.0<1>:ud cr0.0<0;1,0>:ud 0x4C0:uw
    # Activer SEND capability
    inst2 = bytes([
        0x05, 0x00, 0x00, 0x00,  # Opcode OR
        0xC0, 0x04, 0x00, 0x00,  # Immediate 0x4C0
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    ])
    kernel.extend(inst2)
    
    # Instruction 3: mov (1) r4.0<1>:uq r3.0<0;1,0>:uq
    # Copier adresse depuis CURBE
    inst3 = bytes([
        0x01, 0x00, 0x00, 0x00,  # Opcode MOV
        0x40, 0x00, 0x00, 0x00,  # dst: r4.0 (64-bit)
        0x30, 0x00, 0x00, 0x00,  # src0: r3.0 (64-bit)
        0x00, 0x00, 0x00, 0x00
    ])
    kernel.extend(inst3)
    
    # Instruction 4: mov (1) r6.0<1>:d 0xDEADBEEF:d
    # Charger payload magique
    inst4 = bytes([
        0x01, 0x00, 0x00, 0x00,  # Opcode MOV
        0x60, 0x00, 0x00, 0x00,  # dst: r6.0
        0xEF, 0xBE, 0xAD, 0xDE,  # imm: 0xDEADBEEF
        0x00, 0x00, 0x00, 0x00
    ])
    kernel.extend(inst4)
    
    # Instruction 5: mov (8) r127.0<1>:ud r2.0<8;8,1>:ud
    # Sauvegarder pour EOT
    inst5 = bytes([
        0x01, 0x00, 0x00, 0x00,  # Opcode MOV
        0x7F, 0x00, 0x00, 0x00,  # dst: r127.0
        0x20, 0x00, 0x00, 0x00,  # src0: r2.0
        0x00, 0x00, 0x00, 0x00
    ])
    kernel.extend(inst5)
    
    # ⭐ Instruction 6: sends (1) null:ud r4 r6 0x8C 0x04025E00
    # SEND avec descriptor OpenCL
    # Opcode SENDS = 0x7B (split-send)
    inst6 = bytes([
        0x7B, 0x00, 0x00, 0x00,  # Opcode SENDS
        0x00, 0x00, 0x00, 0x00,  # dst: null
        0x40, 0x00, 0x00, 0x00,  # src0: r4 (address)
        0x60, 0x00, 0x00, 0x00,  # src1: r6 (data)
        0x8C, 0x00, 0x00, 0x00,  # Extended descriptor: 0x8C
        0x00, 0x5E, 0x02, 0x04   # Message descriptor: 0x04025E00
    ])
    # Note: SENDS est 24 bytes (pas 16)
    kernel.extend(inst6)
    
    # Instruction 7: send (1) null r127 0x27 0x02000010 {EOT}
    # End of thread
    inst7 = bytes([
        0x31, 0x00, 0x00, 0x00,  # Opcode SEND + EOT flag
        0x00, 0x00, 0x00, 0x00,  # dst: null
        0x7F, 0x00, 0x00, 0x00,  # src0: r127
        0x27, 0x00, 0x00, 0x00,  # Extended descriptor: 0x27
        0x10, 0x00, 0x00, 0x02   # Message descriptor: 0x02000010
    ])
    kernel.extend(inst7)
    
    # Padding pour aligner à 128 bytes
    while len(kernel) < 128:
        kernel.append(0x00)
    
    return bytes(kernel)

def create_elf_wrapper(kernel_code):
    """
    Créer un wrapper ELF minimal pour le kernel
    Compatible avec le format attendu par Level Zero
    """
    
    # ELF Header (64 bytes)
    elf_header = bytearray([
        0x7F, 0x45, 0x4C, 0x46,  # Magic: ELF
        0x02,                     # 64-bit
        0x01,                     # Little endian
        0x01,                     # ELF version
        0x00,                     # Generic ABI
    ])
    elf_header.extend([0x00] * 8)  # Padding
    elf_header.extend([
        0x01, 0x00,              # Type: relocatable
        0xCD, 0x00,              # Machine: Intel GPU (custom)
        0x01, 0x00, 0x00, 0x00,  # Version
    ])
    elf_header.extend([0x00] * 32)  # Addresses and offsets
    elf_header.extend([
        0x40, 0x00,              # ELF header size
        0x00, 0x00,              # Program header size
        0x00, 0x00,              # Program header count
        0x40, 0x00,              # Section header size
        0x03, 0x00,              # Section header count
        0x02, 0x00,              # String table index
    ])
    
    # Section 1: .text (kernel code)
    text_section = kernel_code
    text_offset = 0x40  # Right after ELF header
    
    # Section headers (3 sections: NULL, .text, .shstrtab)
    section_headers = bytearray()
    
    # NULL section
    section_headers.extend([0x00] * 64)
    
    # .text section header
    text_sh = bytearray([
        0x01, 0x00, 0x00, 0x00,  # Name offset in string table
        0x01, 0x00, 0x00, 0x00,  # Type: PROGBITS
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Flags: ALLOC+EXEC
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Address
    ])
    text_sh.extend(struct.pack('<Q', text_offset))  # Offset
    text_sh.extend(struct.pack('<Q', len(text_section)))  # Size
    text_sh.extend([0x00] * 24)  # Link, info, align, entsize
    section_headers.extend(text_sh)
    
    # .shstrtab section header
    strtab_offset = text_offset + len(text_section)
    strtab_data = b'\x00.text\x00.shstrtab\x00'
    
    strtab_sh = bytearray([
        0x07, 0x00, 0x00, 0x00,  # Name offset
        0x03, 0x00, 0x00, 0x00,  # Type: STRTAB
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Flags
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Address
    ])
    strtab_sh.extend(struct.pack('<Q', strtab_offset))  # Offset
    strtab_sh.extend(struct.pack('<Q', len(strtab_data)))  # Size
    strtab_sh.extend([0x00] * 24)
    section_headers.extend(strtab_sh)
    
    # Assembler le fichier ELF
    elf_file = bytearray(elf_header)
    elf_file.extend(text_section)
    elf_file.extend(strtab_data)
    elf_file.extend(section_headers)
    
    return bytes(elf_file)

def main():
    print("Création du kernel C396 avec descriptor OpenCL...")
    
    # Créer le code kernel
    kernel_code = create_c396_kernel()
    print(f"Kernel code: {len(kernel_code)} bytes")
    
    # Créer le wrapper ELF
    elf_binary = create_elf_wrapper(kernel_code)
    print(f"ELF binary: {len(elf_binary)} bytes")
    
    # Écrire le fichier
    output_path = "kernels/test_c396_opencl_descriptor.bin"
    with open(output_path, 'wb') as f:
        f.write(elf_binary)
    
    print(f"✅ Kernel créé: {output_path}")
    print("\nPour tester:")
    print("  ./test_c396_opencl_descriptor.sh")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())

# Made with Bob
