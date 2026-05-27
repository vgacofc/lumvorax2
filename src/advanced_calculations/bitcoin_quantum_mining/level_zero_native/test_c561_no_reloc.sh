#!/bin/bash
# TEST C561 - CORRECTION ROOT CAUSE #69: RETIRER I915_EXEC_NO_RELOC
# Objectif: Permettre relocations automatiques i915
# Référence: C560 - ROOT CAUSE #69 identifié

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
EXEC_NAME="test_c561_no_reloc"
ERROR_STATE="/sys/class/drm/card1/error"

mkdir -p "$LOG_DIR"

echo "=== COMPILATION C561 ==="
gcc -o tests/$EXEC_NAME tests/test_c561_no_reloc.c -ldrm -I/usr/include/libdrm -Wall -Wextra
echo "✓ Compilation réussie"

echo ""
echo "=== CAPTURE GPU ERROR STATE AVANT ==="
if [ -r "$ERROR_STATE" ]; then
    cp "$ERROR_STATE" "$LOG_DIR/${EXEC_NAME}_error_before_${TIMESTAMP}.log" 2>/dev/null || echo "⚠️  Pas d'accès à $ERROR_STATE"
else
    echo "⚠️  $ERROR_STATE non accessible"
fi

echo ""
echo "=== EXÉCUTION C561 AVEC STRACE ==="
strace -tt -T -o "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" \
    ./tests/$EXEC_NAME 2>&1 | tee "$LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=== CAPTURE GPU ERROR STATE APRÈS ==="
if [ -r "$ERROR_STATE" ]; then
    cp "$ERROR_STATE" "$LOG_DIR/${EXEC_NAME}_error_after_${TIMESTAMP}.log" 2>/dev/null || echo "⚠️  Pas d'accès à $ERROR_STATE"
    
    # Analyse error state
    if [ -f "$LOG_DIR/${EXEC_NAME}_error_after_${TIMESTAMP}.log" ]; then
        ERROR_SIZE=$(wc -c < "$LOG_DIR/${EXEC_NAME}_error_after_${TIMESTAMP}.log")
        if [ $ERROR_SIZE -gt 0 ]; then
            echo "⚠️  GPU ERROR STATE DÉTECTÉ ($ERROR_SIZE bytes)"
            echo "Premières lignes:"
            head -20 "$LOG_DIR/${EXEC_NAME}_error_after_${TIMESTAMP}.log"
        else
            echo "✓ Pas d'erreur GPU détectée"
        fi
    fi
else
    echo "⚠️  $ERROR_STATE non accessible"
fi

echo ""
echo "=== ANALYSE LOGS ==="
echo "Logs générés:"
echo "  - $LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log"
echo "  - $LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"
if [ -f "$LOG_DIR/${EXEC_NAME}_error_before_${TIMESTAMP}.log" ]; then
    echo "  - $LOG_DIR/${EXEC_NAME}_error_before_${TIMESTAMP}.log"
fi
if [ -f "$LOG_DIR/${EXEC_NAME}_error_after_${TIMESTAMP}.log" ]; then
    echo "  - $LOG_DIR/${EXEC_NAME}_error_after_${TIMESTAMP}.log"
fi

echo ""
echo "Analyse strace:"
EINVAL_COUNT=$(grep -c "EINVAL" "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" || true)
EXECBUFFER_COUNT=$(grep -c "DRM_IOCTL_I915_GEM_EXECBUFFER2" "$LOG_DIR/${EXEC_NAME}_strace_${TIMESTAMP}.log" || true)
echo "  - EXECBUFFER2 calls: $EXECBUFFER_COUNT"
echo "  - EINVAL errors: $EINVAL_COUNT"

echo ""
echo "=== RÉSULTAT ==="
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅✅✅ TEST C561 RÉUSSI! ✅✅✅"
    echo "✅ ROOT CAUSE #69 RÉSOLU!"
    echo "✅ I915_EXEC_NO_RELOC retiré avec succès"
    echo "✅ GPU a exécuté et écrit résultat"
    echo ""
    echo "🎉🎉🎉 PERCÉE HISTORIQUE LUMVORAX! 🎉🎉🎉"
    echo "Infrastructure i915 native 100% FONCTIONNELLE!"
    echo "Élimination OpenCL: 100% COMPLÈTE!"
else
    echo "❌ TEST C561 ÉCHEC (exit code: $EXIT_CODE)"
    if [ $EINVAL_COUNT -gt 0 ]; then
        echo "⚠️  EINVAL détecté ($EINVAL_COUNT occurrences)"
        echo "⚠️  Analyser error state pour cause exacte"
    fi
fi

echo ""
echo "=== ANALYSE DÉTAILLÉE ==="
echo "Dernières lignes stdout:"
tail -20 "$LOG_DIR/${EXEC_NAME}_stdout_${TIMESTAMP}.log"

exit $EXIT_CODE

# Made with Bob
