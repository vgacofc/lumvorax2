#!/usr/bin/env python3
import csv
import json
import statistics
import sys
from pathlib import Path


def read_csv(path: Path):
    with path.open(newline='') as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, headers, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w', newline='') as f:
        w = csv.writer(f)
        w.writerow(headers)
        w.writerows(rows)


def to_float(v, d=0.0):
    try:
        return float(v)
    except Exception:
        return d


def main():
    if len(sys.argv) != 2:
        print('Usage: post_run_low_level_telemetry.py <run_dir>', file=sys.stderr)
        return 2

    run_dir = Path(sys.argv[1]).resolve()
    logs_dir = run_dir / 'logs'
    tests_dir = run_dir / 'tests'

    baseline = read_csv(logs_dir / 'baseline_reanalysis_metrics.csv')
    metadata = json.loads((logs_dir / 'model_metadata.json').read_text())
    per_problem = {p['problem']: p for p in metadata.get('per_problem', [])}

    grouped = {}
    for r in baseline:
        p = r.get('problem', 'unknown')
        grouped.setdefault(p, []).append(r)

    rows = []
    runtime_ns_by_problem = {}
    for problem, recs in grouped.items():
        recs = sorted(recs, key=lambda x: to_float(x.get('step')))
        runtime_ns = to_float(recs[-1].get('elapsed_ns'))
        runtime_ns_by_problem[problem] = runtime_ns

    total_runtime_ns = sum(runtime_ns_by_problem.values()) if runtime_ns_by_problem else 0.0

    for problem, recs in grouped.items():
        recs = sorted(recs, key=lambda x: to_float(x.get('step')))
        runtime_ns = runtime_ns_by_problem.get(problem, 0.0)
        runtime_s = runtime_ns / 1e9 if runtime_ns > 0 else 0.0
        max_step = to_float(recs[-1].get('step'))
        steps_count = int(max_step / 100.0) + 1 if max_step >= 0 else len(recs)
        calc_per_sec = (max_step / runtime_s) if runtime_s > 0 else 0.0
        latency_ns_per_step = runtime_ns / max(1, steps_count)
        util_pct = (100.0 * runtime_ns / total_runtime_ns) if total_runtime_ns > 0 else 0.0

        cpus = [to_float(r.get('cpu_percent')) for r in recs]
        mems = [to_float(r.get('mem_percent')) for r in recs]
        avg_cpu = statistics.fmean(cpus) if cpus else 0.0
        avg_mem = statistics.fmean(mems) if mems else 0.0

        md = per_problem.get(problem, {})
        lattice_sites = int(md.get('lattice_sites', 0))
        rows.append([
            problem,
            lattice_sites,
            f'{runtime_ns:.0f}',
            f'{util_pct:.2f}',
            f'{avg_cpu:.2f}',
            f'{avg_mem:.2f}',
            f'{calc_per_sec:.2f}',
            f'{latency_ns_per_step:.2f}',
            md.get('boundary_conditions', ''),
            md.get('method', ''),
        ])

    out_csv = tests_dir / 'integration_low_level_runtime_metrics.csv'
    write_csv(
        out_csv,
        [
            'problem',
            'qubits_simulated_effective',
            'runtime_ns',
            'module_runtime_share_pct',
            'avg_cpu_percent',
            'avg_mem_percent',
            'calc_per_second',
            'latency_ns_per_step',
            'boundary_conditions',
            'solver_mode',
        ],
        sorted(rows, key=lambda r: r[0]),
    )

    total_qubits = sum(int(r[1]) for r in rows)
    avg_cpu_all = statistics.fmean([float(r[4]) for r in rows]) if rows else 0.0
    avg_mem_all = statistics.fmean([float(r[5]) for r in rows]) if rows else 0.0

    pie_lines = '\n'.join([f'    "{r[0]} ({r[3]}%)" : {float(r[3]):.2f}' for r in sorted(rows, key=lambda r: r[0])])
    md = (
        '# Low-level Telemetry (module/hardware/interoperability)\n\n'
        f'- total_runtime_ns: `{total_runtime_ns:.0f}`\n'
        f'- total_qubits_simulated_effective: `{total_qubits}`\n'
        f'- avg_cpu_percent_global: `{avg_cpu_all:.2f}`\n'
        f'- avg_mem_percent_global: `{avg_mem_all:.2f}`\n\n'
        '## Architecture (mode FULL V4 NEXT)\n'
        '```mermaid\n'
        'flowchart LR\n'
        '  A[run_research_cycle.sh] --> B[metadata_capture]\n'
        '  B --> C[cycle_guard]\n'
        '  C --> D[physics_readiness_pack]\n'
        '  D --> E[v4next_integration_status]\n'
        '  E --> F[v4next_rollout_controller full]\n'
        '  F --> G[v4next_rollout_progress]\n'
        '  G --> H[v4next_realtime_evolution]\n'
        '  H --> I[chatgpt_critical_tests]\n'
        '  I --> J[authenticity_audit]\n'
        '  J --> K[checksums/audit]\n'
        '```\n\n'
        '## Module runtime share\n'
        '```mermaid\n'
        'pie title Module runtime share (%)\n'
        f'{pie_lines}\n'
        '```\n'
    )
    out_md = tests_dir / 'integration_low_level_runtime_metrics.md'
    out_md.write_text(md)

    print(f'[low-level-telemetry] generated: {out_csv}')
    print(f'[low-level-telemetry] generated: {out_md}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
