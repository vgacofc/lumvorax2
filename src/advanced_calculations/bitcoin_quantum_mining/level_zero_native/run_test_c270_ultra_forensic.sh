#!/bin/bash

# LumVorax C270 — Script exécution test ultra-forensique nano bit-level
# OBJECTIF : Identifier cause racine exacte bug errno=5

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  LumVorax C270 — TEST ULTRA-FORENSIQUE NANO BIT-LEVEL     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Créer répertoires logs
mkdir -p logs/forensic/c270_ultra

# Compiler le test
echo "[C270_COMPILE] Compiling ultra-forensic test..."
gcc -o test_c270_ultra_forensic \
    src/btc_gen9_native_runner.c \
    src/btc_gen9_native_runner_test_c270_ultra_forensic.c \
    src/btc_i915_drm_forensic_tracker.c \
    -I./include \
    -I/usr/include/libdrm \
    -ldrm \
    -lpthread \
    -lm \
    -O2 \
    -Wall \
    -Wextra \
    -g

if [ $? -eq 0 ]; then
    echo "[C270_COMPILE] Compilation SUCCESS"
else
    echo "[C270_COMPILE] Compilation FAILED"
    exit 1
fi

echo ""
echo "[C270_INFO] Test configuration:"
echo "  - Reopen DRM: DISABLED (forcing bug reproduction)"
echo "  - Memory tracking: ENABLED (ultra-granular)"
echo "  - Expected crash: After 27 dispatches (errno=5)"
echo "  - Logs directory: logs/forensic/c270_ultra/"
echo ""

# Exécuter le test
echo "[C270_EXECUTE] Starting ultra-forensic test..."
echo "[C270_EXECUTE] Press Ctrl+C to abort"
echo ""

./test_c270_ultra_forensic

echo ""
echo "[C270_COMPLETE] Test execution complete"
echo "[C270_COMPLETE] Analyzing forensic logs..."
echo ""

# Afficher résumé logs
if [ -f logs/forensic/c270_ultra/ultra_forensic.log ]; then
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║  FORENSIC LOGS SUMMARY                                     ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    
    # Compter snapshots
    SNAPSHOT_COUNT=$(grep -c "C270_SNAPSHOT" logs/forensic/c270_ultra/ultra_forensic.log || echo "0")
    echo "Total snapshots captured: $SNAPSHOT_COUNT"
    
    # Chercher anomalies
    ANOMALY_COUNT=$(grep -c "C270_ANOMALY" logs/forensic/c270_ultra/ultra_forensic.log || echo "0")
    echo "Anomalies detected: $ANOMALY_COUNT"
    
    # Chercher crash
    if grep -q "C270_CRASH" logs/forensic/c270_ultra/ultra_forensic.log; then
        echo ""
        echo "BUG REPRODUCED! Crash details:"
        grep "C270_CRASH" logs/forensic/c270_ultra/ultra_forensic.log
        echo ""
        echo "Bug state dump:"
        grep -A 10 "C270_BUG_STATE_DUMP" logs/forensic/c270_ultra/ultra_forensic.log
    else
        echo ""
        echo "WARNING: Bug NOT reproduced (no crash detected)"
    fi
    
    echo ""
    echo "Full logs available at:"
    echo "  - logs/forensic/c270_ultra/ultra_forensic.log"
    echo "  - logs/forensic/c270_ultra/memory_tracking.log"
    echo "  - logs/forensic/c270_ultra/drm_state.log"
    echo "  - logs/forensic/c270_ultra/hardware_state.log"
else
    echo "[C270_ERROR] Forensic logs not found!"
fi

echo ""
echo "[C270_DONE] Analysis complete"

# Made with Bob
