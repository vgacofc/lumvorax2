#!/usr/bin/env bash
# MDBAI — Runner de tests cumulatifs (025/025)
# Conforme STANDARD_NAMES_MDBAI.md Section 7
# Conforme CAHIER_DES_CHARGES Section 9 (Sprints 1-5)
#
# RÈGLE ABSOLUE: JAMAIS supprimer un test qui passe.
# Les tests sont cumulatifs: Phase 1 (001-010) → Phase 2 (011-015) → Phase 3 (021-025)
#
# Usage:
#   ./scripts/run_cumulative_tests.sh          # Tous les tests
#   ./scripts/run_cumulative_tests.sh unit      # Phase 1 uniquement
#   ./scripts/run_cumulative_tests.sh integration # Phase 2 uniquement
#   ./scripts/run_cumulative_tests.sh e2e       # Phase 3 uniquement
#   ./scripts/run_cumulative_tests.sh watch     # Mode watch

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$ROOT_DIR"

TS=$(date +%Y%m%dT%H%M%S)
LOG_FILE="logs/execution/${TS}_test_run.log"
mkdir -p logs/execution logs/errors logs/forensic reports

echo "════════════════════════════════════════════════════════"
echo "  MDBAI — Runner Tests Cumulatifs v0.1.0"
echo "  Forensic Engine: LumVorax C111"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "════════════════════════════════════════════════════════"

MODE="${1:-all}"
PASS=0
FAIL=0
TOTAL=0

run_phase() {
    local phase="$1"
    local pattern="$2"
    local label="$3"
    echo ""
    echo "▶ PHASE $phase — $label"
    echo "─────────────────────────────────────────────────────"
    
    if NODE_OPTIONS="--experimental-vm-modules" npx jest "$pattern" \
        --testTimeout=30000 \
        --forceExit \
        --passWithNoTests \
        --no-coverage \
        2>&1 | tee -a "$LOG_FILE"; then
        echo "✅ Phase $phase PASSÉE"
        return 0
    else
        echo "❌ Phase $phase ÉCHOUÉE (voir $LOG_FILE)"
        return 1
    fi
}

case "$MODE" in
    unit|phase1|1)
        run_phase 1 "tests/unit/" "Infrastructure (001-010)"
        ;;
    integration|phase2|2)
        run_phase 2 "tests/integration/" "Analysis Engine (011-020)"
        ;;
    e2e|phase3|3)
        run_phase 3 "tests/e2e/" "GitHub Integration (021-025)"
        ;;
    watch)
        echo "⚡ Mode WATCH — Ctrl+C pour arrêter"
        NODE_OPTIONS="--experimental-vm-modules" npx jest \
            --watch \
            --testTimeout=30000
        ;;
    all|*)
        echo "▶ Exécution des 25 tests cumulatifs (Phase 1 → 2 → 3)"
        echo ""
        
        PHASE1_OK=0
        PHASE2_OK=0
        PHASE3_OK=0
        
        run_phase 1 "tests/unit/" "Infrastructure (001-010)" && PHASE1_OK=1 || PHASE1_OK=0
        run_phase 2 "tests/integration/" "Analysis Engine (011-015)" && PHASE2_OK=1 || PHASE2_OK=0
        run_phase 3 "tests/e2e/" "GitHub Integration (021-025)" && PHASE3_OK=1 || PHASE3_OK=0
        
        echo ""
        echo "════════════════════════════════════════════════════════"
        echo "  RÉSUMÉ TESTS CUMULATIFS MDBAI"
        echo "════════════════════════════════════════════════════════"
        echo "  Phase 1 (001-010): $([ $PHASE1_OK -eq 1 ] && echo '✅ PASS' || echo '❌ FAIL')"
        echo "  Phase 2 (011-015): $([ $PHASE2_OK -eq 1 ] && echo '✅ PASS' || echo '❌ FAIL')"
        echo "  Phase 3 (021-025): $([ $PHASE3_OK -eq 1 ] && echo '✅ PASS' || echo '❌ FAIL')"
        echo ""
        echo "  Log: $LOG_FILE"
        echo "════════════════════════════════════════════════════════"
        
        TOTAL_PHASES=$((PHASE1_OK + PHASE2_OK + PHASE3_OK))
        if [ "$TOTAL_PHASES" -eq 3 ]; then
            echo "  🏆 TOUS LES TESTS PASSENT (25/25) — MVP READY ✅"
            exit 0
        else
            echo "  ⚠️ $((3 - TOTAL_PHASES)) phase(s) échouée(s)"
            exit 1
        fi
        ;;
esac
