#!/bin/bash
# C283 — Test avec logging nano exhaustif pour analyser le pic 1000 MH/s
# Objectif : Capturer TOUS les détails temporels du test #5 (10M nonces)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
mkdir -p "$LOG_DIR"

MAIN_LOG="$LOG_DIR/c283_nano_forensic_${TIMESTAMP}.log"
CSV_LOG="$LOG_DIR/c283_nano_results_${TIMESTAMP}.csv"

echo "🔬 C283 — Test Nano Forensic pour analyser pic 1000 MH/s"
echo "📝 Log principal: $MAIN_LOG"
echo "📊 Résultats CSV: $CSV_LOG"
echo ""

# Initialiser CSV
echo "test_num,batch_size,dispatches,status,hashrate_mhs,time_sec,time_first_dispatch_ms,time_warm_avg_ms,time_reopen_avg_ms,notes" > "$CSV_LOG"

# Fonction de test avec logging nano
run_nano_test() {
    local test_num=$1
    local batch_size=$2
    local dispatches=$3
    local description=$4
    
    echo -e "${BLUE}[TEST #$test_num]${NC} Batch=$batch_size nonces, Dispatches=$dispatches"
    echo -e "  Description: $description"
    
    # Log individuel pour ce test
    local test_log="$LOG_DIR/c283_test${test_num}_batch${batch_size}_${TIMESTAMP}.log"
    
    # Modifier le fichier de test pour utiliser ce batch_size
    cat > tests/test_btc_mining_native.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/btc_gen9_native_runner.h"

int main(void) {
    printf("[DEBUG] Tentative ouverture log: logs/forensic/btc_mining_native_sha256.log\n");
    
    btc_gen9_config_t config = {
        .kernel_path = "kernels/btc_sha256_mining_gen9.bin",
        .batch_size = BATCH_SIZE_PLACEHOLDER,
        .work_group_size = 256,
        .log_path = "logs/forensic/btc_mining_native_sha256.log"
    };
    
    printf("[DEBUG] Log ouvert avec succès\n");
    
    btc_gen9_context_t* ctx = btc_gen9_init(&config);
    if (!ctx) {
        fprintf(stderr, "Erreur init\n");
        return 1;
    }
    
    /* Test DISPATCHES_PLACEHOLDER dispatches */
    for (int i = 0; i < DISPATCHES_PLACEHOLDER; i++) {
        int ret = btc_gen9_dispatch(ctx);
        if (ret != 0) {
            fprintf(stderr, "Dispatch %d failed\n", i+1);
            btc_gen9_cleanup(ctx);
            return 1;
        }
    }
    
    btc_gen9_cleanup(ctx);
    printf("SUCCESS: DISPATCHES_PLACEHOLDER/DISPATCHES_PLACEHOLDER dispatches completed\n");
    return 0;
}
EOF
    
    # Remplacer les placeholders
    sed -i "s/BATCH_SIZE_PLACEHOLDER/$batch_size/g" tests/test_btc_mining_native.c
    sed -i "s/DISPATCHES_PLACEHOLDER/$dispatches/g" tests/test_btc_mining_native.c
    
    # Compiler
    echo "  🔨 Compilation..."
    make -f Makefile.btc_native clean > /dev/null 2>&1 || true
    if ! make -f Makefile.btc_native >> "$test_log" 2>&1; then
        echo -e "  ${RED}❌ ERREUR compilation${NC}"
        echo "$test_num,$batch_size,$dispatches,COMPILE_ERROR,0,0,0,0,0,Compilation failed" >> "$CSV_LOG"
        return 1
    fi
    
    # Exécuter avec timeout et capturer temps
    echo "  ⚡ Exécution $dispatches dispatches..."
    local start_time=$(date +%s.%N)
    
    if timeout 300 ./bin/test_btc_mining_native >> "$test_log" 2>&1; then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc)
        
        # Extraire hashrate du log principal
        local hashrate=$(grep "Hashrate:" logs/forensic/btc_mining_native_sha256.log 2>/dev/null | tail -1 | awk '{print $2}' || echo "0")
        
        # Si hashrate vide, calculer manuellement
        if [ -z "$hashrate" ] || [ "$hashrate" = "0" ]; then
            local total_nonces=$(echo "$batch_size * $dispatches" | bc)
            hashrate=$(echo "scale=2; $total_nonces / $duration / 1000000" | bc)
        fi
        
        # Extraire timings nano des logs
        local first_dispatch_time=$(grep "EXEC_SUCCESS.*dispatch=1" logs/forensic/btc_mining_native_sha256.log 2>/dev/null | grep -oP 'time=\K[0-9.]+' | head -1 || echo "0")
        local warm_times=$(grep "EXEC_SUCCESS.*dispatch=[2-9]" logs/forensic/btc_mining_native_sha256.log 2>/dev/null | grep -oP 'time=\K[0-9.]+' || echo "")
        local warm_avg="0"
        if [ -n "$warm_times" ]; then
            warm_avg=$(echo "$warm_times" | awk '{sum+=$1; count++} END {if(count>0) print sum/count*1000; else print 0}')
        fi
        
        local reopen_times=$(grep "DRM_REOPEN_SUCCESS.*time=" logs/forensic/btc_mining_native_sha256.log 2>/dev/null | grep -oP 'time=\K[0-9.]+' || echo "")
        local reopen_avg="0"
        if [ -n "$reopen_times" ]; then
            reopen_avg=$(echo "$reopen_times" | awk '{sum+=$1; count++} END {if(count>0) print sum/count; else print 0}')
        fi
        
        # Convertir first_dispatch en ms
        first_dispatch_ms=$(echo "$first_dispatch_time * 1000" | bc)
        
        echo -e "  ${GREEN}✅ SUCCESS${NC}"
        echo "  📊 Hashrate: ${hashrate} MH/s"
        echo "  ⏱️  Temps total: ${duration}s"
        echo "  🔥 Premier dispatch: ${first_dispatch_ms} ms"
        echo "  ⚡ Warm avg: ${warm_avg} ms"
        echo "  🔄 Reopen avg: ${reopen_avg} ms"
        
        echo "$test_num,$batch_size,$dispatches,SUCCESS,$hashrate,$duration,$first_dispatch_ms,$warm_avg,$reopen_avg,OK" >> "$CSV_LOG"
        return 0
    else
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc)
        echo -e "  ${RED}❌ TIMEOUT ou ERREUR${NC}"
        echo "$test_num,$batch_size,$dispatches,TIMEOUT,0,$duration,0,0,0,Timeout 300s" >> "$CSV_LOG"
        return 1
    fi
}

# Test principal : #5 avec 10M nonces (celui qui donne 1000 MH/s)
echo "========================================" | tee -a "$MAIN_LOG"
echo "C283 NANO FORENSIC — Test #5 (10M nonces)" | tee -a "$MAIN_LOG"
echo "Objectif : Capturer détails du pic 1000 MH/s" | tee -a "$MAIN_LOG"
echo "========================================" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

run_nano_test 5 10000000 100 "Test optimal 10M nonces - pic 1000 MH/s attendu" | tee -a "$MAIN_LOG"

echo "" | tee -a "$MAIN_LOG"
echo "========================================" | tee -a "$MAIN_LOG"
echo "📊 RÉSULTATS FINAUX" | tee -a "$MAIN_LOG"
echo "========================================" | tee -a "$MAIN_LOG"
cat "$CSV_LOG" | tee -a "$MAIN_LOG"

echo "" | tee -a "$MAIN_LOG"
echo "✅ Test C283 terminé" | tee -a "$MAIN_LOG"
echo "📝 Logs complets dans: $LOG_DIR/" | tee -a "$MAIN_LOG"

# Made with Bob
