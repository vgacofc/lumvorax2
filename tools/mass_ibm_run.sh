#!/usr/bin/env bash
set -u
LOG=/tmp/c85run/mass_ibm_v2.log
mkdir -p /tmp/c85run
{
  echo "=== MASS-RUN IBM QUANTUM ($(date -u)) ==="
  echo "Budget : 505s restantes / 600s/mois"
  echo ""
  echo "--- Run 1/2 : C85 hts_8q + hts_6q ibm_fez (shots=512 n_rep=2) ---"
  uv run python3 tools/ibm_quantum_runner_c85.py \
    --shots 512 --n-rep 2 \
    --backend-a ibm_fez \
    --circuits hts_8q,hts_6q 2>&1 | tail -30
  echo ""
  echo "--- DONE Run 1 $(date -u) ---"
  echo ""
  echo "--- Run 2/2 : C84 cross-backend ibm_fez vs ibm_marrakesh hts_8q ---"
  uv run python3 tools/ibm_quantum_runner_c85.py \
    --shots 256 --n-rep 2 \
    --backend-a ibm_fez \
    --backend-b ibm_marrakesh \
    --circuits hts_8q 2>&1 | tail -30
  echo ""
  echo "--- DONE Run 2 $(date -u) ---"
  echo "=== FIN MASS-RUN ==="
} > "$LOG" 2>&1
echo "DONE" > /tmp/c85run/mass_ibm.done
