#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-/workspace/Lumvorax}"
TARGET_C="$(find "$ROOT" -type f -path '*/src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.c' | head -n 1)"
FUSION_C="$(find "$ROOT" -type f -path '*/src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator_fusion_v2.c' | head -n 1)"
if [[ -z "$TARGET_C" || -z "$FUSION_C" ]]; then
  echo "Erreur: fichiers staging introuvables sous $ROOT" >&2
  exit 2
fi
REPO_ROOT="$(cd "$(dirname "$TARGET_C")/../../.." && pwd)"
OUT_DIR="$REPO_ROOT/src/advanced_calculations/quantum_simulator_v2_staging/results"
mkdir -p "$OUT_DIR"
OUT_BIN="$OUT_DIR/qs_v2_staging_test"
OUT_JSONL="$OUT_DIR/fusion_forensic_full.jsonl"

pushd "$REPO_ROOT" >/dev/null

gcc -DMODULES_QUANTIQUES_ACTIFS -I./src -I./src/advanced_calculations -I./src/common -I./src/lum \
  src/tests/test_quantum_simulator_complete.c \
  src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.c \
  src/debug/memory_tracker.c src/debug/forensic_logger.c src/lum/lum_core.c \
  -lm -o "$OUT_BIN"

"$OUT_BIN"

cat > "$OUT_DIR/fusion_runner.c" <<'RUNEOF'
#include <stdio.h>
#include "../quantum_simulator_fusion_v2.h"
int main(void){
  quantum_fusion_v2_summary_t s;
  if(!quantum_fusion_v2_run_forensic_benchmark("results/fusion_forensic_full.jsonl",360,1400,QUANTUM_RNG_HARDWARE_PREFERRED,0xC0FFEEu,&s)){
    fprintf(stderr,"fusion benchmark failed\n");
    return 1;
  }
  printf("wins=%zu losses=%zu win_rate=%.9f nqubits_per_sec=%.3f hw_entropy=%s\n",
    s.nqubit_wins,s.baseline_wins,s.nqubit_win_rate,s.nqubits_per_sec,s.used_hardware_entropy?"true":"false");
  return 0;
}
RUNEOF

gcc -I./src -I./src/advanced_calculations -I./src/common -I./src/lum \
  src/advanced_calculations/quantum_simulator_v2_staging/results/fusion_runner.c \
  src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator_fusion_v2.c \
  -lm -o "$OUT_DIR/fusion_runner"

( cd src/advanced_calculations/quantum_simulator_v2_staging && ./results/fusion_runner )

popd >/dev/null

echo "OK: binary test + fusion run terminés"
echo "JSONL: $OUT_JSONL"
