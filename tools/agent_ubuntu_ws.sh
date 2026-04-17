#!/usr/bin/env bash
# ============================================================
# LumVorax C54 — Agent Ubuntu WebSocket — Wrapper bash
#
# Remplace l'ancien agent HTTP polling (agent_ubuntu.sh).
# Utilise une connexion WebSocket persistante bidirectionnelle.
#
# IMPORTANT : Lance avec bash (pas fish) :
#   bash ~/LVX/lumvorax2/tools/agent_ubuntu_ws.sh
#
# Usage direct :
#   bash ~/LVX/lumvorax2/tools/agent_ubuntu_ws.sh
#
# Avec Doppler :
#   doppler run --config dev_lumvorax -- bash ~/LVX/lumvorax2/tools/agent_ubuntu_ws.sh
#
# Sans Doppler :
#   env REPLIT_URL=https://... AGENT_TOKEN=... bash ~/LVX/lumvorax2/tools/agent_ubuntu_ws.sh
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

TOOLS_DIR="$REPO_ROOT/tools"
DOPPLER_CONFIG="${DOPPLER_CONFIG:-dev_lumvorax}"
AGENT_TOKEN="${AGENT_TOKEN:-${LUMVORAX_AGENT_TOKEN:-}}"
REPLIT_URL="${REPLIT_URL:-${LUMVORAX_REPLIT_URL:-}}"

# ─── Auto-relance via Doppler si variables absentes ─────────
if [ -z "$REPLIT_URL" ] || [ -z "$AGENT_TOKEN" ]; then
    if [ "${LUMVORAX_AGENT_DOPPLER_REEXEC:-0}" != "1" ] && command -v doppler >/dev/null 2>&1; then
        echo "[C54-WS] REPLIT_URL/AGENT_TOKEN absents — relance via Doppler config=$DOPPLER_CONFIG"
        export LUMVORAX_AGENT_DOPPLER_REEXEC=1
        export DOPPLER_UPDATE_CHECK=false
        export DOPPLER_NO_UPDATE_NOTIFIER=true
        exec doppler run --config "$DOPPLER_CONFIG" -- bash "$0"
    fi
fi

# ─── Vérifier python3 et python-socketio ────────────────────
if ! command -v python3 >/dev/null 2>&1; then
    echo "[C54-WS] ERREUR : python3 non trouvé"
    exit 1
fi

if ! python3 -c "import socketio" 2>/dev/null; then
    echo "[C54-WS] Installation python-socketio..."
    pip install "python-socketio[client]>=5.11" --quiet
fi

echo "[C54-WS] ============================================"
echo "[C54-WS] LumVorax Agent WebSocket — Cycle C54"
echo "[C54-WS] REPO_ROOT : $REPO_ROOT"
echo "[C54-WS] URL       : ${REPLIT_URL:-NON DÉFINI}"
echo "[C54-WS] Token     : ${AGENT_TOKEN:0:8}..."
echo "[C54-WS] Transport : WebSocket persistant (remplace HTTP polling)"
echo "[C54-WS] ============================================"

# ─── Lancer l'agent Python WebSocket ────────────────────────
exec python3 "$TOOLS_DIR/agent_ubuntu_ws.py"
