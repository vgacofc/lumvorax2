#!/usr/bin/env bash
# LumVorax C94 — script pilote Ubuntu : build + dry-run + (optionnel) submit IBM.
# Usage :
#   bash tools/run_c94_ubuntu.sh dry-12
#   bash tools/run_c94_ubuntu.sh dry-16
#   bash tools/run_c94_ubuntu.sh submit-12
#   bash tools/run_c94_ubuntu.sh submit-16
#   bash tools/run_c94_ubuntu.sh build-c-only
#   bash tools/run_c94_ubuntu.sh full-12
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

MODE="${1:-dry-12}"

# ---- Pre-load libstdc++ pour environnements nix (Replit / NixOS) ----
for P in \
  /nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6 \
  /nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6 \
  /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ; do
  if [ -f "$P" ]; then export LD_PRELOAD="$P"; break; fi
done

INC_HUB="src/advanced_calculations/quantum_problem_hubbard_hts/include"
SRC_HUB="src/advanced_calculations/quantum_problem_hubbard_hts/src"
SRC_VORAX="src/vorax"
SRC_LUM="src/lum"
SRC_PARSER="src/parser"
SRC_BTC="src/advanced_calculations/bitcoin_quantum_mining/src"

BUILD_DIR="build/c94"
mkdir -p "$BUILD_DIR"

build_c_check() {
  echo "=== [C94] Compile-check des modules C avec ibm_quantum_constants.h C94 ==="
  for F in \
    "$SRC_HUB/vorax_kernel.c" \
    "$SRC_VORAX/vorax_operations.c" \
    "$SRC_PARSER/vorax_parser.c" \
    "$SRC_LUM/lum_core.c" \
    "$SRC_BTC/nx48_btc_controller.c" ; do
    if [ -f "$F" ]; then
      OUT="$BUILD_DIR/$(basename "$F" .c).o"
      echo "  -> $F"
      gcc -c -std=c11 -O2 -Wall \
        -I include -I src \
        -I "$INC_HUB" \
        -I src/advanced_calculations/bitcoin_quantum_mining/include \
        "$F" -o "$OUT" 2>&1 | head -8 || true
    else
      echo "  (skip) $F absent"
    fi
  done
  # Mini test runtime du header
  cat > /tmp/c94_smoke.c <<'C'
#include <stdio.h>
#include "ibm_quantum_constants.h"
int main(void){
  printf("C91 HVA8  = %.4f\n", IBM_C91_HVA8_S_PI);
  printf("C93 S_pi  = %.4f +/- %.4f  (gain x%.3f)\n",
         IBM_C93_S_PI, IBM_C93_S_PI_STD, IBM_C93_GAIN_VS_C91_HVA8);
#ifdef IBM_C94_S_PI_N12
  printf("C94 N=12  = %.4f +/- %.4f\n", IBM_C94_S_PI_N12, IBM_C94_S_PI_N12_STD);
#endif
#ifdef IBM_C94_S_PI_N16
  printf("C94 N=16  = %.4f +/- %.4f\n", IBM_C94_S_PI_N16, IBM_C94_S_PI_N16_STD);
#endif
  printf("Best N=12 = %.4f\n", ibm_best_s_pi_for_N(12));
  printf("Best N=16 = %.4f\n", ibm_best_s_pi_for_N(16));
  return 0;
}
C
  gcc -std=c11 -I "$INC_HUB" /tmp/c94_smoke.c -o /tmp/c94_smoke
  /tmp/c94_smoke
}

case "$MODE" in
  dry-12)
    build_c_check
    echo "=== [C94] DRY-RUN AER N=12 ==="
    python tools/ibm_quantum_runner_c94.py --N 12 --n_rep 4 --spsa_iters 15 --dry-run-aer
    ;;
  dry-16)
    build_c_check
    echo "=== [C94] DRY-RUN AER N=16 (peut prendre 5-10 min en sandbox) ==="
    python tools/ibm_quantum_runner_c94.py --N 16 --n_rep 4 --spsa_iters 12 --dry-run-aer
    ;;
  submit-12)
    build_c_check
    echo "=== [C94] SUBMIT IBM N=12 ==="
    python tools/ibm_quantum_runner_c94.py --N 12 --n_rep 4 --spsa_iters 15 --submit-ibm
    ;;
  submit-16)
    build_c_check
    echo "=== [C94] SUBMIT IBM N=16 ==="
    python tools/ibm_quantum_runner_c94.py --N 16 --n_rep 4 --spsa_iters 12 --submit-ibm
    ;;
  full-12)
    build_c_check
    echo "=== [C94] DRY-RUN puis SUBMIT IBM N=12 ==="
    python tools/ibm_quantum_runner_c94.py --N 12 --n_rep 4 --spsa_iters 15 --dry-run-aer
    python tools/ibm_quantum_runner_c94.py --N 12 --n_rep 4 --spsa_iters 15 --submit-ibm
    ;;
  build-c-only)
    build_c_check
    ;;
  *)
    echo "Usage: $0 {dry-12|dry-16|submit-12|submit-16|full-12|build-c-only}"
    exit 2
    ;;
esac
echo "=== [C94] termine MODE=$MODE ==="
