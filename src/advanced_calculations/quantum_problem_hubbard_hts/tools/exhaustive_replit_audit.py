#!/usr/bin/env python3
import argparse
import csv
import pathlib
from collections import Counter


def read_csv(path: pathlib.Path):
    with path.open(newline='') as f:
        return list(csv.DictReader(f))


def discover_runs(results_dir: pathlib.Path):
    return sorted([p for p in results_dir.iterdir() if p.is_dir() and p.name.startswith('research_')])


def run_health(run_dir: pathlib.Path):
    required = [
        'logs/baseline_reanalysis_metrics.csv',
        'tests/new_tests_results.csv',
        'tests/expert_questions_matrix.csv',
        'logs/checksums.sha256',
    ]
    missing = [x for x in required if not (run_dir / x).exists()]
    baseline_rows = 0
    new_tests = []
    if (run_dir / 'logs/baseline_reanalysis_metrics.csv').exists():
        baseline_rows = sum(1 for _ in (run_dir / 'logs/baseline_reanalysis_metrics.csv').open()) - 1
    if (run_dir / 'tests/new_tests_results.csv').exists():
        new_tests = read_csv(run_dir / 'tests/new_tests_results.csv')
    statuses = Counter(x['status'] for x in new_tests)
    return {
        'run_id': run_dir.name,
        'missing_count': len(missing),
        'missing_files': ';'.join(missing),
        'baseline_rows': baseline_rows,
        'new_tests_rows': len(new_tests),
        'new_tests_pass': statuses.get('PASS', 0),
        'new_tests_observed': statuses.get('OBSERVED', 0),
    }


def to_float_or_none(value):
    try:
        if value is None:
            return None
        return float(value)
    except Exception:
        return None


def compare_runs(prev_rows, current_rows):
    key = lambda r: (r.get('problem'), r.get('step'))
    prev_map = {key(r): r for r in prev_rows}
    common = [(r, prev_map[key(r)]) for r in current_rows if key(r) in prev_map]
    out = []
    for field in ['energy', 'pairing', 'sign_ratio', 'elapsed_ns', 'cpu_percent', 'mem_percent']:
        diffs = []
        for a, b in common:
            fa = to_float_or_none(a.get(field))
            fb = to_float_or_none(b.get(field))
            if fa is None or fb is None:
                continue
            diffs.append(abs(fa - fb))
        out.append({
            'metric': field,
            'common_points': len(diffs),
            'max_abs_diff': max(diffs) if diffs else 0.0,
            'mean_abs_diff': (sum(diffs) / len(diffs)) if diffs else 0.0,
        })
    return out


def write_csv(path: pathlib.Path, rows, fieldnames):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        w.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description='Exhaustive audit for Replit research runs')
    parser.add_argument('--results-dir', default='src/advanced_calculations/quantum_problem_hubbard_hts/results')
    parser.add_argument('--out-csv', default='src/advanced_calculations/quantum_problem_hubbard_hts/AUDIT_EXHAUSTIF_REPLIT_RUNS.csv')
    parser.add_argument('--out-drift-csv', default='src/advanced_calculations/quantum_problem_hubbard_hts/AUDIT_EXHAUSTIF_REPLIT_DRIFT.csv')
    args = parser.parse_args()

    results_dir = pathlib.Path(args.results_dir)
    runs = discover_runs(results_dir)
    if len(runs) < 2:
        raise SystemExit('Need at least 2 research runs for exhaustive audit')

    health_rows = [run_health(r) for r in runs]
    write_csv(pathlib.Path(args.out_csv), health_rows, list(health_rows[0].keys()))

    latest = runs[-1]
    previous = runs[-2]
    latest_rows = read_csv(latest / 'logs/baseline_reanalysis_metrics.csv')
    previous_rows = read_csv(previous / 'logs/baseline_reanalysis_metrics.csv')
    drift_rows = compare_runs(previous_rows, latest_rows)
    for row in drift_rows:
        row['reference_run'] = previous.name
        row['current_run'] = latest.name
    write_csv(
        pathlib.Path(args.out_drift_csv),
        drift_rows,
        ['metric', 'common_points', 'max_abs_diff', 'mean_abs_diff', 'reference_run', 'current_run'],
    )

    print(f'runs_audited={len(runs)} latest={latest.name} previous={previous.name}')
    print(f'health_csv={args.out_csv}')
    print(f'drift_csv={args.out_drift_csv}')


if __name__ == '__main__':
    main()
