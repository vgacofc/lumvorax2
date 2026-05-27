#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
RUN_DIR="${1:-$ROOT_DIR/results/$(ls -1 "$ROOT_DIR/results" | rg '^research_' | tail -n 1)}"

python3 "$ROOT_DIR/tools/fullscale_strict_protocol_runner.py" "$ROOT_DIR" "$RUN_DIR"
echo "[fullscale-strict] audit generated in: $RUN_DIR/audit"
