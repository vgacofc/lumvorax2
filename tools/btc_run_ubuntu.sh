#!/usr/bin/env bash
# ============================================================
# LumVorax C48 — Script de mining BTC Ubuntu — BASH UNIQUEMENT
#
# Sous fish, toujours lancer avec :
#   bash ~/LVX/lumvorax2/tools/btc_run_ubuntu.sh
#
# Ce script détecte automatiquement Replit vs Ubuntu.
# Il compile le binaire, récupère un header Bitcoin réel,
# et lance un run illimité en loggant dans ~/btc_ubuntu.log.
# ============================================================

set -euo pipefail

# ─── Détection environnement ───────────────────────────────
if [ -d "$HOME/LVX/lumvorax2" ]; then
    REPO_ROOT="$HOME/LVX/lumvorax2"
    ENV_NAME="ubuntu_lvx"
elif [ -d "/home/runner/workspace" ]; then
    REPO_ROOT="/home/runner/workspace"
    ENV_NAME="replit"
else
    REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
    ENV_NAME="unknown"
fi

BTC_DIR="$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining"
SCRIPTS_DIR="$BTC_DIR/scripts"
LOG_FILE="$HOME/btc_ubuntu.log"
NPROC=$(nproc)
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
RUN_ID="btc_${STAMP}_c48_${ENV_NAME}"

echo "[C48] ============================================"
echo "[C48] LumVorax BTC Mining Runner"
echo "[C48] Environnement : $ENV_NAME"
echo "[C48] REPO_ROOT     : $REPO_ROOT"
echo "[C48] CPUs dispo    : $NPROC"
echo "[C48] Run ID        : $RUN_ID"
echo "[C48] Log           : $LOG_FILE"
echo "[C48] ============================================"

# ─── Étape 1 : Compilation propre ─────────────────────────
echo "[C48] Étape 1/3 — Compilation binaire BTC..."
# Sync depuis GitHub si on est dans un dépôt git
if [ -d "$REPO_ROOT/.git" ]; then
    echo "[C48] Sync git (git pull origin main)..."
    cd "$REPO_ROOT" && git pull origin main --quiet 2>&1 | tail -3 || echo "[WARN] git pull échoué — compilation avec état local"
fi

cd "$BTC_DIR"
make clean 2>&1 | tail -3
make all 2>&1 | tail -5
echo "[C48] Binaire : $(ls -lh btc_mining_runner | awk '{print $5, $9}')"

# ─── Étape 2 : Récupérer header Bitcoin réel ──────────────
echo "[C48] Étape 2/3 — Récupération header Bitcoin réel..."
REAL_HEADER=""

# Essayer d'abord avec les secrets Doppler (BTC_RPC_URL si dispo)
if [ -n "${BTC_RPC_URL:-}" ]; then
    echo "[C48]   Via RPC : $BTC_RPC_URL"
    RPC_RESULT=$(python3 "$SCRIPTS_DIR/btc_getblocktemplate_job.py" \
        --rpc-url "$BTC_RPC_URL" \
        --rpc-user "${BTC_RPC_USER:-lumvorax}" \
        --rpc-pass "${BTC_RPC_PASS:-LumVorax2026BTC!}" \
        --output /tmp/btc_job_c48.json 2>&1) || true
    REAL_HEADER=$(python3 -c "
import json
try:
    d = json.load(open('/tmp/btc_job_c48.json'))
    print(d.get('header_hex',''))
except:
    print('')
" 2>/dev/null)
fi

# Fallback : Blockstream API
if [ -z "$REAL_HEADER" ]; then
    echo "[C48]   Fallback → Blockstream API..."
    REAL_HEADER=$(python3 "$SCRIPTS_DIR/fetch_btc_real_pow.py" 2>/dev/null | grep '^[0-9a-f]' | head -1 || echo "")
fi

if [ -n "$REAL_HEADER" ]; then
    echo "[C48]   Header : ${REAL_HEADER:0:20}... (${#REAL_HEADER} chars)"
else
    echo "[C48]   Header non disponible — mining avec header par défaut"
fi

# ─── Étape 3 : Lancement du mining ────────────────────────
echo "[C48] Étape 3/3 — Mining illimité (CTRL+C pour arrêter)..."
echo "[C48] Log complet : $LOG_FILE"
echo "[C48] ============================================"

cd "$BTC_DIR"

# Construire les arguments
ARGS="--threads $NPROC --run-id $RUN_ID"
if [ -n "$REAL_HEADER" ]; then
    ARGS="$ARGS --header-hex $REAL_HEADER"
fi

# Lancer le mining illimité (duration-s 0 = infini)
./btc_mining_runner $ARGS 2>&1 | tee "$LOG_FILE"
