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
        ctypes.CDLL(_p)
        break

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "src", "visualization"))
from server import app

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
