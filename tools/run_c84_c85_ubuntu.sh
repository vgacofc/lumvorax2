#!/usr/bin/env bash
# LumVorax C84+C85 — Lanceur Ubuntu via Doppler
# Usage : doppler run --config dev_lumvorax -- bash tools/run_c84_c85_ubuntu.sh [--full]
#   sans argument : C84-B uniquement (budget IBM ~30s)
#   --full        : C84-B puis C85 (n_rep=5 cross-backend, budget IBM ~5min)
set -euo pipefail

REPO_ROOT="${REPO_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$REPO_ROOT"

MODE="${1:-light}"
echo "[C84-C85] Repo : $REPO_ROOT"
echo "[C84-C85] Mode : $MODE"

# Charge secrets (Doppler prioritaire, Replit fallback)
if [ -f tools/doppler_secrets.sh ]; then
    # shellcheck disable=SC1091
    source tools/doppler_secrets.sh
fi

if [ -z "${IBM_API_KEY:-}" ]; then
    echo "[C84-C85] ❌ IBM_API_KEY absent. Verifier Doppler ou export."
    exit 2
fi
echo "[C84-C85] IBM_API_KEY present (len=${#IBM_API_KEY})"

# 1) Verification quota IBM (lecture seule)
echo ""
echo "[C84-C85] === Etape 1 : verification quota IBM ==="
bash tools/check_ibm_free_tier.sh || true

# 2) C84-B (validation experimentale, budget mince)
echo ""
echo "[C84-C85] === Etape 2 : C84-B Experimental Validation Layer ==="
if command -v uv &>/dev/null; then
    uv run python3 tools/ibm_quantum_runner_c84.py --shots 1024 --n-rep 3
else
    python3 tools/ibm_quantum_runner_c84.py --shots 1024 --n-rep 3
fi

# 3) C85 publication-grade (uniquement si --full)
if [ "$MODE" = "--full" ] || [ "$MODE" = "full" ]; then
    echo ""
    echo "[C84-C85] === Etape 3 : C85 Publication-grade Proof Layer ==="
    if command -v uv &>/dev/null; then
        uv run python3 tools/ibm_quantum_runner_c85.py \
            --shots 4096 --n-rep 5 \
            --backend-a ibm_fez --circuits hts_8q,hts_6q
    else
        python3 tools/ibm_quantum_runner_c85.py \
            --shots 4096 --n-rep 5 \
            --backend-a ibm_fez --circuits hts_8q,hts_6q
    fi
else
    echo ""
    echo "[C84-C85] === Etape 3 ignoree (relancer avec --full pour C85 publication) ==="
fi

# 4) Recap
echo ""
echo "[C84-C85] === Resume des artefacts produits ==="
ls -lt src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C84*.md 2>/dev/null | head -3 || true
ls -lt src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C85*.md 2>/dev/null | head -3 || true
ls -lt src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c84_*.json 2>/dev/null | head -3 || true
ls -lt src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c85_*.json 2>/dev/null | head -3 || true
ls -lt src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/c84_*.lum 2>/dev/null | head -3 || true
ls -lt src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/c85_*.lum 2>/dev/null | head -3 || true

echo ""
echo "[C84-C85] === Termine ==="
