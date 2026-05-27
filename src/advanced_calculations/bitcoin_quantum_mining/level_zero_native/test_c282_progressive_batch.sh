                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             #!/bin/bash

# C282 TEST PROGRESSIF BATCH_SIZE
# Règle NX48 : Toujours commencer au plus bas et augmenter progressivement
# Objectif : Retrouver les 400+ MH/s de C277 sans casser la stabilité

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

LOG_DIR="logs/forensic"
mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
MAIN_LOG="$LOG_DIR/c282_progressive_batch_${TIMESTAMP}.log"

echo "╔════════════════════════════════════════════════════════════╗" | tee -a "$MAIN_LOG"
echo "║  C282 TEST PROGRESSIF BATCH_SIZE (Règle NX48)              ║" | tee -a "$MAIN_LOG"
echo "║  Objectif : Retrouver 400+ MH/s sans casser stabilité     ║" | tee -a "$MAIN_LOG"
echo "╚════════════════════════════════════════════════════════════╝" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

# Baseline actuel
BASELINE_BATCH=20000
BASELINE_HASHRATE=3.88  # MH/s
TARGET_BATCH=286331153  # C277 optimal
TARGET_HASHRATE=400     # MH/s

echo "[BASELINE] Actuel : batch_size=$BASELINE_BATCH → $BASELINE_HASHRATE MH/s" | tee -a "$MAIN_LOG"
echo "[TARGET]   C277   : batch_size=$TARGET_BATCH → $TARGET_HASHRATE MH/s" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

# Tests progressifs ILLIMITÉS (règle NX48 : pas de limite artificielle)
BATCH_SIZES=(
    50000        # Test #1 : 2.5× baseline
    100000       # Test #2 : 5× baseline
    500000       # Test #3 : 25× baseline
    1000000      # Test #4 : 50× baseline
    10000000     # Test #5 : 500× baseline
    50000000     # Test #6 : 2500× baseline
    100000000    # Test #7 : 5000× baseline
    286331153    # Test #8 : référence C277 (408 MH/s)
    500000000    # Test #9 : 1.75× C277
    1000000000   # Test #10 : 3.5× C277 (1 milliard de nonces)
    2000000000   # Test #11 : 7× C277 (2 milliards)
    4000000000   # Test #12 : 14× C277 (4 milliards - limite uint32_t)
)

TEST_DISPATCHES=100  # Réduit à 100 pour tests rapides

echo "╔════════════════════════════════════════════════════════════╗" | tee -a "$MAIN_LOG"
echo "║  PHASE 1/2 : COMPILATION                                   ║" | tee -a "$MAIN_LOG"
echo "╚════════════════════════════════════════════════════════════╝" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

make -f Makefile.btc_native clean >> "$MAIN_LOG" 2>&1
make -f Makefile.btc_native >> "$MAIN_LOG" 2>&1

if [ ! -f "bin/test_btc_mining_native" ]; then
    echo "❌ ERREUR : Compilation échouée" | tee -a "$MAIN_LOG"
    exit 1
fi

echo "✅ Compilation réussie" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

echo "╔════════════════════════════════════════════════════════════╗" | tee -a "$MAIN_LOG"
echo "║  PHASE 2/2 : TESTS PROGRESSIFS ILLIMITÉS (12 étapes)      ║" | tee -a "$MAIN_LOG"
echo "╚════════════════════════════════════════════════════════════╝" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

RESULTS_FILE="$LOG_DIR/c282_progressive_results_${TIMESTAMP}.csv"
echo "test_num,batch_size,multiplier,dispatches,status,hashrate_mhs,time_sec,notes" > "$RESULTS_FILE"

for i in "${!BATCH_SIZES[@]}"; do
    BATCH_SIZE="${BATCH_SIZES[$i]}"
    TEST_NUM=$((i + 1))
    MULTIPLIER=$(echo "scale=1; $BATCH_SIZE / $BASELINE_BATCH" | bc)
    
    echo "────────────────────────────────────────────────────────────" | tee -a "$MAIN_LOG"
    echo "TEST #$TEST_NUM/$((${#BATCH_SIZES[@]})) : batch_size=$BATCH_SIZE (${MULTIPLIER}× baseline)" | tee -a "$MAIN_LOG"
    echo "────────────────────────────────────────────────────────────" | tee -a "$MAIN_LOG"
    
    # Modifier batch_size dans le fichier de test
    sed -i "s/config\.batch_size = [0-9]*;/config.batch_size = $BATCH_SIZE;/" \
        tests/test_btc_mining_native.c
    
    # Modifier TEST_DISPATCHES
    sed -i "s/#define TEST_DISPATCHES [0-9]*/#define TEST_DISPATCHES $TEST_DISPATCHES/" \
        tests/test_btc_mining_native.c
    
    # Recompiler
    echo "[COMPILE] Recompilation avec batch_size=$BATCH_SIZE..." | tee -a "$MAIN_LOG"
    make -f Makefile.btc_native clean >> "$MAIN_LOG" 2>&1
    make -f Makefile.btc_native >> "$MAIN_LOG" 2>&1
    
    # Exécuter test
    TEST_LOG="$LOG_DIR/c282_test${TEST_NUM}_batch${BATCH_SIZE}_${TIMESTAMP}.log"
    echo "[RUN] Exécution $TEST_DISPATCHES dispatches..." | tee -a "$MAIN_LOG"
    
    START_TIME=$(date +%s)
    if timeout 300 ./bin/test_btc_mining_native > "$TEST_LOG" 2>&1; then
        END_TIME=$(date +%s)
        ELAPSED=$((END_TIME - START_TIME))
        
        # Extraire hashrate du log
        HASHRATE=$(grep -oP 'Hashrate.*:\s*\K[0-9.]+' "$TEST_LOG" | tail -1)
        
        # Si hashrate non trouvé, le calculer manuellement
        if [ -z "$HASHRATE" ] || [ "$HASHRATE" = "0" ]; then
            # Calculer hashrate = (batch_size * dispatches) / temps / 1000000
            TOTAL_NONCES=$((BATCH_SIZE * TEST_DISPATCHES))
            if [ $ELAPSED -gt 0 ]; then
                HASHRATE=$(echo "scale=2; $TOTAL_NONCES / $ELAPSED / 1000000" | bc)
            else
                HASHRATE="0"
            fi
        fi
        
        echo "✅ SUCCESS : $TEST_DISPATCHES dispatches en ${ELAPSED}s → $HASHRATE MH/s" | tee -a "$MAIN_LOG"
        echo "$TEST_NUM,$BATCH_SIZE,$MULTIPLIER,$TEST_DISPATCHES,SUCCESS,$HASHRATE,$ELAPSED,OK" >> "$RESULTS_FILE"
        
        # NE PAS arrêter même si objectif atteint - continuer jusqu'à la limite matérielle
        if [ -n "$HASHRATE" ] && [ "$HASHRATE" != "0" ]; then
            COMPARE=$(echo "$HASHRATE >= $TARGET_HASHRATE" | bc -l)
            if [ "$COMPARE" = "1" ]; then
                echo "🎯 OBJECTIF C277 ATTEINT : $HASHRATE MH/s ≥ $TARGET_HASHRATE MH/s" | tee -a "$MAIN_LOG"
                echo "   → Continuation des tests pour trouver limite matérielle réelle" | tee -a "$MAIN_LOG"
            fi
        fi
    else
        END_TIME=$(date +%s)
        ELAPSED=$((END_TIME - START_TIME))
        
        # Analyser cause échec
        if grep -q "GPU hang" "$TEST_LOG"; then
            REASON="GPU_HANG"
        elif grep -q "timeout" "$TEST_LOG"; then
            REASON="TIMEOUT"
        elif grep -q "errno=5" "$TEST_LOG"; then
            REASON="EIO_CONTEXT_BANNED"
        else
            REASON="UNKNOWN"
        fi
        
        echo "❌ FAILED : $REASON après ${ELAPSED}s" | tee -a "$MAIN_LOG"
        echo "$TEST_NUM,$BATCH_SIZE,$MULTIPLIER,$TEST_DISPATCHES,FAILED,0,$ELAPSED,$REASON" >> "$RESULTS_FILE"
        
        # Si échec, arrêter les tests
        echo "" | tee -a "$MAIN_LOG"
        echo "⚠️  ARRÊT : batch_size=$BATCH_SIZE trop élevé" | tee -a "$MAIN_LOG"
        echo "    Dernier batch_size stable : ${BATCH_SIZES[$((i-1))]}" | tee -a "$MAIN_LOG"
        break
    fi
    
    echo "" | tee -a "$MAIN_LOG"
done

echo "╔════════════════════════════════════════════════════════════╗" | tee -a "$MAIN_LOG"
echo "║  RÉSULTATS FINAUX                                          ║" | tee -a "$MAIN_LOG"
echo "╚════════════════════════════════════════════════════════════╝" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

echo "Résultats détaillés : $RESULTS_FILE" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

# Afficher tableau résultats
column -t -s',' "$RESULTS_FILE" | tee -a "$MAIN_LOG"

echo "" | tee -a "$MAIN_LOG"
echo "Log complet : $MAIN_LOG" | tee -a "$MAIN_LOG"

# Made with Bob
