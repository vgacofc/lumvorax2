#!/bin/bash

# TEST C602 - KERNEL MANUEL ULTRA-SIMPLE
# ROOT CAUSE #151: Isoler problème ISA vs infrastructure
# Date: 2026-05-27

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Créer répertoire logs si nécessaire
mkdir -p logs

# Timestamp pour log unique
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="logs/execution_c602_kernel_manual_${TIMESTAMP}.log"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  COMPILATION TEST C602 - KERNEL MANUEL ULTRA-SIMPLE       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Compilation
echo "[COMPILATION]"
echo "Compilation test_c602_kernel_manual_simple.c..."

gcc -o test_c602_kernel_manual \
    tests/test_c602_kernel_manual_simple.c \
    src/batch_generator_auto.c \
    -I./include \
    -I./src \
    -I/usr/include/libdrm \
    -ldrm \
    -Wall -Wextra

if [ $? -eq 0 ]; then
    echo "✓ Compilation réussie"
else
    echo "❌ Échec compilation"
    exit 1
fi

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  EXÉCUTION TEST C602                                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Exécution avec timeout 10s et capture complète
timeout 10s ./test_c602_kernel_manual 2>&1 | tee "$LOG_FILE"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  RÉSULTAT FINAL                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ TEST C602 RÉUSSI"
elif [ $EXIT_CODE -eq 124 ]; then
    echo "⏱️  TIMEOUT (10 secondes)"
else
    echo "❌ TEST C602 ÉCHOUÉ (code: $EXIT_CODE)"
fi

echo ""
echo "Log sauvegardé: $LOG_FILE"
echo ""
echo "Pour analyser les logs:"
echo "  cat $LOG_FILE"
echo ""

exit $EXIT_CODE

# Made with Bob
