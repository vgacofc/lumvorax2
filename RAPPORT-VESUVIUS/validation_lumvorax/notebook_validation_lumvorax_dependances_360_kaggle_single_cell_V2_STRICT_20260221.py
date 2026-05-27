# ================================================================
# LUMVORAX DEPENDENCY 360 VALIDATION (KAGGLE SINGLE CELL - V2 STRICT)
# ================================================================
# Purpose:
# - Strict/Fail-Closed validation for Kaggle.
# - NO compression fallback: TIFF must be writable with LZW only.
# - Native C/C++ source inventory must be present (no silent skip).
# - Self-contained in one cell/script for copy-paste in Kaggle.

from __future__ import annotations

import importlib
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

LUM_MAGIC = b"LUMV1\x00\x00\x00"
FUNC_RE = re.compile(r"^\s*([a-zA-Z_][\w\s\*]+?)\s+([a-zA-Z_]\w*)\s*\(([^;]*?)\)\s*;\s*$")

# Strict policy toggles (intentionally hard requirements)
REQUIRE_IMAGECODECS = True
REQUIRE_LZW_WRITE = True
REQUIRE_NATIVE_SOURCES = True
REQUIRE_C_SYNTAX_SMOKE_PASS = True

KAGGLE_DEP_PATHS = [
    Path('/kaggle/input/datasets/ndarray2000/nx47-dependencies'),
    Path('/kaggle/input/nx47-dependencies'),
    Path('/kaggle/input/lum-vorax-dependencies'),
    Path('/kaggle/input/lumvorax-dependencies'),
]
SOURCE_SCAN_DIRS = [
    Path('/kaggle/working/src/lum'), Path('/kaggle/working/src/file_formats'), Path('/kaggle/working/src/vorax'),
    Path('/kaggle/working/src/logger'), Path('/kaggle/working/src/debug'),
    Path('src/lum'), Path('src/file_formats'), Path('src/vorax'), Path('src/logger'), Path('src/debug'),
]


class StrictValidationError(RuntimeError):
    pass


def pkg_available(name: str) -> bool:
    try:
        importlib.import_module(name)
        return True
    except Exception:
        return False


def install_offline_if_missing(pkg: str) -> Dict[str, Any]:
    if pkg_available(pkg):
        return {"package": pkg, "status": "already_installed"}
    py = sys.executable
    exact = {
        'numpy': 'numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl',
        'tifffile': 'tifffile-2026.1.28-py3-none-any.whl',
        'imagecodecs': 'imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl',
    }
    last = 'not found'
    for root in KAGGLE_DEP_PATHS:
        if not root.exists():
            continue
        wheel = root / exact.get(pkg, '')
        try:
            if wheel.exists():
                subprocess.check_call([py, '-m', 'pip', 'install', '--disable-pip-version-check', '--no-index', str(wheel)])
            else:
                subprocess.check_call([py, '-m', 'pip', 'install', '--disable-pip-version-check', '--no-index', f'--find-links={root}', pkg])
            if pkg_available(pkg):
                return {"package": pkg, "status": "installed", "root": str(root)}
        except Exception as exc:
            last = str(exc)
    return {"package": pkg, "status": "missing", "error": last}


def role_for_path(path: Path) -> str:
    p = str(path).replace('\\', '/')
    if '/src/lum/' in p or p.startswith('src/lum/'):
        return 'LUM core structures/security/memory'
    if '/src/file_formats/' in p or p.startswith('src/file_formats/'):
        return 'Universal .lum format encoding/decoding and file manager'
    if '/src/vorax/' in p or p.startswith('src/vorax/'):
        return 'VORAX 3D operators/processing'
    if '/src/logger/' in p or p.startswith('src/logger/') or '/src/debug/' in p or p.startswith('src/debug/'):
        return 'Forensic/logging/monitoring'
    return 'Other'


def scan_headers_and_functions() -> Dict[str, Any]:
    modules, headers, funcs_total = [], 0, 0
    for d in SOURCE_SCAN_DIRS:
        if not d.exists() or not d.is_dir():
            continue
        for h in sorted(d.glob('*.h')):
            lines = h.read_text(encoding='utf-8', errors='replace').splitlines()
            funcs = []
            for ln in lines:
                m = FUNC_RE.match(ln)
                if m:
                    funcs.append({'return': m.group(1).strip(), 'name': m.group(2), 'args': m.group(3).strip()})
            modules.append({'header': str(h), 'role': role_for_path(h), 'function_count': len(funcs), 'functions': funcs})
            headers += 1
            funcs_total += len(funcs)
    return {'summary': {'headers': headers, 'functions': funcs_total}, 'modules': modules}


def smoke_compile_c_modules() -> List[Dict[str, Any]]:
    gcc = shutil.which('gcc')
    if not gcc:
        raise StrictValidationError('gcc_not_found: strict mode requires gcc for C syntax smoke checks')

    candidates = [
        Path('/kaggle/working/src/lum/lum_core.c'), Path('/kaggle/working/src/file_formats/lum_native_universal_format.c'),
        Path('/kaggle/working/src/file_formats/lum_native_file_handler.c'), Path('/kaggle/working/src/file_formats/lum_secure_serialization.c'),
        Path('/kaggle/working/src/vorax/vorax_3d_volume.c'), Path('/kaggle/working/src/vorax/vorax_operations.c'),
        Path('src/lum/lum_core.c'), Path('src/file_formats/lum_native_universal_format.c'),
        Path('src/file_formats/lum_native_file_handler.c'), Path('src/file_formats/lum_secure_serialization.c'),
        Path('src/vorax/vorax_3d_volume.c'), Path('src/vorax/vorax_operations.c'),
    ]

    existing, seen = [], set()
    for c in candidates:
        if c.exists() and str(c.resolve()) not in seen:
            existing.append(c)
            seen.add(str(c.resolve()))

    if REQUIRE_NATIVE_SOURCES and not existing:
        raise StrictValidationError('no_c_sources_found: strict mode forbids empty native source scan')

    out = []
    for c in existing:
        cmd = [gcc, '-fsyntax-only', str(c), '-I', str(Path('src').resolve())]
        r = subprocess.run(cmd, capture_output=True, text=True)
        item = {'file': str(c), 'status': 'ok' if r.returncode == 0 else 'fail', 'returncode': r.returncode, 'stderr_head': (r.stderr or '')[:1000]}
        out.append(item)

    if REQUIRE_C_SYNTAX_SMOKE_PASS:
        bad = [x for x in out if x['status'] != 'ok']
        if bad:
            raise StrictValidationError(f'c_syntax_smoke_failed: {len(bad)} files failed syntax check')

    return out


def normalize_volume(arr):
    import numpy as np
    if arr.ndim == 2:
        arr = arr[np.newaxis, :, :]
    if arr.ndim != 3:
        raise ValueError(f'Expected 2D/3D volume, got {arr.shape}')
    return np.ascontiguousarray(arr.astype(np.float32, copy=False))


def encode_lum_v1(arr3d):
    z, h, w = arr3d.shape
    payload = arr3d.tobytes()
    digest16 = sha512(payload).digest()[:16]
    header = struct.pack('<8sIII16s', LUM_MAGIC, z, h, w, digest16)
    return header + payload


def decode_lum_v1(blob: bytes):
    import numpy as np
    hs = struct.calcsize('<8sIII16s')
    magic, z, h, w, digest16 = struct.unpack('<8sIII16s', blob[:hs])
    if magic != LUM_MAGIC:
        raise ValueError('invalid LUM magic')
    payload = blob[hs:]
    expected = int(z) * int(h) * int(w) * 4
    if len(payload) != expected:
        raise ValueError('payload size mismatch')
    if sha512(payload).digest()[:16] != digest16:
        raise ValueError('payload checksum mismatch')
    return np.frombuffer(payload, dtype=np.float32).reshape((z, h, w))


def assert_lzw_capable(tifffile_mod) -> None:
    """Hard preflight: strict mode requires a successful LZW write."""
    import numpy as np

    if REQUIRE_IMAGECODECS and not pkg_available('imagecodecs'):
        raise StrictValidationError('imagecodecs_missing: strict mode requires imagecodecs')

    codec_err = None
    try:
        import imagecodecs  # noqa: F401
        if not hasattr(imagecodecs, 'lzw_encode'):
            raise StrictValidationError('imagecodecs_lzw_missing: imagecodecs.lzw_encode is unavailable')
    except Exception as exc:
        codec_err = str(exc)

    with tempfile.TemporaryDirectory() as td:
        test_path = Path(td) / 'lzw_preflight.tif'
        arr = (np.arange(64, dtype=np.uint8).reshape(8, 8))
        try:
            tifffile_mod.imwrite(test_path, arr, compression='LZW')
            _ = tifffile_mod.imread(test_path)
        except Exception as exc:
            raise StrictValidationError(f'lzw_write_failed: {exc}; imagecodecs_hint={codec_err}')


def tiff_lum_roundtrip_test() -> Dict[str, Any]:
    if not pkg_available('numpy') or not pkg_available('tifffile'):
        raise StrictValidationError('numpy_or_tifffile_unavailable')

    import numpy as np
    import tifffile

    tifffile = importlib.reload(tifffile)

    if REQUIRE_LZW_WRITE:
        assert_lzw_capable(tifffile)

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        tif_path = td / 'synthetic_teacher.tif'
        lum_path = td / 'synthetic_teacher.lum'

        rng = np.random.default_rng(42)
        vol = (rng.random((8, 32, 32)) > 0.87).astype(np.uint8)

        tifffile.imwrite(tif_path, vol, compression='LZW')

        arr3d = normalize_volume(np.asarray(tifffile.imread(tif_path)))
        blob = encode_lum_v1(arr3d)
        lum_path.write_bytes(blob)
        restored = decode_lum_v1(blob)

        slice_logs = []
        for z in range(restored.shape[0]):
            s = restored[z]
            slice_logs.append({'z': z, 'min': float(s.min()), 'max': float(s.max()), 'mean': float(s.mean()), 'std': float(s.std()), 'nonzero_pct': float((s > 0).mean() * 100.0)})

        return {
            'status': 'ok',
            'strict_mode': True,
            'required_compression': 'LZW',
            'shape': [int(x) for x in restored.shape],
            'dtype': str(restored.dtype),
            'global_min': float(restored.min()), 'global_max': float(restored.max()), 'global_mean': float(restored.mean()),
            'global_nonzero_pct': float((restored > 0).mean() * 100.0),
            'slice_logs': slice_logs,
            'roundtrip_ok': bool(np.allclose(arr3d, restored, atol=0.0)),
            'tiff_sha512': sha512(tif_path.read_bytes()).hexdigest(),
            'lum_sha512': sha512(lum_path.read_bytes()).hexdigest(),
            'write_compression_used': 'LZW',
            'forensic_write': {'write_compression_used': 'LZW', 'write_errors': []},
        }


def main() -> None:
    t0 = time.time()
    report: Dict[str, Any] = {
        'report_name': 'lumvorax_dependency_360_kaggle_single_cell_v2_strict',
        'timestamp_ns': time.time_ns(),
        'runtime': {
            'python': sys.version,
            'platform': platform.platform(),
            'cwd': str(Path.cwd()),
            'is_kaggle': Path('/kaggle').exists(),
        },
        'strict_policy': {
            'require_imagecodecs': REQUIRE_IMAGECODECS,
            'require_lzw_write': REQUIRE_LZW_WRITE,
            'require_native_sources': REQUIRE_NATIVE_SOURCES,
            'require_c_syntax_smoke_pass': REQUIRE_C_SYNTAX_SMOKE_PASS,
            'fallback_allowed': False,
        },
        'dependency_dataset_paths': [
            {'path': str(p), 'exists': p.exists(), 'files_count': (len(list(p.iterdir())) if p.exists() and p.is_dir() else 0)}
            for p in KAGGLE_DEP_PATHS
        ],
    }

    try:
        report['install_report'] = [install_offline_if_missing(pkg) for pkg in ('numpy', 'tifffile', 'imagecodecs')]
        report['imports'] = {p: pkg_available(p) for p in ('numpy', 'tifffile', 'imagecodecs', 'pyarrow')}

        report['module_inventory'] = scan_headers_and_functions()
        if REQUIRE_NATIVE_SOURCES and report['module_inventory']['summary']['headers'] <= 0:
            raise StrictValidationError('no_native_headers_found: strict mode requires headers > 0')

        report['c_syntax_smoke'] = smoke_compile_c_modules()
        report['tiff_lum_roundtrip_test'] = tiff_lum_roundtrip_test()

        report['status'] = 'ok'
    except Exception as exc:
        report['status'] = 'fail'
        report['error_type'] = type(exc).__name__
        report['error'] = str(exc)

    report['elapsed_s'] = time.time() - t0

    out = Path('/kaggle/working/lumvorax_360_validation_report_v2_strict.json')
    if not out.parent.exists():
        out = Path('lumvorax_360_validation_report_v2_strict.json')
    out.write_text(json.dumps(report, indent=2), encoding='utf-8')

    print(json.dumps({
        'status': report.get('status'),
        'error_type': report.get('error_type'),
        'error': report.get('error'),
        'report': str(out),
        'module_headers': report.get('module_inventory', {}).get('summary', {}).get('headers'),
        'public_functions': report.get('module_inventory', {}).get('summary', {}).get('functions'),
        'roundtrip_status': report.get('tiff_lum_roundtrip_test', {}).get('status') if isinstance(report.get('tiff_lum_roundtrip_test'), dict) else None,
        'write_compression_used': report.get('tiff_lum_roundtrip_test', {}).get('write_compression_used') if isinstance(report.get('tiff_lum_roundtrip_test'), dict) else None,
        'elapsed_s': report.get('elapsed_s'),
    }, indent=2))

    if report.get('status') != 'ok':
        raise SystemExit(2)


if __name__ == '__main__':
    main()
