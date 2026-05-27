#!/bin/bash
# TEST C372 - VALIDATION MINING AVEC KERNEL CORRIGÉ BUG #5
# Exécution immédiate avec forensics maximum

set -e

cd "$(dirname "$0")"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  TEST C372 - VALIDATION KERNEL C371 CORRIGÉ BUG #5        ║"
echo "║  hash(nonce=0) != hash(nonce=1) ?                          ║"
echo "║  near_miss_gpu ≥ 1 bit ?                                   ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Vérifier que le kernel corrigé existe
if [ ! -f "kernels/btc_sha256_c371_fixed.bin_kbl.bin_kbl.bin" ]; then
    echo "❌ Kernel C371 non trouvé"
    exit 1
fi

KERNEL_SIZE=$(stat -c%s kernels/btc_sha256_c371_fixed.bin_kbl.bin_kbl.bin)
echo "[1/4] Kernel C371 corrigé: ${KERNEL_SIZE} bytes"
echo ""

# Compiler le runner si nécessaire
if [ ! -f "bin/test_gen9_native" ] || [ "src/btc_gen9_native_runner.c" -nt "bin/test_gen9_native" ]; then
    echo "[2/4] Compilation runner..."
    make -f Makefile.gen9_native clean > /dev/null 2>&1
    make -f Makefile.gen9_native > /dev/null 2>&1
    echo "✅ Runner compilé"
else
    echo "[2/4] Runner déjà compilé"
fi
echo ""

# Créer répertoire logs si nécessaire
mkdir -p logs/forensic

# Exécuter avec kernel C371 corrigé
echo "[3/4] Exécution mining avec kernel C371 (10 secondes)..."
echo "Kernel: kernels/btc_sha256_c371_fixed.bin_kbl.bin_kbl.bin"
echo "Target bits: 1 (50% probabilité hit)"
echo "Forensics: MAXIMUM"
echo ""

# Lancer l'exécution
timeout 15 bin/test_gen9_native kernels/btc_sha256_c371_fixed.bin_kbl.bin_kbl.bin 10 2>&1 | tee logs/execution_c372_validation_$(date +%Y%m%d_%H%M%S).log

echo ""
echo "[4/4] Analyse résultats..."
echo ""

# Analyser les logs forensiques
if [ -f "logs/forensic/btc_mining_native_sha256.log" ]; then
    echo "📊 Analyse forensique:"
    echo ""
    
    # Chercher near_miss
    NEAR_MISS=$(tail -100 logs/forensic/btc_mining_native_sha256.log | grep -o "leading_zeros=[0-9]*" | tail -1 | cut -d= -f2)
    
    if [ -n "$NEAR_MISS" ] && [ "$NEAR_MISS" -gt 0 ]; then
        echo "✅ SUCCESS: near_miss_gpu = $NEAR_MISS bits"
        echo "✅ Le nonce affecte le hash !"
        echo "✅ Mining réel activé !"
    else
        echo "❌ FAILED: near_miss_gpu = 0 bits"
        echo "❌ Le bug persiste ou nouveau bug introduit"
    fi
    
    echo ""
    echo "Logs complets: logs/forensic/btc_mining_native_sha256.log"
else
    echo "⚠️  Logs forensiques non trouvés"
fi

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  TEST C372 TERMINÉ                                         ║"
echo "╚════════════════════════════════════════════════════════════╝"

# Made with Bob
