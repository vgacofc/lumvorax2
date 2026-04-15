#!/usr/bin/env bash
# LumVorax — Module 17 — Bitcoin Quantum Mining Engine
# scripts/run_btc_infinite.sh — Run INFINI vers 256 bits
#
# Version C42 — STANDARD_NAMES.md v4.2 §M-BTC17-C42 — 2026-04-13
#
# CORRECTIONS C42 :
#   [C42-WATCHDOG]     Thread watchdog C (RAM/CPU) intégré dans le binaire
#   [C42-RESTART-LOOP] Boucle de restart infinie — relance automatique sur crash/OOM
#   [C42-NICE]         Priorité nice +5 — moins agressif pour Replit (évite kill)
#   [C42-WIF-DECODE]   Décode BTC_WALLET_WIF → BTC_WALLET_PRIV_HEX automatiquement
#   [C42-SUPRA-INIT]   Supermemory --init récupère le dernier état NX48 au démarrage
#   [C42-CSV-UNIFIED]  CSV path unique : config/btc_nx48_last.csv (lecture + écriture)
#   [C42-WEIGHTS]      weights[8]+bias persistés dans CSV (voir nx48_btc_controller.c)
#   [C42-BASH-WATCHDOG] Watchdog bash surveille RAM/CPU et envoie SIGSTOP/SIGCONT
#
# Système secrets :
#   Priorité 1 : Doppler (projet=lumvorax, config=dev_lumvorax)
#   Priorité 2 : Secrets Replit (fallback automatique)
#
# Intégrations :
#   Supermemory : container lumvorax_nx48 — vraie mémoire persistante inter-sessions
#   Supabase    : tables btc_mining_runs, btc_records, btc_metrics_realtime
#   Aristocle   : fallback local logs/aristocle_pending_discoveries.jsonl

# ── Pas de set -e : on gère nous-mêmes les erreurs pour le restart ──
set +e
cd "$(dirname "$0")/.."  # Se positionne dans bitcoin_quantum_mining/

BINARY="./btc_mining_runner"
SCRIPT_POW="scripts/fetch_btc_real_pow.py"
N_THREADS=6           # C42: réduit 8→6 pour économiser CPU (Replit = 6 CPU)
CYCLE="C42"
VERSION="1.0.0-C43-DUAL-POW"
NX48_CSV="config/btc_nx48_last.csv"   # C42-CSV-UNIFIED : chemin unique

# ── Système de secrets : Doppler priorité → Replit fallback ──────
if [ -n "${DOPPLER_TOKEN:-}" ] && command -v doppler &>/dev/null; then
    echo "[BTC_RUN] Chargement secrets Doppler (lumvorax/dev_lumvorax)..."
    _DOPPLER_ENV=$(doppler secrets download \
        --token "$DOPPLER_TOKEN" \
        --project lumvorax \
        --config dev_lumvorax \
        --no-file --format env 2>/dev/null)
    if [ $? -eq 0 ] && [ -n "$_DOPPLER_ENV" ]; then
        eval "$_DOPPLER_ENV"
        echo "[BTC_RUN] Secrets Doppler OK"
    else
        echo "[BTC_RUN] Doppler FAIL — secrets Replit utilisés"
    fi
else
    echo "[BTC_RUN] Secrets Replit actifs (Doppler indisponible)"
fi

if [ -n "${BTC_WALLET_WIF:-}" ] && [ -z "${BTC_WALLET_PRIV_HEX:-}" ]; then
    echo "[BTC_RUN] Décodage BTC_WALLET_WIF → BTC_WALLET_PRIV_HEX..."
    _PRIV_HEX=$(python3 - "$BTC_WALLET_WIF" << 'WIFEOF' 2>/dev/null
import sys, hashlib
def wif_to_hex(wif):
    BASE58 = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'
    try:
        import base58  # si disponible
        decoded = base58.b58decode(wif)
    except ImportError:
        n = 0
        for ch in wif:
            if ch not in BASE58:
                return None
            n = n * 58 + BASE58.index(ch)
        h = format(n, 'x')
        if len(h) % 2: h = '0' + h
        decoded = bytes.fromhex(h)
    if len(decoded) < 37:
        return None
    checksum = hashlib.sha256(hashlib.sha256(decoded[:-4]).digest()).digest()[:4]
    if decoded[-4:] != checksum:
        return None
    payload = decoded[:-4]
    if payload[0] not in (0x80, 0xEF):
        return None
    if len(payload) == 34 and payload[-1] != 0x01:
        return None
    privkey_bytes = payload[1:33]  # 32 bytes clé privée
    out = privkey_bytes.hex()
    return out if len(out) == 64 else None
wif = sys.argv[1] if len(sys.argv) > 1 else ""
result = wif_to_hex(wif) if wif else None
print(result if result else "")
WIFEOF
)
    if [ -n "$_PRIV_HEX" ] && [ ${#_PRIV_HEX} -eq 64 ]; then
        export BTC_WALLET_PRIV_HEX="$_PRIV_HEX"
        echo "[BTC_RUN] Wallet WIF décodé → PRIV_HEX OK (${#_PRIV_HEX} chars)"
    else
        echo "[BTC_RUN] WARN: décodage WIF échoué — wallet aléatoire sera utilisé"
    fi
elif [ -n "${BTC_WALLET_PRIV_HEX:-}" ]; then
    echo "[BTC_RUN] BTC_WALLET_PRIV_HEX déjà disponible ✓"
else
    echo "[BTC_RUN] WARN: ni BTC_WALLET_WIF ni BTC_WALLET_PRIV_HEX — wallet aléatoire"
fi

echo "[BTC_RUN] Wallet testnet: ${BTC_WALLET_ADDRESS_TESTNET:-[non défini]}"
echo "[BTC_RUN] Wallet mainnet: ${BTC_WALLET_ADDRESS:-[non défini]}"

# Vérification du binaire
if [ ! -x "$BINARY" ]; then
    echo "[ERREUR] Binaire $BINARY non trouvé — compilation en cours..."
    if [ -f "Makefile" ]; then
        make all 2>&1 | tail -5
    fi
    if [ ! -x "$BINARY" ]; then
        echo "[ERREUR FATAL] Binaire introuvable après compilation — arrêt"
        exit 1
    fi
fi

SUPERMEMORY_SCRIPT=""
for candidate in "../../../tools/nx48_supermemory.py" "../quantum_problem_hubbard_hts/tools/nx48_supermemory.py" "../../tools/nx48_supermemory.py"; do
    if [ -f "$candidate" ]; then
        SUPERMEMORY_SCRIPT="$candidate"
        break
    fi
done
if [ -f "$SUPERMEMORY_SCRIPT" ]; then
    STAMP_INIT=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
    echo "[BTC_RUN] Supermemory --init : récupération état NX48 précédent..."
    python3 "$SUPERMEMORY_SCRIPT" --init "$STAMP_INIT" --csv "$NX48_CSV" 2>&1 | \
        sed 's/^/[NX48-MEM] /' || true
else
    echo "[BTC_RUN] WARN: nx48_supermemory.py introuvable — mémoire longue désactivée"
fi

MODULE_BRIDGE_SCRIPT=""
for candidate in "../../../tools/nx48_module_bridge.py" "../../tools/nx48_module_bridge.py"; do
    if [ -f "$candidate" ]; then
        MODULE_BRIDGE_SCRIPT="$candidate"
        break
    fi
done
if [ -f "$MODULE_BRIDGE_SCRIPT" ]; then
    MODULE_BRIDGE_JSON="config/btc_module_bridge_manifest.json"
    python3 "$MODULE_BRIDGE_SCRIPT" \
        --src-root "../../../src" \
        --output "$MODULE_BRIDGE_JSON" 2>&1 | sed 's/^/[BTC-MODULE-BRIDGE] /' || true
    if [ -f "$MODULE_BRIDGE_JSON" ]; then
        export LUMVORAX_BTC_MODULE_BRIDGE_MANIFEST="$MODULE_BRIDGE_JSON"
        export LUMVORAX_BTC_MODULE_BRIDGE_COUNT="$(python3 - "$MODULE_BRIDGE_JSON" <<'PYEOF' 2>/dev/null
import json, sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    print(len(json.load(f).get("modules", [])))
PYEOF
)"
        export LUMVORAX_BTC_MODULE_BRIDGE_KIND_COUNTS="$(python3 - "$MODULE_BRIDGE_JSON" <<'PYEOF' 2>/dev/null
import json, sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    data = json.load(f)
print(json.dumps(data.get("kind_counts", {}), sort_keys=True))
PYEOF
)"
        echo "[BTC_RUN] Pont modules src actif: ${LUMVORAX_BTC_MODULE_BRIDGE_COUNT:-0} modules → $MODULE_BRIDGE_JSON"
        echo "[BTC_RUN] Familles modules: ${LUMVORAX_BTC_MODULE_BRIDGE_KIND_COUNTS:-{}}"
    fi
fi

# ── RAM dispo au démarrage ────────────────────────────────────────
RAM_AVAIL_MB=$(awk '/MemAvailable/{printf "%d", $2/1024}' /proc/meminfo 2>/dev/null || echo "0")
echo "[BTC_RUN] RAM disponible: ${RAM_AVAIL_MB}MB | CPU: $(nproc) cores"

# ── C42-NICE : Priorité nice +5 pour ne pas saturer l'OS Replit ──
NICE_CMD=""
if command -v nice &>/dev/null; then
    NICE_CMD="nice -n 5"
    echo "[BTC_RUN] Priorité CPU: nice +5 (évite kill OS)"
fi

# ── Fonction watchdog bash : surveille RAM/CPU externe + throttle ─
btc_bash_watchdog() {
    local MINING_PID="$1"
    local PAUSED=0
    while kill -0 "$MINING_PID" 2>/dev/null; do
        RAM_FREE=$(awk '/MemAvailable/{print $2}' /proc/meminfo 2>/dev/null || echo "999999999")
        RAM_MB=$((RAM_FREE / 1024))
        if [ "$RAM_MB" -lt 150 ]; then
            # RAM ultra-critique → SIGSTOP temporaire
            if [ "$PAUSED" -eq 0 ]; then
                echo "[C42-BASH-WATCHDOG] 🔴 RAM ${RAM_MB}MB CRITIQUE — SIGSTOP mining"
                kill -SIGSTOP "$MINING_PID" 2>/dev/null
                PAUSED=1
            fi
            sleep 15
        elif [ "$PAUSED" -eq 1 ] && [ "$RAM_MB" -gt 400 ]; then
            echo "[C42-BASH-WATCHDOG] 🟢 RAM ${RAM_MB}MB OK — SIGCONT reprise mining"
            kill -SIGCONT "$MINING_PID" 2>/dev/null
            PAUSED=0
        fi
        sleep 8
    done
}

# ── Enregistrement démarrage dans Supabase (REST API) ────────────
python3 - "$CYCLE" "$VERSION" "$N_THREADS" << 'PYEOF' 2>&1 | sed 's/^/[SUPABASE] /' || true
import os, sys, json, urllib.request
from datetime import datetime, timezone
cycle, version, threads = sys.argv[1], sys.argv[2], sys.argv[3]
import os as _os; pid = str(_os.getpid())
from datetime import datetime, timezone
stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
run_id = f"btc_{stamp}_{pid}"
supa_url = os.environ.get("SUPABASE_URL","") or os.environ.get("SUPABASE8_API_URL","")
supa_key = os.environ.get("SUPABASE_SERVICE_ROLE_KEY","")
if not supa_url or not supa_key:
    print("SUPABASE_URL/SUPABASE_SERVICE_ROLE_KEY manquant — skip"); sys.exit(0)
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
    USE_REAL_HEADER=0
else
    echo "[BTC_RUN] Header Bitcoin reel recupere (${#REAL_HEADER} chars)"
    echo "[BTC_RUN] Premiers 40 chars: ${REAL_HEADER:0:40}..."
    USE_REAL_HEADER=1
fi

echo "============================================================"
echo " LumVorax — Module 17 — Run INFINI vers 256 bits"
echo " Version : $VERSION | Cycle : $CYCLE | Threads : $N_THREADS"
echo " C43: Dual-neuron NX48 | POW candidate export | Watchdog | WIF"
echo " CSV: $NX48_CSV (weights[8]+bias persistés)"
echo " Wallet: ${BTC_WALLET_ADDRESS_TESTNET:-mg4hhuNLQwcrL2g2jJamzswgb4ChbZ5tcj}"
echo "============================================================"

# ══════════════════════════════════════════════════════════════════
# C42-RESTART-LOOP : Boucle de restart INFINIE
# Le binaire est relancé automatiquement sur tout code de sortie.
# Code 42 = arrêt propre via signal handler C (CSV déjà sauvegardé).
# Code 1/2 = erreur → restart après délai adaptatif.
# Code 0   = session normale → restart immédiat.
# ══════════════════════════════════════════════════════════════════
RESTART_COUNT=0
RESTART_DELAY=3      # Délai initial entre restarts (secondes)
MAX_RESTART_DELAY=60 # Délai max si erreurs répétées

while true; do
    STAMP=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
    RESTART_COUNT=$((RESTART_COUNT + 1))
    echo ""
    echo "[BTC_RUN] ═══ START #$RESTART_COUNT — $STAMP ═══"
    echo "[BTC_RUN] NX48 neurones actifs: 2 (producteur apprend, applicateur décide)"
    echo "[BTC_RUN] RAM dispo: $(awk '/MemAvailable/{printf "%.0fMB", $2/1024}' /proc/meminfo)"

    # Lancement du binaire (en arrière-plan pour pouvoir surveiller)
    if [ "$USE_REAL_HEADER" -eq 1 ]; then
        $NICE_CMD "$BINARY" \
            --mode BENCHMARK \
            --threads "$N_THREADS" \
            --duration-s 0 \
            --nx48-csv "$NX48_CSV" \
            --header-hex "$REAL_HEADER" &
    else
        $NICE_CMD "$BINARY" \
            --mode BENCHMARK \
            --threads "$N_THREADS" \
            --duration-s 0 \
            --nx48-csv "$NX48_CSV" &
    fi
    MINING_PID=$!
    echo "[BTC_RUN] PID mining: $MINING_PID"

    # Lancer le watchdog bash en parallèle
    btc_bash_watchdog "$MINING_PID" &
    WATCHDOG_PID=$!

    # Attendre la fin du binaire
    wait "$MINING_PID"
    EXIT_CODE=$?

    # Arrêter le watchdog bash
    kill "$WATCHDOG_PID" 2>/dev/null
    wait "$WATCHDOG_PID" 2>/dev/null

    # Analyse du code de sortie
    STAMP_END=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
    echo "[BTC_RUN] Binaire terminé à $STAMP_END (code=$EXIT_CODE) — restart #$((RESTART_COUNT+1)) dans ${RESTART_DELAY}s"

    # Envoie état NX48 à Supermemory après chaque run
    if [ -f "$SUPERMEMORY_SCRIPT" ] && [ -f "$NX48_CSV" ]; then
        python3 "$SUPERMEMORY_SCRIPT" \
            --store-run \
            --csv "$NX48_CSV" \
            --cycle "$CYCLE" \
            --run-id "btc_${STAMP}_run${RESTART_COUNT}" 2>&1 | \
            sed 's/^/[NX48-MEM] /' || true
    fi

    # Adapter le délai selon le code de sortie
    if [ "$EXIT_CODE" -eq 42 ]; then
        # Arrêt propre via signal handler → restart rapide
        RESTART_DELAY=3
        echo "[BTC_RUN] Arrêt propre (signal) — CSV sauvegardé — restart rapide"
    elif [ "$EXIT_CODE" -eq 0 ]; then
        # Session normale terminée → restart rapide
        RESTART_DELAY=3
    else
        # Erreur → délai progressif (backoff) pour éviter crash loop
        RESTART_DELAY=$((RESTART_DELAY * 2))
        [ "$RESTART_DELAY" -gt "$MAX_RESTART_DELAY" ] && RESTART_DELAY=$MAX_RESTART_DELAY
        echo "[BTC_RUN] Erreur (code=$EXIT_CODE) — backoff ${RESTART_DELAY}s"
    fi

    # Récupérer un nouveau header Bitcoin à chaque restart
    echo "[BTC_RUN] Récupération nouveau header Bitcoin..."
    NEW_HEADER=$(python3 "$SCRIPT_POW" 2>/dev/null | grep '^[0-9a-f]' | head -1)
    if [ -n "$NEW_HEADER" ] && [ ${#NEW_HEADER} -ge 160 ]; then
        REAL_HEADER="$NEW_HEADER"
        USE_REAL_HEADER=1
        echo "[BTC_RUN] Nouveau header OK (${#REAL_HEADER} chars)"
    fi

    sleep "$RESTART_DELAY"
done
