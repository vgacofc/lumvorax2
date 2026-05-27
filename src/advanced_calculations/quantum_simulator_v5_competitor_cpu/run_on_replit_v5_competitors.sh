#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-/workspace/Lumvorax}"
RUNS="${2:-30}"
SCENARIOS="${3:-360}"
STEPS="${4:-1400}"
MAX_QUBITS_WIDTH="${5:-36}"
PLAN_ONLY="${6:-0}"
SKIP_INSTALL="${7:-0}"
STRICT_COMPETITORS="${8:-}"
STRICT_MAXIMIZE="${9:-0}"
STRICT_MAX_QUBITS_CAP="${10:-24}"
STRICT_SHOTS_SCALE="${11:-1}"

REPO_ROOT="$ROOT"
cd "$REPO_ROOT"

# 1) Run our isolated V4-next benchmark first (non regression path).
bash src/advanced_calculations/quantum_simulator_v4_staging_next/run_on_replit_v4_next.sh \
  "$ROOT" "$RUNS" "$SCENARIOS" "$STEPS" "$MAX_QUBITS_WIDTH"

# 2) Run competitor CPU integration pack (Replit-fully-supported=5 competitors).
ARGS=()
if [[ "$PLAN_ONLY" == "1" ]]; then
  ARGS+=(--plan-only)
fi
if [[ "$SKIP_INSTALL" == "1" ]]; then
  ARGS+=(--skip-install)
fi
if [[ -n "$STRICT_COMPETITORS" ]]; then
  ARGS+=(--strict-competitors "$STRICT_COMPETITORS")
fi
if [[ "$STRICT_MAXIMIZE" == "1" ]]; then
  ARGS+=(--strict-maximize-resources --strict-max-qubits-cap "$STRICT_MAX_QUBITS_CAP" --strict-shots-scale "$STRICT_SHOTS_SCALE")
fi
python src/advanced_calculations/quantum_simulator_v5_competitor_cpu/tools/benchmark_competitors_cpu.py "${ARGS[@]}"
