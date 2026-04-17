#!/usr/bin/env python3
"""
LumVorax C54 — Agent Ubuntu WebSocket (remplacement du polling HTTP)
=====================================================================
Connexion WebSocket persistante bidirectionnelle vers Replit.
- Replit PUSH les jobs → Ubuntu exécute immédiatement (latence ~0ms)
- Ubuntu PUSH les résultats → Replit stocke
- Reconnexion automatique si réseau coupé ou nœud Replit redémarré
- Authentification par token dans le handshake

Usage :
  python3 tools/agent_ubuntu_ws.py
  # ou via Doppler :
  doppler run --config dev_lumvorax -- python3 tools/agent_ubuntu_ws.py

Variables d'environnement :
  REPLIT_URL     = https://xxx.replit.dev  (sans /ws/)
  AGENT_TOKEN    = token (identique à l'agent HTTP)
  POLL_INTERVAL  = délai entre reconnexions (défaut 5s)
  DEFAULT_JOB_TIMEOUT_S = 0 (illimité)

Compatibilité :
  pip install "python-socketio[client]>=5.11"
"""

import os
import sys
import json
import subprocess
import time
import hashlib
import socket
import threading
import logging

try:
    import socketio
except ImportError:
    print("[C54-WS] ERREUR : python-socketio non installé.")
    print("  Installer : pip install 'python-socketio[client]>=5.11'")
    sys.exit(1)

# ─── Configuration ────────────────────────────────────────────────────────────

REPLIT_URL           = os.environ.get("REPLIT_URL", "").rstrip("/")
AGENT_TOKEN          = os.environ.get("AGENT_TOKEN", "") or os.environ.get("LUMVORAX_AGENT_TOKEN", "")
DEFAULT_JOB_TIMEOUT  = int(os.environ.get("DEFAULT_JOB_TIMEOUT_S", "0"))
RECONNECT_DELAY      = int(os.environ.get("POLL_INTERVAL", "5"))
LOG_FILE             = os.path.expanduser("~/lumvorax_agent_ws.log")

# Détection REPO_ROOT
if os.path.isdir(os.path.expanduser("~/LVX/lumvorax2")):
    REPO_ROOT = os.path.expanduser("~/LVX/lumvorax2")
    ENV_NAME  = "ubuntu_lvx"
elif os.path.isdir("/home/runner/workspace"):
    REPO_ROOT = "/home/runner/workspace"
    ENV_NAME  = "replit"
else:
    REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ENV_NAME  = "unknown"

BTC_DIR   = os.path.join(REPO_ROOT, "src/advanced_calculations/bitcoin_quantum_mining")
TOOLS_DIR = os.path.join(REPO_ROOT, "tools")

# ─── Logging ─────────────────────────────────────────────────────────────────

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
    datefmt="%Y-%m-%dT%H:%M:%SZ",
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler(LOG_FILE, encoding="utf-8"),
    ]
)
log = logging.getLogger("lv-agent-ws")

# ─── Validation ───────────────────────────────────────────────────────────────

if not REPLIT_URL:
    log.error("[C54-WS] REPLIT_URL absent. Utiliser Doppler ou env REPLIT_URL=...")
    sys.exit(1)
if not AGENT_TOKEN:
    log.error("[C54-WS] AGENT_TOKEN absent. Utiliser Doppler ou env AGENT_TOKEN=...")
    sys.exit(1)

# Construire l'URL WebSocket
WS_URL = REPLIT_URL
log.info(f"[C54-WS] LumVorax Agent WebSocket — Cycle C54")
log.info(f"[C54-WS] Environnement : {ENV_NAME}")
log.info(f"[C54-WS] REPO_ROOT     : {REPO_ROOT}")
log.info(f"[C54-WS] Replit URL    : {REPLIT_URL}")
log.info(f"[C54-WS] Token (8ch)   : {AGENT_TOKEN[:8]}...")
log.info(f"[C54-WS] Log           : {LOG_FILE}")
log.info(f"[C54-WS] Job timeout   : {'illimité' if DEFAULT_JOB_TIMEOUT == 0 else str(DEFAULT_JOB_TIMEOUT) + 's'}")

# ─── Exécution de commande ────────────────────────────────────────────────────

def _resolve_cmd(cmd):
    """Remplace les variables de chemin dans la commande."""
    cmd = cmd.replace("$REPO_ROOT", REPO_ROOT)
    cmd = cmd.replace("$BTC_DIR", BTC_DIR)
    cmd = cmd.replace("$TOOLS_DIR", TOOLS_DIR)
    cmd = cmd.replace("~/LVX/lumvorax2", REPO_ROOT)
    return cmd

def _run_job(job, sio):
    """Exécute un job et envoie le résultat via WebSocket."""
    job_id  = job.get("id", "?")
    cmd     = job.get("cmd", "")
    label   = job.get("label", "")
    timeout = int(job.get("timeout_s", DEFAULT_JOB_TIMEOUT))

    log.info(f"[C54-WS] JOB id={job_id} label='{label}'")
    log.info(f"[C54-WS]   cmd: {cmd[:80]}...")

    cmd_resolved = _resolve_cmd(cmd)
    start = time.time()
    try:
        if timeout == 0:
            result = subprocess.run(
                ["bash", "-c", cmd_resolved],
                capture_output=True, text=True
            )
        else:
            result = subprocess.run(
                ["bash", "-c", cmd_resolved],
                capture_output=True, text=True,
                timeout=timeout
            )
        stdout = (result.stdout + result.stderr)[:8192]
        rc = result.returncode
    except subprocess.TimeoutExpired:
        stdout = f"[TIMEOUT] Commande interrompue après {timeout}s"
        rc = 124
    except Exception as e:
        stdout = f"[ERREUR] {e}"
        rc = 1

    duration = int(time.time() - start)
    log.info(f"[C54-WS]   rc={rc} durée={duration}s")

    payload = {
        "job_id":      job_id,
        "label":       label,
        "cmd":         cmd_resolved,
        "stdout":      stdout,
        "returncode":  rc,
        "duration_s":  duration,
        "host":        socket.gethostname(),
        "env":         ENV_NAME,
        "cycle":       "C54",
        "transport":   "websocket",
    }
    try:
        sio.emit("result", payload, namespace="/agent")
        log.info(f"[C54-WS]   Résultat envoyé via WebSocket")
    except Exception as e:
        log.error(f"[C54-WS]   Erreur envoi résultat WS : {e}")

# ─── Client SocketIO ──────────────────────────────────────────────────────────

sio = socketio.Client(
    reconnection=True,
    reconnection_attempts=0,  # 0 = infini
    reconnection_delay=RECONNECT_DELAY,
    reconnection_delay_max=30,
    logger=False,
    engineio_logger=False,
)

@sio.event(namespace="/agent")
def connect():
    log.info("[C54-WS] ✅ Connecté au serveur Replit WebSocket (/agent)")

@sio.event(namespace="/agent")
def connect_error(data):
    log.warning(f"[C54-WS] ❌ Erreur de connexion : {data}")

@sio.event(namespace="/agent")
def disconnect():
    log.warning("[C54-WS] ⚠️  Déconnecté — reconnexion automatique en cours...")

@sio.on("connected", namespace="/agent")
def on_connected(data):
    pending = data.get("pending_jobs", 0)
    log.info(f"[C54-WS] 🟢 Agent authentifié — jobs en attente : {pending}")

@sio.on("job", namespace="/agent")
def on_job(data):
    """Réception d'un job depuis Replit — exécution dans un thread séparé."""
    t = threading.Thread(target=_run_job, args=(data, sio), daemon=True)
    t.start()

@sio.on("pong_agent", namespace="/agent")
def on_pong(data):
    log.debug(f"[C54-WS] pong reçu : {data}")

# ─── Connexion principale ────────────────────────────────────────────────────

def main():
    log.info("[C54-WS] Connexion au serveur WebSocket Replit...")
    while True:
        try:
            sio.connect(
                WS_URL,
                socketio_path="/ws/socket.io",
                auth={"token": AGENT_TOKEN},
                namespaces=["/agent"],
                transports=["websocket"],
                wait_timeout=15,
            )
            sio.wait()
        except socketio.exceptions.ConnectionError as e:
            log.warning(f"[C54-WS] Connexion refusée : {e} — retry dans {RECONNECT_DELAY}s")
            time.sleep(RECONNECT_DELAY)
        except KeyboardInterrupt:
            log.info("[C54-WS] Arrêt demandé (CTRL+C)")
            break
        except Exception as e:
            log.error(f"[C54-WS] Erreur inattendue : {e} — retry dans {RECONNECT_DELAY}s")
            time.sleep(RECONNECT_DELAY)
    sio.disconnect()

if __name__ == "__main__":
    main()
