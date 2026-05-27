#!/bin/bash
# C399: Test Pipeline Hybride - Binary OpenCL → Runtime i915 DRM Natif

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== C399 HYBRID PIPELINE TEST ==="
echo "Timestamp: $(date -Iseconds)"
echo ""

# Vérifier que le binary OpenCL existe
if [ ! -f "kernels/btc_sha256_c398_raw.bin" ]; then
    echo "ERROR: kernels/btc_sha256_c398_raw.bin not found"
    echo "Run test_c398_sha256.sh first to generate it"
    exit 1
fi

echo "✓ OpenCL binary found: $(stat -c%s kernels/btc_sha256_c398_raw.bin) bytes"
echo ""

# Compiler le runner hybride
echo "Compiling hybrid runner..."
gcc -o tests/test_c399_hybrid_runner \
    tests/test_c399_hybrid_runner.c \
    -I/usr/include/libdrm \
    -I/usr/include/drm \
    -ldrm \
    -Wall -Wextra -O2

echo "✓ Compilation successful"
echo ""

# Créer le répertoire de logs
mkdir -p logs/forensic

# Exécuter avec capture complète
LOG_FILE="logs/forensic/test_c399_hybrid_$(date +%Y%m%d_%H%M%S).log"

echo "Executing hybrid pipeline..."
echo "Log file: $LOG_FILE"
echo ""

# Vérifier les permissions DRM
if [ ! -r /dev/dri/card1 ] && [ ! -r /dev/dri/card0 ]; then
    echo "ERROR: No read access to DRM devices"
    echo "Current user: $(whoami)"
    echo "Groups: $(groups)"
    echo "DRM devices:"
    ls -la /dev/dri/
    echo ""
    echo "You may need to add user to 'video' or 'render' group:"
    echo "  sudo usermod -a -G video,render $(whoami)"
    exit 1
fi

# Exécuter et capturer tout (sans sudo si permissions OK)
./tests/test_c399_hybrid_runner 2>&1 | tee "$LOG_FILE"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=== EXECUTION SUMMARY ==="
echo "Exit code: $EXIT_CODE"
echo "Log file: $LOG_FILE"
echo "Log size: $(stat -c%s "$LOG_FILE") bytes"
echo ""

# Analyser les résultats
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ SUCCESS: GPU compute produced valid results"
    
    # Extraire les statistiques
    NON_ZERO=$(grep "Non-zero results:" "$LOG_FILE" | awk '{print $4}')
    MAX_LZ=$(grep "Max leading zeros:" "$LOG_FILE" | awk '{print $4}')
    
    echo ""
    echo "=== RESULTS ==="
    echo "Non-zero hashes: $NON_ZERO / 256"
    echo "Max leading zeros: $MAX_LZ bits"
    echo ""
    
    if [ "$MAX_LZ" -ge 6 ]; then
        echo "🎉 EXCELLENT: Matches C398 OpenCL results (6 bits)"
        echo "   → Runtime i915 DRM natif fonctionne correctement!"
    elif [ "$MAX_LZ" -ge 1 ]; then
        echo "⚠️  PARTIAL: Some leading zeros found but less than C398"
        echo "   → Possible runtime divergence to investigate"
    fi
else
    echo "❌ FAILURE: GPU compute failed"
    
    # Chercher les erreurs
    echo ""
    echo "=== ERRORS FOUND ==="
    grep "ERROR:" "$LOG_FILE" || echo "No explicit errors"
    echo ""
    
    # Vérifier si c'est un problème de cache
    if grep -q "All results are zero" "$LOG_FILE"; then
        echo "Diagnostic: Cache coherency issue suspected"
        echo "  → CPU reads stale data"
        echo "  → GPU may have computed correctly"
        echo "  → Need stronger domain sync"
    fi
    
    # Vérifier si c'est un problème de dispatch
    if grep -q "EXECBUFFER2 failed" "$LOG_FILE"; then
        echo "Diagnostic: Dispatch failed"
        echo "  → Check batch buffer format"
        echo "  → Check state configuration"
    fi
fi

echo ""
echo "=== FORENSIC DATA AVAILABLE ==="
echo "Full log with hex dumps: $LOG_FILE"
echo ""
echo "To analyze:"
echo "  grep 'FORENSIC:' $LOG_FILE"
echo "  grep '===' $LOG_FILE"
echo ""

exit $EXIT_CODE

# Made with Bob
