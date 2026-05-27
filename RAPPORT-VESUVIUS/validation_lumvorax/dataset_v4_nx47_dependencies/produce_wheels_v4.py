#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import time
from pathlib import Path

TARGETS = [
    ("numpy", "2.4.2"),
    ("imagecodecs", "2026.1.14"),
    ("imageio", "2.37.2"),
    ("lazy_loader", "0.4"),
    ("networkx", "3.6.1"),
    ("opencv_python", "4.13.0.92"),
    ("packaging", "26.0"),
    ("pillow", "12.1.1"),
    ("scikit_image", "0.26.0"),
    ("scipy", "1.17.0"),
    ("tifffile", "2026.1.28"),
    ("tifffile", "2026.2.16"),
]


def main() -> int:
    root = Path(__file__).resolve().parent
    out = root / "wheelhouse_v4"
    out.mkdir(parents=True, exist_ok=True)

    report = {"ts_ns": time.time_ns(), "output": str(out), "downloads": []}

    for name, ver in TARGETS:
        cmd = [
            "python3", "-m", "pip", "download", f"{name}=={ver}",
            "--dest", str(out),
            "--only-binary=:all:",
            "--platform", "manylinux_2_28_x86_64",
            "--python-version", "311",
            "--implementation", "cp",
            "--abi", "cp311",
            "--no-deps",
        ]
        t0 = time.time_ns()
        p = subprocess.run(cmd, capture_output=True, text=True)
        report["downloads"].append({
            "name": name,
            "version": ver,
            "returncode": p.returncode,
            "elapsed_ns": time.time_ns() - t0,
            "stdout_head": p.stdout[-2000:],
            "stderr_head": p.stderr[-2000:],
        })

    report["files"] = sorted([x.name for x in out.glob("*.whl")])
    (root / "wheelhouse_v4_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps({"wheel_count": len(report["files"]), "report": str(root / 'wheelhouse_v4_report.json')}, indent=2))
    # Return nonzero if any download failed
    return 0 if all(x["returncode"] == 0 for x in report["downloads"]) else 2


if __name__ == "__main__":
    raise SystemExit(main())
