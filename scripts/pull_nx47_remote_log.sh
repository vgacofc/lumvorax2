#!/usr/bin/env bash
set -euo pipefail

REPO_OWNER="${1:-lumc01}"
REPO_NAME="${2:-Lumvorax}"
BRANCH="${3:-main}"
LOG_FILE="${4:-nx47-vesu-kernel-new-v134.1.log}"
OUT_FILE="${5:-$LOG_FILE}"

URL="https://raw.githubusercontent.com/${REPO_OWNER}/${REPO_NAME}/${BRANCH}/${LOG_FILE}"

echo "[INFO] Downloading: ${URL}"
curl -L -f -sS "${URL}" -o "${OUT_FILE}"

echo "[OK] Saved ${OUT_FILE}"
wc -l "${OUT_FILE}"
