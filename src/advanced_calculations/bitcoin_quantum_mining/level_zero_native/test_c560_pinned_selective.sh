#!/bin/bash
# TEST C560 - CORRECTION ROOT CAUSE #68: PINNED SÉLECTIF
# Objectif: Éviter EINVAL avec output_gtt=0x0 et PINNED
# Référence: C559 - ROOT CAUSE #68 identifié

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
EXEC_NAME="test_c560_pinned_selective"

mkdir -p "$LOG_DIR"

echo "=== COMPILATION C560 ==="
gcc -o tests/$EXEC_NAME tests/test_c560_pinned_selective.c -ldrm -I/usr/include/libdrm -Wall -Wextra
echo "✓ Compilation réussie"

echo ""
echo "=== EXÉCUTION C560 AVEC STRACE ==="
strace -tt -T -o "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" \
    ./tests/$EXEC_NAME 2>&1 | tee "$LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=== ANALYSE LOGS ==="
echo "Logs générés:"
echo "  - $LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log"
echo "  - $LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"

echo ""
echo "Analyse strace:"
EINVAL_COUNT=$(grep -c "EINVAL" "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" || true)
EXECBUFFER_COUNT=$(grep -c "DRM_IOCTL_I915_GEM_EXECBUFFER2" "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" || true)
echo "  - EXECBUFFER2 calls: $EXECBUFFER_COUNT"
echo "  - EINVAL errors: $EINVAL_COUNT"

echo ""
echo "=== RÉSULTAT ==="
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ TEST C560 RÉUSSI!"
    echo "✅ ROOT CAUSE #68 RÉSOLU!"
    echo "✅ PINNED sélectif fonctionne"
    echo "✅ GPU a exécuté et écrit résultat"
    echo ""
    echo "🎉🎉🎉 PERCÉE HISTORIQUE! 🎉🎉🎉"
    echo "Infrastructure i915 native FONCTIONNELLE!"
else
    echo "❌ TEST C560 ÉCHEC (exit code: $EXIT_CODE)"
    if [ $EINVAL_COUNT -gt 0 ]; then
        echo "⚠️  EINVAL détecté ($EINVAL_COUNT occurrences)"
    fi
fi

echo ""
echo "=== ANALYSE DÉTAILLÉE ==="
echo "Dernières lignes stdout:"
tail -15 "$LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"

exit $EXIT_CODE

# Made with Bob
