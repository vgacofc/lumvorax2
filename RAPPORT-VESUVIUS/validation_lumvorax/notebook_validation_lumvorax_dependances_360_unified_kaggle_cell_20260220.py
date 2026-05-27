# ================================================================
# LUMVORAX DEPENDENCY 360 VALIDATION (UNIFIED / SINGLE-CELL STYLE)
# ================================================================
# This script is intentionally self-contained so it can be pasted into
# one single Kaggle notebook code cell without relying on local helper .py.
#
# It performs:
#  1) Kaggle dependency dataset presence checks
#  2) Python package availability checks
#  3) Optional C/C++ module inventory scan (if sources are present)
#  4) Optional C syntax-only smoke compile (if gcc + sources are present)
#  5) TIFF -> LUMV1 conversion + roundtrip decode verification
#  6) Per-slice forensic metrics and final JSON report export
#
# Output report (default): /kaggle/working/lumvorax_360_validation_report.json

from __future__ import annotations

import json
import os
import platform
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from hashlib import sha512
from pathlib import Path
from typing import Any, Dict, List

# ---------- Runtime imports (fail-loud but captured) ----------
_IMPORT_ERRORS: List[str] = []
try:
    import numpy as np
except Exception as exc:  # pragma: no cover
    _IMPORT_ERRORS.append(f"numpy import failed: {exc}")
    np = None

try:
    import tifffile
except Exception as exc:  # pragma: no cover
    _IMPORT_ERRORS.append(f"tifffile import failed: {exc}")
    tifffile = None

# ---------- Config ----------
KAGGLE_DEP_PATHS = [
    Path("/kaggle/input/datasets/ndarray2000/nx47-dependencies"),
    Path("/kaggle/input/nx47-dependencies"),
    Path("/kaggle/input/lum-vorax-dependencies"),
    Path("/kaggle/input/lumvorax-dependencies"),
]

SOURCE_SCAN_DIRS = [
    Path("/kaggle/working/src/lum"),
    Path("/kaggle/working/src/file_formats"),
    Path("/kaggle/working/src/vorax"),
    Path("/kaggle/working/src/logger"),
    Path("/kaggle/working/src/debug"),
    Path("src/lum"),
    Path("src/file_formats"),
    Path("src/vorax"),
    Path("src/logger"),
    Path("src/debug"),
]

LUM_MAGIC = b"LUMV1\x00\x00\x00"
FUNC_RE = re.compile(r"^\s*([a-zA-Z_][\w\s\*]+?)\s+([a-zA-Z_]\w*)\s*\(([^;]*?)\)\s*;\s*$")


# ---------- Utilities ----------
def now_ns() -> int:
    return time.time_ns()


def pkg_available(name: str) -> bool:
    try:
        __import__(name)
        return True
    except Exception:
        return False


def list_existing_paths(paths: List[Path]) -> List[Path]:
    return [p for p in paths if p.exists()]


def file_sha512(path: Path) -> str:
    h = sha512()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def role_for_path(path: Path) -> str:
    p = str(path).replace("\\", "/")
    if "/src/lum/" in p or p.startswith("src/lum/"):
        return "LUM core structures/security/memory"
    if "/src/file_formats/" in p or p.startswith("src/file_formats/"):
        return "Universal .lum format encoding/decoding and file manager"
    if "/src/vorax/" in p or p.startswith("src/vorax/"):
        return "VORAX 3D operators/processing"
    if "/src/logger/" in p or p.startswith("src/logger/") or "/src/debug/" in p or p.startswith("src/debug/"):
        return "Forensic/logging/monitoring"
    return "Other"


# ---------- LUMV1 encode/decode ----------
def normalize_volume(arr: "np.ndarray") -> "np.ndarray":
    if arr.ndim == 2:
        arr = arr[np.newaxis, :, :]
    if arr.ndim != 3:
        raise ValueError(f"Expected 2D/3D volume, got shape={arr.shape}")
    return np.ascontiguousarray(arr.astype(np.float32, copy=False))


def encode_lum_v1(arr3d: "np.ndarray") -> bytes:
    z, h, w = arr3d.shape
    payload = arr3d.tobytes()
    digest16 = sha512(payload).digest()[:16]
    header = struct.pack("<8sIII16s", LUM_MAGIC, z, h, w, digest16)
    return header + payload


def decode_lum_v1(blob: bytes) -> "np.ndarray":
    header_size = struct.calcsize("<8sIII16s")
    if len(blob) < header_size:
        raise ValueError("LUM blob too small")
    magic, z, h, w, digest16 = struct.unpack("<8sIII16s", blob[:header_size])
    if magic != LUM_MAGIC:
        raise ValueError("Invalid LUM magic")
    payload = blob[header_size:]
    expected = int(z) * int(h) * int(w) * 4
    if len(payload) != expected:
        raise ValueError(f"LUM payload size mismatch: got={len(payload)} expected={expected}")
    if sha512(payload).digest()[:16] != digest16:
        raise ValueError("LUM payload checksum mismatch")
    return np.frombuffer(payload, dtype=np.float32).reshape((z, h, w))


# ---------- Inventory scan ----------
def scan_headers_and_functions() -> Dict[str, Any]:
    modules: List[Dict[str, Any]] = []
    headers = 0
    funcs_total = 0

    for d in SOURCE_SCAN_DIRS:
        if not d.exists() or not d.is_dir():
            continue
        for h in sorted(d.glob("*.h")):
            txt = h.read_text(encoding="utf-8", errors="replace").splitlines()
            funcs = []
            for ln in txt:
                m = FUNC_RE.match(ln)
                if m:
                    funcs.append(
                        {
                            "return": m.group(1).strip(),
                            "name": m.group(2),
                            "args": m.group(3).strip(),
                        }
                    )
            modules.append(
                {
                    "header": str(h),
                    "role": role_for_path(h),
                    "function_count": len(funcs),
                    "functions": funcs,
                }
            )
            headers += 1
            funcs_total += len(funcs)

    return {
        "summary": {"headers": headers, "functions": funcs_total},
        "modules": modules,
    }


# ---------- Optional C syntax smoke compile ----------
def smoke_compile_c_modules() -> List[Dict[str, Any]]:
    gcc = shutil.which("gcc")
    if not gcc:
        return [{"status": "skipped", "reason": "gcc_not_found"}]

    candidates = [
        Path("/kaggle/working/src/lum/lum_core.c"),
        Path("/kaggle/working/src/file_formats/lum_native_universal_format.c"),
        Path("/kaggle/working/src/file_formats/lum_native_file_handler.c"),
        Path("/kaggle/working/src/file_formats/lum_secure_serialization.c"),
        Path("/kaggle/working/src/vorax/vorax_3d_volume.c"),
        Path("/kaggle/working/src/vorax/vorax_operations.c"),
        Path("src/lum/lum_core.c"),
        Path("src/file_formats/lum_native_universal_format.c"),
        Path("src/file_formats/lum_native_file_handler.c"),
        Path("src/file_formats/lum_secure_serialization.c"),
        Path("src/vorax/vorax_3d_volume.c"),
        Path("src/vorax/vorax_operations.c"),
    ]

    existing = []
    seen = set()
    for c in candidates:
        if c.exists() and str(c.resolve()) not in seen:
            existing.append(c)
            seen.add(str(c.resolve()))

    if not existing:
        return [{"status": "skipped", "reason": "no_c_sources_found"}]

    results = []
    for c in existing:
        cmd = [gcc, "-fsyntax-only", str(c), "-I", str(Path("src").resolve())]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        results.append(
            {
                "file": str(c),
                "returncode": proc.returncode,
                "status": "ok" if proc.returncode == 0 else "fail",
                "stderr_head": (proc.stderr or "")[:1000],
            }
        )
    return results


# ---------- Synthetic TIFF->LUM test (self-contained) ----------
def tiff_lum_roundtrip_test() -> Dict[str, Any]:
    if np is None or tifffile is None:
        return {
            "status": "skipped",
            "reason": "numpy_or_tifffile_unavailable",
            "import_errors": list(_IMPORT_ERRORS),
        }

    with tempfile.TemporaryDirectory() as td:
        tdp = Path(td)
        tif_path = tdp / "synthetic_teacher.tif"
        lum_path = tdp / "synthetic_teacher.lum"

        # 3D binary-like synthetic volume for deterministic forensic test
        rng = np.random.default_rng(42)
        vol = (rng.random((8, 32, 32)) > 0.87).astype(np.uint8)
        tifffile.imwrite(tif_path, vol, compression="LZW")

        tiff_loaded = np.asarray(tifffile.imread(tif_path))
        arr3d = normalize_volume(tiff_loaded)
        blob = encode_lum_v1(arr3d)
        lum_path.write_bytes(blob)

        restored = decode_lum_v1(blob)

        slice_logs = []
        for z in range(restored.shape[0]):
            s = restored[z]
            slice_logs.append(
                {
                    "z": z,
                    "min": float(s.min()),
                    "max": float(s.max()),
                    "mean": float(s.mean()),
                    "std": float(s.std()),
                    "nonzero_pct": float((s > 0).mean() * 100.0),
                }
            )

        return {
            "status": "ok",
            "tiff_path": str(tif_path),
            "lum_path": str(lum_path),
            "tiff_sha512": file_sha512(tif_path),
            "lum_sha512": file_sha512(lum_path),
            "shape": [int(x) for x in restored.shape],
            "dtype": str(restored.dtype),
            "global_min": float(restored.min()),
            "global_max": float(restored.max()),
            "global_mean": float(restored.mean()),
            "global_nonzero_pct": float((restored > 0).mean() * 100.0),
            "slice_logs": slice_logs,
            "roundtrip_ok": bool(np.allclose(arr3d, restored, atol=0.0)),
            "payload_sha512": sha512(restored.tobytes()).hexdigest(),
        }


# ---------- Main ----------
def main() -> None:
    t0 = time.time()

    report: Dict[str, Any] = {
        "report_name": "lumvorax_dependency_360_unified_single_cell",
        "timestamp_ns": now_ns(),
        "runtime": {
            "python": sys.version,
            "platform": platform.platform(),
            "cwd": str(Path.cwd()),
            "is_kaggle": Path("/kaggle").exists(),
        },
        "imports": {
            "numpy": pkg_available("numpy"),
            "tifffile": pkg_available("tifffile"),
            "imagecodecs": pkg_available("imagecodecs"),
            "pyarrow": pkg_available("pyarrow"),
            "import_errors": list(_IMPORT_ERRORS),
        },
    }

    # 1) Dependency dataset paths
    report["dependency_dataset_paths"] = [
        {
            "path": str(p),
            "exists": p.exists(),
            "files_count": (len(list(p.iterdir())) if p.exists() and p.is_dir() else 0),
        }
        for p in KAGGLE_DEP_PATHS
    ]

    # 2) Module inventory
    report["module_inventory"] = scan_headers_and_functions()

    # 3) C compile smoke
    report["c_syntax_smoke"] = smoke_compile_c_modules()

    # 4) TIFF->LUM roundtrip forensic test
    report["tiff_lum_roundtrip_test"] = tiff_lum_roundtrip_test()

    report["elapsed_s"] = time.time() - t0

    out_default = Path("/kaggle/working/lumvorax_360_validation_report.json")
    if not out_default.parent.exists():
        out_default = Path("lumvorax_360_validation_report.json")

    out_default.parent.mkdir(parents=True, exist_ok=True)
    out_default.write_text(json.dumps(report, indent=2), encoding="utf-8")

    # concise console summary
    summary = {
        "status": "ok",
        "report": str(out_default),
        "imports": report["imports"],
        "module_headers": report["module_inventory"]["summary"]["headers"],
        "public_functions": report["module_inventory"]["summary"]["functions"],
        "roundtrip_status": report["tiff_lum_roundtrip_test"].get("status"),
        "elapsed_s": report["elapsed_s"],
    }
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
