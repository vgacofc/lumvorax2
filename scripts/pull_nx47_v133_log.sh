#!/usr/bin/env bash
set -euo pipefail

REPO_OWNER="${1:-lumc01}"
REPO_NAME="${2:-Lumvorax}"
BRANCH="${3:-main}"
OUT_FILE="${4:-nx47-vesu-kernel-new-v133.log}"

URL="https://raw.githubusercontent.com/${REPO_OWNER}/${REPO_NAME}/${BRANCH}/nx47-vesu-kernel-new-v133.log"

echo "[INFO] Downloading: ${URL}"
curl -L -f -sS "${URL}" -o "${OUT_FILE}"

echo "[OK] Saved ${OUT_FILE}"
wc -l "${OUT_FILE}"
