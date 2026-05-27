#!/usr/bin/env python3
import argparse
import hashlib
import json
import statistics
import time
from pathlib import Path


def read_source(path: str, nbytes: int):
    try:
        with open(path, 'rb', buffering=0) as f:
            t0 = time.perf_counter()
            data = f.read(nbytes)
            elapsed = time.perf_counter() - t0
        return data, None, elapsed
    except Exception as e:
        return b'', str(e), None


def byte_stats(blob: bytes):
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


def collect_jitter(samples: int):
    t0 = time.perf_counter()
    prev = time.perf_counter_ns()
    diffs = []
    for _ in range(samples):
        now = time.perf_counter_ns()
        diffs.append(now - prev)
        prev = now
    return diffs, (time.perf_counter() - t0)


def main() -> int:
    ap = argparse.ArgumentParser(description='Acquire real hardware entropy/jitter on Replit-like env')
    ap.add_argument('--output-dir', default='results')
    ap.add_argument('--bytes', type=int, default=32768)
    ap.add_argument('--jitter-samples', type=int, default=50000)
    args = ap.parse_args()

    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)

    sources = ['/dev/hwrng', '/dev/random', '/dev/urandom']
    source_reports = []

    for src in sources:
        data, err, elapsed_s = read_source(src, args.bytes)
        outfile = out / f"hardware_entropy_{Path(src).name}_v6.bin"
        if data:
            outfile.write_bytes(data)
        stats = byte_stats(data)
        bytes_per_sec = (stats['size'] / elapsed_s) if (elapsed_s and elapsed_s > 0 and stats['size']) else None
        source_reports.append({
            'source': src,
            'output_file': outfile.as_posix() if data else None,
            'error': err,
            'read_elapsed_s': elapsed_s,
            'bytes_per_sec': bytes_per_sec,
            **stats,
        })

    diffs, jitter_elapsed_s = collect_jitter(args.jitter_samples)
    jitter_file = out / 'hardware_jitter_samples_v6.csv'
    with jitter_file.open('w', encoding='utf-8') as f:
        f.write('idx,delta_ns\n')
        for i, d in enumerate(diffs):
            f.write(f'{i},{d}\n')
    report = {
        'version': 'v6',
        'timestamp_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'goal': 'real_hardware_capture_on_replit',
        'sources': source_reports,
        'jitter': {
            'samples': len(diffs),
            'elapsed_s': jitter_elapsed_s,
            'ops_per_sec': (len(diffs) / jitter_elapsed_s) if jitter_elapsed_s > 0 else None,
            'nqubit_per_sec_proxy': (len(diffs) / jitter_elapsed_s) if jitter_elapsed_s > 0 else None,
            'mean_ns': statistics.fmean(diffs) if diffs else None,
            'stddev_ns': statistics.pstdev(diffs) if diffs else None,
            'min_ns': min(diffs) if diffs else None,
            'max_ns': max(diffs) if diffs else None,
            'file': jitter_file.as_posix(),
        },
        'notes': [
            'If /dev/hwrng unavailable, /dev/random and /dev/urandom remain captured.',
            'This is hardware-backed entropy acquisition where provided by host kernel/devices.',
        ],
    }

    rpt_file = out / 'ADC_REAL_HARDWARE_CAPTURE_V6.json'
    rpt_file.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'report_written={rpt_file}')
    for s in source_reports:
        print(f"source={s['source']} size={s['size']} error={s['error']} bytes_per_sec={s['bytes_per_sec']}")
    print(f"jitter_ops_per_sec={report['jitter']['ops_per_sec']}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
