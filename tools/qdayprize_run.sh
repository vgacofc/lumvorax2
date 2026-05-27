#!/usr/bin/env bash
# ============================================================
# LumVorax C55 — QDAYPRIZE Runner
# Simulateur classique d'attaque quantique ECDLP (Bitcoin)
#
# Usage :
#   bash tools/qdayprize_run.sh
#   bash tools/qdayprize_run.sh --sim-bits 20 --duration 600
#
# En parallèle du BTC mining :
#   bash tools/qdayprize_run.sh &
#   bash tools/btc_run_ubuntu.sh
#
# QDAYPRIZE — Compétition d'attaque quantique Bitcoin
# Cible : 6 924 807 BTC (~490B USD) sur 13 978 108 adresses vulnérables
# ============================================================

set -euo pipefail

# ─── Détection repo ─────────────────────────────────────────
if [ -d "$HOME/LVX/lumvorax2" ]; then
    REPO_ROOT="$HOME/LVX/lumvorax2"
elif [ -d "/home/runner/workspace" ]; then
    REPO_ROOT="/home/runner/workspace"
else
    REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fi

QDPR_DIR="$REPO_ROOT/src/qdayprize"
LOG_DIR="$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/logs/forensic"

# ─── Paramètres par défaut ──────────────────────────────────
SIM_BITS="${QDPR_SIM_BITS:-16}"
DURATION="${QDPR_DURATION_S:-0}"
THREADS="${QDPR_THREADS:-1}"
PARALLEL_BTC="${QDPR_PARALLEL_BTC:-1}"

STAMP=$(date +%Y%m%dT%H%M%SZ)
RUN_ID="qdpr_c55_${STAMP}"

# ─── Passer les args en ligne de commande ───────────────────
EXTRA_ARGS=""
for arg in "$@"; do
    EXTRA_ARGS="$EXTRA_ARGS $arg"
done

echo "[QDPR] ============================================"
echo "[QDPR] LumVorax QDAYPRIZE Runner — Cycle C55"
echo "[QDPR] REPO_ROOT : $REPO_ROOT"
echo "[QDPR] RUN_ID    : $RUN_ID"
echo "[QDPR] SIM_BITS  : ${SIM_BITS} bits"
echo "[QDPR] DURATION  : ${DURATION}s (0=illimité)"
echo "[QDPR] THREADS   : ${THREADS}"
echo "[QDPR] THEORY    : secp256k1 → 2330 qubits logiques, ~4.47M physiques"
echo "[QDPR] ============================================"

# ─── Compilation ────────────────────────────────────────────
echo "[QDPR] Étape 1/2 — Compilation QDAYPRIZE..."
cd "$QDPR_DIR"
# C59-FIX-SIGILL : make clean OBLIGATOIRE pour supprimer les .o Replit (AVX-512)
# compilés avec -march=native sur CPU Replit et committés dans git.
# Sans make clean, le binaire hérite des .o AVX-512 → SIGILL sur i5-8265U.
make clean 2>&1 | tail -1
make 2>&1 | tail -5
if [ ! -f qdayprize_runner ]; then
    echo "[QDPR] ERREUR : Compilation échouée"
    exit 1
fi
echo "[QDPR] Binaire : $(du -h qdayprize_runner | cut -f1) qdayprize_runner"

# ─── Exécution ──────────────────────────────────────────────
echo "[QDPR] Étape 2/2 — Simulation QDAYPRIZE..."
mkdir -p "$LOG_DIR"

PARALLEL_FLAG=""
[ "$PARALLEL_BTC" = "1" ] && PARALLEL_FLAG="--parallel-btc"

./qdayprize_runner \
    --sim-bits "$SIM_BITS" \
    --duration "$DURATION" \
    --threads  "$THREADS" \
    --run-id   "$RUN_ID" \
    --log-dir  "$LOG_DIR" \
    $PARALLEL_FLAG \
    $EXTRA_ARGS 2>&1 | tee "/tmp/qdayprize_${RUN_ID}.log"

echo "[QDPR] Run terminé. Log : /tmp/qdayprize_${RUN_ID}.log"
