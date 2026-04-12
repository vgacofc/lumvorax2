#!/usr/bin/env bash
# LumVorax — Module 17 — Bitcoin Quantum Mining Engine
# scripts/run_btc_infinite.sh — Run INFINI vers 256 bits
#
# Usage:
#   bash scripts/run_btc_infinite.sh
#
# Description:
#   1. Récupère le vrai header Bitcoin depuis blockstream.info (API publique)
#   2. Lance btc_mining_runner en mode ILLIMITÉ (--duration-s 0)
#   3. Le moteur s'arrête uniquement si :
#      - Un bloc valide est trouvé (256 bits de leading zeros)
#      - L'utilisateur envoie SIGTERM/SIGINT (Ctrl+C)
#
# Corrections C39 actives :
#   - mutex PT-MC global (C39-P2)
#   - Orbital radius 50k, threshold 22 bits (C39-P3)
#   - Micro-perturbations OGY (C39-P4)
#   - Focused Scan Lebesgue (C39-P5)
#   - delta_nonce_scale max 50.0 (C39)
#
# Conformité : STANDARD_NAMES.md v4.2 §M-BTC17-C38
# Ref : analysechatgpt91.38.md §10 Plan C39 — 2026-04-12

set -e
cd "$(dirname "$0")/.."  # Se positionne dans bitcoin_quantum_mining/

BINARY="./btc_mining_runner"
SCRIPT_POW="scripts/fetch_btc_real_pow.py"
N_THREADS=8

# Vérification du binaire
if [ ! -x "$BINARY" ]; then
    echo "[ERREUR] Binaire $BINARY non trouvé — relancez : make all"
    exit 1
fi

echo "============================================================"
echo " LumVorax — Module 17 — Run INFINI vers 256 bits"
echo " $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
echo " Corrections : C39-P2 mutex | C39-P3 orbital 50k | C39-P4 OGY | C39-P5 Lebesgue"
echo "============================================================"

# Récupération du vrai header Bitcoin
echo "[BTC_RUN] Récupération du vrai header Bitcoin (blockstream.info)..."
REAL_HEADER=$(python3 "$SCRIPT_POW" 2>/dev/null | grep '^[0-9a-f]' | head -1)

if [ -z "$REAL_HEADER" ] || [ ${#REAL_HEADER} -lt 160 ]; then
    echo "[BTC_RUN] AVERTISSEMENT: API blockstream inaccessible — header testnet synthétique"
    echo "[BTC_RUN] Lancement sans --header-hex (difficulté testnet minimale)"
    exec "$BINARY" \
        --mode BENCHMARK \
        --threads "$N_THREADS" \
        --duration-s 0
else
    echo "[BTC_RUN] Header Bitcoin réel récupéré (${#REAL_HEADER} chars)"
    echo "[BTC_RUN] Premiers 40 chars: ${REAL_HEADER:0:40}..."
    echo "[BTC_RUN] Lancement moteur NX48 — $N_THREADS threads — DURÉE : INFINIE"
    echo "[BTC_RUN] Objectif : 256 bits de leading zeros"
    echo "------------------------------------------------------------"
    exec "$BINARY" \
        --mode BENCHMARK \
        --threads "$N_THREADS" \
        --duration-s 0 \
        --header-hex "$REAL_HEADER"
fi
