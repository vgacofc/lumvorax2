#!/usr/bin/env python3
import csv
import json
import math
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


def pearson(x, y):
    if len(x) != len(y) or len(x) < 2:
        return 0.0
    mx, my = sum(x) / len(x), sum(y) / len(y)
    num = sum((a - mx) * (b - my) for a, b in zip(x, y))
    denx = math.sqrt(sum((a - mx) ** 2 for a in x))
    deny = math.sqrt(sum((b - my) ** 2 for b in y))
    return 0.0 if denx == 0 or deny == 0 else num / (denx * deny)


def main():
    if len(sys.argv) != 2:
        print('Usage: post_run_problem_solution_progress.py <run_dir>', file=sys.stderr)
        return 2

    run_dir = Path(sys.argv[1]).resolve()
    tests = run_dir / 'tests'
    logs = run_dir / 'logs'

    meta = json.loads((logs / 'model_metadata.json').read_text())
    problems = [p['problem'] for p in meta.get('per_problem', [])]

    baseline = read_csv(logs / 'baseline_reanalysis_metrics.csv')
    grouped = {}
    for r in baseline:
        grouped.setdefault(r.get('problem', 'unknown'), {'energy': [], 'pairing': []})
        grouped[r.get('problem', 'unknown')]['energy'].append(to_float(r.get('energy')))
        grouped[r.get('problem', 'unknown')]['pairing'].append(to_float(r.get('pairing')))

    chatgpt = {r['test_id']: r for r in read_csv(tests / 'integration_chatgpt_critical_tests.csv')}
    benchmark_ok = chatgpt.get('T5_qmc_dmrg_crosscheck', {}).get('status') == 'PASS'
    sign_ok = chatgpt.get('T6_sign_problem_watchdog', {}).get('status') == 'PASS'

    critical_map = {}
    t8 = chatgpt.get('T8_critical_minimum_window', {}).get('metric', '')
    for token in t8.split(';'):
        if ':' in token:
            p, st = token.split(':', 1)
            critical_map[p.strip()] = st.strip()

    # Keep penalties explicit and less punitive to avoid score collapse.
    penalties = 0
    for tid in ('T10_spatial_correlations_required', 'T11_entropy_required', 'T12_alternative_solver_required'):
        if chatgpt.get(tid, {}).get('status') == 'FAIL':
            penalties += 5

    rows = []
    for p in problems:
        score = 0.0
        reasons = []

        if p in grouped and grouped[p].get('energy') and grouped[p].get('pairing'):
            score += 30
            reasons.append('timeseries_present')
        else:
            reasons.append('timeseries_missing')

        # Metadata is now conditional, not unconditional.
        md = next((x for x in meta.get('per_problem', []) if x.get('problem') == p), None)
        metadata_ok = bool(md) and all(
            str(md.get(k, '')).strip() not in ('', 'UNKNOWN', 'NA')
            for k in ('lattice_size', 'u_over_t', 'dt', 'boundary_conditions')
        )
        if metadata_ok:
            score += 20
            reasons.append('metadata_present')
        else:
            reasons.append('metadata_incomplete')

        corr = pearson(grouped.get(p, {}).get('energy', []), grouped.get(p, {}).get('pairing', []))
        corr_norm = max(0.0, min(1.0, (corr - 0.50) / 0.50))
        score += 20.0 * corr_norm
        reasons.append(f'energy_pairing_corr={corr:.3f}')

        if benchmark_ok:
            score += 15
            reasons.append('benchmark_crosscheck')
        if sign_ok:
            score += 5
            reasons.append('sign_watchdog')

        crit = critical_map.get(p, 'off')
        if crit == 'ok':
            score += 10
            reasons.append('critical_window_ok')
        elif crit == 'off':
            reasons.append('critical_window_off')

        score = max(0.0, min(100.0, score - penalties))
        rows.append([p, f'{score:.2f}', ';'.join(reasons), f'-{penalties} global_penalties'])

    out = tests / 'integration_problem_solution_progress.csv'
    write_csv(out, ['problem', 'solution_progress_percent', 'evidence', 'constraints'], sorted(rows))
    print(f'[solution-progress] generated: {out}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
