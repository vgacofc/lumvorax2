#!/usr/bin/env python3
"""
LumVorax C57 — Agent Ubuntu WebSocket
======================================
Connexion WebSocket persistante bidirectionnelle vers Replit.
- Replit PUSH les jobs → Ubuntu exécute immédiatement (<1s latence)
- Ubuntu PUSH les résultats → Replit stocke et retourne via /agent/results
- Reconnexion automatique si réseau coupé
- Authentification par token dans le handshake SocketIO
- Fallback polling si upgrade WS refusé (gunicorn gthread compatible)

Usage :
  doppler run --config dev_lumvorax -- bash tools/agent_ubuntu_ws.sh

Variables :
  REPLIT_URL          = https://xxx.replit.dev
  AGENT_TOKEN         = token 552ced77...
  DEFAULT_JOB_TIMEOUT_S = 0 (illimité)
"""

import os
import sys
import json
import subprocess
import time
import socket
import threading
import logging

try:
    import socketio
except ImportError:
    print("[C57-WS] ERREUR : python-socketio non disponible dans le venv.")
    print("  Lance via : bash tools/agent_ubuntu_ws.sh")
    sys.exit(1)

# ─── Configuration ─────────────────────────────────────────────────────────

REPLIT_URL          = os.environ.get("REPLIT_URL", "").rstrip("/")
AGENT_TOKEN         = os.environ.get("AGENT_TOKEN", "") or os.environ.get("LUMVORAX_AGENT_TOKEN", "")
DEFAULT_JOB_TIMEOUT = int(os.environ.get("DEFAULT_JOB_TIMEOUT_S", "0"))
RECONNECT_DELAY     = int(os.environ.get("POLL_INTERVAL", "5"))
LOG_FILE            = os.path.expanduser("~/lumvorax_agent_ws.log")

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

# ─── Logging ───────────────────────────────────────────────────────────────

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
    datefmt="%Y-%m-%dT%H:%M:%SZ",
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler(LOG_FILE, encoding="utf-8"),
    ]
)
log = logging.getLogger("lv-ws")

# ─── Validation ────────────────────────────────────────────────────────────

if not REPLIT_URL:
    log.error("[C57-WS] REPLIT_URL absent. Lancer via Doppler ou env REPLIT_URL=...")
    sys.exit(1)
if not AGENT_TOKEN:
    log.error("[C57-WS] AGENT_TOKEN absent. Lancer via Doppler ou env AGENT_TOKEN=...")
    sys.exit(1)

log.info("[C57-WS] LumVorax Agent WebSocket — Cycle C57")
log.info(f"[C57-WS] Env          : {ENV_NAME}")
log.info(f"[C57-WS] REPO_ROOT    : {REPO_ROOT}")
log.info(f"[C57-WS] Replit URL   : {REPLIT_URL}")
log.info(f"[C57-WS] Token (8ch)  : {AGENT_TOKEN[:8]}...")
log.info(f"[C57-WS] Log          : {LOG_FILE}")
log.info(f"[C57-WS] Job timeout  : {'illimité' if DEFAULT_JOB_TIMEOUT == 0 else str(DEFAULT_JOB_TIMEOUT) + 's'}")

# ─── Résolution des variables de chemin ────────────────────────────────────

def _resolve_cmd(cmd: str) -> str:
    cmd = cmd.replace("$REPO_ROOT", REPO_ROOT)
    cmd = cmd.replace("$BTC_DIR", BTC_DIR)
    cmd = cmd.replace("$TOOLS_DIR", TOOLS_DIR)
    cmd = cmd.replace("~/LVX/lumvorax2", REPO_ROOT)
    return cmd

# ─── Exécution de commande ─────────────────────────────────────────────────

def _run_job(job: dict, sio_client: socketio.Client) -> None:
    """Exécute un job reçu via WebSocket et envoie le résultat."""
    job_id  = job.get("id", "?")
    cmd     = job.get("cmd", "")
    label   = job.get("label", "")
    timeout = int(job.get("timeout_s", DEFAULT_JOB_TIMEOUT)) or None

    log.info(f"[C57-WS] JOB id={job_id} label='{label}'")
    log.info(f"[C57-WS]   cmd: {cmd[:100]}")

    cmd_resolved = _resolve_cmd(cmd)
    start = time.time()
    try:
        result = subprocess.run(
            ["bash", "-c", cmd_resolved],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        stdout = (result.stdout + result.stderr)[:16384]
        rc = result.returncode
    except subprocess.TimeoutExpired:
        stdout = f"[TIMEOUT] Commande interrompue après {timeout}s"
        rc = 124
    except Exception as exc:
        stdout = f"[ERREUR] {exc}"
        rc = 1

    duration = round(time.time() - start, 2)
    log.info(f"[C57-WS]   rc={rc} durée={duration}s output={len(stdout)}c")

    payload = {
        "job_id":     job_id,
        "label":      label,
        "cmd":        cmd_resolved,
        "output":     stdout,
        "stdout":     stdout,
        "returncode": rc,
        "duration_s": duration,
        "host":       socket.gethostname(),
        "env":        ENV_NAME,
        "cycle":      "C57",
        "transport":  "websocket",
    }
    try:
        sio_client.emit("result", payload, namespace="/agent")
        log.info(f"[C57-WS]   ✅ Résultat envoyé via WebSocket")
    except Exception as exc:
        log.error(f"[C57-WS]   ❌ Erreur envoi WS : {exc}")

# ─── Client SocketIO ───────────────────────────────────────────────────────

sio = socketio.Client(
    reconnection=True,
    reconnection_attempts=0,       # 0 = infini
    reconnection_delay=RECONNECT_DELAY,
    reconnection_delay_max=30,
    logger=False,
    engineio_logger=False,
)

@sio.event(namespace="/agent")
def connect():
    log.info("[C57-WS] ✅ Connecté au serveur Replit (/agent namespace)")

@sio.event(namespace="/agent")
def connect_error(data):
    log.warning(f"[C57-WS] ❌ Erreur de connexion : {data}")

@sio.event(namespace="/agent")
def disconnect():
    log.warning("[C57-WS] ⚠️  Déconnecté — reconnexion automatique...")

@sio.on("connected", namespace="/agent")
def on_connected(data):
    pending = data.get("pending_jobs", 0) if isinstance(data, dict) else 0
    log.info(f"[C57-WS] 🟢 Agent authentifié — jobs en attente : {pending}")

@sio.on("job", namespace="/agent")
def on_job(data):
    """Réception d'un job → exécution dans thread daemon."""
    t = threading.Thread(target=_run_job, args=(data, sio), daemon=True)
    t.start()

@sio.on("pong_agent", namespace="/agent")
def on_pong(data):
    log.debug(f"[C57-WS] pong : {data}")

# ─── Boucle principale ─────────────────────────────────────────────────────

def main():
    log.info(f"[C57-WS] Connexion WebSocket → {REPLIT_URL}/ws/socket.io ...")
    while True:
        try:
            # C57 : transports=["websocket","polling"] permet le handshake
            # polling initial puis upgrade WebSocket automatique.
            # AVANT C57 : transports=["websocket"] seul → connexion refusée
            # (Socket.IO nécessite un handshake HTTP polling d'abord)
            sio.connect(
                REPLIT_URL,
                socketio_path="/ws/socket.io",
                auth={"token": AGENT_TOKEN},
                namespaces=["/agent"],
                transports=["websocket", "polling"],
                wait_timeout=20,
            )
            log.info("[C57-WS] Connexion établie — boucle d'attente active")
            sio.wait()
        except socketio.exceptions.ConnectionError as exc:
            log.warning(f"[C57-WS] Connexion refusée : {exc} — retry {RECONNECT_DELAY}s")
            time.sleep(RECONNECT_DELAY)
        except KeyboardInterrupt:
            log.info("[C57-WS] Arrêt (CTRL+C)")
            break
        except Exception as exc:
            log.error(f"[C57-WS] Erreur : {exc} — retry {RECONNECT_DELAY}s")
            time.sleep(RECONNECT_DELAY)
    try:
        sio.disconnect()
    except Exception:
        pass

if __name__ == "__main__":
    main()
