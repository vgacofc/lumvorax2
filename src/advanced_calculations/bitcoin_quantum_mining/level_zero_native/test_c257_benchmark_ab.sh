#!/bin/bash
# Benchmark A/B: Kernel actuel vs C257 optimisé
# Date: 2026-05-17

set -e

LOG_DIR="logs/benchmark_c257"
mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="$LOG_DIR/benchmark_ab_${TIMESTAMP}.log"

echo "╔════════════════════════════════════════════════════════════╗" | tee "$LOG_FILE"
echo "║  BENCHMARK A/B — Kernel Actuel vs C257 Optimisé           ║" | tee -a "$LOG_FILE"
echo "║  Date: $(date '+%Y-%m-%d %H:%M:%S')                        ║" | tee -a "$LOG_FILE"
echo "╚════════════════════════════════════════════════════════════╝" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Test A: Kernel actuel (btc_sha256_gen9.bin)
echo "[TEST A] Kernel actuel: btc_sha256_gen9.bin" | tee -a "$LOG_FILE"
echo "─────────────────────────────────────────────────────────────" | tee -a "$LOG_FILE"

# Backup kernel actuel
cp kernels/btc_sha256_gen9.bin kernels/btc_sha256_gen9.bin.backup 2>/dev/null || true

# Exécuter test avec kernel actuel
echo "[A] Exécution avec kernel actuel..." | tee -a "$LOG_FILE"
./test_c255v11_nx48_NEW 2>&1 | tee "$LOG_DIR/test_a_kernel_actuel_${TIMESTAMP}.log"

# Extraire métriques Test A
HASHRATE_A=$(grep "hashrate=" "$LOG_DIR/test_a_kernel_actuel_${TIMESTAMP}.log" | tail -1 | grep -oP '\d+\.\d+(?= MH/s)' || echo "0")
TIME_A=$(grep "Temps total:" "$LOG_DIR/test_a_kernel_actuel_${TIMESTAMP}.log" | grep -oP '\d+\.\d+(?= sec)' || echo "0")

echo "" | tee -a "$LOG_FILE"
echo "[A] Résultats Kernel Actuel:" | tee -a "$LOG_FILE"
echo "  - Hashrate: ${HASHRATE_A} MH/s" | tee -a "$LOG_FILE"
echo "  - Temps: ${TIME_A} sec" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Test B: Kernel C257 optimisé
echo "[TEST B] Kernel C257: btc_sha256_mining_c257.bin" | tee -a "$LOG_FILE"
echo "─────────────────────────────────────────────────────────────" | tee -a "$LOG_FILE"

# Remplacer par kernel C257
cp kernels/btc_sha256_mining_c257.bin kernels/btc_sha256_gen9.bin

echo "[B] Exécution avec kernel C257 optimisé..." | tee -a "$LOG_FILE"
./test_c255v11_nx48_NEW 2>&1 | tee "$LOG_DIR/test_b_kernel_c257_${TIMESTAMP}.log"

# Extraire métriques Test B
HASHRATE_B=$(grep "hashrate=" "$LOG_DIR/test_b_kernel_c257_${TIMESTAMP}.log" | tail -1 | grep -oP '\d+\.\d+(?= MH/s)' || echo "0")
TIME_B=$(grep "Temps total:" "$LOG_DIR/test_b_kernel_c257_${TIMESTAMP}.log" | grep -oP '\d+\.\d+(?= sec)' || echo "0")

echo "" | tee -a "$LOG_FILE"
echo "[B] Résultats Kernel C257:" | tee -a "$LOG_FILE"
echo "  - Hashrate: ${HASHRATE_B} MH/s" | tee -a "$LOG_FILE"
echo "  - Temps: ${TIME_B} sec" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Restaurer kernel actuel
cp kernels/btc_sha256_gen9.bin.backup kernels/btc_sha256_gen9.bin 2>/dev/null || true

# Calcul gains
if [ "$HASHRATE_A" != "0" ] && [ "$HASHRATE_B" != "0" ]; then
    GAIN=$(echo "scale=2; (($HASHRATE_B - $HASHRATE_A) / $HASHRATE_A) * 100" | bc)
    echo "╔════════════════════════════════════════════════════════════╗" | tee -a "$LOG_FILE"
    echo "║  RÉSULTATS COMPARATIFS                                     ║" | tee -a "$LOG_FILE"
    echo "╚════════════════════════════════════════════════════════════╝" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    echo "Kernel Actuel:  ${HASHRATE_A} MH/s (${TIME_A} sec)" | tee -a "$LOG_FILE"
    echo "Kernel C257:    ${HASHRATE_B} MH/s (${TIME_B} sec)" | tee -a "$LOG_FILE"
    echo "Gain:           ${GAIN}%" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    
    if (( $(echo "$GAIN > 50" | bc -l) )); then
        echo "✅ SUCCÈS: Gain significatif > 50%" | tee -a "$LOG_FILE"
    elif (( $(echo "$GAIN > 0" | bc -l) )); then
        echo "⚠️  Gain modéré: ${GAIN}%" | tee -a "$LOG_FILE"
    else
        echo "❌ Régression: ${GAIN}%" | tee -a "$LOG_FILE"
    fi
else
    echo "❌ Erreur: Impossible de calculer les gains" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Logs détaillés:" | tee -a "$LOG_FILE"
echo "  - Test A: $LOG_DIR/test_a_kernel_actuel_${TIMESTAMP}.log" | tee -a "$LOG_FILE"
echo "  - Test B: $LOG_DIR/test_b_kernel_c257_${TIMESTAMP}.log" | tee -a "$LOG_FILE"
echo "  - Rapport: $LOG_FILE" | tee -a "$LOG_FILE"
