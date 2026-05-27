#!/usr/bin/env python3
"""Validate required artifacts for nx47-dependencies Kaggle dataset V3."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

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
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", required=True, help="Directory containing Kaggle dataset artifacts")
    args = parser.parse_args()

    root = Path(args.dir)
    if not root.exists() or not root.is_dir():
        print(f"ERROR: directory not found: {root}")
        return 2

    present = {p.name for p in root.iterdir() if p.is_file()}
    missing = sorted(REQUIRED - present)

    if missing:
        print("MISSING_ARTIFACTS:")
        for name in missing:
            print(f" - {name}")
        return 1

    print("dependency_bundle_v3_ok")
    print(f"files_present={len(present)} required={len(REQUIRED)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
