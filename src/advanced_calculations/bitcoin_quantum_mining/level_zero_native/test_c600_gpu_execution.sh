#!/bin/bash

# TEST C600 - VALIDATION GPU HARDWARE RÉELLE
# Compilation et exécution du premier test GPU natif i915
# Date: 2026-05-26
# Cycle: C600

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  COMPILATION TEST C600 - GPU HARDWARE RÉEL                 ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Répertoires
SRC_DIR="src"
TEST_DIR="tests"
LOG_DIR="logs"
FORENSIC_DIR="$LOG_DIR/forensic"

# Créer répertoires si nécessaire
mkdir -p "$LOG_DIR"
mkdir -p "$FORENSIC_DIR"

# Timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="$LOG_DIR/execution_c600_gpu_test_${TIMESTAMP}.log"

echo "[COMPILATION]"
echo "Compilation test_c600_gpu_execution.c..."

gcc -o test_c600_gpu_execution \
    "$TEST_DIR/test_c600_gpu_execution.c" \
    "$SRC_DIR/batch_generator_auto.c" \
    -I"$SRC_DIR" \
    -I"$TEST_DIR" \
    -I/usr/include/drm \
    -I/usr/include/libdrm \
    -ldrm \
    -Wall -Wextra \
    -g \
    -O2

if [ $? -eq 0 ]; then
    echo "✓ Compilation réussie"
else
    echo "❌ Échec compilation"
    exit 1
fi

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  EXÉCUTION TEST C600                                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Exécuter avec logging complet
./test_c600_gpu_execution 2>&1 | tee "$LOG_FILE"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  RÉSULTAT FINAL                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ TEST C600 RÉUSSI!"
    echo "🎉 PREMIER RÉSULTAT VALIDE GPU NATIF i915!"
    echo ""
    echo "Log sauvegardé: $LOG_FILE"
elif [ $EXIT_CODE -eq 1 ]; then
    echo "⚠️  TEST C600 PARTIEL"
    echo "GPU a exécuté mais résultat différent de l'attendu"
    echo ""
    echo "Log sauvegardé: $LOG_FILE"
else
    echo "❌ TEST C600 ÉCHOUÉ"
    echo "Code erreur: $EXIT_CODE"
    echo ""
    echo "Log sauvegardé: $LOG_FILE"
fi

echo ""
echo "Pour analyser les logs:"
echo "  cat $LOG_FILE"
echo ""

exit $EXIT_CODE

# Made with Bob
