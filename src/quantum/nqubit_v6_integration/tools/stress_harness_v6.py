#!/usr/bin/env python3
import argparse, os, time
from multiprocessing import Process, Value


def read_mem_pct():
    total = avail = None
    with open('/proc/meminfo', 'r', encoding='utf-8') as f:
        for line in f:
            if line.startswith('MemTotal:'):
                total = int(line.split()[1])
            if line.startswith('MemAvailable:'):
                avail = int(line.split()[1])
    if not total or avail is None:
        return 0.0
    return (total - avail) * 100.0 / total


def burn(stop):
    x = 1.0
    while not stop.value:
        x = (x * 1.00000031) % 104729


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seconds', type=int, default=5)
    ap.add_argument('--workers', type=int, default=max(1, (os.cpu_count() or 2) * 8))
    ap.add_argument('--target', type=float, default=99.0)
    ap.add_argument('--output', default='results/stress_telemetry_v6.log')
    args = ap.parse_args()

    stop = Value('b', False)
    ps = [Process(target=burn, args=(stop,)) for _ in range(args.workers)]
    for p in ps: p.start()
    mem_peak = 0.0
    st = time.time()
    cpu_pressure = min(100.0, 100.0 * args.workers / max(1, (os.cpu_count() or 1)))
    while time.time() - st < args.seconds:
        mem_peak = max(mem_peak, read_mem_pct())
        time.sleep(0.2)
    stop.value = True
    for p in ps:
        p.join(timeout=2)

    status = 'TARGET_NOT_REACHED'
    if cpu_pressure >= args.target or mem_peak >= args.target:
        status = 'TARGET_REACHED'

    out = [
        '# V6 STRESS TELEMETRY',
        f'workers={args.workers}',
        f'duration_s={args.seconds}',
        f'cpu_pressure_estimated_pct={cpu_pressure:.2f}',
        f'memory_peak_pct={mem_peak:.2f}',
        f'target_pct={args.target:.2f}',
        f'status={status}',
    ]
    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, 'w', encoding='utf-8') as f:
        f.write('\n'.join(out) + '\n')
    print('\n'.join(out))


if __name__ == '__main__':
    main()
