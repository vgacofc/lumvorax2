#!/usr/bin/env python3
"""
Créer kernel C396 en COPIANT les opcodes OpenCL réels
Modifier uniquement le payload: 0xCAFE0000 → 0xDEADBEEF
OBJECTIF: Indépendance OpenCL avec opcodes VALIDÉS
"""

import sys
import struct

def main():
    print("🔧 Création kernel C396 depuis opcodes OpenCL réels...")
    
    # Lire le kernel OpenCL fonctionnel (section .text pure)
    with open('/tmp/c363_raw.bin', 'rb') as f:
        opencl_code = f.read()
    
    print(f"✅ Kernel OpenCL lu: {len(opencl_code)} bytes")
    
    # Analyser pour trouver la constante 0xCAFE0000
    # Dans le hexdump on voit: "00 00 fe ca" (little-endian)
    cafe_bytes = b'\x00\x00\xfe\xca'
    dead_bytes = b'\xef\xbe\xad\xde'  # 0xDEADBEEF en little-endian
    
    # Chercher et remplacer
    modified_code = bytearray(opencl_code)
    replacements = 0
    
    for i in range(len(modified_code) - 3):
        if modified_code[i:i+4] == cafe_bytes:
            print(f"   Trouvé 0xCAFE0000 à offset 0x{i:04x}")
            modified_code[i:i+4] = dead_bytes
            replacements += 1
    
    print(f"✅ Remplacements effectués: {replacements}")
    
    if replacements == 0:
        print("⚠️  ATTENTION: Aucun 0xCAFE0000 trouvé!")
        print("   Hexdump du début:")
        for i in range(0, min(128, len(opencl_code)), 16):
            hex_str = ' '.join(f'{b:02x}' for b in opencl_code[i:i+16])
            print(f"   {i:04x}: {hex_str}")
        return 1
    
    # Créer wrapper ELF (copier structure d'OpenCL)
    # Lire le fichier ELF complet d'OpenCL
    with open('tests/test_c363_magic_values.bin_skl.bin', 'rb') as f:
        opencl_elf = bytearray(f.read())
    
    # Trouver offset de la section .text dans l'ELF
    # D'après readelf: offset 0x40
    text_offset = 0x40
    text_size = len(opencl_code)
    
    # Remplacer la section .text
    opencl_elf[text_offset:text_offset+text_size] = modified_code
    
    # Écrire le nouveau kernel
    output_path = "kernels/test_c396_from_opencl.bin"
    with open(output_path, 'wb') as f:
        f.write(bytes(opencl_elf))
    
    print(f"✅ Kernel créé: {output_path}")
    print(f"   Taille: {len(opencl_elf)} bytes")
    print()
    print("📊 Modifications:")
    print("   - Payload: 0xCAFE0000 → 0xDEADBEEF")
    print("   - Structure ELF: Identique à OpenCL")
    print("   - Opcodes SEND: Identiques à OpenCL")
    print("   - Message descriptor: 0x04025E00 (validé)")
    print()
    print("🚀 Test:")
    print("   ./test_c396_from_opencl.sh")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())

# Made with Bob
