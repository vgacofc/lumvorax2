#!/usr/bin/env bash
# ============================================================
# LumVorax C47 — Agent Ubuntu (client pull)
# Usage recommandé (avec Doppler) :
#   doppler run -- bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
#
# Usage manuel (sans Doppler) :
#   AGENT_TOKEN=<token> REPLIT_URL=<url> bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
#
# Doppler injecte automatiquement : AGENT_TOKEN, REPLIT_URL
# ============================================================

# ─── Valeurs par défaut (remplacées par Doppler ou env) ───
AGENT_TOKEN="${AGENT_TOKEN:-0ce121419a08e95af480ce37dad5c17f}"
REPLIT_URL="${REPLIT_URL:-https://e40e29e0-9d6e-4d1d-83d7-75fba79991fc-00-2xfnracaqcp0l.picard.replit.dev}"
POLL_INTERVAL=5
LOG_FILE="$HOME/lumvorax_agent.log"

# ─── Couleurs ─────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'

log() { echo -e "$(date '+%Y-%m-%dT%H:%M:%SZ') $1" | tee -a "$LOG_FILE"; }

# ─── Vérifications préalables ────────────────────────────
if [ -z "$AGENT_TOKEN" ]; then
    log "${RED}[ERREUR] AGENT_TOKEN absent${NC}"
    log "${YELLOW}  Lancer avec : doppler run -- bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh${NC}"
    log "${YELLOW}  OU : AGENT_TOKEN=<token> REPLIT_URL=<url> bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh${NC}"
    exit 1
fi

log "${CYAN}[LumVorax Agent C47] Démarrage — Ubuntu lvx${NC}"
log "${CYAN}  Replit URL : $REPLIT_URL${NC}"
log "${CYAN}  Poll interval : ${POLL_INTERVAL}s${NC}"
log "${CYAN}  Log : $LOG_FILE${NC}"
log "${CYAN}  Token (4 chars) : ${AGENT_TOKEN:0:4}...${NC}"

# ─── Vérification connectivité ───────────────────────────
STATUS_RESP=$(curl -s --max-time 10 "$REPLIT_URL/agent/status" 2>/dev/null)
if echo "$STATUS_RESP" | grep -q '"ok":true'; then
    log "${GREEN}[CONNECT] Replit accessible — statut OK${NC}"
else
    log "${RED}[ERREUR] Impossible de joindre $REPLIT_URL${NC}"
    log "${RED}  Réponse: $STATUS_RESP${NC}"
    log "${YELLOW}  Vérifier que le workflow Replit 'Start application' tourne${NC}"
    log "${YELLOW}  Si l'URL Replit a changé : doppler secrets set REPLIT_URL=<nouvelle_url>${NC}"
    exit 1
fi

# ─── Boucle agent principale ─────────────────────────────
log "${GREEN}[AGENT] Boucle poll démarrée — C47 (CTRL+C pour arrêter)${NC}"

while true; do
    # Poll : récupérer prochain job
    JOB_RESP=$(curl -s --max-time 10 \
        -H "X-Agent-Token: $AGENT_TOKEN" \
        "$REPLIT_URL/agent/job" 2>/dev/null)

    JOB=$(echo "$JOB_RESP" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('job') or '')" 2>/dev/null)

    if [ -z "$JOB" ] || [ "$JOB" = "None" ] || [ "$JOB" = "null" ]; then
        sleep $POLL_INTERVAL
        continue
    fi

    # Écrire le job dans un fichier temp pour un parsing robuste (évite les bugs de guillemets)
    echo "$JOB" > /tmp/lv_job_c47.json
    JOB_ID=$(python3 -c "import json; d=json.load(open('/tmp/lv_job_c47.json')); print(d.get('id',''))" 2>/dev/null)
    CMD=$(python3 -c "import json; d=json.load(open('/tmp/lv_job_c47.json')); print(d.get('cmd',''))" 2>/dev/null)
    LABEL=$(python3 -c "import json; d=json.load(open('/tmp/lv_job_c47.json')); print(d.get('label',''))" 2>/dev/null)
    TIMEOUT=$(python3 -c "import json; d=json.load(open('/tmp/lv_job_c47.json')); print(d.get('timeout_s',60))" 2>/dev/null)
    TIMEOUT="${TIMEOUT:-60}"

    log "${YELLOW}[JOB] id=$JOB_ID label='$LABEL'${NC}"
    log "${YELLOW}  cmd: $CMD${NC}"

    # Exécuter la commande
    START_TS=$(date +%s)
    STDOUT=$(timeout "$TIMEOUT" bash -c "$CMD" 2>&1)
    RC=$?
    END_TS=$(date +%s)
    DURATION=$((END_TS - START_TS))

    if [ $RC -eq 0 ]; then
        log "${GREEN}[RÉSULTAT] rc=0 durée=${DURATION}s${NC}"
    else
        log "${RED}[RÉSULTAT] rc=$RC durée=${DURATION}s${NC}"
    fi

    # Tronquer stdout si trop long
    STDOUT_TRUNC=$(echo "$STDOUT" | head -c 4096)

    # Envoyer résultat à Replit
    RESULT_JSON=$(python3 -c "
import json, sys
d = {
    'job_id': '$JOB_ID',
    'label': '$LABEL',
    'cmd': $(echo "$CMD" | python3 -c "import sys,json; print(json.dumps(sys.stdin.read().strip()))"),
    'stdout': $(echo "$STDOUT_TRUNC" | python3 -c "import sys,json; print(json.dumps(sys.stdin.read()))"),
    'returncode': $RC,
    'duration_s': $DURATION,
    'host': 'lvx-Vostro-5481',
    'cycle': 'C47',
}
print(json.dumps(d))
" 2>/dev/null)

    curl -s --max-time 10 \
        -H "X-Agent-Token: $AGENT_TOKEN" \
        -H "Content-Type: application/json" \
        -X POST \
        -d "$RESULT_JSON" \
        "$REPLIT_URL/agent/result" > /dev/null 2>&1

    log "${CYAN}[AGENT] Résultat envoyé. En attente du prochain job...${NC}"
    sleep 1
done
