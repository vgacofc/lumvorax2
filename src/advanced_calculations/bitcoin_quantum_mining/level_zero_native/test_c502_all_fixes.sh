#!/bin/bash

# TEST C502 - TOUTES CORRECTIONS MULTI-RAPPORTS APPLIQUÉES
# ROOT CAUSES: ThreadWidth=7 (C501) + GEM_WAIT/SET_DOMAIN (C427) + MEDIA_IDD_LOAD (C431)

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  TEST C502 - TOUTES CORRECTIONS MULTI-RAPPORTS"
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
LOG_FILE="$LOG_DIR/execution_c502_all_fixes_${TIMESTAMP}.log"
FORENSIC_LOG="$FORENSIC_DIR/test_c502_all_fixes.log"

echo "[1/5] Compilation test_c502_all_fixes.c..."
gcc -o "$TEST_DIR/test_c502_all_fixes" \
    "$TEST_DIR/test_c502_all_fixes.c" \
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

echo "[3/5] Exécution test C502..."
echo "  Corrections appliquées:"
echo "    #1: GPGPU_WALKER DW4 ThreadWidth=7 (C501)"
echo "    #2: MEDIA_INTERFACE_DESCRIPTOR_LOAD présent (C431)"
echo "    #3: GEM_WAIT + GEM_SET_DOMAIN (C427)"
echo ""

# Exécution avec timeout 10s
timeout 10s "$TEST_DIR/test_c502_all_fixes" 2>&1 | tee "$LOG_FILE"
EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "[4/5] Capture dmesg..."
echo emmaus | sudo -S dmesg > "$FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log"
echo "  ✓ dmesg capturé: $FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log"
echo ""

echo "[5/5] Analyse résultat..."
if [ $EXIT_CODE -eq 0 ]; then
    if grep -q "SUCCÈS TOTAL" "$LOG_FILE"; then
        echo ""
        echo "🎉🎉🎉 SUCCÈS TOTAL C502! 🎉🎉🎉"
        echo "✓✓✓ PREMIER RÉSULTAT GPU NATIF! ✓✓✓"
        echo "✓✓✓ TOUTES ROOT CAUSES CORRIGÉES! ✓✓✓"
        echo ""
        echo "Logs sauvegardés:"
        echo "  - Exécution: $LOG_FILE"
        echo "  - dmesg: $FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log"
        exit 0
    elif grep -q "MODIFIÉ" "$LOG_FILE"; then
        echo ""
        echo "⚠ GPU EXÉCUTÉ mais résultat incorrect"
        echo "→ Analyser logs pour comprendre"
        echo ""
        echo "Logs sauvegardés:"
        echo "  - Exécution: $LOG_FILE"
        echo "  - dmesg: $FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log"
        exit 0
    else
        echo ""
        echo "❌ Pas de modification détectée"
        echo "→ Vérifier dmesg pour GPU HANG"
        echo ""
        echo "Logs sauvegardés:"
        echo "  - Exécution: $LOG_FILE"
        echo "  - dmesg: $FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log"
        
        # Vérifier GPU HANG
        if grep -q "GPU HANG" "$FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log"; then
            echo ""
            echo "🚨 GPU HANG DÉTECTÉ!"
            grep "GPU HANG" "$FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log" | tail -5
        fi
        exit 1
    fi
else
    echo ""
    echo "❌ Test terminé avec erreur (code $EXIT_CODE)"
    echo ""
    echo "Logs sauvegardés:"
    echo "  - Exécution: $LOG_FILE"
    echo "  - dmesg: $FORENSIC_DIR/dmesg_c502_${TIMESTAMP}.log"
    exit 1
fi

# Made with Bob
