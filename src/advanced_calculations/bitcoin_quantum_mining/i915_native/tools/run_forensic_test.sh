#!/bin/bash
# Script de Test Forensique avec Capture dmesg — Phase 15X
# Exécute le test Bitcoin mining et capture tous les logs système

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
I915_DIR="$(dirname "$SCRIPT_DIR")"
LOG_DIR="$I915_DIR/logs/forensic"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Créer répertoire logs
mkdir -p "$LOG_DIR"

# Fichiers de sortie
LUM_FILE="$LOG_DIR/btc_mining_forensic_${TIMESTAMP}.lum"
DMESG_BEFORE="$LOG_DIR/dmesg_before_${TIMESTAMP}.log"
DMESG_AFTER="$LOG_DIR/dmesg_after_${TIMESTAMP}.log"
DMESG_DIFF="$LOG_DIR/dmesg_diff_${TIMESTAMP}.log"
ERROR_STATE="$LOG_DIR/i915_error_state_${TIMESTAMP}.log"
TEST_OUTPUT="$LOG_DIR/test_output_${TIMESTAMP}.log"

echo "═══════════════════════════════════════════════════════════"
echo "  TEST FORENSIQUE BITCOIN MINING — Phase 15X"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Répertoire: $I915_DIR"
echo "Logs:       $LOG_DIR"
echo "Timestamp:  $TIMESTAMP"
echo ""

# Capturer dmesg avant
echo "[1/5] Capture dmesg AVANT test..."
if dmesg > "$DMESG_BEFORE" 2>/dev/null; then
    echo "✅ Sauvegardé: $DMESG_BEFORE"
else
    echo "⚠️  dmesg non accessible (permissions). Essai avec sudo..."
    if sudo dmesg > "$DMESG_BEFORE" 2>/dev/null; then
        echo "✅ Sauvegardé avec sudo: $DMESG_BEFORE"
    else
        echo "❌ dmesg non disponible. Analyse limitée."
        touch "$DMESG_BEFORE"
    fi
fi

# Vérifier si i915 error state existe
if [ -f /sys/class/drm/card0/error ]; then
    echo "[2/5] Réinitialisation i915 error state..."
    sudo sh -c 'echo 1 > /sys/class/drm/card0/error' 2>/dev/null || echo "⚠️  Impossible de réinitialiser error state (permissions?)"
else
    echo "[2/5] ⚠️  /sys/class/drm/card0/error non disponible"
fi

# Compiler test
echo "[3/5] Compilation test..."
cd "$I915_DIR"
make clean > /dev/null 2>&1
make test_btc > /dev/null 2>&1
echo "✅ Compilation réussie"

# Exécuter test avec capture
echo "[4/5] Exécution test Bitcoin mining..."
echo ""
echo "─────────────────────────────────────────────────────────────"

# Modifier temporairement le log path pour utiliser notre fichier
export BTC_LOG_PATH="$LUM_FILE"

# Exécuter test (peut échouer, c'est normal)
set +e
./bin/test_btc_mining_native 2>&1 | tee "$TEST_OUTPUT"
TEST_EXIT_CODE=$?
set -e

echo "─────────────────────────────────────────────────────────────"
echo ""
echo "Exit code: $TEST_EXIT_CODE"

# Capturer dmesg après
echo "[5/5] Capture dmesg APRÈS test..."
if dmesg > "$DMESG_AFTER" 2>/dev/null; then
    echo "✅ Sauvegardé: $DMESG_AFTER"
elif sudo dmesg > "$DMESG_AFTER" 2>/dev/null; then
    echo "✅ Sauvegardé avec sudo: $DMESG_AFTER"
else
    echo "❌ dmesg non disponible"
    touch "$DMESG_AFTER"
fi

# Calculer diff dmesg
echo ""
echo "Calcul différence dmesg..."
diff "$DMESG_BEFORE" "$DMESG_AFTER" > "$DMESG_DIFF" || true
DMESG_LINES=$(wc -l < "$DMESG_DIFF")
echo "✅ $DMESG_LINES nouvelles lignes dmesg"

# Capturer i915 error state si disponible
if [ -f /sys/class/drm/card0/error ]; then
    echo ""
    echo "Capture i915 error state..."
    sudo cat /sys/class/drm/card0/error > "$ERROR_STATE" 2>/dev/null || echo "⚠️  Impossible de lire error state"
    
    if [ -s "$ERROR_STATE" ]; then
        ERROR_SIZE=$(wc -c < "$ERROR_STATE")
        echo "✅ Error state capturé: $ERROR_SIZE bytes"
    else
        echo "ℹ️  Error state vide (pas de crash GPU détecté par i915)"
    fi
fi

# Analyser fichier .lum si créé
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ANALYSE FICHIER .LUM"
echo "═══════════════════════════════════════════════════════════"
echo ""

if [ -f "$LUM_FILE" ]; then
    LUM_SIZE=$(wc -c < "$LUM_FILE")
    echo "✅ Fichier .lum créé: $LUM_SIZE bytes"
    echo ""
    
    # Analyser avec outil Python
    python3 "$SCRIPT_DIR/analyze_lum_forensic.py" "$LUM_FILE"
else
    echo "❌ Fichier .lum non créé"
fi

# Résumé
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  RÉSUMÉ FORENSIQUE"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Fichiers générés:"
echo "  - Test output:    $TEST_OUTPUT"
echo "  - Fichier .lum:   $LUM_FILE"
echo "  - dmesg before:   $DMESG_BEFORE"
echo "  - dmesg after:    $DMESG_AFTER"
echo "  - dmesg diff:     $DMESG_DIFF ($DMESG_LINES lignes)"

if [ -f "$ERROR_STATE" ] && [ -s "$ERROR_STATE" ]; then
    echo "  - i915 error:     $ERROR_STATE"
fi

echo ""
echo "Analyse dmesg diff pour messages critiques..."
echo ""

# Rechercher patterns critiques dans dmesg
if grep -i "gpu hang\|reset\|timeout\|error\|crash" "$DMESG_DIFF" > /dev/null 2>&1; then
    echo "🔴 MESSAGES CRITIQUES DÉTECTÉS:"
    echo "─────────────────────────────────────────────────────────────"
    grep -i "gpu hang\|reset\|timeout\|error\|crash" "$DMESG_DIFF" | head -20
    echo "─────────────────────────────────────────────────────────────"
else
    echo "✅ Aucun message critique dans dmesg"
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  FIN TEST FORENSIQUE"
echo "═══════════════════════════════════════════════════════════"

# Made with Bob
