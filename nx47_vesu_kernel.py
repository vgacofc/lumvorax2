# ================================================================
# LUMVORAX DEPENDENCY 360 VALIDATION (KAGGLE SINGLE CELL - V13 STRICT)
# ================================================================
# Purpose:
# - Binary-first validation for Kaggle dependency datasets.
# - NO C/.h validation and NO native source compilation checks.
# - Validate wheel set + shared object presence + enforced .so load + roundtrip.

from __future__ import annotations

import ctypes
import importlib
import json
import os
import platform
import re
import struct
import subprocess
import sys
import tempfile
import time
from hashlib import sha256, sha512
from pathlib import Path
from typing import Any, Dict, List, Optional

from packaging.tags import sys_tags
from packaging.utils import canonicalize_name, parse_wheel_filename

LUM_MAGIC = b"LUMV1\x00\x00\x00"

KAGGLE_DEP_PATHS = [
    Path('/kaggle/input/datasets/ndarray2000/nx47-dependencies'),
    Path('/kaggle/input/nx47-dependencies'),
    Path('/kaggle/input/lum-vorax-dependencies'),
    Path('/kaggle/input/lumvorax-dependencies'),
]

# Package contract for NX47 dependency dataset.
# We validate by package names (not one unique wheel filename), then enforce runtime tag compatibility.
REQUIRED_PACKAGES = [
    'imagecodecs',
    'imageio',
    'lazy-loader',
    'networkx',
    'numpy',
    'opencv-python',
    'packaging',
    'pillow',
    'scikit-image',
    'scipy',
    'tifffile',
]

# Hard expectation for native lib.
EXPECTED_NATIVE_LIB = 'liblumvorax.so'

IS_KAGGLE = Path('/kaggle').exists()
STRICT_NO_FALLBACK = os.environ.get('LUMVORAX_STRICT_NO_FALLBACK', '1') == '1'
REQUIRE_DATASET = os.environ.get('LUMVORAX_REQUIRE_DATASET', '1' if IS_KAGGLE else '0') == '1'
REQUIRE_SO_PRESENCE = os.environ.get('LUMVORAX_REQUIRE_SO_PRESENCE', '1') == '1'
ENFORCE_SO_LOAD = os.environ.get('LUMVORAX_ENFORCE_SO_LOAD', '1' if STRICT_NO_FALLBACK else '0') == '1'
SKIP_ROUNDTRIP = os.environ.get('LUMVORAX_SKIP_ROUNDTRIP', '0' if IS_KAGGLE else '1') == '1'


def now_ns() -> int:
    return time.time_ns()


def log_event(report: Dict[str, Any], step: str, **payload: Any) -> None:
    report.setdefault('events', []).append({'ts_ns': now_ns(), 'step': step, **payload})


def pkg_available(name: str) -> bool:
    try:
        importlib.import_module(name)
        return True
    except Exception:
        return False


def detect_dataset_root(report: Dict[str, Any]) -> Optional[Path]:
    for root in KAGGLE_DEP_PATHS:
        if root.exists() and root.is_dir():
            log_event(report, 'dataset_root_selected', root=str(root))
            return root
    log_event(report, 'dataset_root_missing', tried=[str(p) for p in KAGGLE_DEP_PATHS])
    return None


def _wheel_index(files: Dict[str, Path]) -> Dict[str, List[Dict[str, Any]]]:
    runtime_tags = set(sys_tags())
    index: Dict[str, List[Dict[str, Any]]] = {}

    for wheel_name, wheel_path in sorted(files.items()):
        if not wheel_name.endswith('.whl'):
            continue
        entry: Dict[str, Any] = {'wheel': wheel_name, 'path': str(wheel_path)}
        try:
            wheel_dist, wheel_version, _, wheel_tags = parse_wheel_filename(wheel_name)
            package = canonicalize_name(str(wheel_dist))
            tag_objects = set(wheel_tags)
            compatible = bool(tag_objects.intersection(runtime_tags))
            entry.update({
                'package': package,
                'version': str(wheel_version),
                'compatible': compatible,
                'tag_count': len(tag_objects),
            })
            index.setdefault(package, []).append(entry)
        except Exception as exc:
            entry.update({'package': None, 'compatible': False, 'parse_error': str(exc)})
            index.setdefault('_parse_errors', []).append(entry)

    return index


def _best_compatible_wheel(index: Dict[str, List[Dict[str, Any]]], package_name: str) -> Optional[Dict[str, Any]]:
    pkg = canonicalize_name(package_name)
    candidates = [x for x in index.get(pkg, []) if x.get('compatible')]
    if not candidates:
        return None
    # deterministic: choose lexicographically last wheel name (typically highest version)
    candidates.sort(key=lambda x: x['wheel'])
    return candidates[-1]


def install_wheel_file(wheel_path: Path, report: Dict[str, Any], reason: str) -> Dict[str, Any]:
    cmd = [sys.executable, '-m', 'pip', 'install', '--disable-pip-version-check', '--no-index', str(wheel_path)]
    try:
        log_event(report, 'wheel_install_attempt', wheel=str(wheel_path), reason=reason, cmd=cmd)
        subprocess.check_call(cmd)
        return {'status': 'installed', 'wheel': str(wheel_path), 'reason': reason}
    except Exception as exc:
        log_event(report, 'wheel_install_fail', wheel=str(wheel_path), reason=reason, error=str(exc))
        return {'status': 'failed', 'wheel': str(wheel_path), 'reason': reason, 'error': str(exc)}


def install_offline_if_missing(pkg: str, report: Dict[str, Any], dataset_root: Optional[Path]) -> Dict[str, Any]:
    if pkg_available(pkg):
        return {'package': pkg, 'status': 'already_installed'}

    if dataset_root and dataset_root.exists():
        files = {p.name: p for p in dataset_root.iterdir() if p.is_file()}
        wheel_idx = _wheel_index(files)
        best = _best_compatible_wheel(wheel_idx, pkg)
        if best:
            res = install_wheel_file(Path(best['path']), report, reason=f'best_compatible_{canonicalize_name(pkg)}')
            if res['status'] == 'installed' and pkg_available(pkg):
                return {
                    'package': pkg,
                    'status': 'installed',
                    'method': 'best_compatible_wheel',
                    'wheel': best['wheel'],
                }

    last = 'no_compatible_wheel_found'
    for root in KAGGLE_DEP_PATHS:
        if not root.exists():
            continue
        cmd = [sys.executable, '-m', 'pip', 'install', '--disable-pip-version-check', '--no-index', f'--find-links={root}', pkg]
        try:
            log_event(report, 'dependency_install_attempt', package=pkg, root=str(root), cmd=cmd)
            subprocess.check_call(cmd)
            if pkg_available(pkg):
                return {'package': pkg, 'status': 'installed', 'method': 'find_links', 'root': str(root)}
        except Exception as exc:
            last = str(exc)
            log_event(report, 'dependency_install_fail', package=pkg, root=str(root), error=last)

    return {'package': pkg, 'status': 'missing', 'error': last}


def inspect_dataset_artifacts(root: Optional[Path], report: Dict[str, Any]) -> Dict[str, Any]:
    if root is None:
        return {'status': 'missing', 'reason': 'dataset_root_not_found'}

    files = {p.name: p for p in root.iterdir() if p.is_file()}
    native = files.get(EXPECTED_NATIVE_LIB)

    wheel_index = _wheel_index(files)
    missing_required_packages: List[str] = []
    incompatible_required_packages: List[str] = []
    selected_compatible_wheels: Dict[str, str] = {}

    pkg_map = {
        'opencv-python': 'cv2',
        'scikit-image': 'skimage',
        'pillow': 'PIL',
    }

    for pkg in REQUIRED_PACKAGES:
        canon = canonicalize_name(pkg)
        import_name = pkg_map.get(canon, canon)
        
        # Check if package is ALREADY installed in the environment (e.g. Kaggle default)
        if pkg_available(import_name):
            selected_compatible_wheels[canon] = "pre-installed"
            continue

        pkg_entries = wheel_index.get(canon, [])
        if not pkg_entries:
            missing_required_packages.append(canon)
            continue
        best = _best_compatible_wheel(wheel_index, canon)
        if best is None:
            incompatible_required_packages.append(canon)
            continue
        selected_compatible_wheels[canon] = best['wheel']

    status_ok = (
        (not REQUIRE_SO_PRESENCE or native is not None)
        and not missing_required_packages
        and not incompatible_required_packages
    )

    out = {
        'status': 'ok' if status_ok else 'fail',
        'dataset_root': str(root),
        'expected_native_lib': EXPECTED_NATIVE_LIB,
        'resolved_native_lib_name': native.name if native else None,
        'native_lib': str(native) if native else None,
        'native_lib_sha256': sha256(native.read_bytes()).hexdigest() if native else None,
        'native_lib_size': native.stat().st_size if native else None,
        'required_packages': REQUIRED_PACKAGES,
        'missing_required_packages': missing_required_packages,
        'incompatible_required_packages': incompatible_required_packages,
        'selected_compatible_wheels': selected_compatible_wheels,
        'wheel_compatibility': {
            'runtime_tag_head': [str(t) for t in list(sys_tags())[:24]],
            'wheel_inventory_count': sum(1 for n in files if n.endswith('.whl')),
            'parse_errors': wheel_index.get('_parse_errors', []),
        },
    }

    log_event(
        report,
        'dataset_artifacts_checked',
        status=out['status'],
        missing_packages=len(missing_required_packages),
        incompatible_packages=len(incompatible_required_packages),
        native_found=bool(native),
    )
    return out


def check_native_load(lib_path: Optional[str], report: Dict[str, Any]) -> Dict[str, Any]:
    if not lib_path:
        return {'status': 'missing', 'reason': 'native_library_missing'}

    try:
        ctypes.CDLL(lib_path)
        log_event(report, 'native_load_ok', lib=lib_path)
        return {'status': 'ok', 'lib': lib_path}
    except Exception as exc:
        err = str(exc)
        m = re.search(r'GLIBC_(\d+\.\d+)', err)
        diag = {
            'status': 'abi_incompatible' if m else 'fail',
            'lib': lib_path,
            'error': err,
            'required_glibc': m.group(1) if m else None,
            'runtime_glibc': platform.libc_ver()[1] or 'unknown',
            'blocking': bool(ENFORCE_SO_LOAD),
        }
        log_event(report, 'native_load_check', **diag)
        if ENFORCE_SO_LOAD:
            raise RuntimeError(f'native_so_load_enforced_failed: {err}')
        return diag


def inspect_so_symbols(lib_path: Optional[str], report: Dict[str, Any]) -> Dict[str, Any]:
    if not lib_path:
        return {'status': 'missing', 'reason': 'no_lib_path'}

    nm = os.environ.get('NM_BIN', 'nm')
    cmd = [nm, '-D', lib_path]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            return {'status': 'fail', 'returncode': proc.returncode, 'stderr_head': (proc.stderr or '')[:1200]}

        symbols = []
        undefined = []
        for line in proc.stdout.splitlines():
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            # nm -D formats: "address type symbol" or "type symbol"
            sym_type = parts[-2] if len(parts) >= 3 else parts[0]
            sym_name = parts[-1]
            if sym_type.lower() == 't':
                symbols.append(sym_name)
            if sym_type == 'U':
                undefined.append(sym_name)

        lum_related = [s for s in symbols if s.startswith(('lum_', 'vorax_', 'log_', 'forensic_'))]
        out = {
            'status': 'ok',
            'symbol_count_total': len(symbols),
            'symbol_count_lum_related': len(lum_related),
            'undefined_symbol_count': len(undefined),
            'undefined_symbol_head': undefined[:120],
            'lum_related_head': lum_related[:200],
        }
        log_event(report, 'so_symbols_scanned', total=out['symbol_count_total'], lum_related=out['symbol_count_lum_related'], undefined=out['undefined_symbol_count'])
        return out
    except Exception as exc:
        return {'status': 'fail', 'error': str(exc), 'cmd': cmd}


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


def _assert_lzw_backend() -> Dict[str, Any]:
    import imagecodecs  # type: ignore

    lzw_encode = hasattr(imagecodecs, 'lzw_encode')
    lzw_decode = hasattr(imagecodecs, 'lzw_decode')
    version = getattr(imagecodecs, '__version__', 'unknown')

    if not (lzw_encode and lzw_decode):
        raise RuntimeError(f'imagecodecs_lzw_unavailable: version={version}, lzw_encode={lzw_encode}, lzw_decode={lzw_decode}')

    return {
        'imagecodecs_version': version,
        'lzw_encode': lzw_encode,
        'lzw_decode': lzw_decode,
    }


def tiff_lum_roundtrip_test(report: Dict[str, Any]) -> Dict[str, Any]:
    import numpy as np
    import tifffile

    codec_diag = _assert_lzw_backend()

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        tif_path = td / 'synthetic_teacher.tif'
        lum_path = td / 'synthetic_teacher.lum'

        rng = np.random.default_rng(42)
        vol = (rng.random((8, 32, 32)) > 0.87).astype(np.uint8)

        try:
            tifffile.imwrite(tif_path, vol, compression='LZW')
        except Exception as exc:
            raise RuntimeError(f'tiff_lzw_write_failed: {exc}') from exc

        arr3d = normalize_volume(tifffile.imread(tif_path))
        blob = encode_lum_v1(arr3d)
        lum_path.write_bytes(blob)
        restored = decode_lum_v1(blob)

        if not bool((arr3d == restored).all()):
            raise RuntimeError('lum_roundtrip_mismatch')

        return {
            'status': 'ok',
            'shape': [int(x) for x in restored.shape],
            'roundtrip_ok': True,
            'write_compression_used': 'LZW',
            'codec_diagnostics': codec_diag,
        }


def main() -> None:
    t0 = now_ns()
    report: Dict[str, Any] = {
        'report_name': 'lumvorax_dependency_360_kaggle_single_cell_v13_strict',
        'timestamp_ns': now_ns(),
        'runtime': {
            'python': sys.version,
            'platform': platform.platform(),
            'cwd': str(Path.cwd()),
            'is_kaggle': IS_KAGGLE,
        },
        'policy': {
            'strict_no_fallback': STRICT_NO_FALLBACK,
            'require_dataset': REQUIRE_DATASET,
            'require_so_presence': REQUIRE_SO_PRESENCE,
            'enforce_so_load': ENFORCE_SO_LOAD,
            'skip_roundtrip': SKIP_ROUNDTRIP,
            'source_header_validation_enabled': False,
            'c_syntax_smoke_enabled': False,
        },
        'dataset_expected': {
            'native_lib': EXPECTED_NATIVE_LIB,
            'required_packages': REQUIRED_PACKAGES,
        },
        'events': [],
        'warnings': [],
    }

    try:
        dataset_root = detect_dataset_root(report)
        report['dataset_root'] = str(dataset_root) if dataset_root else None

        if REQUIRE_DATASET and dataset_root is None:
            raise RuntimeError('dataset_root_required_but_not_found')

        report['install_report'] = [install_offline_if_missing(p, report, dataset_root) for p in ('numpy', 'tifffile', 'imagecodecs')]
        report['imports'] = {p: pkg_available(p) for p in ('numpy', 'tifffile', 'imagecodecs', 'pyarrow')}

        report['dataset_artifacts'] = inspect_dataset_artifacts(dataset_root, report)
        if report['dataset_artifacts'].get('status') != 'ok':
            if REQUIRE_DATASET:
                raise RuntimeError('dataset_artifacts_incomplete')
            report.setdefault('notes', []).append({'type': 'dataset', 'detail': report['dataset_artifacts']})
            report['so_symbol_inventory'] = {'status': 'skipped', 'reason': 'dataset_artifacts_not_ok'}
            report['so_load_check'] = {'status': 'skipped', 'reason': 'dataset_artifacts_not_ok'}
        else:
            native_lib = report['dataset_artifacts'].get('native_lib')
            report['so_symbol_inventory'] = inspect_so_symbols(native_lib, report)
            report['so_load_check'] = check_native_load(native_lib, report)

            if report['so_load_check'].get('status') != 'ok':
                raise RuntimeError(f"native_so_load_failed: {report['so_load_check']}")

        if SKIP_ROUNDTRIP:
            report['tiff_lum_roundtrip_test'] = {'status': 'skipped', 'reason': 'LUMVORAX_SKIP_ROUNDTRIP=1'}
        else:
            report['tiff_lum_roundtrip_test'] = tiff_lum_roundtrip_test(report)

        if report['warnings']:
            raise RuntimeError(f'unexpected_warnings_present: {report["warnings"]}')

        report['status'] = 'ok'
    except Exception as exc:
        report['status'] = 'fail'
        report['error_type'] = type(exc).__name__
        report['error'] = str(exc)
        log_event(report, 'fatal_error', error_type=type(exc).__name__, error=str(exc))

    report['elapsed_ns'] = now_ns() - t0
    report['elapsed_s'] = report['elapsed_ns'] / 1_000_000_000

    out = Path('/kaggle/working/lumvorax_360_validation_report_v13_strict.json')
    if not out.parent.exists():
        out = Path('lumvorax_360_validation_report_v13_strict.json')
    out.write_text(json.dumps(report, indent=2), encoding='utf-8')

    print(json.dumps({
        'status': report.get('status'),
        'error_type': report.get('error_type'),
        'error': report.get('error'),
        'report': str(out),
        'dataset_root': report.get('dataset_root'),
        'native_lib': report.get('dataset_artifacts', {}).get('native_lib') if isinstance(report.get('dataset_artifacts'), dict) else None,
        'missing_required_packages': report.get('dataset_artifacts', {}).get('missing_required_packages') if isinstance(report.get('dataset_artifacts'), dict) else None,
        'incompatible_required_packages': report.get('dataset_artifacts', {}).get('incompatible_required_packages') if isinstance(report.get('dataset_artifacts'), dict) else None,
        'so_load_status': report.get('so_load_check', {}).get('status') if isinstance(report.get('so_load_check'), dict) else None,
        'so_symbol_count': report.get('so_symbol_inventory', {}).get('symbol_count_lum_related') if isinstance(report.get('so_symbol_inventory'), dict) else None,
        'roundtrip_status': report.get('tiff_lum_roundtrip_test', {}).get('status') if isinstance(report.get('tiff_lum_roundtrip_test'), dict) else None,
        'warnings_count': len(report.get('warnings', [])),
        'events_count': len(report.get('events', [])),
        'elapsed_ns': report.get('elapsed_ns'),
    }, indent=2))

    if report.get('status') == 'fail':
        raise SystemExit(2)


if __name__ == '__main__':
    main()
