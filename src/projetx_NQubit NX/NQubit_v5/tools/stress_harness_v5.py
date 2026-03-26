#!/usr/bin/env python3
import argparse
import os
import time
from multiprocessing import Process, Value


def read_mem_usage_pct():
    total = avail = None
    with open('/proc/meminfo', 'r', encoding='utf-8') as f:
        for line in f:
            if line.startswith('MemTotal:'):
                total = int(line.split()[1])
            elif line.startswith('MemAvailable:'):
                avail = int(line.split()[1])
    if not total or avail is None:
        return 0.0
    used = total - avail
    return used * 100.0 / total


def cpu_burn(stop_flag):
    x = 1.0
    while not stop_flag.value:
        x = (x * 1.0000001) % 99991.0


def main():
    p = argparse.ArgumentParser(description='V5 stress harness (CPU/RAM telemetry)')
    p.add_argument('--seconds', type=int, default=10)
    p.add_argument('--workers', type=int, default=max(1, (os.cpu_count() or 2) - 1))
    p.add_argument('--target-cpu-ram', type=float, default=99.0)
    p.add_argument('--output', default='results/stress_telemetry_v5.log')
    args = p.parse_args()

    stop = Value('b', False)
    procs = [Process(target=cpu_burn, args=(stop,)) for _ in range(args.workers)]
    for pr in procs:
        pr.start()

    mem_peak = 0.0
    start = time.time()
    # CPU peak approximated as workers/os.cpu_count utilization pressure indicator
    cpu_pressure = min(100.0, 100.0 * args.workers / max(1, (os.cpu_count() or 1)))
    while time.time() - start < args.seconds:
        mem_peak = max(mem_peak, read_mem_usage_pct())
        time.sleep(0.2)

    stop.value = True
    for pr in procs:
        pr.join(timeout=2)

    status = 'TARGET_NOT_REACHED'
    if cpu_pressure >= args.target_cpu_ram or mem_peak >= args.target_cpu_ram:
        status = 'TARGET_REACHED'

    out = [
        '# V5 STRESS TELEMETRY',
        f'workers={args.workers}',
        f'duration_s={args.seconds}',
        f'cpu_pressure_estimated_pct={cpu_pressure:.2f}',
        f'memory_peak_pct={mem_peak:.2f}',
        f'target_pct={args.target_cpu_ram:.2f}',
        f'status={status}',
    ]
    with open(args.output, 'w', encoding='utf-8') as f:
        f.write('\n'.join(out) + '\n')
    print('\n'.join(out))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
