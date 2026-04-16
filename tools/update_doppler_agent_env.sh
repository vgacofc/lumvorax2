#!/usr/bin/env bash
set -euo pipefail

echo "[INFO] Doppler sync start"

export DOPPLER_UPDATE_CHECK=false
export DOPPLER_NO_UPDATE_NOTIFIER=true

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
DOPPLER_CONFIG="${DOPPLER_CONFIG:-dev_lumvorax}"

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
if [ -z "${REPLIT_DEV_DOMAIN:-}" ]; then
    echo "[ERROR] REPLIT_DEV_DOMAIN absent. Lance ce script depuis le shell Replit pendant que le workflow tourne."
    exit 1
fi

URL="https://${REPLIT_DEV_DOMAIN}"
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

SET_ARGS=(
    REPLIT_URL="$URL"
    AGENT_TOKEN="$TOKEN"
    DEFAULT_JOB_TIMEOUT_S="$DEFAULT_JOB_TIMEOUT_S"
    BTC_DURATION_S="$BTC_DURATION_S"
)

"$DOPPLER_BIN" secrets set "${SET_ARGS[@]}" >/dev/null

if "$DOPPLER_BIN" configs get "$DOPPLER_CONFIG" >/dev/null 2>&1; then
    echo "[INFO] Applying same secrets to Doppler config=$DOPPLER_CONFIG"
    "$DOPPLER_BIN" secrets set --config "$DOPPLER_CONFIG" "${SET_ARGS[@]}" >/dev/null
else
    echo "[WARN] Doppler config '$DOPPLER_CONFIG' not found from this environment; default config was updated."
fi

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
echo "DOPPLER_CONFIG=$DOPPLER_CONFIG"
echo "----------------------------------------"
