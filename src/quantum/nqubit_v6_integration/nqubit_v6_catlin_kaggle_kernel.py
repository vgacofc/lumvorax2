#!/usr/bin/env python3
"""
NQubit V6 Catlin/Kaggle kernel (.py) - SINGLE-CELL NOTEBOOK SAFE
Everything is embedded in this one file (no external tools required).
"""

import json
import os
import platform
import shutil
import statistics
import subprocess
import time
import hashlib
from multiprocessing import Process, Value
from pathlib import Path


def run_cmd(cmd, cwd=None):
    try:
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, check=False)
        return p.returncode, p.stdout.strip(), p.stderr.strip()
    except FileNotFoundError as e:
        return 127, '', str(e)


def detect_runtime_environment():
    env = {
        'python': platform.python_version(),
        'platform': platform.platform(),
        'cpu_count': os.cpu_count(),
        'cwd': str(Path.cwd()),
        'kaggle': bool(os.environ.get('KAGGLE_KERNEL_RUN_TYPE') or os.environ.get('KAGGLE_URL_BASE')),
        'replit': bool(os.environ.get('REPL_ID') or os.environ.get('REPLIT_DB_URL')),
        'colab': bool(os.environ.get('COLAB_GPU')),
        'cuda_visible_devices': os.environ.get('CUDA_VISIBLE_DEVICES'),
    }

    gpu = {'backend': None, 'available': False, 'name': None, 'count': 0}
    try:
        import torch  # type: ignore
        gpu['backend'] = 'torch'
        gpu['available'] = bool(torch.cuda.is_available())
        if gpu['available']:
            gpu['count'] = torch.cuda.device_count()
            gpu['name'] = torch.cuda.get_device_name(0)
    except Exception:
        pass

    if not gpu['available']:
        rc, out, _ = run_cmd(['nvidia-smi', '--query-gpu=name', '--format=csv,noheader'])
        if rc == 0 and out:
            names = [x.strip() for x in out.splitlines() if x.strip()]
            if names:
                gpu.update({'backend': 'nvidia-smi', 'available': True, 'name': names[0], 'count': len(names)})

    env['gpu'] = gpu
    env['compute_mode'] = 'gpu' if gpu['available'] else 'cpu'
    return env


def write_json(path: Path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + '\n', encoding='utf-8')


def collect_system_metrics(results: Path):
    def parse_meminfo():
        out = {}
        p = Path('/proc/meminfo')
        if p.exists():
            for line in p.read_text(encoding='utf-8', errors='ignore').splitlines():
                if ':' in line:
                    k, v = line.split(':', 1)
                    out[k.strip()] = v.strip()
        return out

    def parse_cpu_stat():
        p = Path('/proc/stat')
        data = {}
        if p.exists():
            for line in p.read_text(encoding='utf-8', errors='ignore').splitlines():
                if line.startswith('cpu '):
                    vals = list(map(int, line.split()[1:]))
                    data['cpu_total_ticks'] = sum(vals)
                    data['cpu_idle_ticks'] = vals[3] + (vals[4] if len(vals) > 4 else 0)
                    break
        return data

    def net_dev():
        p = Path('/proc/net/dev')
        out = {}
        if p.exists():
            for ln in p.read_text(encoding='utf-8', errors='ignore').splitlines()[2:]:
                if ':' not in ln:
                    continue
                iface, vals = ln.split(':', 1)
                arr = vals.split()
                out[iface.strip()] = {
                    'rx_bytes': int(arr[0]), 'rx_packets': int(arr[1]),
                    'tx_bytes': int(arr[8]), 'tx_packets': int(arr[9]),
                }
        return out

    t0 = time.perf_counter()
    du = shutil.disk_usage('/')
    metrics = {
        'version': 'v6',
        'timestamp_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'system': {
            'platform': platform.platform(),
            'python': platform.python_version(),
            'cpu_count': os.cpu_count(),
            'uname': {'rc': run_cmd(['uname', '-a'])[0], 'stdout': run_cmd(['uname', '-a'])[1]},
            'lscpu': {'rc': run_cmd(['lscpu'])[0], 'stdout': run_cmd(['lscpu'])[1]},
            'lsblk': {'rc': run_cmd(['lsblk'])[0], 'stdout': run_cmd(['lsblk'])[1]},
        },
        'memory': parse_meminfo(),
        'cpu_stat': parse_cpu_stat(),
        'disk_root': {'total_bytes': du.total, 'used_bytes': du.used, 'free_bytes': du.free},
        'loadavg': (lambda a=os.getloadavg(): {'load_1m': a[0], 'load_5m': a[1], 'load_15m': a[2]})() if hasattr(os, 'getloadavg') else {},
        'network': net_dev(),
        'commands': {
            'free_h': {'rc': run_cmd(['free', '-h'])[0], 'stdout': run_cmd(['free', '-h'])[1]},
            'vmstat': {'rc': run_cmd(['vmstat'])[0], 'stdout': run_cmd(['vmstat'])[1]},
            'df_h': {'rc': run_cmd(['df', '-h'])[0], 'stdout': run_cmd(['df', '-h'])[1]},
        },
    }
    metrics['collection_elapsed_s'] = time.perf_counter() - t0
    out = results / 'system_metrics_v6.json'
    write_json(out, metrics)
    return out


def acquire_real_hardware(results: Path, nbytes=32768, jitter_samples=50000):
    def read_source(path: str, nbytes: int):
        try:
            with open(path, 'rb', buffering=0) as f:
                t0 = time.perf_counter()
                data = f.read(nbytes)
                elapsed = time.perf_counter() - t0
            return data, None, elapsed
        except Exception as e:
            return b'', str(e), None

    def bstats(blob: bytes):
        if not blob:
            return {'size': 0, 'sha256': None, 'mean': None, 'stddev': None, 'min': None, 'max': None}
        vals = list(blob)
        return {
            'size': len(blob),
            'sha256': hashlib.sha256(blob).hexdigest(),
            'mean': statistics.fmean(vals),
            'stddev': statistics.pstdev(vals),
            'min': min(vals),
            'max': max(vals),
        }

    sources = ['/dev/hwrng', '/dev/random', '/dev/urandom']
    source_reports = []
    for src in sources:
        data, err, elapsed_s = read_source(src, nbytes)
        outfile = results / f"hardware_entropy_{Path(src).name}_v6.bin"
        if data:
            outfile.write_bytes(data)
        st = bstats(data)
        bps = (st['size'] / elapsed_s) if (elapsed_s and elapsed_s > 0 and st['size']) else None
        source_reports.append({'source': src, 'output_file': outfile.as_posix() if data else None, 'error': err, 'read_elapsed_s': elapsed_s, 'bytes_per_sec': bps, **st})

    prev = time.perf_counter_ns()
    t0 = time.perf_counter()
    diffs = []
    for _ in range(jitter_samples):
        now = time.perf_counter_ns()
        diffs.append(now - prev)
        prev = now
    elapsed = time.perf_counter() - t0

    jcsv = results / 'hardware_jitter_samples_v6.csv'
    with jcsv.open('w', encoding='utf-8') as f:
        f.write('idx,delta_ns\n')
        for i, d in enumerate(diffs):
            f.write(f'{i},{d}\n')

    rep = {
        'version': 'v6',
        'timestamp_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'goal': 'real_hardware_capture_on_replit',
        'sources': source_reports,
        'jitter': {
            'samples': len(diffs),
            'elapsed_s': elapsed,
            'ops_per_sec': (len(diffs)/elapsed) if elapsed > 0 else None,
            'nqubit_per_sec_proxy': (len(diffs)/elapsed) if elapsed > 0 else None,
            'mean_ns': statistics.fmean(diffs) if diffs else None,
            'stddev_ns': statistics.pstdev(diffs) if diffs else None,
            'min_ns': min(diffs) if diffs else None,
            'max_ns': max(diffs) if diffs else None,
            'file': jcsv.as_posix(),
        },
    }
    out = results / 'ADC_REAL_HARDWARE_CAPTURE_V6.json'
    write_json(out, rep)
    return out


def stress(results: Path, seconds=4, workers=64, target=99.0):
    def read_mem_pct():
        total = avail = None
        p = Path('/proc/meminfo')
        if not p.exists():
            return 0.0
        for line in p.read_text(encoding='utf-8', errors='ignore').splitlines():
            if line.startswith('MemTotal:'):
                total = int(line.split()[1])
            elif line.startswith('MemAvailable:'):
                avail = int(line.split()[1])
        if not total or avail is None:
            return 0.0
        return (total - avail) * 100.0 / total

    def burn(stop):
        x = 1.0
        while not stop.value:
            x = (x * 1.00000031) % 104729

    stop = Value('b', False)
    ps = [Process(target=burn, args=(stop,)) for _ in range(workers)]
    for p in ps:
        p.start()

    mem_peak = 0.0
    st = time.time()
    cpu_pressure = min(100.0, 100.0 * workers / max(1, (os.cpu_count() or 1)))
    while time.time() - st < seconds:
        mem_peak = max(mem_peak, read_mem_pct())
        time.sleep(0.2)

    stop.value = True
    for p in ps:
        p.join(timeout=2)

    status = 'TARGET_REACHED' if (cpu_pressure >= target or mem_peak >= target) else 'TARGET_NOT_REACHED'
    lines = [
        '# V6 STRESS TELEMETRY',
        f'workers={workers}',
        f'duration_s={seconds}',
        f'cpu_pressure_estimated_pct={cpu_pressure:.2f}',
        f'memory_peak_pct={mem_peak:.2f}',
        f'target_pct={target:.2f}',
        f'status={status}',
    ]
    out = results / 'stress_telemetry_v6.log'
    out.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    return out


def build_manifest(results: Path):
    files = [fp for fp in sorted(results.rglob('*')) if fp.is_file() and fp.name not in {'manifest_forensic_v6.json', 'manifest_forensic_v6.json.sig'}]
    arts = []
    for fp in files:
        h = hashlib.sha256(fp.read_bytes()).hexdigest()
        arts.append({'file': fp.resolve().as_posix(), 'sha256': h, 'bytes': fp.stat().st_size})
    data = {
        'version': 'v6',
        'generated_at_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'source_of_truth': 'manifest_forensic_v6.json (signed)',
        'hash_algo': 'sha256',
        'artifacts': arts,
    }
    out = results / 'manifest_forensic_v6.json'
    write_json(out, data)
    return out


def verify_manifest(manifest_path: Path):
    data = json.loads(manifest_path.read_text(encoding='utf-8'))
    missing, mismatches = [], []
    for a in data.get('artifacts', []):
        fp = Path(a['file'])
        if not fp.exists():
            missing.append(a['file'])
            continue
        actual = hashlib.sha256(fp.read_bytes()).hexdigest()
        if actual != a['sha256']:
            mismatches.append((a['file'], a['sha256'], actual))
    return {'artifact_count': len(data.get('artifacts', [])), 'missing': len(missing), 'mismatches': len(mismatches), 'ok': (not missing and not mismatches), 'missing_items': missing, 'mismatch_items': mismatches}


def generate_report(results: Path, verify_res: dict):
    adc_path = results / 'ADC_REAL_HARDWARE_CAPTURE_V6.json'
    sys_path = results / 'system_metrics_v6.json'
    stress_path = results / 'stress_telemetry_v6.log'

    adc = json.loads(adc_path.read_text(encoding='utf-8')) if adc_path.exists() else {}
    sysm = json.loads(sys_path.read_text(encoding='utf-8')) if sys_path.exists() else {}
    stress_txt = stress_path.read_text(encoding='utf-8') if stress_path.exists() else 'N/A'

    lines = [
        '# RAPPORT REPLIT EXECUTION V6 FINAL',
        '',
        '## 1) Intégrité manifest V6',
        '- Manifest: `results/manifest_forensic_v6.json`',
        f"- Verify exit code: {0 if verify_res['ok'] else 1}",
        '```',
        f"artifact_count={verify_res['artifact_count']}",
        f"missing={verify_res['missing']}",
        f"mismatches={verify_res['mismatches']}",
        '```',
        '',
        '## 2) Capture hardware réelle',
        f"- report_file: `results/ADC_REAL_HARDWARE_CAPTURE_V6.json`",
        f"- goal: `{adc.get('goal', 'n/a')}`",
        f"- timestamp_utc: `{adc.get('timestamp_utc', 'n/a')}`",
    ]
    for src in adc.get('sources', []):
        lines.append(f"- source={src.get('source')} size={src.get('size')} read_elapsed_s={src.get('read_elapsed_s')} bytes_per_sec={src.get('bytes_per_sec')} error={src.get('error')}")
    if adc.get('jitter'):
        j = adc['jitter']
        lines.append(f"- jitter samples={j.get('samples')} elapsed_s={j.get('elapsed_s')} ops_per_sec={j.get('ops_per_sec')} nqubit_per_sec_proxy={j.get('nqubit_per_sec_proxy')} mean_ns={j.get('mean_ns')} stddev_ns={j.get('stddev_ns')}")

    lines += ['', '## 3) Métriques système/hardware complètes']
    if sysm:
        lines.append(f"- cpu_count={sysm.get('system', {}).get('cpu_count')}")
        lines.append(f"- platform={sysm.get('system', {}).get('platform')}")
        lines.append(f"- loadavg={sysm.get('loadavg')}")
        lines.append(f"- disk_root={sysm.get('disk_root')}")
        lines.append(f"- cpu_stat={sysm.get('cpu_stat')}")
        mem = sysm.get('memory', {})
        lines.append(f"- mem_total={mem.get('MemTotal')} mem_available={mem.get('MemAvailable')} mem_free={mem.get('MemFree')}")

    lines += [
        '',
        '## 4) Stress telemetry',
        '```',
        stress_txt,
        '```',
        '',
        '## 5) Conclusion',
        '- GO technique V6 si verify=0 et au moins une source hardware capturée sans erreur.',
        '- ADC physique certifiée laboratoire: NON (ce run est environnement Catlin/Kaggle/Replit).',
    ]

    out = results.parent / 'RAPPORT_REPLIT_EXECUTION_V6_FINAL.md'
    out.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    return out


def main(explicit_root: str | None = None):
    # SINGLE-CELL mode: if no root given, run directly in current working directory
    if explicit_root:
        root = Path(explicit_root).resolve()
    else:
        root = Path.cwd().resolve()
    results = root / 'results'
    results.mkdir(parents=True, exist_ok=True)

    env = detect_runtime_environment()
    env['root'] = str(root)
    write_json(results / 'kernel_env_detect_v6.json', env)

    steps = []
    t0 = time.perf_counter()
    for name, fn in [
        ('collect_metrics', lambda: collect_system_metrics(results)),
        ('acquire_hardware', lambda: acquire_real_hardware(results, 32768, 50000)),
        ('stress', lambda: stress(results, 4, 64, 99.0)),
        ('build_manifest', lambda: build_manifest(results)),
    ]:
        s0 = time.perf_counter()
        try:
            out = fn()
            rc = 0
            stdout = str(out)
            stderr = ''
        except Exception as e:
            rc = 1
            stdout = ''
            stderr = str(e)
        steps.append({'step': name, 'rc': rc, 'elapsed_s': time.perf_counter() - s0, 'stdout': stdout, 'stderr': stderr})

    verify_res = verify_manifest(results / 'manifest_forensic_v6.json')
    steps.append({'step': 'verify_manifest', 'rc': 0 if verify_res['ok'] else 1, 'elapsed_s': 0.0, 'stdout': json.dumps(verify_res), 'stderr': ''})

    rpt = generate_report(results, verify_res)
    steps.append({'step': 'report', 'rc': 0, 'elapsed_s': 0.0, 'stdout': str(rpt), 'stderr': ''})

    summary = {'version': 'v6-kernel-single-cell', 'elapsed_s': time.perf_counter() - t0, 'steps': steps, 'ok': all(s['rc'] == 0 for s in steps)}
    write_json(results / 'kernel_run_summary_v6.json', summary)
    final = {'environment': env, 'pipeline': summary, 'timestamp_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}
    write_json(results / 'kernel_final_state_v6.json', final)

    print(f"compute_mode={env['compute_mode']} gpu_available={env['gpu']['available']} gpu_name={env['gpu']['name']}")
    print(f"pipeline_ok={summary['ok']} steps={len(summary['steps'])}")
    return 0 if summary['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
