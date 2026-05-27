#!/bin/bash
# run_test_simple.sh — Test simplifié sans debugfs i915

set -e

RESULTS_DIR="./test_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

echo "=========================================="
echo "Test GPU Hang — Version Simplifiée"
echo "Date: $(date)"
echo "Résultats: $RESULTS_DIR"
echo "=========================================="
echo ""

# Vérifier programme compilé
if [ ! -f "./test_j" ]; then
    echo "❌ Programme test_j non trouvé"
    echo "Compiler avec: gcc -I/usr/include/libdrm -o test_j test_j_context_save_restore.c -ldrm"
    exit 1
fi

echo "✅ Programme test_j trouvé"
echo ""

# Nettoyer dmesg
echo "Nettoyage dmesg..."
sudo dmesg -C
echo "✅ dmesg nettoyé"
echo ""

# Capturer état GPU avant
echo "=== État GPU AVANT Test ==="
echo ""

echo "1. Modules kernel chargés:"
lsmod | grep -E "i915|drm" | tee "$RESULTS_DIR/modules_before.txt"
echo ""

echo "2. Devices DRM:"
ls -la /dev/dri/ | tee "$RESULTS_DIR/drm_devices.txt"
echo ""

echo "3. GPU Info (lspci):"
lspci | grep -i vga | tee "$RESULTS_DIR/gpu_info.txt"
echo ""

# Exécuter test
echo "=== EXÉCUTION TEST ==="
echo ""
echo "Lancement test_j..."
echo "Le programme va s'exécuter jusqu'au crash ou 30 dispatches..."
echo ""

# Exécuter avec timeout et capturer sortie
timeout 60 ./test_j 2>&1 | tee "$RESULTS_DIR/test_output.txt" || TEST_EXIT=$?

echo ""
echo "Test terminé (exit code: ${TEST_EXIT:-0})"
echo ""

# Attendre que kernel se stabilise
sleep 2

# Capturer dmesg après
echo "=== Capture dmesg APRÈS Test ==="
sudo dmesg > "$RESULTS_DIR/dmesg_after.txt"
echo "✅ dmesg sauvegardé"
echo ""

# Analyse dmesg
echo "=== ANALYSE DMESG ==="
echo ""

PREEMPTION_COUNT=$(grep -c "preemption" "$RESULTS_DIR/dmesg_after.txt" 2>/dev/null || echo "0")
HANG_COUNT=$(grep -c "GPU hang" "$RESULTS_DIR/dmesg_after.txt" 2>/dev/null || echo "0")
RESET_COUNT=$(grep -c "Resetting" "$RESULTS_DIR/dmesg_after.txt" 2>/dev/null || echo "0")
ERROR_COUNT=$(grep -c "ERROR" "$RESULTS_DIR/dmesg_after.txt" 2>/dev/null || echo "0")

echo "Preemption timeout: $PREEMPTION_COUNT occurrences"
echo "GPU hang: $HANG_COUNT occurrences"
echo "Reset messages: $RESET_COUNT occurrences"
echo "Erreurs: $ERROR_COUNT occurrences"
echo ""

# Extraire messages critiques
if [ $PREEMPTION_COUNT -gt 0 ]; then
    echo "Messages preemption:"
    grep "preemption" "$RESULTS_DIR/dmesg_after.txt" | head -10
    echo ""
fi

if [ $HANG_COUNT -gt 0 ]; then
    echo "Messages GPU hang:"
    grep "GPU hang" "$RESULTS_DIR/dmesg_after.txt" | head -10
    echo ""
fi

if [ $RESET_COUNT -gt 0 ]; then
    echo "Messages reset:"
    grep "Resetting" "$RESULTS_DIR/dmesg_after.txt" | head -10
    echo ""
fi

# Analyse sortie test
echo "=== ANALYSE SORTIE TEST ==="
echo ""

DISPATCH_COUNT=$(grep -c "Dispatch" "$RESULTS_DIR/test_output.txt" 2>/dev/null || echo "0")
echo "Nombre de dispatches: $DISPATCH_COUNT"

if [ $DISPATCH_COUNT -gt 28 ]; then
    echo "✅ SUCCÈS: Dépassé limite 28 dispatches!"
else
    echo "⚠️  Crash avant/à dispatch 28"
fi
echo ""

# Générer rapport
REPORT="$RESULTS_DIR/RAPPORT_TEST.txt"

{
    echo "=========================================="
    echo "RAPPORT TEST GPU HANG"
    echo "Date: $(date)"
    echo "=========================================="
    echo ""
    
    echo "=== RÉSULTATS ==="
    echo ""
    echo "Dispatches exécutés: $DISPATCH_COUNT"
    echo "Preemption timeout: $PREEMPTION_COUNT"
    echo "GPU hangs: $HANG_COUNT"
    echo "Resets GPU: $RESET_COUNT"
    echo "Erreurs kernel: $ERROR_COUNT"
    echo ""
    
    echo "=== DIAGNOSTIC ==="
    echo ""
    
    if [ $PREEMPTION_COUNT -gt 0 ]; then
        echo "🔴 HYPOTHÈSE B (Preemption watchdog) — CONFIRMÉE"
        echo ""
        echo "Actions recommandées:"
        echo "  1. Réduire work-group size"
        echo "  2. Activer scoreboard dans MEDIA_VFE_STATE"
        echo "  3. Ajouter yield points dans kernel"
    elif [ $HANG_COUNT -gt 0 ]; then
        echo "🔴 GPU HANG DÉTECTÉ"
        echo ""
        echo "Cause probable: États GPU manquants ou buffer saturation"
        echo "Actions recommandées:"
        echo "  1. Implémenter 25 états GPU identifiés"
        echo "  2. Tester flush ring buffer"
    elif [ $DISPATCH_COUNT -le 28 ]; then
        echo "⚠️  CRASH DISPATCH 28 CONFIRMÉ"
        echo ""
        echo "Analyser dmesg_after.txt pour cause exacte"
    else
        echo "✅ TEST RÉUSSI - Pas de crash détecté"
    fi
    echo ""
    
    echo "=========================================="
    echo "Fichiers générés:"
    echo "  - test_output.txt (sortie programme)"
    echo "  - dmesg_after.txt (logs kernel)"
    echo "  - RAPPORT_TEST.txt (ce fichier)"
    echo "=========================================="
    
} | tee "$REPORT"

echo ""
echo "✅ Test terminé"
echo ""
echo "Rapport complet: $REPORT"
echo "Tous les fichiers: $RESULTS_DIR/"
echo ""

# Made with Bob
