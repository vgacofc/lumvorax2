#!/bin/bash
# Test C341 - FIX GPGPU_WALKER dimensions X/Y/Z (256/1/1)
# ROOT CAUSE: dimensions=0 → GPU ne lance AUCUN thread
# FIX: dimensions=256/1/1 → GPU lance 10M threads

set -e

echo "═══════════════════════════════════════════════════════════════"
echo "TEST C341 - FIX GPGPU_WALKER DIMENSIONS (256/1/1)"
echo "═══════════════════════════════════════════════════════════════"

cd "$(dirname "$0")"

# Compilation
echo "[C341] Compilation avec FIX GPGPU_WALKER..."
gcc -Wall -Wextra -O3 -march=native -std=gnu11 \
    -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -pthread \
    -Iinclude -I/usr/include/libdrm \
    -o test_btc_mining_native \
    tests/test_c255v11_nx48_dynamic.c \
    src/btc_gen9_native_runner.c \
    src/btc_i915_drm_forensic_tracker.c \
    -ldrm -lm 2>&1 | grep -E "(error|warning)" || echo "✅ Compilation réussie"

ls -lh test_btc_mining_native

# Test 10 secondes
echo ""
echo "[C341] Exécution test 10 secondes..."
timeout 15s ./test_btc_mining_native 10 2>&1 | tee logs/execution_c341_test_10s.log

echo ""
echo "✅ Exécution terminée"
echo "📄 Logs: $LOG_FILE"
echo ""

# Analyse rapide
echo "[3/3] Analyse rapide des résultats..."
echo ""

# Extraire métriques clés
HASHRATE=$(grep -oP "hashrate=\K[0-9.]+(?=MH/s)" "$LOG_FILE" | tail -1)
BEST_LEADING=$(grep -oP "best_leading=\K[0-9]+" "$LOG_FILE" | tail -1)
NEAR_MISS_GPU=$(grep -oP "near-miss GPU best: \K[0-9]+" "$LOG_FILE" | tail -1)
DISPATCHES=$(grep -c "GEN9-EXECUTE" "$LOG_FILE" || echo "0")

echo "📊 RÉSULTATS C341:"
echo "  - Hashrate: ${HASHRATE:-N/A} MH/s"
echo "  - Best leading: ${BEST_LEADING:-N/A} bits"
echo "  - Near-miss GPU: ${NEAR_MISS_GPU:-N/A} bits"
echo "  - Dispatches: $DISPATCHES"
echo ""

# Vérifier si fix fonctionne
if [ ! -z "$NEAR_MISS_GPU" ] && [ "$NEAR_MISS_GPU" -gt 0 ]; then
    echo "✅ SUCCÈS: GPU calcule maintenant ! (near-miss > 0)"
else
    echo "⚠️  ATTENTION: GPU ne calcule toujours pas (near-miss = 0)"
fi

echo ""
echo "=== FIN TEST C341 ==="

# Made with Bob
