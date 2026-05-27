"""Reusable Kaggle bootstrap for NX/LUM-VORAX dependencies.

This module is designed to be vendored into notebooks (copy/paste or import) so that
all active versions can install wheels from both existing NX47 dependency datasets and
new LUM-VORAX wheel datasets.
"""

from __future__ import annotations

import importlib
import os
import subprocess
import sys
from pathlib import Path
from typing import Iterable


def _is_available(pkg: str) -> bool:
    return importlib.util.find_spec(pkg) is not None


def install_offline(pkg: str, extra_roots: Iterable[str] = ()) -> None:
    if _is_available(pkg):
        return

    roots = [
        Path("/kaggle/input/datasets/ndarray2000/nx47-dependencies"),
        Path("/kaggle/input/nx47-dependencies"),
        Path("/kaggle/input/lum-vorax-dependencies"),
        Path("/kaggle/input/lumvorax-dependencies"),
    ] + [Path(x) for x in extra_roots]

    for root in roots:
        if root.exists():
            subprocess.check_call(
                [sys.executable, "-m", "pip", "install", "--no-index", f"--find-links={root}", pkg]
            )
            if _is_available(pkg):
                return

    raise RuntimeError(
        f"Offline dependency not found for {pkg}. Checked roots: {', '.join(str(r) for r in roots)}"
    )


def bootstrap_nx47_lum_vorax(extra_roots: Iterable[str] = ()) -> None:
    for pkg in ("numpy", "pandas", "pyarrow", "tifffile", "imagecodecs"):
        install_offline(pkg, extra_roots=extra_roots)


def compile_optional_lum_vorax_so(output_path: str = "/kaggle/working/liblumvorax.so") -> str:
    """Best-effort local compile for optional ctypes bridge.

    No-op if gcc is unavailable.
    """

    src_candidates = [
        "/kaggle/working/src/vorax/vorax_operations.c",
        "/kaggle/working/src/vorax/vorax_3d_volume.c",
        "/kaggle/working/src/lum/lum_core.c",
        "/kaggle/working/src/logger/lum_logger.c",
        "src/vorax/vorax_operations.c",
        "src/vorax/vorax_3d_volume.c",
        "src/lum/lum_core.c",
        "src/logger/lum_logger.c",
    ]

    available = [p for p in src_candidates if os.path.exists(p)]
    if len(available) < 2:
        return ""

    cmd = [
        "gcc",
        "-shared",
        "-fPIC",
        "-O3",
        "-o",
        output_path,
        *available,
    ]

    try:
        subprocess.check_call(cmd)
    except Exception:
        return ""

    return output_path if os.path.exists(output_path) else ""

