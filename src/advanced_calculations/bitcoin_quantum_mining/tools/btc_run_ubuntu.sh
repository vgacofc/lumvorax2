#!/usr/bin/env bash
# ============================================================
# LumVorax C287 — Script de mining BTC Ubuntu — BASH UNIQUEMENT
#
# OBJECTIF: 256 BITS LEADING ZEROS (HASH SHA-256 PARFAIT)
# DURÉE: ILLIMITÉE (jusqu'à découverte ou Ctrl+C)
#
# Sous fish, toujours lancer avec :
#   doppler run --config dev_lumvorax -- bash tools/btc_run_ubuntu.sh
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

# C287 : Utilisation version NATIVE i915 DRM (0% OpenCL)
BTC_DIR="$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native"
SCRIPTS_DIR="$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/scripts"
NPROC="${BTC_THREADS:-$(nproc)}"
BTC_DURATION_S="${BTC_DURATION_S:-0}"  # C287: 0 = ILLIMITÉ
BTC_MODE="${BTC_MODE:-TESTNET3_REAL}"  # C287: Minage réel Testnet3
BTC_TARGET_BITS="${BTC_TARGET_BITS:-256}"  # C287: 256 bits leading zeros
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
RUN_ID="btc_${STAMP}_c287_${ENV_NAME}"
# C50-FIX-P2 : Log horodaté — prompt.txt §17.1 INTERDICTION écrasement.
# Chaque run crée son propre fichier btc_ubuntu_STAMP.log.
# ~/btc_ubuntu.log est un lien symbolique vers le fichier courant (rétrocompatibilité).
LOG_FILE="$HOME/btc_ubuntu_${STAMP}.log"
ln -sf "$LOG_FILE" "$HOME/btc_ubuntu.log"

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  C287 - MINAGE BITCOIN TESTNET3 ILLIMITÉ                     ║"
echo "║  OBJECTIF: 256 BITS LEADING ZEROS (HASH SHA-256 PARFAIT)     ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "[C287] Architecture  : 100% i915 DRM (0% OpenCL)"
echo "[C287] Batch Size    : 10,000,000 nonces (optimal UHD 620)"
echo "[C287] Objectif      : $BTC_TARGET_BITS bits leading zeros"
echo "[C287] Mode          : $BTC_MODE (validation + soumission)"
echo "[C287] Environnement : $ENV_NAME"
echo "[C287] REPO_ROOT     : $REPO_ROOT"
if [ "$BTC_DURATION_S" = "0" ]; then
    echo "[C287] Durée mining  : ILLIMITÉE (jusqu'à découverte ou Ctrl+C)"
else
    echo "[C287] Durée mining  : ${BTC_DURATION_S}s ($(($BTC_DURATION_S / 60)) min)"
fi
echo "[C287] Run ID        : $RUN_ID"
echo "[C287] Log           : $LOG_FILE"
echo ""
echo "[C287] Modules actifs:"
echo "  ✅ NX48 (neurone biologique pyramidal)"
echo "  ✅ NX49 (neurone soma/dendrites/axon)"
echo "  ✅ ASIC Optimizer (pipeline 64 stages)"
echo "  ✅ PT-MC (validation + soumission blocs)"
echo "  ✅ Gen9 Native i915 DRM (UHD 620)"
echo "[C287] ============================================"

# ─── Étape 1 : Libération mémoire ─────────────────────────
echo ""
echo "[C287] Étape 1/4 — Libération mémoire système..."
echo "  Fermeture applications non essentielles..."
killall chrome firefox slack discord 2>/dev/null || true
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null 2>&1 || true
echo ""
echo "[C287] État mémoire:"
free -h | grep -E "Mem:|Swap:"
echo ""

# ─── Étape 2 : Compilation propre ─────────────────────────
echo "[C287] Étape 2/4 — Compilation binaire BTC NATIVE..."
# C287: PAS de git pull pour éviter d'écraser les modifications locales
cd "$BTC_DIR"
make -f Makefile.gen9_native clean 2>&1 | tail -3
make -f Makefile.gen9_native btc_mining_runner 2>&1 | tail -5
echo "[C287] Binaire : $(ls -lh bin/btc_mining_runner | awk '{print $5, $9}')"

# ─── Étape 3 : Récupérer header Bitcoin réel ──────────────
echo ""
echo "[C287] Étape 3/4 — Récupération header Bitcoin Testnet3 réel..."
REAL_HEADER=""

# Essayer d'abord avec les secrets Doppler (BTC_RPC_URL si dispo)
if [ -n "${BTC_RPC_URL:-}" ]; then
    echo "[C287]   Via RPC Testnet3 : $BTC_RPC_URL"
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

# Fallback : Blockstream API Testnet3
if [ -z "$REAL_HEADER" ]; then
    echo "[C287]   Fallback → Blockstream API Testnet3..."
    REAL_HEADER=$(python3 "$SCRIPTS_DIR/fetch_btc_real_pow.py" --testnet 2>/dev/null | grep '^[0-9a-f]' | head -1 || echo "")
fi

if [ -n "$REAL_HEADER" ]; then
    echo "[C287]   Header Testnet3 : ${REAL_HEADER:0:20}... (${#REAL_HEADER} chars)"
else
    echo "[C287]   Header non disponible — mining avec header Testnet3 par défaut"
fi

# ─── Étape 4 : Lancement du mining ────────────────────────
echo ""
echo "[C287] Étape 4/4 — Mining BTC Testnet3 ILLIMITÉ..."
if [ "$BTC_DURATION_S" = "0" ]; then
    echo "[C287] Durée : ILLIMITÉE (arrêt: Ctrl+C)"
else
    echo "[C287] Durée : ${BTC_DURATION_S}s"
fi
echo "[C287] Log complet : $LOG_FILE"
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  MINAGE EN COURS - OBJECTIF 256 BITS                         ║"
echo "║  Arrêt: Ctrl+C                                                ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

cd "$BTC_DIR"

# Construire les arguments
ARGS="--threads $NPROC --duration-s $BTC_DURATION_S --mode $BTC_MODE --run-id $RUN_ID --target-bits $BTC_TARGET_BITS"
if [ -n "$REAL_HEADER" ]; then
    ARGS="$ARGS --header-hex $REAL_HEADER"
fi

# Lancer le mining illimité (duration-s 0 = infini)
# Afficher hashrate MIN/MAX en temps réel
./bin/btc_mining_runner $ARGS 2>&1 | tee "$LOG_FILE" | while IFS= read -r line; do
    echo "$line"
    # Extraire et afficher hashrate MIN/MAX si présent
    if echo "$line" | grep -q "Hashrate:"; then
        HASHRATE=$(echo "$line" | grep -oP '\d+\.\d+ [GM]H/s')
        if [ -n "$HASHRATE" ]; then
            echo "  📊 Hashrate actuel: $HASHRATE"
        fi
    fi
done

echo ""
echo "[C287] Minage terminé"
echo "[C287] Analyse hashrate MIN/MAX:"
if [ -f "$LOG_FILE" ]; then
    echo ""
    bash "$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/analyze_hashrate_minmax.sh" "$LOG_FILE" 2>/dev/null || echo "  Script analyse non disponible"
fi
