#!/bin/bash

# TEST C495: EXTRACTION METADATA KERNEL OPENCL
# Objectif: Comprendre la configuration correcte pour i915 natif

set -e

echo "=== COMPILATION C495 ==="
echo ""

cd "$(dirname "$0")"

# Compilation avec OpenCL
echo "[1/3] Compilation test_c495_extract_metadata.c..."
gcc -o test_c495_extract_metadata \
    tests/test_c495_extract_metadata.c \
    -lOpenCL \
    -I. \
    -Wall -Wextra

echo "  ✓ Compilation réussie"
echo ""

# Compilation kernel avec ocloc
echo "[2/3] Compilation kernel avec ocloc..."
if command -v ocloc &> /dev/null; then
    ocloc compile \
        -file tests/test_c495_kernel_minimal.cl \
        -device skl \
        -output tests/test_c495_kernel_minimal
    echo "  ✓ Kernel compilé avec ocloc"
    
    # Analyse binaire
    if [ -f "tests/test_c495_kernel_minimal_Gen9core.bin" ]; then
        echo ""
        echo "=== ANALYSE BINAIRE OCLOC ==="
        ls -lh tests/test_c495_kernel_minimal_Gen9core.bin
        hexdump -C tests/test_c495_kernel_minimal_Gen9core.bin | head -20
    fi
else
    echo "  ⚠️  ocloc non disponible (optionnel)"
fi
echo ""

# Exécution extraction metadata
echo "[3/3] Extraction metadata OpenCL..."
echo ""
./test_c495_extract_metadata 2>&1 | tee logs/execution_c495_metadata.log

echo ""
echo "=== RÉSUMÉ C495 ==="
echo "✓ Metadata extraites"
echo "✓ Log sauvegardé: logs/execution_c495_metadata.log"
echo "✓ Prochaine étape: Appliquer configuration en C496"

# Made with Bob
