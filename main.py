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

if __name__ == "__main__":
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
