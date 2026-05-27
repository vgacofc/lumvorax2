#!/bin/bash
# TEST C371 - SHA-256 Known Vector Validation
# Compile et teste le kernel SHA-256 avec vecteur connu "abc"

set -e

cd "$(dirname "$0")"

echo "=== TEST C371: SHA-256 Known Vector Validation ==="
echo ""

# Compilation du kernel OpenCL → ISA Gen9
echo "[1/4] Compilation kernel SHA-256 test..."
cd kernels
ocloc compile -file ../tests/test_c371_sha256_vector.cl \
    -device kbl \
    -output test_c371_sha256_vector.bin_kbl.bin \
    -options "-cl-std=CL2.0"

if [ ! -f "test_c371_sha256_vector.bin_kbl.bin" ]; then
    echo "❌ Compilation failed"
    exit 1
fi

KERNEL_SIZE=$(stat -c%s test_c371_sha256_vector.bin_kbl.bin)
echo "✅ Kernel compiled: ${KERNEL_SIZE} bytes"
cd ..

# Compilation du runner C
echo ""
echo "[2/4] Compilation runner C..."
gcc -o tests/test_c371_sha256_runner \
    tests/test_c371_sha256_runner.c \
    -I/usr/include/libdrm \
    -ldrm \
    -Wall -Wextra

echo "✅ Runner compiled"

# Exécution du test
echo ""
echo "[3/4] Exécution test SHA-256..."
echo "Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
echo ""

./tests/test_c371_sha256_runner kernels/test_c371_sha256_vector.bin_kbl.bin

# Analyse résultat
echo ""
echo "[4/4] Analyse..."
echo ""
echo "⚠️  Ce test prouve si le kernel SHA-256 fonctionne correctement"
echo "Si FAILED → le kernel SHA-256 est cassé (BUG #5 confirmé)"
echo "Si PASSED → le problème est ailleurs (padding, nonce position, etc.)"

# Made with Bob
