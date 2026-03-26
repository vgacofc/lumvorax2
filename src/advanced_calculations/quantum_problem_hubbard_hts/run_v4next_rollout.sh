#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODE="${1:-shadow}"
LATEST_RUN="$(ls -1 "$ROOT_DIR/results" | rg '^research_' | tail -n 1)"
RUN_DIR="$ROOT_DIR/results/$LATEST_RUN"

python3 "$ROOT_DIR/tools/v4next_rollout_controller.py" "$RUN_DIR" "$MODE"

echo "Rollout mode '$MODE' évalué pour $RUN_DIR"
