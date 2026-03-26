#!/usr/bin/env python3
# G-C23-06 : Granularité 100% — logging step-by-step de chaque itération
# via LumVoraxBridge. Chaque traitement de module est tracé individuellement.
import csv
import math
import os
import sys
from pathlib import Path

def _lv_log(phase: str, step: str, metric: str, value):
    """G-C23-06 : Log forensique Python step-by-step vers stdout (capturé par lv_wrap)."""
    print(f"[FORENSIC_PY] phase={phase} step={step} metric={metric} value={value}", flush=True)


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


def group_by_problem(rows):
    grouped = {}
    for r in rows:
        p = r.get('problem', 'unknown')
        grouped.setdefault(p, {'step': [], 'energy': [], 'pairing': [], 'sign_ratio': []})
        grouped[p]['step'].append(to_float(r.get('step')))
        grouped[p]['energy'].append(to_float(r.get('energy')))
        grouped[p]['pairing'].append(to_float(r.get('pairing')))
        grouped[p]['sign_ratio'].append(to_float(r.get('sign_ratio')))
    return grouped


def corr_lag(series, lag):
    if len(series) <= lag + 2:
        return 0.0
    x = series[:-lag]
    y = series[lag:]
    mx = sum(x) / len(x)
    my = sum(y) / len(y)
    num = sum((a - mx) * (b - my) for a, b in zip(x, y))
    denx = math.sqrt(sum((a - mx) ** 2 for a in x))
    deny = math.sqrt(sum((b - my) ** 2 for b in y))
    if denx == 0 or deny == 0:
        return 0.0
    return num / (denx * deny)


def shannon_entropy(values, bins=16):
    if not values:
        return 0.0
    lo = min(values)
    hi = max(values)
    if hi <= lo:
        return 0.0
    width = (hi - lo) / bins
    hist = [0] * bins
    for v in values:
        idx = min(bins - 1, int((v - lo) / width))
        hist[idx] += 1
    n = len(values)
    probs = [c / n for c in hist if c > 0]
    return -sum(p * math.log(p, 2) for p in probs)


def main():
    if len(sys.argv) != 2:
        print('Usage: post_run_advanced_observables_pack.py <run_dir>', file=sys.stderr)
        return 2

    run_dir = Path(sys.argv[1]).resolve()
    tests_dir = run_dir / 'tests'
    logs_dir = run_dir / 'logs'

    baseline = read_csv(logs_dir / 'baseline_reanalysis_metrics.csv')
    bench = read_csv(tests_dir / 'benchmark_comparison_qmc_dmrg.csv') if (tests_dir / 'benchmark_comparison_qmc_dmrg.csv').exists() else []
    grouped = group_by_problem(baseline)

    spatial_rows = []
    entropy_rows = []
    for problem, data in sorted(grouped.items()):
        pairing = data['pairing']
        energy = data['energy']
        sign = data['sign_ratio']
        _lv_log("adv_obs", problem, "n_steps", len(pairing))
        for lag in (1, 2, 4, 8, 16):
            c_pair = corr_lag(pairing, lag)
            c_ener = corr_lag(energy, lag)
            c_sign = corr_lag(sign, lag)
            _lv_log("adv_obs_corr", problem, f"lag{lag}_pairing", c_pair)
            _lv_log("adv_obs_corr", problem, f"lag{lag}_energy",  c_ener)
            _lv_log("adv_obs_corr", problem, f"lag{lag}_sign",    c_sign)
            spatial_rows.append([
                problem,
                lag,
                f'{c_pair:.6f}',
                f'{c_ener:.6f}',
                f'{c_sign:.6f}',
                'autocorrelation_over_step_lag'
            ])
        entropy_rows.append([
            problem,
            f'{shannon_entropy(energy):.6f}',
            f'{shannon_entropy(pairing):.6f}',
            f'{shannon_entropy([abs(v) for v in sign]):.6f}',
            f'{(shannon_entropy(energy) + shannon_entropy(pairing)) / 2.0:.6f}',
            'shannon_entropy_fullscale_from_timeseries_distribution'
        ])

    alt_rows = []
    ok_rows = 0
    for i_bench, r in enumerate(bench):
        within = to_float(r.get('within_error_bar'))
        verdict = 'PASS' if within >= 1.0 else 'FAIL'
        if verdict == 'PASS':
            ok_rows += 1
        _lv_log("adv_obs_bench", f"row{i_bench}", "within_error_bar", within)
        _lv_log("adv_obs_bench", f"row{i_bench}", "verdict", 1 if verdict == 'PASS' else 0)
        alt_rows.append([
            r.get('problem', r.get('module', 'unknown')),
            r.get('solver_family', 'qmc_dmrg_reference'),
            r.get('model_value', r.get('model', '')),
            r.get('reference_value', r.get('reference', '')),
            r.get('abs_error', ''),
            r.get('error_bar', ''),
            verdict
        ])

    # Decoupled independent-solver evidence from dedicated module outputs
    independent_files = [
        ('qmc_module', tests_dir / 'integration_independent_qmc_results.csv'),
        ('dmrg_module', tests_dir / 'integration_independent_dmrg_results.csv'),
        ('arpes_module', tests_dir / 'integration_independent_arpes_results.csv'),
        ('stm_module', tests_dir / 'integration_independent_stm_results.csv'),
    ]
    independent_pass = 0
    independent_total = 0
    for family, fp in independent_files:
        if not fp.exists():
            continue
        for row in read_csv(fp):
            independent_total += 1
            st = row.get('status', 'FAIL')
            if st == 'PASS':
                independent_pass += 1
            alt_rows.append([
                row.get('problem', 'unknown'),
                family,
                row.get('mean', row.get('effective_ground_energy', row.get('peak_amplitude', row.get('ldos_index', '')))),
                '',
                '',
                '',
                st,
            ])

    if bench:
        campaign_status = 'PASS' if ok_rows >= max(1, int(0.8 * len(bench))) else 'FAIL'
        alt_rows.append(['GLOBAL_BENCHMARK', 'qmc_dmrg_reference', '', '', '', '', campaign_status])
    if independent_total > 0:
        indep_status = 'PASS' if independent_pass == independent_total else 'FAIL'
        alt_rows.append(['GLOBAL_INDEPENDENT', 'independent_modules', str(independent_pass), str(independent_total), '', '', indep_status])
        global_status = 'PASS' if (bench and campaign_status == 'PASS' and indep_status == 'PASS') or (not bench and indep_status == 'PASS') else 'FAIL'
        alt_rows.append(['GLOBAL', 'combined_campaign', '', '', '', '', global_status])

    spatial_csv = tests_dir / 'integration_spatial_correlations.csv'
    entropy_csv = tests_dir / 'integration_entropy_observables.csv'
    alt_csv = tests_dir / 'integration_alternative_solver_campaign.csv'

    write_csv(
        spatial_csv,
        ['problem', 'lag_step', 'pairing_corr', 'energy_corr', 'sign_corr', 'method'],
        spatial_rows,
    )
    write_csv(
        entropy_csv,
        ['problem', 'energy_entropy_bits', 'pairing_entropy_bits', 'sign_abs_entropy_bits', 'combined_entropy_bits', 'method'],
        entropy_rows,
    )
    write_csv(
        alt_csv,
        ['problem', 'solver_family', 'model_value', 'reference_value', 'abs_error', 'error_bar', 'status'],
        alt_rows,
    )

    print(f'[advanced-observables] generated: {spatial_csv}')
    print(f'[advanced-observables] generated: {entropy_csv}')
    print(f'[advanced-observables] generated: {alt_csv}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
