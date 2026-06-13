#!/bin/bash
# Script d'exécution des tests MAGEN locaux avec jeux ARC-AGI-3 réels
# TOUT RESTE EN LOCAL - Aucune soumission Kaggle

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAGEN_ROOT="$(dirname "$SCRIPT_DIR")"
VENV_DIR="$MAGEN_ROOT/venv"

echo "🚀 Exécution tests MAGEN - ARC-AGI-3 (LOCAL UNIQUEMENT)"
echo "========================================================"
echo ""

# Vérifier venv
if [ ! -d "$VENV_DIR" ]; then
    echo "❌ Environnement virtuel introuvable"
    echo "Exécutez: bash scripts/install_arc_dependencies.sh"
    exit 1
fi

# Activer venv
source "$VENV_DIR/bin/activate"

# Vérifier arc_agi
python3 -c "from arc_agi import EnvironmentWrapper" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "❌ arc_agi non installé"
    echo "Exécutez: bash scripts/install_arc_dependencies.sh"
    exit 1
fi

echo "✅ Environnement prêt"
echo ""

# Parser arguments
GAMES=3
FORENSIC=""
DEBUG=""
GAME_IDS=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --games)
            GAMES="$2"
            shift 2
            ;;
        --game-ids)
            shift
            while [[ $# -gt 0 ]] && [[ ! "$1" =~ ^-- ]]; do
                GAME_IDS="$GAME_IDS $1"
                shift
            done
            ;;
        --forensic-full)
            FORENSIC="--forensic-full"
            shift
            ;;
        --debug)
            DEBUG="--debug"
            shift
            ;;
        *)
            echo "❌ Option inconnue: $1"
            echo "Usage: $0 [--games N] [--game-ids id1 id2 ...] [--forensic-full] [--debug]"
            exit 1
            ;;
    esac
done

# Construire commande
CMD="python tests/test_magen_real_arc_games.py"

if [ -n "$GAME_IDS" ]; then
    CMD="$CMD --game-ids$GAME_IDS"
else
    CMD="$CMD --games $GAMES"
fi

if [ -n "$FORENSIC" ]; then
    CMD="$CMD $FORENSIC"
fi

if [ -n "$DEBUG" ]; then
    CMD="$CMD $DEBUG"
fi

echo "📋 Commande: $CMD"
echo ""

# Créer répertoires logs
mkdir -p "$MAGEN_ROOT/logs/forensic"
mkdir -p "$MAGEN_ROOT/logs/results"

# Exécuter tests
cd "$MAGEN_ROOT"
$CMD

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ Tests terminés avec succès"
    echo ""
    echo "📊 Résultats disponibles dans:"
    echo "   - logs/results/magen_arc_results_*.json"
    echo "   - logs/forensic/magen_arc_*.lum"
    echo "   - logs/magen_arc_test.log"
else
    echo "❌ Tests échoués (code: $EXIT_CODE)"
    echo "Consultez: logs/magen_arc_test.log"
fi

exit $EXIT_CODE

# Made with Bob
