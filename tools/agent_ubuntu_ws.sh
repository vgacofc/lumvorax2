#!/usr/bin/env bash
# ============================================================
# LumVorax C57 — Agent Ubuntu WebSocket — Wrapper bash
#
# Corrige : PEP 668 "externally-managed-environment" Ubuntu 22.04+
# Solution : venv Python isolé dans ~/.lumvorax_ws_venv
#
# Usage :
#   doppler run --config dev_lumvorax -- bash tools/agent_ubuntu_ws.sh
#   # ou sans Doppler :
#   env REPLIT_URL=https://... AGENT_TOKEN=... bash tools/agent_ubuntu_ws.sh
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
        echo "[C57-WS] REPLIT_URL/AGENT_TOKEN absents — relance via Doppler config=$DOPPLER_CONFIG"
        export LUMVORAX_AGENT_DOPPLER_REEXEC=1
        export DOPPLER_UPDATE_CHECK=false
        export DOPPLER_NO_UPDATE_NOTIFIER=true
        exec doppler run --config "$DOPPLER_CONFIG" -- bash "$0"
    fi
fi

# ─── Venv Python isolé (évite PEP 668 "externally-managed") ─
VENV_DIR="$HOME/.lumvorax_ws_venv"

if [ ! -d "$VENV_DIR" ]; then
    echo "[C57-WS] Création du venv Python : $VENV_DIR"
    python3 -m venv "$VENV_DIR"
fi

VENV_PY="$VENV_DIR/bin/python3"
VENV_PIP="$VENV_DIR/bin/pip"

# ─── Installer python-socketio si absent ────────────────────
if ! "$VENV_PY" -c "import socketio" 2>/dev/null; then
    echo "[C57-WS] Installation python-socketio[client] dans le venv..."
    "$VENV_PIP" install --quiet --upgrade pip
    "$VENV_PIP" install --quiet "python-socketio[client]>=5.11" "websocket-client>=1.6"
    echo "[C57-WS] Installation OK"
fi

echo "[C57-WS] ============================================"
echo "[C57-WS] LumVorax Agent WebSocket — Cycle C57"
echo "[C57-WS] REPO_ROOT : $REPO_ROOT"
echo "[C57-WS] URL       : ${REPLIT_URL:-NON DÉFINI}"
echo "[C57-WS] Token     : ${AGENT_TOKEN:0:8}..."
echo "[C57-WS] Venv      : $VENV_DIR"
echo "[C57-WS] Transport : WebSocket + polling fallback"
echo "[C57-WS] ============================================"

# ─── Lancer l'agent Python WebSocket avec le venv ───────────
export REPO_ROOT TOOLS_DIR AGENT_TOKEN REPLIT_URL
exec "$VENV_PY" "$TOOLS_DIR/agent_ubuntu_ws.py"
