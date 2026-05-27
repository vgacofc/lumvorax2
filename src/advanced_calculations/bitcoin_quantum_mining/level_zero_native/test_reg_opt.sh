#!/bin/bash

# Script de test du kernel optimisé registres C255V14
# Compare performance: kernel original vs kernel optimisé

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Test Performance Kernel Optimisé - C255V14               ║"
echo "║  Objectif: 697ms → <450ms (sous timeout 650ms)            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Vérifier que les binaires existent
if [ ! -f "kernels/btc_sha256_gen9.bin" ]; then
    echo "❌ Erreur: kernel original introuvable"
    exit 1
fi

if [ ! -f "kernels/btc_sha256_reg_opt_gen9.bin" ]; then
    echo "❌ Erreur: kernel optimisé introuvable"
    exit 1
fi

# Créer répertoire de logs
LOG_DIR="logs/test_reg_opt_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$LOG_DIR"

echo "📁 Logs: $LOG_DIR"
echo ""

# Test 1: Kernel ORIGINAL (baseline)
echo "═══════════════════════════════════════════════════════════"
echo "TEST 1: Kernel ORIGINAL (baseline)"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Sauvegarder le kernel actuel
if [ -f "kernels/btc_sha256_current.bin" ]; then
    cp kernels/btc_sha256_current.bin kernels/btc_sha256_current.bin.backup
fi

# Utiliser le kernel original
cp kernels/btc_sha256_gen9.bin kernels/btc_sha256_current.bin

echo "🔧 Compilation avec kernel original..."
make clean > /dev/null 2>&1 || true
if ! make -j$(nproc) > "$LOG_DIR/build_original.log" 2>&1; then
    echo "❌ Erreur compilation kernel original"
    cat "$LOG_DIR/build_original.log"
    exit 1
fi
echo "✅ Compilation réussie"
echo ""

echo "🚀 Exécution test (10 dispatches)..."
if sudo -S <<< "emmaus" ./lum_btc_miner --dispatches 10 --log-level 2 \
    > "$LOG_DIR/run_original.log" 2>&1; then
    echo "✅ Test réussi"
else
    echo "⚠️  Test terminé avec erreurs (voir logs)"
fi

# Extraire métriques
ORIG_TIME=$(grep "Temps dispatch" "$LOG_DIR/run_original.log" | tail -1 | awk '{print $3}' | sed 's/ms//')
ORIG_HASH=$(grep "Total hashes" "$LOG_DIR/run_original.log" | tail -1 | awk '{print $3}')
ORIG_RATE=$(grep "Hashrate" "$LOG_DIR/run_original.log" | tail -1 | awk '{print $2}')

echo ""
echo "📊 Résultats ORIGINAL:"
echo "   - Temps/dispatch: ${ORIG_TIME:-N/A} ms"
echo "   - Total hashes: ${ORIG_HASH:-N/A}"
echo "   - Hashrate: ${ORIG_RATE:-N/A} MH/s"
echo ""

# Test 2: Kernel OPTIMISÉ
echo "═══════════════════════════════════════════════════════════"
echo "TEST 2: Kernel OPTIMISÉ (C255V14)"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Utiliser le kernel optimisé
cp kernels/btc_sha256_reg_opt_gen9.bin kernels/btc_sha256_current.bin

echo "🔧 Compilation avec kernel optimisé..."
make clean > /dev/null 2>&1 || true
if ! make -j$(nproc) > "$LOG_DIR/build_optimized.log" 2>&1; then
    echo "❌ Erreur compilation kernel optimisé"
    cat "$LOG_DIR/build_optimized.log"
    exit 1
fi
echo "✅ Compilation réussie"
echo ""

echo "🚀 Exécution test (10 dispatches)..."
if sudo -S <<< "emmaus" ./lum_btc_miner --dispatches 10 --log-level 2 \
    > "$LOG_DIR/run_optimized.log" 2>&1; then
    echo "✅ Test réussi"
else
    echo "⚠️  Test terminé avec erreurs (voir logs)"
fi

# Extraire métriques
OPT_TIME=$(grep "Temps dispatch" "$LOG_DIR/run_optimized.log" | tail -1 | awk '{print $3}' | sed 's/ms//')
OPT_HASH=$(grep "Total hashes" "$LOG_DIR/run_optimized.log" | tail -1 | awk '{print $3}')
OPT_RATE=$(grep "Hashrate" "$LOG_DIR/run_optimized.log" | tail -1 | awk '{print $2}')

echo ""
echo "📊 Résultats OPTIMISÉ:"
echo "   - Temps/dispatch: ${OPT_TIME:-N/A} ms"
echo "   - Total hashes: ${OPT_HASH:-N/A}"
echo "   - Hashrate: ${OPT_RATE:-N/A} MH/s"
echo ""

# Comparaison
echo "═══════════════════════════════════════════════════════════"
echo "COMPARAISON FINALE"
echo "═══════════════════════════════════════════════════════════"
echo ""

if [ -n "$ORIG_TIME" ] && [ -n "$OPT_TIME" ]; then
    SPEEDUP=$(echo "scale=2; $ORIG_TIME / $OPT_TIME" | bc)
    REDUCTION=$(echo "scale=1; ($ORIG_TIME - $OPT_TIME) * 100 / $ORIG_TIME" | bc)
    
    echo "⏱️  TEMPS D'EXÉCUTION:"
    echo "   Original:  ${ORIG_TIME} ms"
    echo "   Optimisé:  ${OPT_TIME} ms"
    echo "   Réduction: ${REDUCTION}%"
    echo "   Speedup:   ${SPEEDUP}x"
    echo ""
    
    # Vérifier si objectif atteint
    if (( $(echo "$OPT_TIME < 450" | bc -l) )); then
        echo "✅ OBJECTIF ATTEINT: ${OPT_TIME}ms < 450ms (timeout 650ms)"
    else
        echo "❌ OBJECTIF NON ATTEINT: ${OPT_TIME}ms >= 450ms"
        echo "   Marge restante: $(echo "$OPT_TIME - 450" | bc)ms"
    fi
else
    echo "⚠️  Impossible de comparer (métriques manquantes)"
fi

echo ""
echo "📁 Logs complets: $LOG_DIR"
echo ""

# Restaurer kernel original
if [ -f "kernels/btc_sha256_current.bin.backup" ]; then
    mv kernels/btc_sha256_current.bin.backup kernels/btc_sha256_current.bin
fi

echo "✅ Test terminé"

# Made with Bob
