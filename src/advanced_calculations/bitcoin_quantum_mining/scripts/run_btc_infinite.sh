#!/usr/bin/env bash
# LumVorax — Module 17 — Bitcoin Quantum Mining Engine
# scripts/run_btc_infinite.sh — Run INFINI vers 256 bits
#
# Version C41 — STANDARD_NAMES.md v4.2 §M-BTC17-C41 — 2026-04-13
#
# Corrections C41 actives :
#   [C41-1] Lockfree CAS atomique best_leading_global
#   [C41-2] Timestamp cache 1ms thread-local (8× moins de syscalls)
#   [C41-3] Cache-line align 64B btc_ptmc_replica_t (zéro false sharing)
#   [C41-4] Mutex orbital éliminé → atomic_load_relaxed
#   [C41-5] Batch 512 (vs 256) + nx48_every 200k (vs 100k)
#   [C41-6] Thermal throttle usleep 500µs si CPU>90%
#   [C41-7] FORENSIC_LOG hors record = no-op (0 I/O hot path)
#   [C41-8] btc_thread_work_t aligned(64)
#   [C41-SIMD-PREDICT] nx48_btc_predict déroulé 8 features (AVX2 vectorisé)
#   [C41-SIMD-ISTA]    ISTA gradient déroulé 8 features (AVX2 vectorisé)
#
# Système secrets :
#   Priorité 1 : Doppler (projet=lumvorax, config=dev_lumvorax)
#   Priorité 2 : Secrets Replit (fallback automatique)
#
# Intégrations :
#   Supermemory : container lumvorax_nx48 (662 docs / 237 mémoires)
#   Supabase    : tables btc_mining_runs, btc_records, btc_metrics_realtime
#   Aristocle   : envoi découvertes inconnues (fallback local si DNS fail)

set -e
cd "$(dirname "$0")/.."  # Se positionne dans bitcoin_quantum_mining/

BINARY="./btc_mining_runner"
SCRIPT_POW="scripts/fetch_btc_real_pow.py"
N_THREADS=8
CYCLE="C41"
VERSION="1.0.0-C41"
STAMP=$(date -u '+%Y-%m-%dT%H:%M:%SZ')

# ── Système de secrets : Doppler priorité → Replit fallback ──────
if [ -n "${DOPPLER_TOKEN:-}" ] && command -v doppler &>/dev/null; then
    echo "[BTC_RUN] Chargement secrets Doppler (lumvorax/dev_lumvorax)..."
    _DOPPLER_ENV=$(doppler secrets download \
        --token "$DOPPLER_TOKEN" \
        --project lumvorax \
        --config dev_lumvorax \
        --no-file --format env 2>/dev/null) && \
    eval "$_DOPPLER_ENV" && \
    echo "[BTC_RUN] Secrets Doppler OK" || \
    echo "[BTC_RUN] Doppler FAIL — secrets Replit utilisés"
else
    echo "[BTC_RUN] Secrets Replit actifs (Doppler indisponible)"
fi

# Vérification du binaire
if [ ! -x "$BINARY" ]; then
    echo "[ERREUR] Binaire $BINARY non trouvé — relancez : make all"
    exit 1
fi

echo "============================================================"
echo " LumVorax — Module 17 — Run INFINI vers 256 bits"
echo " $STAMP"
echo " Version : $VERSION | Cycle : $CYCLE | Threads : $N_THREADS"
echo " C41: SIMD AVX2 | Lockfree | Cache-line 64B | Batch 512"
echo "============================================================"

# ── Supermemory : init session ───────────────────────────────────
SUPERMEMORY_SCRIPT="../../tools/nx48_supermemory.py"
[ ! -f "$SUPERMEMORY_SCRIPT" ] && SUPERMEMORY_SCRIPT="$(pwd)/../../tools/nx48_supermemory.py"
if [ -f "$SUPERMEMORY_SCRIPT" ]; then
    python3 "$SUPERMEMORY_SCRIPT" --init "$STAMP" 2>&1 | \
        sed 's/^/[NX48-MEM] /' || true
fi

# ── Enregistrement démarrage dans Supabase (REST API — pas psycopg2) ─
python3 - "$CYCLE" "$VERSION" "$N_THREADS" "$STAMP" << 'PYEOF' 2>&1 | sed 's/^/[SUPABASE] /' || true
import os, sys, json, urllib.request
from datetime import datetime, timezone
cycle, version, threads, stamp = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
import os as _os
pid = str(_os.getpid())
run_id = f"btc_{stamp.replace('-','').replace(':','').replace('T','T').replace('Z','Z')}_{pid}"
supa_url = os.environ.get("SUPABASE_URL","")
supa_key = os.environ.get("SUPABASE_KEY","") or os.environ.get("SUPABASE_ANON_KEY","")
if not supa_url or not supa_key:
    print("SUPABASE_URL/KEY manquant — skip"); sys.exit(0)
headers = {"apikey": supa_key, "Authorization": f"Bearer {supa_key}",
           "Content-Type": "application/json", "Prefer": "return=minimal"}
payload = {"run_id": run_id, "cycle": cycle, "version": version,
           "mode": "BENCHMARK", "network": "MAINNET", "threads": int(threads),
           "batch_size": 512, "wallet_address": os.environ.get("BTC_WALLET_ADDRESS_TESTNET","unknown"),
           "started_at": datetime.now(timezone.utc).isoformat()}
try:
    req = urllib.request.Request(f"{supa_url}/rest/v1/btc_mining_runs",
        data=json.dumps(payload).encode(), headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=10) as r:
        print(f"Run inscrit OK dans btc_mining_runs — {r.status}")
except Exception as e:
    print(f"WARN: {e}")
PYEOF

# ── Récupération du vrai header Bitcoin ──────────────────────────
echo "[BTC_RUN] Recuperation header Bitcoin (blockstream.info)..."
REAL_HEADER=$(python3 "$SCRIPT_POW" 2>/dev/null | grep '^[0-9a-f]' | head -1)

if [ -z "$REAL_HEADER" ] || [ ${#REAL_HEADER} -lt 160 ]; then
    echo "[BTC_RUN] WARN: API blockstream inaccessible — header testnet synthetique"
    exec "$BINARY" \
        --mode BENCHMARK \
        --threads "$N_THREADS" \
        --duration-s 0
else
    echo "[BTC_RUN] Header Bitcoin reel recupere (${#REAL_HEADER} chars)"
    echo "[BTC_RUN] Premiers 40 chars: ${REAL_HEADER:0:40}..."
    echo "[BTC_RUN] Lancement moteur NX48 — $N_THREADS threads — DUREE : INFINIE"
    echo "[BTC_RUN] Objectif : bloc Bitcoin valide (recompense reelle si trouve)"
    echo "[BTC_RUN] Wallet: ${BTC_WALLET_ADDRESS_TESTNET:-mg4hhuNLQwcrL2g2jJamzswgb4ChbZ5tcj}"
    echo "------------------------------------------------------------"
    exec "$BINARY" \
        --mode BENCHMARK \
        --threads "$N_THREADS" \
        --duration-s 0 \
        --header-hex "$REAL_HEADER"
fi
