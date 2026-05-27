#!/bin/bash
# C402: Test complet du kernel SHA256 natif (64 rounds)
# Compile l'assembleur Gen9 ISA et exécute avec runtime i915 DRM natif

set -e

echo "=== C402 NATIVE SHA256 KERNEL TEST ==="
echo "Timestamp: $(date -Iseconds)"
echo ""
echo "Étapes:"
echo "  1. Compiler l'assembleur Gen9 ISA (4155 lignes)"
echo "  2. Compiler le test runner C"
echo "  3. Exécuter le test avec runtime natif"
echo "  4. Analyser les résultats"
echo ""

# Vérifier que iga64 est disponible
if ! command -v iga64 &> /dev/null; then
    echo "❌ iga64 not found"
    echo "   Install Intel Graphics Assembler:"
    echo "   git clone https://github.com/intel/intel-graphics-compiler"
    exit 1
fi

# Compiler l'assembleur
echo "Compiling Gen9 ISA kernel..."
iga64 -p=9 -a \
    kernels/btc_sha256_gen9_c402_complete_64rounds.asm \
    -o kernels/btc_sha256_gen9_c402_native.bin

if [ $? -ne 0 ]; then
    echo "❌ Assembly compilation failed"
    exit 1
fi

KERNEL_SIZE=$(stat -c%s kernels/btc_sha256_gen9_c402_native.bin)
echo "✓ Kernel compiled: $KERNEL_SIZE bytes"

# Compiler le test runner
echo ""
echo "Compiling C402 test runner..."
gcc -o tests/test_c402_native_sha256_runner \
    tests/test_c402_native_sha256_runner.c \
    -I/usr/include/libdrm \
    -I/usr/include/drm \
    -ldrm \
    -O2 \
    -Wall

if [ $? -ne 0 ]; then
    echo "❌ C compilation failed"
    exit 1
fi

echo "✓ Test runner compiled"

# Créer le répertoire de logs
mkdir -p logs/forensic

# Exécuter le test
LOG_FILE="logs/forensic/test_c402_native_sha256_$(date +%Y%m%d_%H%M%S).log"

echo ""
echo "Executing C402 native SHA256 kernel..."
echo "Log: $LOG_FILE"
echo ""

./tests/test_c402_native_sha256_runner 2>&1 | tee "$LOG_FILE"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=== SUMMARY ==="
echo "Exit code: $EXIT_CODE"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo "🎉 SUCCESS: Native SHA256 kernel working!"
    echo ""
    echo "✅ Achievements:"
    echo "   - 64 rounds SHA256 complets en Gen9 ISA"
    echo "   - 4155 lignes d'assembleur"
    echo "   - Runtime i915 DRM natif (sans OpenCL)"
    echo "   - near_miss_gpu > 0 bits achieved"
    echo "   - Indépendance d'OpenCL: COMPLETE"
    echo ""
    echo "Next steps:"
    echo "   - Optimiser le kernel (SIMD, registres)"
    echo "   - Mesurer le hashrate"
    echo "   - Comparer avec C398 (OpenCL)"
else
    echo "❌ FAILURE: Tests did not succeed"
    echo ""
    echo "Possible causes:"
    echo "   - Erreur dans l'assembleur Gen9 ISA"
    echo "   - Problème de STATE_BASE_ADDRESS"
    echo "   - Interface Descriptor incorrect"
    echo "   - CURBE mal configuré"
    echo ""
    echo "Debug info in log: $LOG_FILE"
fi

echo ""
echo "Full log: $LOG_FILE"

# Made with Bob
