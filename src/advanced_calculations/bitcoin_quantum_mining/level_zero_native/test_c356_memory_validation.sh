#!/bin/bash
# Test C356 - Validation Mémoire GPU Ultra Simple
# Objectif: Vérifier que le GPU peut écrire 0xCAFEBABE dans output[0]

set -e

cd "$(dirname "$0")"

echo "=== TEST C356 - VALIDATION MÉMOIRE GPU ==="
echo "Kernel: tests/test_c356_memory_write.bin_kbl.bin"
echo "Objectif: Vérifier écriture GPU de valeurs magiques"
echo ""

# Copier le kernel vers l'emplacement attendu par le runner
cp tests/test_c356_memory_write.bin_kbl.bin kernels/btc_sha256_gen9.bin_kbl.bin

echo "✓ Kernel copié vers kernels/btc_sha256_gen9.bin_kbl.bin"
echo ""

# Créer un block header Bitcoin factice (80 bytes)
# Le kernel ne l'utilise pas, mais le runner l'attend
BLOCK_HEADER="0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a29ab5f49ffff001d1dac2b7c"

echo "Exécution test avec 256 work-items (1 work-group)..."
echo ""

# Exécuter avec timeout 5s
timeout 5s ./test_btc_mining_native <<EOF || true
$BLOCK_HEADER
256
EOF

echo ""
echo "=== ANALYSE RÉSULTATS ==="
echo ""

# Lire les derniers logs forensiques
if [ -f "logs/forensic/btc_mining_native_sha256.log" ]; then
    echo "Dernières lignes du log forensique:"
    tail -50 logs/forensic/btc_mining_native_sha256.log | grep -E "(OUTPUT|EXEC_SUCCESS|memory_changed|C325)" || true
    echo ""
    
    # Vérifier si 0xCAFEBABE apparaît
    if grep -q "CAFEBABE" logs/forensic/btc_mining_native_sha256.log; then
        echo "✅ SUCCESS: 0xCAFEBABE détecté dans les logs!"
        echo "   → Pipeline mémoire GPU fonctionne correctement"
    elif grep -q "output\[0\]=0" logs/forensic/btc_mining_native_sha256.log; then
        echo "❌ FAIL: output[0]=0 (pas 0xCAFEBABE)"
        echo "   → Problème pipeline/surface/binding"
    else
        echo "⚠️  UNKNOWN: Impossible de déterminer output[0]"
    fi
else
    echo "❌ Pas de log forensique trouvé"
fi

echo ""
echo "=== FIN TEST C356 ==="

# Made with Bob
