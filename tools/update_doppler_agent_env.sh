#!/usr/bin/env bash
set -euo pipefail

echo "[INFO] Doppler sync start"

# ------------------------------------------------------------
# 1. Detect Replit
# ------------------------------------------------------------
IS_REPLIT=0
if [[ -n "${REPL_ID:-}" || -n "${REPLIT_DEV_DOMAIN:-}" ]]; then
    IS_REPLIT=1
    echo "[INFO] Replit environment detected (Nix-managed Doppler)"
else
    echo "[INFO] Local/Ubuntu environment detected"
fi

# ------------------------------------------------------------
# 2. Check Doppler availability
# ------------------------------------------------------------
if ! command -v doppler >/dev/null 2>&1; then
    echo "[ERROR] Doppler CLI not found"
    exit 1
fi

DOPPLER_BIN=$(command -v doppler)
echo "[INFO] Using Doppler: $DOPPLER_BIN"

# ------------------------------------------------------------
# 3. Get token
# ------------------------------------------------------------
echo "[INFO] Fetching agent token..."

TOKEN=$(python3 - <<'PY'
import json
import urllib.request
import sys

try:
    with urllib.request.urlopen("http://127.0.0.1:5000/agent/token", timeout=10) as r:
        print(json.load(r)["token"])
except Exception as e:
    print(f"[ERROR] {e}", file=sys.stderr)
    sys.exit(1)
PY
)

# ------------------------------------------------------------
# 4. Env values
# ------------------------------------------------------------
URL="https://${REPLIT_DEV_DOMAIN:-localhost}"
DEFAULT_JOB_TIMEOUT_S="${DEFAULT_JOB_TIMEOUT_S:-0}"
BTC_DURATION_S="${BTC_DURATION_S:-0}"

# ------------------------------------------------------------
# 5. IMPORTANT: NO UPDATE ATTEMPT (Replit-safe rule)
# ------------------------------------------------------------
if [[ "$IS_REPLIT" -eq 1 ]]; then
    echo "[INFO] Skipping any Doppler update (Nix immutable environment)"
fi

# ------------------------------------------------------------
# 6. Apply secrets (ONLY supported operation)
# ------------------------------------------------------------
echo "[INFO] Applying Doppler secrets..."

$DOPPLER_BIN secrets set \
    REPLIT_URL="$URL" \
    AGENT_TOKEN="$TOKEN" \
    DEFAULT_JOB_TIMEOUT_S="$DEFAULT_JOB_TIMEOUT_S" \
    BTC_DURATION_S="$BTC_DURATION_S" \
    >/dev/null

# ------------------------------------------------------------
# 7. Output
# ------------------------------------------------------------
echo "----------------------------------------"
echo "[SUCCESS] Doppler sync completed"
echo "REPLIT_URL=$URL"
echo "AGENT_TOKEN=${TOKEN:0:8}..."
echo "DEFAULT_JOB_TIMEOUT_S=$DEFAULT_JOB_TIMEOUT_S"
echo "BTC_DURATION_S=$BTC_DURATION_S"
echo "DOPPLER_BIN=$DOPPLER_BIN"
echo "----------------------------------------"
