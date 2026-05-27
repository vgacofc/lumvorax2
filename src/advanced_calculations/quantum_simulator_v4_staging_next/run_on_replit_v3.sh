#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-/workspace/Lumvorax}"
RUNS="${2:-30}"
SCENARIOS="${3:-360}"
STEPS="${4:-1400}"

TARGET="$(find "$ROOT" -type f -path '*/src/advanced_calculations/quantum_simulator_v3_staging/tools/run_campaign_v3.py' | head -n 1)"
if [[ -z "$TARGET" ]]; then
  echo "Erreur: run_campaign_v3.py introuvable" >&2
  exit 2
fi

REPO_ROOT="$(cd "$(dirname "$TARGET")/../../../.." && pwd)"
pushd "$REPO_ROOT" >/dev/null
python src/advanced_calculations/quantum_simulator_v3_staging/tools/run_campaign_v3.py --runs-per-mode "$RUNS" --scenarios "$SCENARIOS" --steps "$STEPS"
popd >/dev/null
