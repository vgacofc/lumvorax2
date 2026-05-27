#!/usr/bin/env bash
# ============================================================
# run_dual_seeds.sh — Double run parallèle avec seeds distincts
# C24-01 : PTMC_RUN_INDEX=1 (seed A) + PTMC_RUN_INDEX=2 (seed B)
# en parallèle simultané sur la même machine.
#
# RÈGLE : Ne jamais modifier les seeds par défaut — toujours
# utiliser PTMC_RUN_INDEX pour des seeds déterministes distincts.
# Ne PAS recompiler (binaire déjà à jour avec C25-STEPLOG).
# ============================================================
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
STAMP_UTC="$(date -u +%Y%m%dT%H%M%SZ)"
SESSION_LOG_DIR="$ROOT_DIR/logs"
SESSION_LOG="$SESSION_LOG_DIR/dual_seeds_${STAMP_UTC}.log"

mkdir -p "$SESSION_LOG_DIR"

exec > >(stdbuf -oL tee -a "$SESSION_LOG") 2>&1

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] === run_dual_seeds.sh DÉMARRAGE stamp=${STAMP_UTC} ==="
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Binaire : $ROOT_DIR/hubbard_hts_research_runner_advanced_parallel"
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Run A   : PTMC_RUN_INDEX=1 (seed_xor = 1 × 0x9e3779b97f4a7c15)"
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Run B   : PTMC_RUN_INDEX=2 (seed_xor = 2 × 0x9e3779b97f4a7c15)"

BIN="$ROOT_DIR/hubbard_hts_research_runner_advanced_parallel"

if [ ! -x "$BIN" ]; then
    echo "[ERREUR] Binaire introuvable ou non exécutable : $BIN" >&2
    echo "[INFO]   Recompilation avec C25-STEPLOG..." >&2
    make -C "$ROOT_DIR" hubbard_hts_research_runner_advanced_parallel
fi

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Timestamp binaire : $(stat -c '%y' "$BIN" 2>/dev/null || echo inconnu)"

ulimit -v unlimited 2>/dev/null || true
ulimit -m unlimited 2>/dev/null || true
ulimit -s unlimited 2>/dev/null || true

export LUMVORAX_FORENSIC_REALTIME="1"
export LUMVORAX_LOG_PERSISTENCE="1"
export LUMVORAX_HFBL360_ENABLED="1"
export LUMVORAX_MEMORY_TRACKER="1"
export LUMVORAX_SOLVER_VARIANT="advanced_parallel"

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Lancement Run A (PTMC_RUN_INDEX=1) en arrière-plan..."
PTMC_RUN_INDEX=1 LUMVORAX_RUN_GROUP="dual_seeds_A_${STAMP_UTC}" \
    "$BIN" "$ROOT_DIR" \
    > "$SESSION_LOG_DIR/run_seed1_${STAMP_UTC}.log" 2>&1 &
PID_A=$!
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] PID Run A = $PID_A"

sleep 1

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Lancement Run B (PTMC_RUN_INDEX=2) en arrière-plan..."
PTMC_RUN_INDEX=2 LUMVORAX_RUN_GROUP="dual_seeds_B_${STAMP_UTC}" \
    "$BIN" "$ROOT_DIR" \
    > "$SESSION_LOG_DIR/run_seed2_${STAMP_UTC}.log" 2>&1 &
PID_B=$!
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] PID Run B = $PID_B"

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Attente terminaison Run A (PID $PID_A)..."
wait $PID_A
EXIT_A=$?
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Run A terminé (exit=$EXIT_A)"

echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Attente terminaison Run B (PID $PID_B)..."
wait $PID_B
EXIT_B=$?
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Run B terminé (exit=$EXIT_B)"

# Identifier les répertoires de résultats créés par A et B
RUN_A="$(ls -1t "$ROOT_DIR/results" | rg '^research_' | sed -n '2p' || true)"
RUN_B="$(ls -1t "$ROOT_DIR/results" | rg '^research_' | sed -n '1p' || true)"

echo ""
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] === RÉSULTATS ==="
echo "Run A (seed 1) : $ROOT_DIR/results/$RUN_A  (exit=$EXIT_A)"
echo "Run B (seed 2) : $ROOT_DIR/results/$RUN_B  (exit=$EXIT_B)"
echo ""

if [ "$EXIT_A" -eq 0 ] && [ "$EXIT_B" -eq 0 ]; then
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] ✓ Deux runs RÉUSSIS — schemas guard en cours..."

    # Schema guard Python pour les deux runs
    for RUN_DIR in "$ROOT_DIR/results/$RUN_A" "$ROOT_DIR/results/$RUN_B"; do
        if [ -d "$RUN_DIR" ]; then
            echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] Schema guard : $RUN_DIR"
            python3 "$ROOT_DIR/tools/post_run_csv_schema_guard.py" "$RUN_DIR" 2>/dev/null || true
        fi
    done

    # Comparaison rapide des Tc
    echo ""
    echo "=== COMPARAISON Tc (seed 1 vs seed 2) ==="
    for LABEL RUN in "Seed1 (PTMC_RUN_INDEX=1)" "$RUN_A" "Seed2 (PTMC_RUN_INDEX=2)" "$RUN_B"; do
        TC_FILE="$ROOT_DIR/results/$RUN/tests/tc_estimation_ptmc.csv"
        if [ -f "$TC_FILE" ]; then
            TC1=$(grep "^# Tc_method1" "$TC_FILE" | awk -F',' '{print $2}')
            TC2=$(grep "^# Tc_method2" "$TC_FILE" | awk -F',' '{print $2}')
            echo "$LABEL : Tc_dPdT=$TC1 K | Tc_chi=$TC2 K"
        fi
    done

    # Comparaison GLOBAL:total_metric_events pour vérifier la divergence
    echo ""
    echo "=== GLOBAL METRIC EVENTS (vérification divergence seeds) ==="
    for LABEL RUN in "Seed1" "$RUN_A" "Seed2" "$RUN_B"; do
        CSV_FILE=$(find "$ROOT_DIR/results/$RUN/logs" -name 'lumvorax_*.csv' 2>/dev/null | head -1)
        if [ -n "$CSV_FILE" ]; then
            EVENTS=$(grep "GLOBAL:total_metric_events" "$CSV_FILE" | awk -F',' '{print $6}' | head -1)
            LINES=$(wc -l < "$CSV_FILE")
            echo "$LABEL : total_metric_events=$EVENTS | lignes CSV=$LINES"
        fi
    done
else
    echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] AVERTISSEMENT — Un ou deux runs ont échoué (A=$EXIT_A B=$EXIT_B)"
    echo "Voir logs : $SESSION_LOG_DIR/run_seed1_${STAMP_UTC}.log"
    echo "           $SESSION_LOG_DIR/run_seed2_${STAMP_UTC}.log"
fi

echo ""
echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] === run_dual_seeds.sh TERMINÉ ==="
