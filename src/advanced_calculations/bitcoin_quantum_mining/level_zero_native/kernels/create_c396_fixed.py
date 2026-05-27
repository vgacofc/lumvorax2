#!/usr/bin/env python3
"""
Créer kernel C396 - VERSION CORRIGÉE
Remplacer 0xCAFE0000 par 0xDEADBEEF dans le binaire ELF complet
"""
import struct

# Lire le fichier ELF OpenCL complet
with open('tests/test_c363_magic_values.bin_skl.bin', 'rb') as f:
    elf_data = bytearray(f.read())

print(f"📂 Fichier ELF lu: {len(elf_data)} bytes")

# Chercher et remplacer 0xCAFE0000 (00 00 fe ca en little-endian)
cafe_bytes = b'\x00\x00\xfe\xca'
dead_bytes = b'\xef\xbe\xad\xde'  # 0xDEADBEEF en little-endian

replacements = []
for i in range(len(elf_data) - 3):
    if elf_data[i:i+4] == cafe_bytes:
        print(f"   ✓ Trouvé 0xCAFE0000 à offset 0x{i:04x}")
        elf_data[i:i+4] = dead_bytes
        replacements.append(i)

print(f"\n✅ Remplacements effectués: {len(replacements)}")
print(f"   Offsets: {[hex(r) for r in replacements]}")

# Écrire le nouveau kernel
output_path = "kernels/test_c396_fixed.bin"
with open(output_path, 'wb') as f:
    f.write(bytes(elf_data))

print(f"\n✅ Kernel créé: {output_path}")
print(f"   Taille: {len(elf_data)} bytes")
print("\n🔍 Vérification:")

# Vérifier qu'il n'y a plus de 0xCAFE0000
remaining = elf_data.count(cafe_bytes)
dead_count = elf_data.count(dead_bytes)
print(f"   0xCAFE0000 restants: {remaining}")
print(f"   0xDEADBEEF présents: {dead_count}")

if remaining == 0 and dead_count == len(replacements):
    print("\n✅ SUCCÈS: Toutes les occurrences remplacées!")
else:
    print("\n❌ ERREUR: Remplacement incomplet")
    exit(1)
