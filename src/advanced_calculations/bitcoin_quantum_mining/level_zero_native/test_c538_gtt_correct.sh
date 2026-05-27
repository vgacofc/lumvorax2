#!/bin/bash
# TEST C538 - Compilation et exécution avec adresses GTT correctes

set -e

echo "=== TEST C538 - 2-PASS AVEC ADRESSES GTT CORRECTES ==="
echo ""

cd "$(dirname "$0")"

# Compiler le test
echo "Compilation test_c538_gtt_addresses.c..."
gcc -o test_c538_gtt_addresses \
    tests/test_c538_gtt_addresses.c \
    -I/usr/include/libdrm \
    -ldrm \
    -Wall -Wextra \
    || { echo "❌ Erreur compilation"; exit 1; }

echo "✓ Compilation réussie"
echo ""

# Exécuter
echo "Exécution test C538..."
echo ""

./test_c538_gtt_addresses 2>&1 | tee logs/test_c538_gtt_addresses.log

echo ""
echo "=== RÉSULTAT ==="

# Vérifier le résultat
if grep -q "✓✓✓ SUCCÈS! output\[0\] = 0x12345678" logs/test_c538_gtt_addresses.log; then
    echo ""
    echo "🎉🎉🎉 VICTOIRE TOTALE! 🎉🎉🎉"
    echo ""
    echo "✓ GPU NATIF i915 FONCTIONNE"
    echo "✓ output[0] = 0x12345678"
    echo "✓ ROOT CAUSE #43 RÉSOLU"
    echo "✓ OpenCL éliminé à 100%"
    echo ""
    exit 0
else
    echo ""
    echo "❌ Test échoué - Analyse nécessaire"
    echo ""
    echo "Dernières lignes du log:"
    tail -n 20 logs/test_c538_gtt_addresses.log
    echo ""
    exit 1
fi

# Made with Bob