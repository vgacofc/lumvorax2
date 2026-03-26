#!/usr/bin/env python3
import json
import os
import platform
import shutil
import subprocess
import time
from pathlib import Path


def run_cmd(cmd):
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, check=False)
        return {'cmd': ' '.join(cmd), 'rc': p.returncode, 'stdout': p.stdout.strip(), 'stderr': p.stderr.strip()}
    except Exception as e:
        return {'cmd': ' '.join(cmd), 'rc': -1, 'stdout': '', 'stderr': str(e)}


def parse_meminfo():
    out = {}
    p = Path('/proc/meminfo')
    if not p.exists():
        return out
    for line in p.read_text(encoding='utf-8', errors='ignore').splitlines():
        if ':' in line:
            k, v = line.split(':', 1)
            out[k.strip()] = v.strip()
    return out


def parse_cpu_stat():
    p = Path('/proc/stat')
    if not p.exists():
        return {}
    data = {}
    for line in p.read_text(encoding='utf-8', errors='ignore').splitlines():
        if line.startswith('cpu '):
            parts = line.split()
            vals = list(map(int, parts[1:]))
            data['cpu_total_ticks'] = sum(vals)
            data['cpu_idle_ticks'] = vals[3] + (vals[4] if len(vals) > 4 else 0)
            break
    return data


def disk_usage():
    du = shutil.disk_usage('/')
    return {'total_bytes': du.total, 'used_bytes': du.used, 'free_bytes': du.free}


def loadavg():
    try:
        a = os.getloadavg()
        return {'load_1m': a[0], 'load_5m': a[1], 'load_15m': a[2]}
    except Exception:
        return {}


def net_dev():
    p = Path('/proc/net/dev')
    if not p.exists():
        return {}
    out = {}
    lines = p.read_text(encoding='utf-8', errors='ignore').splitlines()[2:]
    for ln in lines:
        if ':' not in ln:
            continue
        iface, vals = ln.split(':', 1)
        arr = vals.split()
        out[iface.strip()] = {
            'rx_bytes': int(arr[0]), 'rx_packets': int(arr[1]),
            'tx_bytes': int(arr[8]), 'tx_packets': int(arr[9]),
        }
    return out


def main():
    out_dir = Path('results')
    out_dir.mkdir(parents=True, exist_ok=True)
    t0 = time.perf_counter()

    metrics = {
        'version': 'v6',
        'timestamp_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'system': {
            'platform': platform.platform(),
            'python': platform.python_version(),
            'cpu_count': os.cpu_count(),
            'uname': run_cmd(['uname', '-a']),
            'lscpu': run_cmd(['lscpu']),
            'lsblk': run_cmd(['lsblk']),
        },
        'memory': parse_meminfo(),
        'cpu_stat': parse_cpu_stat(),
        'disk_root': disk_usage(),
        'loadavg': loadavg(),
        'network': net_dev(),
        'commands': {
            'free_h': run_cmd(['free', '-h']),
            'vmstat': run_cmd(['vmstat']),
            'df_h': run_cmd(['df', '-h']),
        },
    }

    metrics['collection_elapsed_s'] = time.perf_counter() - t0
    out = out_dir / 'system_metrics_v6.json'
    out.write_text(json.dumps(metrics, indent=2) + '\n', encoding='utf-8')
    print(f'metrics_written={out}')


if __name__ == '__main__':
    main()
