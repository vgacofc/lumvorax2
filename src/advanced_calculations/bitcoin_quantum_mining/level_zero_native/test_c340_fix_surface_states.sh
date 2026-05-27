#!/bin/bash
# Test C340 - FIX Surface States avec offsets relatifs (pas adresses CPU)
# Compile et teste avec 10 dispatches (10 secondes max)

set -e

echo "═══════════════════════════════════════════════════════════════"
echo "TEST C340 - FIX SURFACE STATES (offsets relatifs)"
echo "═══════════════════════════════════════════════════════════════"

cd "$(dirname "$0")"

# Compilation
echo "[C340] Compilation..."
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
echo "[C340] Exécution test 10 secondes (10 dispatches)..."
timeout 15s ./test_btc_mining_native 10 2>&1 | tee logs/execution_c340_test_10s.log

# Analyse rapide
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "ANALYSE RAPIDE C340"
echo "═══════════════════════════════════════════════════════════════"

LOG="logs/forensic/btc_mining_native_sha256.log"

if [ -f "$LOG" ]; then
    echo "[C340] Surface states configurés:"
    grep "C340_SURFACE_STATES" "$LOG" | head -10
    
    echo ""
    echo "[C340] Exec objects:"
    grep "C340_EXEC_OBJECTS" "$LOG" | head -5
    
    echo ""
    echo "[C340] Résultats GPU:"
    grep "EXEC_SUCCESS" "$LOG" | tail -5
    
    echo ""
    echo "[C340] Sentinel validation:"
    grep "sentinel_after" "$LOG" | tail -5
    
    echo ""
    echo "[C340] Leading zeros:"
    grep "leading_zeros" "$LOG" | tail -5
else
    echo "❌ Log forensique introuvable: $LOG"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "Test C340 terminé - Voir logs/execution_c340_test_10s.log"
echo "═══════════════════════════════════════════════════════════════"

# Made with Bob
