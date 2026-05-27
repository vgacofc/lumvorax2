#!/usr/bin/env python3
import argparse
import csv
import json
from collections import Counter
from pathlib import Path
from statistics import mean


def read_csv(path: Path):
    if not path.exists():
        return []
    with path.open(newline='') as f:
        return list(csv.DictReader(f))


def safe_float(v):
    try:
        return float(v)
    except Exception:
        return None


def baseline_summary(run_dir: Path):
    rows = read_csv(run_dir / 'logs' / 'baseline_reanalysis_metrics.csv')
    out = {
        'rows': len(rows),
        'energy_mean': None,
        'pairing_mean': None,
        'sign_ratio_mean': None,
        'elapsed_ns_mean': None,
        'cpu_percent_mean': None,
        'mem_percent_mean': None,
    }
    if not rows:
        return out
    for k in ['energy', 'pairing', 'sign_ratio', 'elapsed_ns', 'cpu_percent', 'mem_percent']:
        vals = [safe_float(r.get(k)) for r in rows]
        vals = [v for v in vals if v is not None]
        out[f'{k}_mean'] = mean(vals) if vals else None
    return out


def tests_summary(run_dir: Path):
    rows = read_csv(run_dir / 'tests' / 'new_tests_results.csv')
    status = Counter(r.get('status', '') for r in rows)
    fails = [r for r in rows if r.get('status') == 'FAIL']
    return {
        'rows': len(rows),
        'pass': status.get('PASS', 0),
        'fail': status.get('FAIL', 0),
        'observed': status.get('OBSERVED', 0),
        'fail_ids': sorted({r.get('test_id', '') for r in fails}),
    }


def numerical_stability(run_dir: Path):
    rows = read_csv(run_dir / 'tests' / 'numerical_stability_suite.csv')
    status = Counter(r.get('status', '') for r in rows)
    energy_fails = []
    for r in rows:
        if r.get('metric') == 'energy_density_drift_max' and r.get('status') == 'FAIL':
            energy_fails.append((r.get('module', ''), safe_float(r.get('value'))))
    energy_fails.sort(key=lambda x: x[0])
    return {
        'rows': len(rows),
        'pass': status.get('PASS', 0),
        'fail': status.get('FAIL', 0),
        'energy_drift_fails': energy_fails,
    }


def integrity_summary(run_dir: Path):
    checksums = run_dir / 'logs' / 'checksums.sha256'
    exists = checksums.exists()
    missing_refs = []
    if exists:
        for line in checksums.read_text().splitlines():
            if '  ' not in line:
                continue
            _, rel = line.split('  ', 1)
            p = run_dir / rel
            if not p.exists():
                missing_refs.append(rel)
    return {
        'checksums_present': exists,
        'checksum_missing_references': missing_refs,
    }


def compare_baseline(prev_rows, cur_rows):
    key = lambda r: (r.get('problem'), r.get('step'))
    pmap = {key(r): r for r in prev_rows}
    common = [key(r) for r in cur_rows if key(r) in pmap]
    metrics = {}
    for field in ['energy', 'pairing', 'sign_ratio', 'elapsed_ns', 'cpu_percent', 'mem_percent']:
        diffs = []
        for k in common:
            a = safe_float(pmap[k].get(field))
            b = safe_float(next(r for r in cur_rows if key(r) == k).get(field))
            if a is None or b is None:
                continue
            diffs.append(abs(b - a))
        metrics[field] = {
            'common_points': len(diffs),
            'max_abs_diff': max(diffs) if diffs else 0.0,
            'mean_abs_diff': mean(diffs) if diffs else 0.0,
        }
    return metrics


def markdown_report(payload, run_order):
    lines = []
    lines.append('# RAPPORT_EVOLUTION_REPLIT_MULTI_RUNS_20260312')
    lines.append('')
    lines.append('## Synthèse exécutive')
    lines.append('- Analyse comparative multi-runs générée automatiquement.')
    lines.append('- Focus: intégrité, stabilité numérique, tendances métriques, tests en échec persistants, progression et reste-à-faire.')
    lines.append('')
    lines.append('## Runs analysés (ordre chronologique donné)')
    for r in run_order:
        x = payload[r]
        lines.append(f"- {r}: baseline_rows={x['baseline']['rows']}, new_tests_fail={x['new_tests']['fail']}, numerical_fail={x['num_stability']['fail']}, checksums_present={x['integrity']['checksums_present']}")
    lines.append('')

    lines.append('## Détails par run')
    for r in run_order:
        x = payload[r]
        lines.append(f'### {r}')
        lines.append(f"- Intégrité: checksums_present={x['integrity']['checksums_present']}, missing_refs={len(x['integrity']['checksum_missing_references'])}")
        if x['integrity']['checksum_missing_references']:
            for m in x['integrity']['checksum_missing_references']:
                lines.append(f'  - MISSING_REF: `{m}`')
        lines.append(f"- new_tests: PASS={x['new_tests']['pass']} FAIL={x['new_tests']['fail']} OBSERVED={x['new_tests']['observed']}")
        if x['new_tests']['fail_ids']:
            lines.append(f"- FAIL test_id: {', '.join(x['new_tests']['fail_ids'])}")
        lines.append(f"- numerical_stability: PASS={x['num_stability']['pass']} FAIL={x['num_stability']['fail']}")
        if x['num_stability']['energy_drift_fails']:
            lines.append('- Modules en FAIL energy_density_drift_max:')
            for mod, val in x['num_stability']['energy_drift_fails']:
                lines.append(f'  - {mod}: drift={val}')
        b = x['baseline']
        lines.append(
            f"- Baseline moyennes: energy={b['energy_mean']}, pairing={b['pairing_mean']}, sign_ratio={b['sign_ratio_mean']}, elapsed_ns={b['elapsed_ns_mean']}, cpu={b['cpu_percent_mean']}, mem={b['mem_percent_mean']}"
        )
        lines.append('')

    lines.append('## Comparaison run précédent -> suivant')
    for i in range(1, len(run_order)):
        prev_r = run_order[i - 1]
        cur_r = run_order[i]
        comp = payload[cur_r]['baseline_diff_vs_prev']
        lines.append(f'### {prev_r} -> {cur_r}')
        for m in ['energy', 'pairing', 'sign_ratio', 'elapsed_ns', 'cpu_percent', 'mem_percent']:
            lines.append(f"- {m}: max_abs_diff={comp[m]['max_abs_diff']}, mean_abs_diff={comp[m]['mean_abs_diff']}, common_points={comp[m]['common_points']}")
        lines.append('')

    lines.append('## Ce qui reste à faire (priorisé)')
    lines.append('1. Corriger les écarts benchmark QMC/DMRG et modules externes (`qmc_dmrg_*`, `external_modules_*`) encore en FAIL.')
    lines.append('2. Revoir les règles cluster-scale (`cluster_pair_trend`, `cluster_energy_trend`) qui restent en FAIL de manière récurrente.')
    lines.append('3. Verrouiller la traçabilité: empêcher des références checksum vers des logs supprimés ou non persistés.')
    lines.append('4. Exécuter un cycle complet après patch et comparer contre 2829/3447 avec ce même script pour vérifier la tendance.')
    lines.append('')
    lines.append('## Commandes reproductibles')
    lines.append('```bash')
    lines.append('python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/analyze_replit_run_evolution.py \\')
    lines.append('  --results-root src/advanced_calculations/quantum_problem_hubbard_hts/results \\')
    lines.append('  --runs research_20260312T075921Z_523 research_20260312T181131Z_2715 research_20260312T182101Z_2854 research_20260312T222246Z_2829 research_20260312T222838Z_3447 \\')
    lines.append('  --out-json src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/reports/replit_evolution_20260312.json \\')
    lines.append('  --out-md src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/RAPPORT_EVOLUTION_REPLIT_MULTI_RUNS_20260312.md')
    lines.append('')
    lines.append('bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh')
    lines.append('```')
    return '\n'.join(lines) + '\n'


def main():
    ap = argparse.ArgumentParser(description='Analyse comparative multi-runs Replit pour Hubbard HTS')
    ap.add_argument('--results-root', required=True)
    ap.add_argument('--runs', nargs='+', required=True)
    ap.add_argument('--out-json', required=True)
    ap.add_argument('--out-md', required=True)
    args = ap.parse_args()

    root = Path(args.results_root)
    payload = {}
    prev_baseline_rows = None

    for run in args.runs:
        run_dir = root / run
        if not run_dir.exists():
            raise SystemExit(f'Run not found: {run_dir}')
        b_rows = read_csv(run_dir / 'logs' / 'baseline_reanalysis_metrics.csv')
        item = {
            'baseline': baseline_summary(run_dir),
            'new_tests': tests_summary(run_dir),
            'num_stability': numerical_stability(run_dir),
            'integrity': integrity_summary(run_dir),
            'baseline_diff_vs_prev': None,
        }
        if prev_baseline_rows is not None:
            item['baseline_diff_vs_prev'] = compare_baseline(prev_baseline_rows, b_rows)
        else:
            item['baseline_diff_vs_prev'] = {
                m: {'common_points': 0, 'max_abs_diff': 0.0, 'mean_abs_diff': 0.0}
                for m in ['energy', 'pairing', 'sign_ratio', 'elapsed_ns', 'cpu_percent', 'mem_percent']
            }
        prev_baseline_rows = b_rows
        payload[run] = item

    out_json = Path(args.out_json)
    out_md = Path(args.out_md)
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_md.parent.mkdir(parents=True, exist_ok=True)

    out_json.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + '\n')
    out_md.write_text(markdown_report(payload, args.runs))

    print(f'wrote_json={out_json}')
    print(f'wrote_md={out_md}')


if __name__ == '__main__':
    main()
