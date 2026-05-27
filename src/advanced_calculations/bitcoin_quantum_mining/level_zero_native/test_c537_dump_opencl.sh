#!/bin/bash
# TEST C537 - Compilation et exécution dump OpenCL complet

set -e

echo "=== TEST C537 - DUMP COMPLET BUFFERS OPENCL ==="
echo ""

cd "$(dirname "$0")"

# Vérifier que le kernel existe
if [ ! -f "tests/test_c532_opencl_compare.cl" ]; then
    echo "❌ Kernel test_c532_opencl_compare.cl introuvable"
    exit 1
fi

# Compiler le test
echo "Compilation test_c537_opencl_dump_all.c..."
gcc -o test_c537_opencl_dump_all \
    tests/test_c537_opencl_dump_all.c \
    -I/usr/include/libdrm \
    -lOpenCL -ldrm \
    -Wall -Wextra \
    || { echo "❌ Erreur compilation"; exit 1; }

echo "✓ Compilation réussie"
echo ""

# Nettoyer les anciens fichiers
rm -f /tmp/gem_objects_c537_*.txt
rm -f /tmp/gem_diff_c537.txt
rm -f /tmp/buffer_c537_*.bin

# Exécuter avec sudo pour accès debugfs
echo "Exécution (nécessite sudo pour debugfs)..."
echo "Mot de passe sudo: emmaus"
echo ""

sudo -S ./test_c537_opencl_dump_all <<< "emmaus" 2>&1 | tee logs/test_c537_dump_opencl.log

echo ""
echo "=== RÉSUMÉ FICHIERS GÉNÉRÉS ==="
ls -lh /tmp/gem_objects_c537_*.txt 2>/dev/null || echo "Aucun fichier gem_objects"
ls -lh /tmp/gem_diff_c537.txt 2>/dev/null || echo "Aucun fichier diff"
ls -lh /tmp/buffer_c537_*.bin 2>/dev/null || echo "Aucun buffer dumpé"

echo ""
echo "=== ANALYSE BUFFERS DUMPÉS ==="

# Analyser chaque buffer
for f in /tmp/buffer_c537_*.bin; do
    if [ -f "$f" ]; then
        size=$(stat -c%s "$f")
        echo ""
        echo "Fichier: $f (${size} bytes)"
        
        if [ $size -eq 32 ]; then
            echo "→ IDRT (Interface Descriptor)"
        elif [ $size -eq 64 ]; then
            echo "→ Surface State"
        elif [ $size -le 256 ]; then
            echo "→ Binding Table ou CURBE"
        fi
        
        echo "Premiers 64 bytes:"
        hexdump -C "$f" | head -n 4
    fi
done

echo ""
echo "✓ Test C537 terminé"
echo "Log complet: logs/test_c537_dump_opencl.log"

# Made with Bob