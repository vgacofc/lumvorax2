#!/bin/bash
# TEST C559 - CORRECTION ROOT CAUSE #67: BATCH CRÉÉ EN DERNIER
# Objectif: Obtenir batch_gtt != 0x0 en créant batch EN DERNIER
# Référence: RAPPORT_C558 - ROOT CAUSE #67 identifié

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
EXEC_NAME="test_c559_batch_last"

mkdir -p "$LOG_DIR"

echo "=== COMPILATION C559 ==="
gcc -o tests/$EXEC_NAME tests/test_c559_batch_last.c -ldrm -I/usr/include/libdrm -Wall -Wextra
echo "✓ Compilation réussie"

echo ""
echo "=== CAPTURE DMESG AVANT ==="
dmesg | tail -20 > "$LOG_DIR/${EXEC_NAME}_dmesg_before_${TIMESTAMP}.log"
echo "✓ dmesg avant capturé"

echo ""
echo "=== EXÉCUTION C559 AVEC STRACE ==="
strace -tt -T -o "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" \
    ./tests/$EXEC_NAME 2>&1 | tee "$LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=== CAPTURE DMESG APRÈS ==="
dmesg | tail -50 > "$LOG_DIR/${EXEC_NAME}_dmesg_after_${TIMESTAMP}.log"
echo "✓ dmesg après capturé"

echo ""
echo "=== ANALYSE LOGS ==="
echo "Logs générés:"
echo "  - $LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log"
echo "  - $LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"
echo "  - $LOG_DIR/${EXEC_NAME}_dmesg_before_${TIMESTAMP}.log"
echo "  - $LOG_DIR/${EXEC_NAME}_dmesg_after_${TIMESTAMP}.log"

echo ""
echo "Analyse strace:"
EINVAL_COUNT=$(grep -c "EINVAL" "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" || true)
EXECBUFFER_COUNT=$(grep -c "DRM_IOCTL_I915_GEM_EXECBUFFER2" "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" || true)
echo "  - EXECBUFFER2 calls: $EXECBUFFER_COUNT"
echo "  - EINVAL errors: $EINVAL_COUNT"

echo ""
echo "Analyse dmesg:"
GPU_HANG=$(grep -c "GPU HANG" "$LOG_DIR/${EXEC_NAME}_dmesg_after_${TIMESTAMP}.log" || true)
CONTEXT_RESET=$(grep -c "context reset" "$LOG_DIR/${EXEC_NAME}_dmesg_after_${TIMESTAMP}.log" || true)
echo "  - GPU HANG: $GPU_HANG"
echo "  - Context reset: $CONTEXT_RESET"

echo ""
echo "=== RÉSULTAT ==="
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ TEST C559 RÉUSSI!"
    echo "✅ ROOT CAUSE #67 RÉSOLU!"
    echo "✅ batch_gtt != 0x0 obtenu"
    echo "✅ GPU a exécuté et écrit résultat"
else
    echo "❌ TEST C559 ÉCHEC (exit code: $EXIT_CODE)"
    if [ $EINVAL_COUNT -gt 0 ]; then
        echo "⚠️  EINVAL détecté ($EINVAL_COUNT occurrences)"
    fi
    if [ $GPU_HANG -gt 0 ]; then
        echo "⚠️  GPU HANG détecté ($GPU_HANG occurrences)"
    fi
fi

echo ""
echo "=== ANALYSE DÉTAILLÉE ==="
echo "Dernières lignes stdout:"
tail -10 "$LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"

echo ""
echo "Dernières lignes dmesg:"
tail -10 "$LOG_DIR/${EXEC_NAME}_dmesg_after_${TIMESTAMP}.log"

exit $EXIT_CODE

# Made with Bob
