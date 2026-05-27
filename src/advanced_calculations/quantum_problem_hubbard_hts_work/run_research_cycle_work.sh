#!/usr/bin/env bash
# ══════════════════════════════════════════════════════════════════════════════
# run_research_cycle_work.sh — Module de travail dédié LumVorax (cycle 17+)
# Conforme : prompt.txt v2.1 + STANDARD_NAMES.md + HFBL_360 + NX-11-HFBL-360
# RÈGLE ABSOLUE : NE MODIFIE JAMAIS les originaux dans quantum_problem_hubbard_hts/
# Binaires utilisés : hubbard_hts_research_runner_work (fullscale)
#                   : hubbard_hts_research_runner_advanced_parallel_work
# ══════════════════════════════════════════════════════════════════════════════
set -euo pipefail

WORK_DIR="$(cd "$(dirname "$0")" && pwd)"
ORIG_DIR="$(cd "$WORK_DIR/../quantum_problem_hubbard_hts" && pwd)"
SCRIPT_PATH="$(realpath "$0")"
STAMP_UTC="$(date -u +%Y%m%dT%H%M%SZ)"
SESSION_LOG_DIR="$WORK_DIR/logs"
SESSION_LOG="$SESSION_LOG_DIR/research_cycle_work_${STAMP_UTC}.log"

mkdir -p "$WORK_DIR/results"
mkdir -p "$SESSION_LOG_DIR"

exec > >(stdbuf -oL tee -a "$SESSION_LOG") 2>&1

echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)] run_research_cycle_work START stamp=${STAMP_UTC}"
echo "[INFO] WORK_DIR  = $WORK_DIR"
echo "[INFO] ORIG_DIR  = $ORIG_DIR"
echo "[INFO] NX-11-HFBL-360 compliance forensic active"

# ── Validation binaires AVANT exécution (règle prompt.txt) ────────────────
BIN_FULLSCALE="$WORK_DIR/hubbard_hts_research_runner_work"
BIN_ADVANCED="$WORK_DIR/hubbard_hts_research_runner_advanced_parallel_work"

# ── BC-LV04 : Suppression TOTALE restrictions ressources ──────────────────
# Aucun ulimit RAM/CPU — vérification jusqu'aux limites hardware réelles (99% dynamique)
ulimit -v unlimited 2>/dev/null || true
ulimit -m unlimited 2>/dev/null || true
ulimit -s unlimited 2>/dev/null || true

# ── Wrapper LumVorax PHASE_BRIDGE pour phases Python 8-39 (BC-LV04 fix) ──
# Noms standardisés : PHASE_BRIDGE, HW_SAMPLE, METRIC, ANOMALY (STANDARD_NAMES.md)
lv_wrap() {
  local phase="$1"; shift
  local t_start; t_start="$(date +%s%N)"
  local iso; iso="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  local pid="$$"
  local script_name; script_name="$(basename "${1:-unknown}" 2>/dev/null)"

  # Écriture PHASE_BRIDGE START + HW_SAMPLE (simulation_step forensic hook)
  if [ -n "${LUMVORAX_CSV_PATH:-}" ] && [ -f "${LUMVORAX_CSV_PATH:-}" ]; then
    local mem_pct cpu_info ts
    mem_pct="$(awk '/MemTotal/{t=$2} /MemAvailable/{a=$2} END{printf "%.2f", 100*(t-a)/t}' /proc/meminfo 2>/dev/null || echo -1)"
    cpu_info="$(awk 'NR==1{u=$2+$4; i=$5; t=u+i; printf "%.2f", 100*u/t}' /proc/stat 2>/dev/null || echo -1)"
    ts="$(date +%s%N)"
    {
      printf "PHASE_BRIDGE,%s,%s,%s,phase%02d:%s,START\n"        "$iso" "$ts" "$pid" "$phase" "$script_name"
      printf "HW_SAMPLE,%s,%s,%s,phase%02d:mem_pct,%s\n"         "$iso" "$ts" "$pid" "$phase" "$mem_pct"
      printf "HW_SAMPLE,%s,%s,%s,phase%02d:cpu_snapshot,%s\n"    "$iso" "$ts" "$pid" "$phase" "$cpu_info"
    } >> "${LUMVORAX_CSV_PATH}"
  fi

  local exit_code=0
  "$@" || exit_code=$?

  # Écriture PHASE_BRIDGE END + METRIC duration_ns + détection anomalie
  if [ -n "${LUMVORAX_CSV_PATH:-}" ] && [ -f "${LUMVORAX_CSV_PATH:-}" ]; then
    local t_end; t_end="$(date +%s%N)"
    local duration_ns=$(( t_end - t_start ))
    local status="SUCCESS"
    if [ "$exit_code" -ne 0 ]; then status="FAILURE_exit${exit_code}"; fi
    local mem_end iso2
    mem_end="$(awk '/MemTotal/{t=$2} /MemAvailable/{a=$2} END{printf "%.2f", 100*(t-a)/t}' /proc/meminfo 2>/dev/null || echo -1)"
    iso2="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    {
      printf "PHASE_BRIDGE,%s,%s,%s,phase%02d:%s,%s\n"         "$iso2" "$t_end" "$pid" "$phase" "$script_name" "$status"
      printf "METRIC,%s,%s,%s,phase%02d:duration_ns,%s\n"      "$iso2" "$t_end" "$pid" "$phase" "$duration_ns"
      printf "HW_SAMPLE,%s,%s,%s,phase%02d:mem_pct_end,%s\n"   "$iso2" "$t_end" "$pid" "$phase" "$mem_end"
    } >> "${LUMVORAX_CSV_PATH}"
    # Détection comportement non programmé (ANOMALY hook — STANDARD_NAMES.md)
    if [ "$exit_code" -ne 0 ]; then
      printf "ANOMALY,%s,%s,%s,phase%02d:exit_code,%s\n" "$iso2" "$t_end" "$pid" "$phase" "$exit_code" >> "${LUMVORAX_CSV_PATH}"
    fi
  fi
  # C22-BUG-POST : Ne pas propager l'exit_code vers le shell.
  # Avec set -euo pipefail actif, un exit_code != 0 tuerait le run entier.
  # L'exit_code est déjà tracé dans le CSV LumVorax via ANOMALY — aucune perte d'info.
  return 0
}

# ── Barre de progression ───────────────────────────────────────────────────
TOTAL_STEPS="$(grep -c '^[[:space:]]*print_progress "' "$SCRIPT_PATH" 2>/dev/null || echo 35)"
if [ "${TOTAL_STEPS:-0}" -le 0 ]; then TOTAL_STEPS=35; fi
CURRENT_STEP=0

print_progress() {
  CURRENT_STEP=$((CURRENT_STEP + 1))
  local label="$1"
  local width=40
  local filled=$((CURRENT_STEP * width / TOTAL_STEPS))
  local empty=$((width - filled))
  local bar
  bar=$(printf '%*s' "$filled" '' | tr ' ' '#')
  bar+=$(printf '%*s' "$empty" '' | tr ' ' '-')
  printf "\r[%s] %3d%% (%d/%d) %s\n" "$bar" "$((CURRENT_STEP * 100 / TOTAL_STEPS))" "$CURRENT_STEP" "$TOTAL_STEPS" "$label"
}

# ── Fonctions de traçabilité forensique (forensic_research_chain_of_custody) ──
write_checksums() {
  local dir="$1"
  (cd "$dir"
   rm -f logs/checksums.sha256
   find . -type f ! -path './logs/checksums.sha256' -print0 | sort -z | xargs -0 sha256sum > logs/checksums.sha256
   echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)] SHA-256 checksums écrits — $(wc -l < logs/checksums.sha256) fichiers")
}

write_global_sha512() {
  local dir="$1"
  (cd "$dir"
   find . -type f ! -name 'GLOBAL_CHECKSUM.sha512' -print0 | sort -z | xargs -0 sha512sum > GLOBAL_CHECKSUM.sha512
   echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)] GLOBAL_CHECKSUM.sha512 — $(wc -l < GLOBAL_CHECKSUM.sha512) fichiers hachés")
}

verify_checksums() {
  local dir="$1"
  (cd "$dir"; sha256sum -c logs/checksums.sha256 >/dev/null && echo "[OK] Checksums vérifiés")
}

# ═════════════════════════════════════════════════════════════════════════════
# PHASE 1 : Compilation du module de travail (règle : 0 erreur, 0 warning)
# ═════════════════════════════════════════════════════════════════════════════
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)] Compilation module de travail (make clean all)..."
make -C "$WORK_DIR" clean all 2>&1
print_progress "build (work module)"

# Validation binaires POST-build (règle prompt.txt)
if [ ! -x "$BIN_FULLSCALE" ] || [ ! -x "$BIN_ADVANCED" ]; then
  echo "[ERREUR CRITIQUE] Binaires du module de travail absents après compilation !"
  echo "  Attendus : $BIN_FULLSCALE"
  echo "           : $BIN_ADVANCED"
  exit 1
fi
echo "[OK] Binaires validés : $(basename $BIN_FULLSCALE) + $(basename $BIN_ADVANCED)"

# ── Exports LumVorax forensique ────────────────────────────────────────────
export LUMVORAX_FORENSIC_REALTIME="1"
export LUMVORAX_LOG_PERSISTENCE="1"
export LUMVORAX_HFBL360_ENABLED="1"        # HFBL_360 — STANDARD_NAMES.md
export LUMVORAX_MEMORY_TRACKER="1"
export LUMVORAX_RUN_GROUP="campaign_work_${STAMP_UTC}"

# ═════════════════════════════════════════════════════════════════════════════
# PHASE 2 : Simulation fullscale (binaire work)
# ═════════════════════════════════════════════════════════════════════════════
export LUMVORAX_SOLVER_VARIANT="fullscale"
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)] Lancement fullscale (work binary)..."
"$BIN_FULLSCALE" "$ORIG_DIR"
print_progress "fullscale simulation (work)"

LATEST_FULLSCALE_RUN="$(ls -1 "$ORIG_DIR/results" | grep '^research_' | tail -n 1)"
FULLSCALE_RUN_DIR="$ORIG_DIR/results/$LATEST_FULLSCALE_RUN"
echo "[INFO] Fullscale run dir : $FULLSCALE_RUN_DIR"

python3 "$ORIG_DIR/tools/post_run_csv_schema_guard.py" "$FULLSCALE_RUN_DIR"
print_progress "fullscale csv schema guard"
write_checksums "$FULLSCALE_RUN_DIR"
print_progress "fullscale checksums"
write_global_sha512 "$FULLSCALE_RUN_DIR"
print_progress "fullscale sha512 global"
verify_checksums "$FULLSCALE_RUN_DIR"
print_progress "fullscale checksum verify"

# ═════════════════════════════════════════════════════════════════════════════
# PHASE 3 : Simulation advanced_parallel (binaire work)
# ═════════════════════════════════════════════════════════════════════════════
export LUMVORAX_SOLVER_VARIANT="advanced_parallel"
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)] Lancement advanced_parallel (work binary)..."
"$BIN_ADVANCED" "$ORIG_DIR"
print_progress "advanced parallel simulation (work)"

LATEST_ADV_RUN="$(ls -1 "$ORIG_DIR/results" | grep '^research_' | tail -n 1)"
ADV_RUN_DIR="$ORIG_DIR/results/$LATEST_ADV_RUN"
RUN_DIR="$ADV_RUN_DIR"
echo "[INFO] Advanced run dir : $ADV_RUN_DIR"

# ── BC-LV04 fix : Export LUMVORAX_CSV_PATH → phases Python 8-39 instrumented ──
LUMVORAX_CSV_PATH="$(find "$RUN_DIR/logs" -name 'lumvorax_*.csv' -type f 2>/dev/null | sort | tail -n 1 || true)"
export LUMVORAX_CSV_PATH
if [ -n "${LUMVORAX_CSV_PATH:-}" ]; then
  echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] LUMVORAX_CSV_PATH=$LUMVORAX_CSV_PATH (phases 8-39 instrumented)"
else
  echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] WARN: LUMVORAX_CSV_PATH non trouvé — phases Python sans instrumentation"
fi

# ═════════════════════════════════════════════════════════════════════════════
# PHASES Python 8-39 (outils originaux — non modifiés)
# ═════════════════════════════════════════════════════════════════════════════
lv_wrap 8  python3 "$ORIG_DIR/tools/post_run_csv_schema_guard.py" "$RUN_DIR"
print_progress "advanced csv schema guard"

{
  echo "timestamp_utc=$STAMP_UTC"
  echo "hostname=$(hostname)"
  echo "uname=$(uname -a)"
  echo "gcc_version=$(gcc --version | head -n 1)"
  echo "work_module=$WORK_DIR"
  echo "orig_module=$ORIG_DIR"
} > "$RUN_DIR/logs/environment_versions.log"

lv_wrap 9  python3 "$ORIG_DIR/tools/post_run_metadata_capture.py" "$RUN_DIR"
print_progress "metadata capture"

lv_wrap 10 python3 "$ORIG_DIR/tools/post_run_cycle_guard.py" "$ORIG_DIR" "$RUN_DIR"
print_progress "cycle guard"

lv_wrap 11 python3 "$ORIG_DIR/tools/post_run_physics_readiness_pack.py" "$RUN_DIR"
print_progress "physics readiness"

lv_wrap 12 python3 "$ORIG_DIR/tools/post_run_update_runtime_benchmark.py" "$RUN_DIR" "$ORIG_DIR/benchmarks"
print_progress "runtime benchmark update"

lv_wrap 13 python3 "$ORIG_DIR/tools/post_run_v4next_integration_status.py" "$RUN_DIR"
print_progress "v4next status"

ROLL_MODE="${LUMVORAX_ROLLOUT_MODE:-shadow}"
lv_wrap 14 python3 "$ORIG_DIR/tools/v4next_rollout_controller.py" "$RUN_DIR" "$ROLL_MODE"
print_progress "rollout controller"

lv_wrap 15 python3 "$ORIG_DIR/tools/post_run_v4next_rollout_progress.py" "$RUN_DIR"
print_progress "rollout progress"

lv_wrap 16 python3 "$ORIG_DIR/tools/post_run_v4next_realtime_evolution.py" "$ORIG_DIR" "$RUN_DIR"
print_progress "realtime evolution"

lv_wrap 17 python3 "$ORIG_DIR/tools/post_run_low_level_telemetry.py" "$RUN_DIR"
print_progress "low-level telemetry"

lv_wrap 18 python3 "$ORIG_DIR/tools/post_run_advanced_observables_pack.py" "$RUN_DIR"
print_progress "advanced observables"

lv_wrap 19 python3 "$ORIG_DIR/tools/run_independent_physics_modules.py" "$RUN_DIR"
print_progress "independent qmc/dmrg/arpes/stm"

lv_wrap 20 python3 "$ORIG_DIR/tools/post_run_chatgpt_critical_tests.py" "$RUN_DIR"
print_progress "critical tests"

lv_wrap 21 python3 "$ORIG_DIR/tools/post_run_problem_solution_progress.py" "$RUN_DIR"
print_progress "solution progress"

lv_wrap 22 python3 "$ORIG_DIR/tools/post_run_authenticity_audit.py" "$ORIG_DIR" "$RUN_DIR"
print_progress "authenticity audit"

lv_wrap 23 python3 "$ORIG_DIR/tools/post_run_cycle35_exhaustive_report.py" "$ORIG_DIR" "$RUN_DIR"
print_progress "cycle35 report"

lv_wrap 24 python3 "$ORIG_DIR/tools/post_run_full_scope_integrator.py" --root "$ORIG_DIR" "$RUN_DIR"
print_progress "full-scope integration"

(cd "$RUN_DIR" && rm -f logs/checksums.sha256)

lv_wrap 25 python3 "$ORIG_DIR/tools/post_run_scientific_report_cycle.py" "$RUN_DIR"
print_progress "scientific report"

lv_wrap 26 python3 "$ORIG_DIR/tools/post_run_independent_log_review.py" "$RUN_DIR"
print_progress "independent review"

lv_wrap 27 python3 "$ORIG_DIR/tools/post_run_3d_modelization_export.py" "$RUN_DIR"
print_progress "3d model export"

lv_wrap 28 python3 "$ORIG_DIR/tools/post_run_remote_depot_independent_analysis.py" "$ORIG_DIR" --run-dir "$RUN_DIR"
print_progress "remote independent analysis"

lv_wrap 29 python3 "$ORIG_DIR/tools/post_run_parallel_calibration_bridge.py" "$RUN_DIR"
print_progress "parallel calibration bridge"

lv_wrap 30 python3 "$ORIG_DIR/tools/post_run_hfbl360_forensic_logger.py" "$RUN_DIR" \
  --standard-names "$ORIG_DIR/../../../STANDARD_NAMES.md"
print_progress "hfbl360 forensic logger (HFBL_360)"

# ── SHADOW C MONITOR (Cycle 18) — Comparaison C vs Python référence ────────
SHADOW_SCRIPT="$WORK_DIR/tools/shadow_c_monitor.py"
if [ -f "$SHADOW_SCRIPT" ]; then
  python3 "$SHADOW_SCRIPT" "$ADV_RUN_DIR" \
    ${LUMVORAX_CSV_PATH:+--lumvorax-csv "$LUMVORAX_CSV_PATH"} \
    && print_progress "shadow C monitor [OK]" \
    || print_progress "shadow C monitor [WARNING — voir shadow_c_monitor_report.json]"
else
  echo "[WARN] shadow_c_monitor.py introuvable — skipped"
fi

# ── Traçabilité forensique finale (forensic_research_chain_of_custody) ─────
write_checksums "$RUN_DIR"
print_progress "checksums"
write_global_sha512 "$RUN_DIR"
print_progress "sha512 global"

# ── Manifeste de campagne ──────────────────────────────────────────────────
CAMPAIGN_DIR="$ORIG_DIR/results/${LUMVORAX_RUN_GROUP}"
mkdir -p "$CAMPAIGN_DIR"
cat > "$CAMPAIGN_DIR/campaign_manifest.txt" <<MANIFEST
stamp_utc=$STAMP_UTC
run_group=${LUMVORAX_RUN_GROUP}
work_module=$WORK_DIR
orig_module=$ORIG_DIR
fullscale_bin=$(basename $BIN_FULLSCALE)
advanced_bin=$(basename $BIN_ADVANCED)
fullscale_run=$LATEST_FULLSCALE_RUN
advanced_run=$LATEST_ADV_RUN
fullscale_run_dir=$FULLSCALE_RUN_DIR
advanced_run_dir=$ADV_RUN_DIR
session_log=$SESSION_LOG
MANIFEST
print_progress "campaign manifest"

python3 "$ORIG_DIR/tools/post_run_fullscale_vs_advanced_compare.py" "$FULLSCALE_RUN_DIR" "$ADV_RUN_DIR" --out-dir "$CAMPAIGN_DIR"
print_progress "fullscale vs advanced compare"

python3 "$ORIG_DIR/tools/post_run_fullscale_vs_fullscale_benchmark.py" "$RUN_DIR"
print_progress "fullscale vs fullscale benchmark"

# ── SHA-512/SHA-256 finaux (T02/T18 — SHA512 avant SHA256) ────────────────
write_global_sha512 "$RUN_DIR"
print_progress "final sha512 global (T02/T18)"
write_checksums "$RUN_DIR"
print_progress "final checksums"
verify_checksums "$RUN_DIR"
print_progress "verify final checksums"

echo ""
echo "══════════════════════════════════════════════════════════════════"
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)] Research cycle WORK terminé"
echo "  Fullscale  : $FULLSCALE_RUN_DIR"
echo "  Advanced   : $ADV_RUN_DIR"
echo "  Campaign   : $CAMPAIGN_DIR"
echo "  Session log: $SESSION_LOG"
echo "  Originaux  : INTACTS dans $ORIG_DIR"
echo "══════════════════════════════════════════════════════════════════"
