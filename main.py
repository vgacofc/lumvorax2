import ctypes
import os
import sys

_lib_paths = [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6",
    "/lib/x86_64-linux-gnu/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]
for _p in _lib_paths:
    if os.path.exists(_p):
        try:
            ctypes.CDLL(_p)
        except OSError:
            pass
        break

_viz_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "src", "visualization")
if _viz_dir not in sys.path:
    sys.path.insert(0, _viz_dir)

from server import app, socketio
from console_banner import print_banner, print_info

print_banner()
print_info("Starting LUM/VORAX visualization server on port 5000")

def _auto_update_doppler():
    """C63 : Met à jour Doppler avec l'URL et le token de cette session au démarrage.
    Exécuté automatiquement avant socketio.run() — ne bloque pas si Doppler absent."""
    import subprocess, threading

    def _run():
        try:
            replit_url = os.environ.get("REPLIT_DEV_DOMAIN", "")
            if not replit_url:
                replit_url = os.environ.get("REPL_SLUG", "")
            if not replit_url:
                print("[DOPPLER-AUTO] REPLIT_DEV_DOMAIN non disponible — skip", flush=True)
                return

            script = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                  "tools", "doppler_update.sh")
            if not os.path.exists(script):
                print(f"[DOPPLER-AUTO] Script non trouvé : {script}", flush=True)
                return

            url = f"https://{replit_url}" if not replit_url.startswith("http") else replit_url
            result = subprocess.run(
                ["bash", script, url],
                capture_output=True, text=True, timeout=15
            )
            if result.returncode == 0:
                print(f"[DOPPLER-AUTO] ✅ URL mise à jour → {url[:50]}...", flush=True)
            else:
                print(f"[DOPPLER-AUTO] ⚠️ rc={result.returncode} : {result.stderr[:200]}", flush=True)
        except Exception as e:
            print(f"[DOPPLER-AUTO] Erreur (non bloquante) : {e}", flush=True)

    t = threading.Thread(target=_run, daemon=True, name="doppler-auto-update")
    t.start()


if __name__ == "__main__":
    _auto_update_doppler()
    port = int(os.environ.get("PORT", 5000))
    # C57-WS : socketio.run() active les vrais WebSockets (async_mode=threading)
    # Remplace gunicorn qui ne supporte pas les upgrades WebSocket nativement.
    socketio.run(
        app,
        host="0.0.0.0",
        port=port,
        allow_unsafe_werkzeug=True,
        use_reloader=False,
        log_output=False,
    )
