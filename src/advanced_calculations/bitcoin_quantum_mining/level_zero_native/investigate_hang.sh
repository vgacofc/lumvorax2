#!/bin/bash
# investigate_hang.sh — Investigation empirique GPU hang dispatch 28
# Session C277 — Analyse croisée hypothèses

set -e

RESULTS_DIR="./investigation_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

echo "=========================================="
echo "Investigation GPU Hang Dispatch 28"
echo "Date: $(date)"
echo "Résultats: $RESULTS_DIR"
echo "=========================================="
echo ""

# Vérifier permissions root
if [ "$EUID" -ne 0 ]; then 
    echo "⚠️  Ce script nécessite sudo pour accéder à debugfs i915"
    echo "Relancer avec: sudo $0"
    exit 1
fi

# Vérifier debugfs monté
if [ ! -d "/sys/kernel/debug/dri/0" ]; then
    echo "❌ debugfs i915 non disponible"
    echo "Monter avec: sudo mount -t debugfs none /sys/kernel/debug"
    exit 1
fi

echo "✅ Permissions et debugfs OK"
echo ""

# ============================================
# PHASE 1: Capture état AVANT exécution
# ============================================

echo "=== PHASE 1: Capture État Initial ==="
echo ""

echo "1. Ring Buffer Info..."
cat /sys/kernel/debug/dri/0/i915_ring_info > "$RESULTS_DIR/ring_before.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/ring_before.txt"
echo "   ✓ Sauvegardé: ring_before.txt"

echo "2. GEM Objects Count..."
GEM_COUNT_BEFORE=$(cat /sys/kernel/debug/dri/0/i915_gem_objects 2>/dev/null | wc -l || echo "0")
echo "$GEM_COUNT_BEFORE" > "$RESULTS_DIR/gem_count_before.txt"
echo "   ✓ GEM objects avant: $GEM_COUNT_BEFORE"

echo "3. Fence Registers..."
cat /sys/kernel/debug/dri/0/i915_gem_fence_regs > "$RESULTS_DIR/fence_before.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/fence_before.txt"
echo "   ✓ Sauvegardé: fence_before.txt"

echo "4. Engine Stats..."
cat /sys/kernel/debug/dri/0/i915_engine_info > "$RESULTS_DIR/engine_before.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/engine_before.txt"
echo "   ✓ Sauvegardé: engine_before.txt"

echo "5. Reset Count..."
RESET_COUNT_BEFORE=$(cat /sys/kernel/debug/dri/0/i915_reset_count 2>/dev/null || echo "0")
echo "$RESET_COUNT_BEFORE" > "$RESULTS_DIR/reset_before.txt"
echo "   ✓ Reset count avant: $RESET_COUNT_BEFORE"

echo "6. GPU Frequency..."
cat /sys/kernel/debug/dri/0/i915_frequency_info > "$RESULTS_DIR/freq_before.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/freq_before.txt"
echo "   ✓ Sauvegardé: freq_before.txt"

echo "7. Clients actifs..."
cat /sys/kernel/debug/dri/0/clients > "$RESULTS_DIR/clients_before.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/clients_before.txt"
echo "   ✓ Sauvegardé: clients_before.txt"

echo ""
echo "État initial capturé ✓"
echo ""

# ============================================
# PHASE 2: Exécution programme
# ============================================

echo "=== PHASE 2: Exécution Programme ==="
echo ""
echo "Lancer bitcoin_miner_drm maintenant..."
echo "Le script attend le crash ou CTRL+C..."
echo ""

# Démarrer monitoring dmesg en arrière-plan
dmesg -C
dmesg -w > "$RESULTS_DIR/dmesg_live.txt" &
DMESG_PID=$!

# Attendre input utilisateur
read -p "Appuyer sur ENTER après crash ou arrêt du programme..."

# Arrêter monitoring dmesg
kill $DMESG_PID 2>/dev/null || true

echo ""
echo "Programme terminé, capture état final..."
echo ""

# ============================================
# PHASE 3: Capture état APRÈS exécution
# ============================================

echo "=== PHASE 3: Capture État Final ==="
echo ""

sleep 2  # Laisser kernel se stabiliser

echo "1. Ring Buffer Info..."
cat /sys/kernel/debug/dri/0/i915_ring_info > "$RESULTS_DIR/ring_after.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/ring_after.txt"
echo "   ✓ Sauvegardé: ring_after.txt"

echo "2. GEM Objects Count..."
GEM_COUNT_AFTER=$(cat /sys/kernel/debug/dri/0/i915_gem_objects 2>/dev/null | wc -l || echo "0")
echo "$GEM_COUNT_AFTER" > "$RESULTS_DIR/gem_count_after.txt"
echo "   ✓ GEM objects après: $GEM_COUNT_AFTER"

echo "3. Fence Registers..."
cat /sys/kernel/debug/dri/0/i915_gem_fence_regs > "$RESULTS_DIR/fence_after.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/fence_after.txt"
echo "   ✓ Sauvegardé: fence_after.txt"

echo "4. Engine Stats..."
cat /sys/kernel/debug/dri/0/i915_engine_info > "$RESULTS_DIR/engine_after.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/engine_after.txt"
echo "   ✓ Sauvegardé: engine_after.txt"

echo "5. Reset Count..."
RESET_COUNT_AFTER=$(cat /sys/kernel/debug/dri/0/i915_reset_count 2>/dev/null || echo "0")
echo "$RESET_COUNT_AFTER" > "$RESULTS_DIR/reset_after.txt"
echo "   ✓ Reset count après: $RESET_COUNT_AFTER"

echo "6. GPU Frequency..."
cat /sys/kernel/debug/dri/0/i915_frequency_info > "$RESULTS_DIR/freq_after.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/freq_after.txt"
echo "   ✓ Sauvegardé: freq_after.txt"

echo "7. Clients actifs..."
cat /sys/kernel/debug/dri/0/clients > "$RESULTS_DIR/clients_after.txt" 2>/dev/null || echo "N/A" > "$RESULTS_DIR/clients_after.txt"
echo "   ✓ Sauvegardé: clients_after.txt"

echo ""
echo "État final capturé ✓"
echo ""

# ============================================
# PHASE 4: Analyse différences
# ============================================

echo "=== PHASE 4: Analyse Différences ==="
echo ""

# Créer rapport analyse
REPORT="$RESULTS_DIR/ANALYSE_REPORT.txt"

{
    echo "=========================================="
    echo "RAPPORT ANALYSE GPU HANG"
    echo "Date: $(date)"
    echo "=========================================="
    echo ""
    
    echo "=== MÉTRIQUES CLÉS ==="
    echo ""
    
    # GEM Objects
    GEM_DIFF=$((GEM_COUNT_AFTER - GEM_COUNT_BEFORE))
    echo "GEM Objects:"
    echo "  Avant:  $GEM_COUNT_BEFORE"
    echo "  Après:  $GEM_COUNT_AFTER"
    echo "  Delta:  $GEM_DIFF"
    if [ $GEM_DIFF -gt 10 ]; then
        echo "  ⚠️  FUITE DÉTECTÉE: +$GEM_DIFF objects (> 10)"
        echo "  → Hypothèse C (GEM leak) PROBABLE"
    else
        echo "  ✓ Pas de fuite significative"
    fi
    echo ""
    
    # Reset Count
    RESET_DIFF=$((RESET_COUNT_AFTER - RESET_COUNT_BEFORE))
    echo "Reset Count:"
    echo "  Avant:  $RESET_COUNT_BEFORE"
    echo "  Après:  $RESET_COUNT_AFTER"
    echo "  Delta:  $RESET_DIFF"
    if [ $RESET_DIFF -gt 0 ]; then
        echo "  ⚠️  GPU RESET DÉTECTÉ: +$RESET_DIFF resets"
        echo "  → Confirme GPU hang"
    else
        echo "  ✓ Pas de reset GPU"
    fi
    echo ""
    
    # Ring Buffer
    echo "Ring Buffer:"
    if diff -q "$RESULTS_DIR/ring_before.txt" "$RESULTS_DIR/ring_after.txt" > /dev/null 2>&1; then
        echo "  ✓ Identique (pas de changement)"
    else
        RING_DIFF_LINES=$(diff "$RESULTS_DIR/ring_before.txt" "$RESULTS_DIR/ring_after.txt" 2>/dev/null | wc -l || echo "0")
        echo "  ⚠️  Changements détectés: $RING_DIFF_LINES lignes"
        echo "  → Voir ring_diff.txt pour détails"
        diff "$RESULTS_DIR/ring_before.txt" "$RESULTS_DIR/ring_after.txt" > "$RESULTS_DIR/ring_diff.txt" 2>/dev/null || true
    fi
    echo ""
    
    # Messages dmesg critiques
    echo "=== MESSAGES DMESG CRITIQUES ==="
    echo ""
    
    PREEMPTION_COUNT=$(grep -c "preemption" "$RESULTS_DIR/dmesg_live.txt" 2>/dev/null || echo "0")
    HANG_COUNT=$(grep -c "GPU hang" "$RESULTS_DIR/dmesg_live.txt" 2>/dev/null || echo "0")
    RESET_MSG_COUNT=$(grep -c "Resetting" "$RESULTS_DIR/dmesg_live.txt" 2>/dev/null || echo "0")
    
    echo "Preemption timeout: $PREEMPTION_COUNT occurrences"
    if [ $PREEMPTION_COUNT -gt 0 ]; then
        echo "  ⚠️  PREEMPTION TIMEOUT DÉTECTÉ"
        echo "  → Hypothèse B (Preemption watchdog) PROBABLE"
        echo ""
        echo "  Messages:"
        grep "preemption" "$RESULTS_DIR/dmesg_live.txt" 2>/dev/null | head -5 || true
    fi
    echo ""
    
    echo "GPU hang: $HANG_COUNT occurrences"
    if [ $HANG_COUNT -gt 0 ]; then
        echo "  ⚠️  GPU HANG CONFIRMÉ"
        echo ""
        echo "  Messages:"
        grep "GPU hang" "$RESULTS_DIR/dmesg_live.txt" 2>/dev/null | head -5 || true
    fi
    echo ""
    
    echo "Reset messages: $RESET_MSG_COUNT occurrences"
    if [ $RESET_MSG_COUNT -gt 0 ]; then
        echo "  ⚠️  RESET GPU CONFIRMÉ"
        echo ""
        echo "  Messages:"
        grep "Resetting" "$RESULTS_DIR/dmesg_live.txt" 2>/dev/null | head -5 || true
    fi
    echo ""
    
    # Synthèse hypothèses
    echo "=== SYNTHÈSE HYPOTHÈSES ==="
    echo ""
    
    SCORE_A=0  # Ring buffer saturation
    SCORE_B=0  # Preemption watchdog
    SCORE_C=0  # GEM leak
    
    # Scoring
    [ $PREEMPTION_COUNT -gt 0 ] && SCORE_B=$((SCORE_B + 3))
    [ $GEM_DIFF -gt 10 ] && SCORE_C=$((SCORE_C + 3))
    [ $RESET_DIFF -gt 0 ] && SCORE_A=$((SCORE_A + 1)) && SCORE_B=$((SCORE_B + 1))
    [ $RING_DIFF_LINES -gt 50 ] && SCORE_A=$((SCORE_A + 2))
    
    echo "Scores hypothèses (0-5):"
    echo "  Hypothèse A (Ring buffer):    $SCORE_A/5"
    echo "  Hypothèse B (Preemption):     $SCORE_B/5"
    echo "  Hypothèse C (GEM leak):       $SCORE_C/5"
    echo ""
    
    # Recommandation
    MAX_SCORE=$(echo -e "$SCORE_A\n$SCORE_B\n$SCORE_C" | sort -rn | head -1)
    
    echo "=== RECOMMANDATION ==="
    echo ""
    
    if [ $SCORE_B -eq $MAX_SCORE ] && [ $SCORE_B -gt 0 ]; then
        echo "🔴 HYPOTHÈSE B (Preemption watchdog) — PRIORITÉ 1"
        echo ""
        echo "Actions recommandées:"
        echo "  1. Réduire work-group size (256 → 64)"
        echo "  2. Activer scoreboard dans MEDIA_VFE_STATE"
        echo "  3. Ajouter yield points dans kernel SHA-256"
        echo "  4. Vérifier: cat /sys/module/i915/parameters/enable_hangcheck"
    elif [ $SCORE_C -eq $MAX_SCORE ] && [ $SCORE_C -gt 0 ]; then
        echo "🔴 HYPOTHÈSE C (GEM leak) — PRIORITÉ 1"
        echo ""
        echo "Actions recommandées:"
        echo "  1. Ajouter drm_gem_object_put() après chaque dispatch"
        echo "  2. Réutiliser buffers entre dispatches"
        echo "  3. Vérifier cleanup dans boucle principale"
    elif [ $SCORE_A -eq $MAX_SCORE ] && [ $SCORE_A -gt 0 ]; then
        echo "🔴 HYPOTHÈSE A (Ring buffer) — PRIORITÉ 1"
        echo ""
        echo "Actions recommandées:"
        echo "  1. Forcer flush ring buffer entre dispatches"
        echo "  2. Ajouter MI_FLUSH_DW explicites"
        echo "  3. Réduire batch buffer size"
    else
        echo "⚠️  Aucune hypothèse dominante"
        echo ""
        echo "Actions recommandées:"
        echo "  1. Analyser manuellement fichiers dans $RESULTS_DIR"
        echo "  2. Tester hypothèse E (États GPU manquants)"
        echo "  3. Consulter RAPPORT_C277_ANALYSE_CROISEE_HYPOTHESES.md"
    fi
    echo ""
    
    echo "=========================================="
    echo "Fichiers générés:"
    echo "  - $RESULTS_DIR/"
    echo "  - Voir ANALYSE_REPORT.txt pour synthèse"
    echo "=========================================="
    
} | tee "$REPORT"

echo ""
echo "✅ Investigation terminée"
echo ""
echo "Rapport complet: $REPORT"
echo "Tous les fichiers: $RESULTS_DIR/"
echo ""

# Made with Bob
