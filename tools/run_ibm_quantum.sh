#!/bin/bash
# LumVorax C63 — IBM Quantum Launcher via Doppler
# Usage : ./tools/run_ibm_quantum.sh [--qdayprize|--hubbard|--pipeline|--test]

set -euo pipefail
cd "$(dirname "$0")/.."

CMD="${1:---test}"

echo "═══════════════════════════════════════════════════════"
echo "  LumVorax C63 — IBM Quantum Runner"
echo "  Commande : $CMD"
echo "  $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
echo "═══════════════════════════════════════════════════════"

# Essayer Doppler d'abord, fallback sur les vars d'env déjà définies
if command -v doppler &>/dev/null; then
  doppler run --config dev_lumvorax -- uv run python3 tools/ibm_quantum_runner.py $CMD
else
  uv run python3 tools/ibm_quantum_runner.py $CMD
fi
