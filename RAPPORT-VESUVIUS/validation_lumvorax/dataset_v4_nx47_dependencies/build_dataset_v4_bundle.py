#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import shutil
import struct
import subprocess
import time
import zipfile
from pathlib import Path
from typing import Dict, List

REQUIRED_WHEELS = [
    "imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "imageio-2.37.2-py3-none-any.whl",
    "lazy_loader-0.4-py3-none-any.whl",
    "networkx-3.6.1-py3-none-any.whl",
    "numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "opencv_python-4.13.0.92-cp37-abi3-manylinux_2_28_x86_64.whl",
    "packaging-26.0-py3-none-any.whl",
    "pillow-12.1.1-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "scikit_image-0.26.0-cp311-cp311-manylinux_2_24_x86_64.manylinux_2_28_x86_64.whl",
    "scipy-1.17.0-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "tifffile-2026.1.28-py3-none-any.whl",
    "tifffile-2026.2.16-py3-none-any.whl",
]

SO_NAME = "liblumvorax_replit.so"
COMPETITOR_RESULTS_ZIP = "RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/results.zip"
COMPETITOR_SUBMISSION_ZIP_NAME = "submission.zip"

C_SOURCES = [
    "src/vorax/vorax_operations.c",
    "src/vorax/vorax_3d_volume.c",
    "src/lum/lum_core.c",
    "src/logger/lum_logger.c",
    "src/debug/forensic_logger.c",
]

HEADER_GLOBS = [
    "src/lum/*.h",
    "src/file_formats/*.h",
    "src/vorax/*.h",
    "src/logger/*.h",
    "src/debug/*.h",
]


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def find_file(name: str, roots: List[Path]) -> Path | None:
    for r in roots:
        p = r / name
        if p.exists():
            return p
    for r in roots:
        for p in r.rglob(name):
            if p.is_file():
                return p
    return None


def now_ns() -> int:
    return time.time_ns()




def ensure_python_pkg(pkg: str) -> None:
    try:
        __import__(pkg)
        return
    except Exception:
        subprocess.check_call(["python3", "-m", "pip", "install", "--disable-pip-version-check", pkg])


def expand_wheel_roots(roots: List[Path]) -> List[Path]:
    out: List[Path] = []
    seen = set()
    for r in roots:
        for cand in [r, r / "wheelhouse_v4", r / "bundle"]:
            if cand.exists() and cand.is_dir():
                key = str(cand.resolve())
                if key not in seen:
                    seen.add(key)
                    out.append(cand)
    return out

def find_competitor_tiff_from_results_zip(results_zip: Path) -> tuple[str, bytes]:
    with zipfile.ZipFile(results_zip) as outer:
        inner_blob = outer.read(COMPETITOR_SUBMISSION_ZIP_NAME)
    with zipfile.ZipFile(io.BytesIO(inner_blob)) as inner:
        tif_names = [n for n in inner.namelist() if n.lower().endswith((".tif", ".tiff"))]
        if not tif_names:
            raise RuntimeError("No TIFF file found inside nested submission.zip")
        tif_name = tif_names[0]
        tif_bytes = inner.read(tif_name)
    return tif_name, tif_bytes


def convert_tiff_bytes_to_lum_bytes(tif_bytes: bytes) -> bytes:
    import numpy as np
    import tifffile

    arr = np.asarray(tifffile.imread(io.BytesIO(tif_bytes)))
    if arr.ndim == 2:
        arr = arr[np.newaxis, :, :]
    if arr.ndim != 3:
        raise RuntimeError(f"Unexpected competitor TIFF shape: {arr.shape}")
    arr = np.ascontiguousarray(arr.astype(np.float32, copy=False))

    payload = arr.tobytes()
    digest16 = hashlib.sha512(payload).digest()[:16]
    header = struct.pack('<8sIII16s', b"LUMV1\x00\x00\x00", arr.shape[0], arr.shape[1], arr.shape[2], digest16)
    return header + payload


def main() -> int:
    ap = argparse.ArgumentParser(description="Build V4 NX47 dependency bundle")
    ap.add_argument("--repo-root", default=".", help="Repository root")
    ap.add_argument("--output-dir", required=True, help="Output bundle directory")
    ap.add_argument("--wheel-source", action="append", default=[], help="Directory containing wheel files; can be repeated")
    ap.add_argument("--dry-run", action="store_true", help="Do not copy/build, only report")
    args = ap.parse_args()

    repo = Path(args.repo_root).resolve()
    out = Path(args.output_dir).resolve()
    wheel_roots = expand_wheel_roots([Path(x).resolve() for x in args.wheel_source])

    if not args.dry_run:
        out.mkdir(parents=True, exist_ok=True)

    report: Dict[str, object] = {
        "report": "dataset_v4_build",
        "timestamp_ns": now_ns(),
        "repo_root": str(repo),
        "output_dir": str(out),
        "dry_run": args.dry_run,
        "wheel_sources": [str(x) for x in wheel_roots],
        "events": [],
        "missing": [],
    }

    def event(step: str, **k):
        report["events"].append({"ts_ns": now_ns(), "step": step, **k})

    # 1) Collect wheels
    wheel_manifest = []
    event("collect_wheels_start", expected=len(REQUIRED_WHEELS))
    for name in REQUIRED_WHEELS:
        src = find_file(name, wheel_roots)
        if src is None:
            report["missing"].append(name)
            event("wheel_missing", filename=name)
            continue
        dst = out / name
        event("wheel_found", filename=name, source=str(src))
        if not args.dry_run:
            shutil.copy2(src, dst)
        wheel_manifest.append({
            "filename": name,
            "source": str(src),
            "sha256": sha256_file(src),
            "size": src.stat().st_size,
        })

    # 2) Compile SO
    src_paths = [repo / s for s in C_SOURCES]
    missing_sources = [str(p) for p in src_paths if not p.exists()]
    for ms in missing_sources:
        report["missing"].append(ms)
    so_path = out / SO_NAME
    compile_cmd = ["gcc", "-shared", "-fPIC", "-O3", "-o", str(so_path)] + [str(p) for p in src_paths] + ["-Isrc"]
    event("compile_start", cmd=compile_cmd)
    compile_result = {"cmd": compile_cmd, "ok": False}
    if not missing_sources and not args.dry_run:
        proc = subprocess.run(compile_cmd, cwd=repo, capture_output=True, text=True)
        compile_result.update({
            "returncode": proc.returncode,
            "stdout_head": proc.stdout[:3000],
            "stderr_head": proc.stderr[:3000],
            "ok": proc.returncode == 0 and so_path.exists(),
        })
    else:
        compile_result.update({"skipped": True, "reason": "missing_sources_or_dry_run", "missing_sources": missing_sources})
    event("compile_end", ok=compile_result.get("ok", False))

    # 3) Exported symbols
    symbols = []
    if so_path.exists() and not args.dry_run:
        nm = shutil.which("nm")
        if nm:
            proc = subprocess.run([nm, "-D", str(so_path)], capture_output=True, text=True)
            for line in proc.stdout.splitlines():
                if " vorax_" in line or " lum_" in line or "forensic" in line:
                    symbols.append(line.strip())
    event("symbols_scanned", count=len(symbols))

    # 3b) Include competitor teacher TIFF + LUM conversion in V4 bundle
    competitor = {
        "results_zip": str((repo / COMPETITOR_RESULTS_ZIP).resolve()),
        "included_tiff": None,
        "included_lum": None,
    }
    results_zip = repo / COMPETITOR_RESULTS_ZIP
    if not results_zip.exists():
        report["missing"].append(str(results_zip))
        event("competitor_results_zip_missing", path=str(results_zip))
    else:
        try:
            tif_name, tif_bytes = find_competitor_tiff_from_results_zip(results_zip)
            safe_tif_name = f"competitor_teacher_{Path(tif_name).name}"
            dst_tif = out / safe_tif_name
            event("competitor_tiff_found", inner_name=tif_name, size=len(tif_bytes))
            if not args.dry_run:
                dst_tif.write_bytes(tif_bytes)
            competitor["included_tiff"] = {
                "filename": safe_tif_name,
                "size": len(tif_bytes),
                "sha256": hashlib.sha256(tif_bytes).hexdigest(),
            }

            # Optional conversion to LUM container for teacher signal
            try:
                ensure_python_pkg("numpy")
                ensure_python_pkg("tifffile")
                lum_bytes = convert_tiff_bytes_to_lum_bytes(tif_bytes)
                dst_lum = out / "competitor_teacher_1407735.lum"
                if not args.dry_run:
                    dst_lum.write_bytes(lum_bytes)
                competitor["included_lum"] = {
                    "filename": dst_lum.name,
                    "size": len(lum_bytes),
                    "sha256": hashlib.sha256(lum_bytes).hexdigest(),
                }
                event("competitor_lum_built", filename=dst_lum.name, size=len(lum_bytes))
            except Exception as exc:
                competitor["lum_error"] = str(exc)
                report["missing"].append("competitor_teacher_1407735.lum")
                event("competitor_lum_build_failed", error=str(exc))
        except Exception as exc:
            report["missing"].append("competitor_teacher_tiff_extract_failed")
            competitor["extract_error"] = str(exc)
            event("competitor_tiff_extract_failed", error=str(exc))

    # 4) Header inventory
    headers = []
    for g in HEADER_GLOBS:
        for h in sorted(repo.glob(g)):
            headers.append(str(h.relative_to(repo)))
    event("headers_inventory", count=len(headers))

    # 5) Manifest
    files = []
    if not args.dry_run and out.exists():
        for p in sorted(out.iterdir()):
            if p.is_file():
                files.append({"name": p.name, "size": p.stat().st_size, "sha256": sha256_file(p)})

    report.update({
        "required_wheels": REQUIRED_WHEELS,
        "wheel_manifest": wheel_manifest,
        "compile": compile_result,
        "exported_symbols_subset": symbols[:500],
        "competitor_teacher_assets": competitor,
        "headers_found": headers,
        "bundle_files": files,
        "status": "ok" if len(report["missing"]) == 0 and (compile_result.get("ok") or args.dry_run) else "fail",
    })

    manifest_name = "dataset_v4_build_report.json"
    if not args.dry_run:
        (out / manifest_name).write_text(json.dumps(report, indent=2), encoding="utf-8")
    else:
        print(json.dumps(report, indent=2))

    return 0 if report["status"] == "ok" else 2


if __name__ == "__main__":
    raise SystemExit(main())
