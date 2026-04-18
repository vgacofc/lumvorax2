#!/usr/bin/env bash
# ============================================================
# LumVorax C60 — Mise à jour Doppler (bash Ubuntu/NixOS/Replit)
# Remplace doppler_update.fish — 100% bash POSIX strict
# Standard : STANDARD_NAMES.md v4.3 | prompt.txt v2.1
#
# USAGE :
#   [REPLIT shell]   bash tools/doppler_update.sh
#   [UBUNTU]         bash ~/LVX/lumvorax2/tools/doppler_update.sh https://NOUVELLE-URL.replit.dev
#
# MODE AUTO (Replit — REPLIT_DEV_DOMAIN est défini) :
#   1. URL auto depuis $REPLIT_DEV_DOMAIN
#   2. Token depuis localhost:5000/agent/token
#   3. setup_key depuis localhost:5000/agent/token/setup-key
#   4. Doppler mis à jour : REPLIT_URL + AGENT_TOKEN + REPLIT_SETUP_KEY
#
# MODE UBUNTU (URL fournie en arg) :
#   1. URL depuis l'argument
#   2. Token récupéré via REPLIT_URL/agent/token?setup_key=... (si REPLIT_SETUP_KEY dans Doppler)
#   3. Doppler mis à jour : REPLIT_URL + AGENT_TOKEN (si récupéré)
#
# Aucun URL ni token hardcodé — tout dynamique.
# ============================================================

set -euo pipefail

DOPPLER_CONFIG="${DOPPLER_CONFIG:-dev_lumvorax}"
FLASK_LOCAL="http://localhost:5000"

echo ""
echo "[DOPPLER-C60] ============================================"
echo "[DOPPLER-C60] LumVorax — Mise à jour Doppler bash C60"
echo "[DOPPLER-C60] Config Doppler : $DOPPLER_CONFIG"
echo "[DOPPLER-C60] ============================================"

# ─── Détection du mode ───────────────────────────────────────
IS_REPLIT=0
[ -n "${REPLIT_DEV_DOMAIN:-}" ] && IS_REPLIT=1

# ─── 1. URL ──────────────────────────────────────────────────
if [ -n "${1:-}" ]; then
    NEW_URL="${1%/}"
    echo "[DOPPLER-C60] URL fournie : $NEW_URL"
elif [ "$IS_REPLIT" -eq 1 ]; then
    NEW_URL="https://${REPLIT_DEV_DOMAIN}"
    echo "[DOPPLER-C60] URL auto Replit : $NEW_URL"
else
    echo "[DOPPLER-C60] URL Replit non fournie."
    echo "              Ex: bash tools/doppler_update.sh https://xxx.spock.replit.dev"
    echo "              Entrez l'URL Replit :"
    read -r NEW_URL
    NEW_URL="${NEW_URL%/}"
fi

[[ "$NEW_URL" != https://* ]] && { echo "[ERREUR] URL invalide (doit commencer par https://)"; exit 1; }

# ─── 2. Token ────────────────────────────────────────────────
NEW_TOKEN=""
NEW_SETUP_KEY=""

# Tentative locale (Replit shell)
if [ "$IS_REPLIT" -eq 1 ] || [ -z "${1:-}" ]; then
    echo "[DOPPLER-C60] Récupération token depuis $FLASK_LOCAL ..."
    TOK_JSON=$(curl -sf --max-time 5 "$FLASK_LOCAL/agent/token" 2>/dev/null || true)
    if [ -n "$TOK_JSON" ]; then
        NEW_TOKEN=$(echo "$TOK_JSON" | python3 -c "import sys,json; print(json.load(sys.stdin).get('token',''))" 2>/dev/null || true)
        [ -n "$NEW_TOKEN" ] && echo "[DOPPLER-C60] Token : ${NEW_TOKEN:0:8}..."
    fi
    # setup_key pour accès Ubuntu futur
    SK_JSON=$(curl -sf --max-time 5 "$FLASK_LOCAL/agent/token/setup-key" 2>/dev/null || true)
    if [ -n "$SK_JSON" ]; then
        NEW_SETUP_KEY=$(echo "$SK_JSON" | python3 -c "import sys,json; print(json.load(sys.stdin).get('setup_key',''))" 2>/dev/null || true)
        [ -n "$NEW_SETUP_KEY" ] && echo "[DOPPLER-C60] Setup-key : $NEW_SETUP_KEY"
    fi
fi

# Tentative externe (Ubuntu avec setup_key existant dans Doppler)
if [ -z "$NEW_TOKEN" ] && command -v doppler >/dev/null 2>&1; then
    EXISTING_SETUP_KEY=$(doppler secrets get REPLIT_SETUP_KEY --config "$DOPPLER_CONFIG" --plain 2>/dev/null || true)
    if [ -n "$EXISTING_SETUP_KEY" ]; then
        echo "[DOPPLER-C60] Tentative récupération token via setup_key externe..."
        EXT_JSON=$(curl -sf --max-time 10 "${NEW_URL}/agent/token?setup_key=${EXISTING_SETUP_KEY}" 2>/dev/null || true)
        if [ -n "$EXT_JSON" ]; then
            NEW_TOKEN=$(echo "$EXT_JSON" | python3 -c "import sys,json; print(json.load(sys.stdin).get('token',''))" 2>/dev/null || true)
            [ -n "$NEW_TOKEN" ] && echo "[DOPPLER-C60] Token récupéré via setup_key : ${NEW_TOKEN:0:8}..."
        fi
    fi
fi

if [ -z "$NEW_TOKEN" ]; then
    echo "[DOPPLER-C60] Info: token non récupéré (seulement URL sera mise à jour)"
    echo "              Pour token complet, exécuter ce script DEPUIS le shell Replit."
fi

# ─── 3. Doppler disponible ? ─────────────────────────────────
if ! command -v doppler >/dev/null 2>&1; then
    echo "[ERREUR] doppler CLI absent — installation requise:"
    echo "  (curl -Ls --tlsv1.2 --proto '=https' 'https://cli.doppler.com/install.sh' || wget -t3 -qO- 'https://cli.doppler.com/install.sh') | sudo sh"
    echo ""
    echo "[DOPPLER-C60] Fallback export :"
    echo "  export REPLIT_URL='$NEW_URL'"
    [ -n "$NEW_TOKEN" ] && echo "  export AGENT_TOKEN='$NEW_TOKEN'"
    exit 1
fi

export DOPPLER_NO_UPDATE_NOTIFIER=true
export DOPPLER_UPDATE_CHECK=false

# ─── 4. Mise à jour Doppler ──────────────────────────────────
if [ -n "$NEW_TOKEN" ] && [ -n "$NEW_SETUP_KEY" ]; then
    echo "[DOPPLER-C60] Mise à jour COMPLÈTE : URL + TOKEN + SETUP_KEY"
    echo "n" | doppler secrets set \
        REPLIT_URL="$NEW_URL" \
        AGENT_TOKEN="$NEW_TOKEN" \
        REPLIT_SETUP_KEY="$NEW_SETUP_KEY" \
        --config "$DOPPLER_CONFIG" --no-interactive 2>&1 || \
    doppler secrets set \
        REPLIT_URL="$NEW_URL" \
        AGENT_TOKEN="$NEW_TOKEN" \
        REPLIT_SETUP_KEY="$NEW_SETUP_KEY" \
        --config "$DOPPLER_CONFIG" 2>&1
elif [ -n "$NEW_TOKEN" ]; then
    echo "[DOPPLER-C60] Mise à jour : URL + TOKEN"
    echo "n" | doppler secrets set \
        REPLIT_URL="$NEW_URL" \
        AGENT_TOKEN="$NEW_TOKEN" \
        --config "$DOPPLER_CONFIG" --no-interactive 2>&1 || \
    doppler secrets set \
        REPLIT_URL="$NEW_URL" \
        AGENT_TOKEN="$NEW_TOKEN" \
        --config "$DOPPLER_CONFIG" 2>&1
else
    echo "[DOPPLER-C60] Mise à jour : URL uniquement (token inchangé)"
    echo "n" | doppler secrets set \
        REPLIT_URL="$NEW_URL" \
        --config "$DOPPLER_CONFIG" --no-interactive 2>&1 || \
    doppler secrets set \
        REPLIT_URL="$NEW_URL" \
        --config "$DOPPLER_CONFIG" 2>&1
fi

# ─── 5. Récapitulatif ────────────────────────────────────────
echo ""
echo "[DOPPLER-C60] ✅ Doppler mis à jour."
echo "              URL       : $NEW_URL"
[ -n "$NEW_TOKEN" ] && echo "              Token     : ${NEW_TOKEN:0:8}..."
[ -n "$NEW_SETUP_KEY" ] && echo "              Setup-key : $NEW_SETUP_KEY"
echo ""
echo "[DOPPLER-C60] ═══ COMMANDES UBUNTU ═══"
echo ""
echo "  # 1. Mettre à jour depuis Ubuntu (si URL change) :"
echo "  bash ~/LVX/lumvorax2/tools/doppler_update.sh $NEW_URL"
echo ""
echo "  # 2. Lancer l'agent WebSocket :"
echo "  cd ~/LVX/lumvorax2 && git pull origin main"
echo "  doppler run --config $DOPPLER_CONFIG -- bash tools/agent_ubuntu_ws.sh"
echo ""
echo "[DOPPLER-C60] ═══ COMMANDE REPLIT SHELL (si nouvelle session) ═══"
echo ""
echo "  bash tools/doppler_update.sh"
echo "  # L'URL est auto-détectée via REPLIT_DEV_DOMAIN"
echo ""
