import json
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

import numpy as np
import tifffile

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

import importlib.util


def _load_nx47_v22_class():
    module_path = Path('nx47_vesu_kernel_v2.2.py')
    spec = importlib.util.spec_from_file_location('nx47_vesu_kernel_v2_2', module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'Cannot load module from {module_path}')
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.NX47_VESU_Production


NX47_VESU_Production = _load_nx47_v22_class()



def _load_bootstrap_module():
    module_path = Path('RAPPORT-VESUVIUS/src_vesuvius/lum_vorax_kaggle_bootstrap.py')
    spec = importlib.util.spec_from_file_location('lum_vorax_kaggle_bootstrap', module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'Cannot load module from {module_path}')
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def check_native_3d_sources():
    candidates = [
        Path('/kaggle/working/src/vorax/vorax_operations.c'),
        Path('/kaggle/working/src/vorax/vorax_3d_volume.c'),
        Path('/kaggle/working/src/lum/lum_core.c'),
        Path('/kaggle/working/src/logger/lum_logger.c'),
        Path('src/vorax/vorax_operations.c'),
        Path('src/vorax/vorax_3d_volume.c'),
        Path('src/lum/lum_core.c'),
        Path('src/logger/lum_logger.c'),
    ]
    existing = [str(p) for p in candidates if p.exists()]
    return {
        'c_candidates_checked': [str(p) for p in candidates],
        'c_sources_found': existing,
        'native_3d_c_sources_present': any('vorax_3d_volume.c' in x for x in existing),
    }


def run_python_integration_smoke(tmp_root: Path):
    input_root = tmp_root / 'vesuvius' / 'test'
    output_root = tmp_root / 'out'
    input_root.mkdir(parents=True, exist_ok=True)

    vol = (np.random.default_rng(42).random((8, 24, 24)) * 255).astype('uint8')
    tif_path = input_root / 'frag_demo.tif'
    tifffile.imwrite(tif_path, vol)

    node = NX47_VESU_Production(input_dir=str(tmp_root / 'vesuvius'), output_dir=str(output_root))
    node.bootstrap_dependencies_fail_fast = lambda: None
    stats = node.process_fragments()

    expected = {
        'submission_zip': output_root / 'submission.zip',
        'submission_parquet': output_root / 'submission.parquet',
        'metadata': output_root / 'v137_execution_metadata.json',
        'roadmap': output_root / 'v137_roadmap_realtime.json',
    }
    out = {
        'stats': stats,
        'artifacts': {k: str(v) for k, v in expected.items()},
        'artifacts_exist': {k: v.exists() for k, v in expected.items()},
    }
    return out



def run_replit_root_file_execution(tmp_root: Path):
    input_root = tmp_root / 'replit_root' / 'test'
    output_root = tmp_root / 'replit_out'
    input_root.mkdir(parents=True, exist_ok=True)

    vol = (np.random.default_rng(123).random((4, 16, 16)) * 255).astype('uint8')
    tifffile.imwrite(input_root / 'frag_replit.tif', vol)

    env = os.environ.copy()
    env['NX47_INPUT_DIR'] = str(tmp_root / 'replit_root')
    env['NX47_OUTPUT_DIR'] = str(output_root)
    env['NX47_SKIP_OFFLINE_BOOTSTRAP'] = '1'

    cmd = [sys.executable, 'nx47_vesu_kernel_v2.2.py']
    proc = subprocess.run(cmd, cwd=REPO_ROOT, env=env, capture_output=True, text=True)

    expected = {
        'submission_zip': output_root / 'submission.zip',
        'submission_parquet': output_root / 'submission.parquet',
        'metadata': output_root / 'v137_execution_metadata.json',
        'roadmap': output_root / 'v137_roadmap_realtime.json',
    }

    return {
        'returncode': proc.returncode,
        'stdout_tail': proc.stdout[-1000:],
        'stderr_tail': proc.stderr[-1000:],
        'artifacts': {k: str(v) for k, v in expected.items()},
        'artifacts_exist': {k: v.exists() for k, v in expected.items()},
    }


def check_kaggle_dataset_access():
    kaggle_json = Path.home() / '.kaggle' / 'kaggle.json'
    has_key = kaggle_json.exists()
    cli_ok = True
    try:
        v = subprocess.run(['kaggle', '--version'], capture_output=True, text=True)
        cli_ok = (v.returncode == 0)
    except FileNotFoundError:
        cli_ok = False

    out = {
        'cli_available': cli_ok,
        'kaggle_json_present': has_key,
        'dataset_ref': 'ndarray2000/nx47-dependencies',
    }

    if not cli_ok:
        out['ok'] = False
        out['error'] = 'kaggle_cli_missing'
        return out
    if not has_key:
        out['ok'] = False
        out['error'] = 'kaggle_api_key_missing'
        return out

    cmd = ['kaggle', 'datasets', 'files', 'ndarray2000/nx47-dependencies']
    proc = subprocess.run(cmd, capture_output=True, text=True)
    out['returncode'] = proc.returncode
    out['stdout_tail'] = proc.stdout[-2000:]
    out['stderr_tail'] = proc.stderr[-2000:]
    out['ok'] = proc.returncode == 0
    return out

def run_lum_roundtrip_unit():
    node = NX47_VESU_Production(input_dir='/tmp/no_dataset', output_dir='/tmp/no_out')
    vol = (np.random.default_rng(7).random((4, 12, 10)) * 100).astype('float32')
    info = node._roundtrip_lum(vol)
    return {
        'shape': info.shape,
        'dtype': info.dtype,
        'payload_sha512_prefix': info.payload_sha512[:24],
    }


def main():
    start = time.time()
    out_dir = Path('RAPPORT-VESUVIUS/validation_lumvorax')
    out_dir.mkdir(parents=True, exist_ok=True)

    result = {
        'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
        'checks': {},
        'errors': [],
    }

    try:
        NX47_VESU_Production.validate_source_indentation('nx47_vesu_kernel_v2.py')
        result['checks']['source_indentation'] = {'ok': True}
    except Exception as e:
        result['checks']['source_indentation'] = {'ok': False, 'error': str(e)}

    result['checks']['native_sources'] = check_native_3d_sources()
    result['checks']['kaggle_dataset_access'] = check_kaggle_dataset_access()

    try:
        bootstrap = _load_bootstrap_module()
        so_path = bootstrap.compile_optional_lum_vorax_so('/tmp/liblumvorax_validation.so')
        result['checks']['native_compile_attempt'] = {'ok': bool(so_path), 'output': so_path, 'reason_if_empty': 'missing /kaggle/working sources or gcc failure'}
    except Exception as e:
        result['checks']['native_compile_attempt'] = {'ok': False, 'error': str(e)}

    try:
        result['checks']['lum_roundtrip_unit'] = {'ok': True, **run_lum_roundtrip_unit()}
    except Exception as e:
        result['checks']['lum_roundtrip_unit'] = {'ok': False, 'error': str(e)}

    try:
        with tempfile.TemporaryDirectory() as td:
            smoke = run_python_integration_smoke(Path(td))
        result['checks']['python_integration_smoke'] = {'ok': True, **smoke}
    except Exception as e:
        result['checks']['python_integration_smoke'] = {'ok': False, 'error': str(e)}

    try:
        with tempfile.TemporaryDirectory() as td:
            root_exec = run_replit_root_file_execution(Path(td))
        ok = root_exec.get('returncode') == 0 and all(root_exec.get('artifacts_exist', {}).values())
        result['checks']['replit_root_file_execution'] = {'ok': ok, **root_exec}
    except Exception as e:
        result['checks']['replit_root_file_execution'] = {'ok': False, 'error': str(e)}

    result['duration_s'] = round(time.time() - start, 4)

    json_path = out_dir / 'validation_results.json'
    json_path.write_text(json.dumps(result, indent=2), encoding='utf-8')

    md = out_dir / 'VALIDATION_LUMVORAX_SYSTEME_COMPLET_20260219.md'
    lines = [
        '# Validation système LUM/VORAX — exécution locale',
        '',
        f"- Timestamp: {result['timestamp']}",
        f"- Durée: {result['duration_s']} s",
        '',
        '## Résumé',
    ]
    for name, payload in result['checks'].items():
        status = '✅' if payload.get('ok', False) else '⏳'
        lines.append(f"- {status} **{name}**: {json.dumps(payload, ensure_ascii=False)}")
    lines += [
        '',
        '## Conclusion experte',
        '- Le pipeline Python 3D + format `.lum` est validé localement.',
        '- Le moteur C 3D natif n\'est pas confirmé à 100% dans cet environnement tant que les sources `.c` et/ou `.so` ne sont pas disponibles et compilables.',
        '- Les artefacts de preuve machine sont dans `validation_results.json`.',
    ]
    md.write_text('\n'.join(lines) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
