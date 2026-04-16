#!/usr/bin/env bash
# ============================================================
# LumVorax C48 — Agent Ubuntu (client pull) — BASH UNIQUEMENT
#
# IMPORTANT : Ce script doit être lancé avec bash, PAS fish.
# Sous fish, toujours écrire :  bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
#
# Usage sans Doppler (recommandé si Doppler token invalide) :
#   bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
#
# Usage avec Doppler (si token valide) :
#   doppler run -- bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
#
# Token Doppler invalide → utiliser sans Doppler, le script
# a les bonnes valeurs par défaut hardcodées.
# ============================================================

# ─── Détection automatique de l'environnement ─────────────
# Fonctionne sur Replit ET sur Ubuntu sans modification
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
TOOLS_DIR="$REPO_ROOT/tools"
BTC_DIR="$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining"

# ─── Valeurs par défaut — mises à jour C48 ────────────────
# Ces valeurs sont utilisées si Doppler n'injecte pas les siennes
# ou si Doppler est indisponible.
AGENT_TOKEN="${AGENT_TOKEN:-3de963ebc09043b3b1b9f22e1e771ecc}"
REPLIT_URL="${REPLIT_URL:-https://de10cf97-9bc3-4cd9-9cb9-e3269b99076a-00-3b0h1avyi2m16.picard.replit.dev}"
POLL_INTERVAL="${POLL_INTERVAL:-5}"
LOG_FILE="$HOME/lumvorax_agent.log"

# ─── Couleurs ─────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'

log() { echo -e "$(date -u '+%Y-%m-%dT%H:%M:%SZ') $1" | tee -a "$LOG_FILE"; }

log "${CYAN}[LumVorax Agent C48] Démarrage${NC}"
log "${CYAN}  Environnement : $ENV_NAME${NC}"
log "${CYAN}  REPO_ROOT     : $REPO_ROOT${NC}"
log "${CYAN}  Replit URL    : $REPLIT_URL${NC}"
log "${CYAN}  Poll interval : ${POLL_INTERVAL}s${NC}"
log "${CYAN}  Log           : $LOG_FILE${NC}"
log "${CYAN}  Token (8ch)   : ${AGENT_TOKEN:0:8}...${NC}"

# ─── Vérification connectivité ────────────────────────────
STATUS_RESP=$(curl -s --max-time 10 "$REPLIT_URL/agent/status" 2>/dev/null)
if echo "$STATUS_RESP" | grep -q '"ok":true'; then
    log "${GREEN}[CONNECT] Replit accessible — statut OK${NC}"
else
    log "${RED}[ERREUR] Impossible de joindre $REPLIT_URL${NC}"
    log "${RED}  Réponse brute : $STATUS_RESP${NC}"
    log "${YELLOW}  Si URL Replit a changé, corriger REPLIT_URL :${NC}"
    log "${YELLOW}  REPLIT_URL=https://nouvelle-url.replit.dev bash $0${NC}"
    exit 1
fi

# ─── Boucle agent principale ──────────────────────────────
log "${GREEN}[AGENT] Boucle poll démarrée — C48 (CTRL+C pour arrêter)${NC}"

while true; do
    # Poll : écrire réponse brute dans fichier temp
    curl -s --max-time 10 \
        -H "X-Agent-Token: $AGENT_TOKEN" \
        "$REPLIT_URL/agent/job" > /tmp/lv_job_resp.json 2>/dev/null

    # Extraire le job via Python (évite les problèmes de repr Python vs JSON)
    JOB_PRESENT=$(python3 - << 'PYEOF_POLL'
import json, sys
try:
    d = json.load(open('/tmp/lv_job_resp.json'))
    job = d.get('job')
    if job and isinstance(job, dict) and job.get('cmd'):
        with open('/tmp/lv_job.json', 'w') as f:
            json.dump(job, f)
        print('yes')
    else:
        print('no')
except Exception:
    print('no')
PYEOF_POLL
)

    if [ "$JOB_PRESENT" != "yes" ]; then
        sleep "$POLL_INTERVAL"
        continue
    fi

    # Lire les champs du job
    JOB_ID=$(python3 -c "import json; d=json.load(open('/tmp/lv_job.json')); print(d.get('id',''))")
    CMD=$(python3 -c "import json; d=json.load(open('/tmp/lv_job.json')); print(d.get('cmd',''))")
    LABEL=$(python3 -c "import json; d=json.load(open('/tmp/lv_job.json')); print(d.get('label',''))")
    TIMEOUT=$(python3 -c "import json; d=json.load(open('/tmp/lv_job.json')); print(d.get('timeout_s',60))")
    TIMEOUT="${TIMEOUT:-60}"

    log "${YELLOW}[JOB] id=$JOB_ID label='$LABEL'${NC}"
    log "${YELLOW}  cmd: ${CMD:0:80}...${NC}"

    # Remplacer les chemins génériques par les chemins Ubuntu réels
    CMD_RESOLVED="${CMD//\$REPO_ROOT/$REPO_ROOT}"
    CMD_RESOLVED="${CMD_RESOLVED//\$BTC_DIR/$BTC_DIR}"
    CMD_RESOLVED="${CMD_RESOLVED//\$TOOLS_DIR/$TOOLS_DIR}"
    CMD_RESOLVED="${CMD_RESOLVED//~/LVX/lumvorax2/$REPO_ROOT}"

    # Exécuter la commande dans bash (pas fish)
    START_TS=$(date +%s)
    STDOUT=$(timeout "$TIMEOUT" bash -c "$CMD_RESOLVED" 2>&1)
    RC=$?
    END_TS=$(date +%s)
    DURATION=$((END_TS - START_TS))

    if [ "$RC" -eq 0 ]; then
        log "${GREEN}[RÉSULTAT] rc=0 durée=${DURATION}s${NC}"
    else
        log "${RED}[RÉSULTAT] rc=$RC durée=${DURATION}s${NC}"
    fi

    # Écrire stdout dans fichier temp pour parsing Python robuste
    echo "$STDOUT" | head -c 8192 > /tmp/lv_stdout.txt
    echo "$CMD_RESOLVED" > /tmp/lv_cmd.txt
    echo "$LABEL"   > /tmp/lv_label.txt
    echo "$JOB_ID"  > /tmp/lv_jobid.txt
    echo "$RC"      > /tmp/lv_rc.txt
    echo "$DURATION"> /tmp/lv_dur.txt
    echo "$ENV_NAME"> /tmp/lv_env.txt

    # Construire JSON résultat via Python (zéro interpolation bash fragile)
    RESULT_JSON=$(python3 - << 'PYEOF_RESULT'
import json
stdout  = open('/tmp/lv_stdout.txt').read()
cmd     = open('/tmp/lv_cmd.txt').read().strip()
label   = open('/tmp/lv_label.txt').read().strip()
job_id  = open('/tmp/lv_jobid.txt').read().strip()
rc      = int(open('/tmp/lv_rc.txt').read().strip() or '0')
dur     = int(open('/tmp/lv_dur.txt').read().strip() or '0')
env     = open('/tmp/lv_env.txt').read().strip()
print(json.dumps({
    'job_id': job_id, 'label': label, 'cmd': cmd,
    'stdout': stdout, 'returncode': rc, 'duration_s': dur,
    'host': 'lvx-Vostro-5481', 'env': env, 'cycle': 'C48',
}))
PYEOF_RESULT
)

    curl -s --max-time 10 \
        -H "X-Agent-Token: $AGENT_TOKEN" \
        -H "Content-Type: application/json" \
        -X POST -d "$RESULT_JSON" \
        "$REPLIT_URL/agent/result" > /dev/null 2>&1

    log "${CYAN}[AGENT] Résultat envoyé. En attente du prochain job...${NC}"
    sleep 1
done
