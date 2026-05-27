#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

REQUIRED = {
    "imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "imageio-2.37.2-py3-none-any.whl",
    "lazy_loader-0.4-py3-none-any.whl",
    "liblumvorax_replit.so",
    "networkx-3.6.1-py3-none-any.whl",
    "numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "opencv_python-4.13.0.92-cp37-abi3-manylinux_2_28_x86_64.whl",
    "packaging-26.0-py3-none-any.whl",
    "pillow-12.1.1-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "scikit_image-0.26.0-cp311-cp311-manylinux_2_24_x86_64.manylinux_2_28_x86_64.whl",
    "scipy-1.17.0-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "tifffile-2026.1.28-py3-none-any.whl",
    "tifffile-2026.2.16-py3-none-any.whl",
    "competitor_teacher_1407735.tif",
    "competitor_teacher_1407735.lum",
    "dataset_v4_build_report.json",
}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", required=True)
    args = ap.parse_args()

    root = Path(args.dir)
    present = {p.name for p in root.iterdir()} if root.exists() else set()
    missing = sorted(REQUIRED - present)
    extras = sorted(present - REQUIRED)

    result = {
        "dir": str(root),
        "missing": missing,
        "extras": extras,
        "present_count": len(present),
        "required_count": len(REQUIRED),
        "status": "ok" if not missing else "fail",
    }

    report = root / "dataset_v4_build_report.json"
    if report.exists():
        try:
            obj = json.loads(report.read_text(encoding="utf-8"))
            result["build_status"] = obj.get("status")
            result["build_missing"] = obj.get("missing", [])
            result["compile_ok"] = obj.get("compile", {}).get("ok")
            result["competitor_teacher_assets"] = obj.get("competitor_teacher_assets")
        except Exception as exc:
            result["build_report_error"] = str(exc)

    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "ok" else 2


if __name__ == "__main__":
    raise SystemExit(main())
