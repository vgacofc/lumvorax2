#!/bin/bash

# Test de performance: Kernel Original vs Kernel Optimisé
# Utilise test_c255v13 (version stable avec réouverture périodique)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Test Performance Kernel - C255V14                        ║"
echo "║  Original vs Optimisé Registres                           ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Créer répertoire de logs
LOG_DIR="logs/perf_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$LOG_DIR"

echo "📁 Logs: $LOG_DIR"
echo ""

# Fonction pour extraire les métriques
extract_metrics() {
    local log_file="$1"
    local prefix="$2"
    
    # Extraire temps moyen (en ms)
    local avg_time=$(grep "✅ Succès:" "$log_file" | \
        awk '{print $3}' | \
        awk '{sum+=$1; count++} END {if(count>0) printf "%.0f", sum*1000/count; else print "N/A"}')
    
    # Extraire hashrate moyen
    local avg_hashrate=$(grep "✅ Succès:" "$log_file" | \
        awk '{print $5}' | \
        awk '{sum+=$1; count++} END {if(count>0) printf "%.2f", sum/count; else print "N/A"}')
    
    # Compter succès/échecs
    local success=$(grep -c "✅ Succès:" "$log_file" || echo "0")
    local total=$(grep -cE "(✅ Succès:|❌ Échec)" "$log_file" || echo "0")
    
    echo "${prefix}_TIME=$avg_time"
    echo "${prefix}_HASHRATE=$avg_hashrate"
    echo "${prefix}_SUCCESS=$success"
    echo "${prefix}_TOTAL=$total"
}

# Test 1: Kernel ORIGINAL
echo "═══════════════════════════════════════════════════════════"
echo "TEST 1: Kernel ORIGINAL (baseline)"
echo "═══════════════════════════════════════════════════════════"
echo ""

cp kernels/btc_sha256_gen9.bin kernels/btc_sha256_current.bin
echo "🔧 Kernel: btc_sha256_gen9.bin (44 KB)"
echo "🚀 Exécution: 10 dispatches..."
echo ""

if sudo -S <<< "emmaus" ./test_c255v13 10 > "$LOG_DIR/original.log" 2>&1; then
    echo "✅ Test terminé"
else
    echo "⚠️  Test terminé avec code: $?"
fi

# Extraire métriques
eval $(extract_metrics "$LOG_DIR/original.log" "ORIG")

echo ""
echo "📊 Résultats ORIGINAL:"
echo "   - Temps moyen: ${ORIG_TIME} ms"
echo "   - Hashrate moyen: ${ORIG_HASHRATE} MH/s"
echo "   - Succès: ${ORIG_SUCCESS}/${ORIG_TOTAL}"
echo ""

# Test 2: Kernel OPTIMISÉ
echo "═══════════════════════════════════════════════════════════"
echo "TEST 2: Kernel OPTIMISÉ (C255V14 - registres réduits)"
echo "═══════════════════════════════════════════════════════════"
echo ""

cp kernels/btc_sha256_reg_opt_gen9.bin kernels/btc_sha256_current.bin
echo "🔧 Kernel: btc_sha256_reg_opt_gen9.bin (63 KB)"
echo "🚀 Exécution: 10 dispatches..."
echo ""

if sudo -S <<< "emmaus" ./test_c255v13 10 > "$LOG_DIR/optimized.log" 2>&1; then
    echo "✅ Test terminé"
else
    echo "⚠️  Test terminé avec code: $?"
fi

# Extraire métriques
eval $(extract_metrics "$LOG_DIR/optimized.log" "OPT")

echo ""
echo "📊 Résultats OPTIMISÉ:"
echo "   - Temps moyen: ${OPT_TIME} ms"
echo "   - Hashrate moyen: ${OPT_HASHRATE} MH/s"
echo "   - Succès: ${OPT_SUCCESS}/${OPT_TOTAL}"
echo ""

# Comparaison
echo "═══════════════════════════════════════════════════════════"
echo "COMPARAISON FINALE"
echo "═══════════════════════════════════════════════════════════"
echo ""

if [ "$ORIG_TIME" != "N/A" ] && [ "$OPT_TIME" != "N/A" ] && [ "$ORIG_TIME" -gt 0 ] && [ "$OPT_TIME" -gt 0 ]; then
    SPEEDUP=$(echo "scale=2; $ORIG_TIME / $OPT_TIME" | bc)
    REDUCTION=$(echo "scale=1; ($ORIG_TIME - $OPT_TIME) * 100 / $ORIG_TIME" | bc)
    
    echo "⏱️  TEMPS D'EXÉCUTION:"
    echo "   Original:  ${ORIG_TIME} ms"
    echo "   Optimisé:  ${OPT_TIME} ms"
    
    if [ "$OPT_TIME" -lt "$ORIG_TIME" ]; then
        echo "   Réduction: ${REDUCTION}% ⬇️"
        echo "   Speedup:   ${SPEEDUP}x 🚀"
    else
        INCREASE=$(echo "scale=1; ($OPT_TIME - $ORIG_TIME) * 100 / $ORIG_TIME" | bc)
        echo "   Augmentation: ${INCREASE}% ⬆️"
        echo "   Slowdown: ${SPEEDUP}x ⚠️"
    fi
    echo ""
    
    # Vérifier objectif
    if [ "$OPT_TIME" -lt 450 ]; then
        echo "✅ OBJECTIF ATTEINT: ${OPT_TIME}ms < 450ms"
        echo "   Marge sécurité: $(echo "650 - $OPT_TIME" | bc)ms avant timeout"
    elif [ "$OPT_TIME" -lt 650 ]; then
        echo "⚠️  SOUS TIMEOUT mais AU-DESSUS OBJECTIF:"
        echo "   ${OPT_TIME}ms < 650ms (timeout) mais >= 450ms (objectif)"
        echo "   Marge: $(echo "650 - $OPT_TIME" | bc)ms avant timeout"
    else
        echo "❌ AU-DESSUS DU TIMEOUT: ${OPT_TIME}ms >= 650ms"
    fi
else
    echo "⚠️  Impossible de comparer (métriques manquantes)"
    echo "   ORIG_TIME=$ORIG_TIME"
    echo "   OPT_TIME=$OPT_TIME"
fi

echo ""
echo "💾 HASHRATE:"
if [ "$ORIG_HASHRATE" != "N/A" ] && [ "$OPT_HASHRATE" != "N/A" ]; then
    echo "   Original:  ${ORIG_HASHRATE} MH/s"
    echo "   Optimisé:  ${OPT_HASHRATE} MH/s"
    
    HASH_DIFF=$(echo "scale=2; $OPT_HASHRATE - $ORIG_HASHRATE" | bc)
    if (( $(echo "$HASH_DIFF > 0" | bc -l) )); then
        echo "   Gain: +${HASH_DIFF} MH/s 📈"
    else
        echo "   Perte: ${HASH_DIFF} MH/s 📉"
    fi
else
    echo "   Original:  ${ORIG_HASHRATE} MH/s"
    echo "   Optimisé:  ${OPT_HASHRATE} MH/s"
fi

echo ""
echo "📁 Logs détaillés: $LOG_DIR"
echo "   - original.log"
echo "   - optimized.log"
echo ""

# Restaurer kernel original
cp kernels/btc_sha256_gen9.bin kernels/btc_sha256_current.bin

echo "✅ Test terminé"

# Made with Bob
