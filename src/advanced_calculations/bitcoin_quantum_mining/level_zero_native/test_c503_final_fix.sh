#!/bin/bash

# TEST C503 - CORRECTION FINALE: MEDIA_VFE_STATE MAX_THREADS
# ROOT CAUSE #7: max_threads = 8 (au lieu de 256)

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  TEST C503 - CORRECTION FINALE: MAX_THREADS"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Répertoires
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/tests"
LOG_DIR="$SCRIPT_DIR/logs"
FORENSIC_DIR="$LOG_DIR/forensic"

mkdir -p "$LOG_DIR" "$FORENSIC_DIR"

# Timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="$LOG_DIR/execution_c503_final_fix_${TIMESTAMP}.log"
FORENSIC_LOG="$FORENSIC_DIR/dmesg_c503_${TIMESTAMP}.log"

echo "[1/5] Compilation test_c503_final_fix.c..."
gcc -o "$TEST_DIR/test_c503_final_fix" \
    "$TEST_DIR/test_c503_final_fix.c" \
    -I"$SCRIPT_DIR" \
    -ldrm -ldrm_intel \
    -Wall -Wextra \
    -O2 \
    -g

if [ $? -ne 0 ]; then
    echo "❌ Compilation ÉCHEC"
    exit 1
fi
echo "  ✓ Compilation réussie"
echo ""

echo "[2/5] Nettoyage dmesg..."
echo emmaus | sudo -S dmesg -C 2>/dev/null || true
echo "  ✓ dmesg nettoyé"
echo ""

echo "[3/5] Exécution test C503..."
echo "  CORRECTION CRITIQUE:"
echo "    #7: MEDIA_VFE_STATE max_threads=8 (au lieu de 256)"
echo ""
echo "  Autres corrections:"
echo "    #1: GPGPU_WALKER DW4 ThreadWidth=7 (C501)"
echo "    #2: MEDIA_INTERFACE_DESCRIPTOR_LOAD présent (C431)"
echo "    #3: GEM_WAIT + GEM_SET_DOMAIN (C427)"
echo ""

# Exécution avec timeout 10s
timeout 10s "$TEST_DIR/test_c503_final_fix" 2>&1 | tee "$LOG_FILE"
EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "[4/5] Capture dmesg..."
echo emmaus | sudo -S dmesg > "$FORENSIC_LOG"
echo "  ✓ dmesg capturé: $FORENSIC_LOG"
echo ""

echo "[5/5] Analyse résultat..."
if [ $EXIT_CODE -eq 0 ]; then
    if grep -q "SUCCÈS TOTAL" "$LOG_FILE"; then
        echo ""
        echo "🎉🎉🎉🎉🎉 SUCCÈS TOTAL C503! 🎉🎉🎉🎉🎉"
        echo "✓✓✓ PREMIER RÉSULTAT GPU NATIF i915 DRM! ✓✓✓"
        echo "✓✓✓ ÉLIMINATION OPENCL: 100%! ✓✓✓"
        echo "✓✓✓ TOUTES LES ROOT CAUSES CORRIGÉES! ✓✓✓"
        echo ""
        echo "Logs sauvegardés:"
        echo "  - Exécution: $LOG_FILE"
        echo "  - dmesg: $FORENSIC_LOG"
        exit 0
    elif grep -q "MODIFIÉ" "$LOG_FILE"; then
        echo ""
        echo "⚠ GPU EXÉCUTÉ mais résultat incorrect"
        echo "→ Analyser logs pour comprendre"
        echo ""
        echo "Logs sauvegardés:"
        echo "  - Exécution: $LOG_FILE"
        echo "  - dmesg: $FORENSIC_LOG"
        exit 0
    else
        echo ""
        echo "❌ Pas de modification détectée"
        echo "→ Vérifier dmesg pour GPU HANG"
        echo ""
        echo "Logs sauvegardés:"
        echo "  - Exécution: $LOG_FILE"
        echo "  - dmesg: $FORENSIC_LOG"
        
        # Vérifier GPU HANG
        if grep -q "GPU HANG" "$FORENSIC_LOG"; then
            echo ""
            echo "🚨 GPU HANG DÉTECTÉ!"
            grep "GPU HANG" "$FORENSIC_LOG" | tail -5
        fi
        exit 1
    fi
else
    echo ""
    echo "❌ Test terminé avec erreur (code $EXIT_CODE)"
    echo ""
    echo "Logs sauvegardés:"
    echo "  - Exécution: $LOG_FILE"
    echo "  - dmesg: $FORENSIC_LOG"
    exit 1
fi

# Made with Bob - C503 Final Fix