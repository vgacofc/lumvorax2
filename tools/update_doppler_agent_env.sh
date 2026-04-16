#!/usr/bin/env bash
set -euo pipefail

if [ -z "${REPLIT_DEV_DOMAIN:-}" ]; then
    echo "REPLIT_DEV_DOMAIN absent. Lance ce script dans le shell Replit."
    exit 1
fi

TOKEN=$(python3 - <<'PY'
import json
import urllib.request

with urllib.request.urlopen("http://127.0.0.1:5000/agent/token", timeout=10) as response:
    print(json.load(response)["token"])
PY
)

URL="https://${REPLIT_DEV_DOMAIN}"
DEFAULT_JOB_TIMEOUT_S="${DEFAULT_JOB_TIMEOUT_S:-0}"
BTC_DURATION_S="${BTC_DURATION_S:-0}"

doppler secrets set \
    REPLIT_URL="$URL" \
    AGENT_TOKEN="$TOKEN" \
    DEFAULT_JOB_TIMEOUT_S="$DEFAULT_JOB_TIMEOUT_S" \
    BTC_DURATION_S="$BTC_DURATION_S"

echo "Doppler mis à jour."
echo "REPLIT_URL=$URL"
echo "AGENT_TOKEN=${TOKEN:0:8}..."
echo "DEFAULT_JOB_TIMEOUT_S=$DEFAULT_JOB_TIMEOUT_S"
echo "BTC_DURATION_S=$BTC_DURATION_S"