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
    # Poll : récupérer prochain job — sauvegarder JOB_RESP brut dans fichier
    curl -s --max-time 10 \
        -H "X-Agent-Token: $AGENT_TOKEN" \
        "$REPLIT_URL/agent/job" > /tmp/lv_job_resp_c47.json 2>/dev/null

    # Extraire le job en JSON VALIDE via python json.dump (évite le bug print(dict) → repr Python)
    JOB_PRESENT=$(python3 - << 'PYEOF_POLL'
import json, sys
try:
    d = json.load(open('/tmp/lv_job_resp_c47.json'))
    job = d.get('job')
    if job and isinstance(job, dict) and job.get('cmd'):
        with open('/tmp/lv_job_c47.json', 'w') as f:
            json.dump(job, f)
        print('yes')
    else:
        print('no')
except Exception:
    print('no')
PYEOF_POLL
)

    if [ "$JOB_PRESENT" != "yes" ]; then
        sleep $POLL_INTERVAL
        continue
    fi

    # Extraire les champs depuis le JSON valide /tmp/lv_job_c47.json
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

    # Tronquer stdout si trop long — écriture dans fichiers temp pour parsing robuste
    echo "$STDOUT" | head -c 4096 > /tmp/lv_stdout_c47.txt
    echo "$CMD"    > /tmp/lv_cmd_c47.txt
    echo "$LABEL"  > /tmp/lv_label_c47.txt
    echo "$JOB_ID" > /tmp/lv_jobid_c47.txt
    echo "$RC"     > /tmp/lv_rc_c47.txt
    echo "$DURATION" > /tmp/lv_dur_c47.txt

    # Construire le JSON résultat via heredoc Python (sans interpolation bash fragile)
    RESULT_JSON=$(python3 - << 'PYEOF'
import json
stdout  = open('/tmp/lv_stdout_c47.txt').read()
cmd     = open('/tmp/lv_cmd_c47.txt').read().strip()
label   = open('/tmp/lv_label_c47.txt').read().strip()
job_id  = open('/tmp/lv_jobid_c47.txt').read().strip()
rc      = int(open('/tmp/lv_rc_c47.txt').read().strip() or '0')
dur     = int(open('/tmp/lv_dur_c47.txt').read().strip() or '0')
d = {
    'job_id': job_id,
    'label':  label,
    'cmd':    cmd,
    'stdout': stdout,
    'returncode': rc,
    'duration_s': dur,
    'host':  'lvx-Vostro-5481',
    'cycle': 'C47',
}
print(json.dumps(d))
PYEOF
)

    curl -s --max-time 10 \
        -H "X-Agent-Token: $AGENT_TOKEN" \
        -H "Content-Type: application/json" \
        -X POST \
        -d "$RESULT_JSON" \
        "$REPLIT_URL/agent/result" > /dev/null 2>&1

    log "${CYAN}[AGENT] Résultat envoyé. En attente du prochain job...${NC}"
    sleep 1
done
