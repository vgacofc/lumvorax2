#!/usr/bin/env python3
import argparse
import csv
import json
import os
import math
import statistics
import subprocess
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
V6_TOOLS = ROOT.parents[1] / 'quantum' / 'nqubit_v6_integration' / 'tools'


def run(cmd, cwd=None):
    p = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
    return p.returncode, p.stdout, p.stderr


def parse_jsonl(path: Path):
    rows = [json.loads(x) for x in path.read_text(encoding='utf-8').splitlines() if x.strip()]
    scenarios = [r for r in rows if r.get('event') == 'scenario_margin']
    summary = [r for r in rows if r.get('event') == 'summary'][0]
    run_config = [r for r in rows if r.get('event') == 'run_config'][0]
    return scenarios, summary, run_config


def mean(v):
    return sum(v) / len(v) if v else 0.0


def stdev(v):
    return statistics.pstdev(v) if len(v) > 1 else 0.0


def ci95(v):
    if not v:
        return {"mean": 0.0, "lower": 0.0, "upper": 0.0, "n": 0}
    m = mean(v)
    if len(v) == 1:
        return {"mean": m, "lower": m, "upper": m, "n": 1}
    # normal approximation; sufficient for campaign-level health checks
    margin = 1.96 * (statistics.pstdev(v) / math.sqrt(len(v)))
    return {"mean": m, "lower": m - margin, "upper": m + margin, "n": len(v)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--runs-per-mode', type=int, default=30)
    ap.add_argument('--scenarios', type=int, default=360)
    ap.add_argument('--steps', type=int, default=1400)
    ap.add_argument('--max-latency-p95-ns', type=float, default=300000.0)
    ap.add_argument('--max-latency-p99-ns', type=float, default=900000.0)
    ap.add_argument('--max-qubits-width', type=int, default=36)
    ap.add_argument('--throughput-regression-threshold', type=float, default=0.15)
    args = ap.parse_args()

    run_id = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')
    out = ROOT / 'results' / run_id
    out.mkdir(parents=True, exist_ok=True)

    compile_cmd = [
        'gcc', '-I./src', '-I./src/advanced_calculations', '-I./src/common', '-I./src/lum',
        'src/advanced_calculations/quantum_simulator_v4_staging_next/tools/fusion_cli_v3.c',
        'src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_simulator_fusion_v3.c',
        '-lm', '-o', str(out / 'fusion_cli_v3')
    ]
    rc, so, se = run(compile_cmd, cwd=ROOT.parents[2])
    if rc != 0:
        raise SystemExit(f'compile fusion_cli_v3 failed:\n{se}')

    # run official staging smoke test once (A side)
    smoke_cmd = [
        'gcc', '-DMODULES_QUANTIQUES_ACTIFS', '-I./src', '-I./src/advanced_calculations', '-I./src/common', '-I./src/lum',
        'src/tests/test_quantum_simulator_complete.c',
        'src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_simulator.c',
        'src/debug/memory_tracker.c', 'src/debug/forensic_logger.c', 'src/lum/lum_core.c',
        '-lm', '-o', str(out / 'official_smoke')
    ]
    rc, so, se = run(smoke_cmd, cwd=ROOT.parents[2])
    if rc != 0:
        raise SystemExit(f'compile official_smoke failed:\n{se}')
    rc, smoke_out, smoke_err = run([str(out / 'official_smoke')], cwd=ROOT.parents[2])

    modes = ['hardware_preferred', 'deterministic_seeded', 'baseline_neutralized']
    run_rows = []
    loss_counter = Counter()
    fail_reasons = Counter()
    loss_freq_by_mode = defaultdict(Counter)

    for mode in modes:
        for i in range(args.runs_per_mode):
            seed = 0 if mode == 'hardware_preferred' else (i + 1)
            jsonl = out / f'{mode}_run_{i:02d}.jsonl'
            summary = out / f'{mode}_run_{i:02d}.summary.json'
            cmd = [str(out / 'fusion_cli_v3'), str(jsonl), mode, str(seed), str(args.scenarios), str(args.steps), str(args.max_qubits_width), str(summary)]
            rc, so, se = run(cmd, cwd=ROOT.parents[2])
            if rc != 0:
                run_rows.append({'mode': mode, 'run': i, 'seed': seed, 'rc': rc, 'error': se.strip()})
                continue

            scenarios, s, rcfg = parse_jsonl(jsonl)
            p = [x.get('scenario_latency_ns', 0) for x in scenarios]
            margins = [x['margin'] for x in scenarios]
            losses = [x for x in scenarios if x['margin'] < 0]
            for x in losses:
                idx = x['scenario']
                loss_counter[idx] += 1
                loss_freq_by_mode[mode][idx] += 1
                fail_reasons[x.get('fail_reason', 'unknown')] += 1

            run_rows.append({
                'mode': mode,
                'run': i,
                'seed': seed,
                'rc': rc,
                'wins': s['nqubit_wins'],
                'losses': s['baseline_wins'],
                'win_rate': s['nqubit_win_rate'],
                'nqubits_per_sec': s['nqubits_per_sec'],
                'nqubit_avg_score': s['nqubit_avg_score'],
                'baseline_avg_score': s['baseline_qubit_avg_score'],
                'latency_p50_ns': s['latency_p50_ns'],
                'latency_p95_ns': s['latency_p95_ns'],
                'latency_p99_ns': s['latency_p99_ns'],
                'used_hardware_entropy': rcfg.get('used_hardware_entropy', False),
                'margin_mean': mean(margins),
                'margin_std': stdev(margins),
                'scenario_latency_mean_ns': mean(p),
                'max_qubits_width': s.get('max_qubits_width', args.max_qubits_width),
            })

    # CSV exports
    runs_csv = out / 'campaign_runs.csv'
    with runs_csv.open('w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=list(run_rows[0].keys()))
        writer.writeheader()
        writer.writerows(run_rows)

    loss_csv = out / 'scenario_losses_frequency.csv'
    with loss_csv.open('w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['scenario', 'loss_count_total', 'loss_frequency_total', 'loss_count_hardware_preferred', 'loss_count_deterministic_seeded', 'loss_count_baseline_neutralized'])
        total_runs = len([r for r in run_rows if r.get('rc') == 0])
        for scenario, cnt in sorted(loss_counter.items(), key=lambda kv: (-kv[1], kv[0])):
            w.writerow([
                scenario,
                cnt,
                cnt / total_runs if total_runs else 0.0,
                loss_freq_by_mode['hardware_preferred'][scenario],
                loss_freq_by_mode['deterministic_seeded'][scenario],
                loss_freq_by_mode['baseline_neutralized'][scenario],
            ])

    fail_csv = out / 'failure_reasons_frequency.csv'
    with fail_csv.open('w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['fail_reason', 'count'])
        for k, v in fail_reasons.most_common():
            w.writerow([k, v])

    # Aggregates and fusion gate
    ok_rows = [r for r in run_rows if r.get('rc') == 0]
    by_mode = {}
    for mode in modes:
        rs = [r for r in ok_rows if r['mode'] == mode]
        w = [r['win_rate'] for r in rs]
        by_mode[mode] = {
            'count': len(rs),
            'win_rate_mean': mean(w),
            'win_rate_std': stdev(w),
            'win_rate_ci95': ci95(w),
            'throughput_mean': mean([r['nqubits_per_sec'] for r in rs]),
            'latency_p95_mean_ns': mean([r['latency_p95_ns'] for r in rs]),
            'latency_p99_mean_ns': mean([r['latency_p99_ns'] for r in rs]),
            'max_qubits_width': mean([r.get('max_qubits_width', args.max_qubits_width) for r in rs]),
        }

    gate = {
        'min_win_rate_mean': 0.60,
        'max_win_rate_std': 0.05,
        'max_latency_p95_ns': args.max_latency_p95_ns,
        'max_latency_p99_ns': args.max_latency_p99_ns,
        'integrity_required': True,
    }

    # build manifest/signature V6 style
    build = ['python', str(V6_TOOLS / 'build_manifest_v6.py'), '--input-dir', str(out), '--output', str(out / 'manifest_forensic_v3.json'), '--exclude', 'private_key.pem', 'public_key.pem']
    rc_b, so_b, se_b = run(build, cwd=ROOT.parents[2])

    key_priv = out / 'private_key.pem'
    key_pub = out / 'public_key.pem'
    rc_k, so_k, se_k = run(['openssl', 'genrsa', '-out', str(key_priv), '2048'])
    rc_p, so_p, se_p = run(['openssl', 'rsa', '-in', str(key_priv), '-pubout', '-out', str(key_pub)])
    rc_s, so_s, se_s = run(['bash', str(V6_TOOLS / 'sign_manifest_v6.sh'), str(out / 'manifest_forensic_v3.json'), str(key_priv), str(key_pub)], cwd=ROOT.parents[2])
    rc_v, so_v, se_v = run(['python', str(V6_TOOLS / 'verify_manifest_v6.py'), '--manifest', str(out / 'manifest_forensic_v3.json')], cwd=ROOT.parents[2])

    integrity_ok = rc_b == 0 and rc_s == 0 and rc_v == 0
    gate_status = (
        by_mode['hardware_preferred']['win_rate_mean'] >= gate['min_win_rate_mean']
        and by_mode['deterministic_seeded']['win_rate_mean'] >= gate['min_win_rate_mean']
        and by_mode['hardware_preferred']['win_rate_std'] <= gate['max_win_rate_std']
        and by_mode['deterministic_seeded']['win_rate_std'] <= gate['max_win_rate_std']
        and by_mode['baseline_neutralized']['win_rate_std'] <= gate['max_win_rate_std']
        and by_mode['hardware_preferred']['latency_p95_mean_ns'] <= gate['max_latency_p95_ns']
        and by_mode['deterministic_seeded']['latency_p95_mean_ns'] <= gate['max_latency_p95_ns']
        and by_mode['baseline_neutralized']['latency_p95_mean_ns'] <= gate['max_latency_p95_ns']
        and by_mode['hardware_preferred']['latency_p99_mean_ns'] <= gate['max_latency_p99_ns']
        and by_mode['deterministic_seeded']['latency_p99_mean_ns'] <= gate['max_latency_p99_ns']
        and by_mode['baseline_neutralized']['latency_p99_mean_ns'] <= gate['max_latency_p99_ns']
        and integrity_ok
    )



    # throughput regression guardrail
    throughput_values = [r['nqubits_per_sec'] for r in ok_rows]
    throughput_regression = False
    throughput_reference = mean(throughput_values)
    min_mode_thr = min(by_mode[m]['throughput_mean'] for m in modes) if modes else 0.0
    if throughput_reference > 0:
        throughput_regression = (throughput_reference - min_mode_thr) / throughput_reference > args.throughput_regression_threshold

    watchlist = [74, 117, 133]
    watchlist_loss = {
        mode: {
            str(sid): loss_freq_by_mode[mode][sid] / args.runs_per_mode if args.runs_per_mode else 0.0
            for sid in watchlist
        }
        for mode in modes
    }
    throughput_gap = 0.0
    hp_thr = by_mode['hardware_preferred']['throughput_mean']
    ds_thr = by_mode['deterministic_seeded']['throughput_mean']
    if hp_thr:
        throughput_gap = (hp_thr - ds_thr) / hp_thr

    report = {
        'run_id': run_id,
        'campaign': {'runs_per_mode': args.runs_per_mode, 'scenarios': args.scenarios, 'steps': args.steps, 'max_qubits_width': args.max_qubits_width},
        'official_smoke': {'rc': rc, 'stdout': smoke_out, 'stderr': smoke_err},
        'modes': by_mode,
        'fusion_gate': {**gate, 'integrity_ok': integrity_ok, 'pass': gate_status},
        'manifest': {
            'build_rc': rc_b,
            'sign_rc': rc_s,
            'verify_rc': rc_v,
            'manifest_path': str(out / 'manifest_forensic_v3.json'),
            'signature_path': str(out / 'manifest_forensic_v3.json.sig')
        },
        'watchlist_scenarios': watchlist,
        'watchlist_loss_frequency_by_mode': watchlist_loss,
        'notes': {
            'baseline_bias_hypothesis': 'baseline uses sigma*0.7 and linear drift 0.03; this may simplify baseline dynamics and bias margin distribution.',
            'fragility_explanation': 'losses vary by seed/context because noise realization changes trajectory crossing around target in stochastic regime.',
            'throughput_gap_hardware_vs_deterministic': throughput_gap,
            'throughput_reference_mean': throughput_reference,
            'throughput_regression_flag': throughput_regression
        }
    }

    (out / 'campaign_summary.json').write_text(json.dumps(report, indent=2), encoding='utf-8')

    md = out / 'campaign_summary.md'
    lines = [
        '# Quantum Simulator V3 Staging Campaign Summary',
        '',
        f'- run_id: `{run_id}`',
        f'- runs_per_mode: **{args.runs_per_mode}**',
        f'- scenarios: **{args.scenarios}**',
        f'- steps: **{args.steps}**',
        f'- max_qubits_width: **{args.max_qubits_width}**',
        '',
        '## A/B officiel vs staging',
        f'- officiel smoke rc: `{rc}`',
        f'- staging fusion gate pass: `{gate_status}`',
        f'- gate latency p95 max ns: `{gate["max_latency_p95_ns"]}`',
        f'- gate latency p99 max ns: `{gate["max_latency_p99_ns"]}`',
        '',
        '## Modes',
    ]
    for mode, d in by_mode.items():
        lines += [
            f'### {mode}',
            f'- count: {d["count"]}',
            f'- win_rate_mean: {d["win_rate_mean"]:.6f}',
            f'- win_rate_std: {d["win_rate_std"]:.6f}',
            f'- win_rate_ci95: [{d["win_rate_ci95"]["lower"]:.6f}, {d["win_rate_ci95"]["upper"]:.6f}]',
            f'- throughput_mean: {d["throughput_mean"]:.3f}',
            f'- latency_p95_mean_ns: {d["latency_p95_mean_ns"]:.1f}',
            f'- latency_p99_mean_ns: {d["latency_p99_mean_ns"]:.1f}',
            f'- max_qubits_width: {d["max_qubits_width"]:.1f}',
            ''
        ]
    lines += [
        '## Intégrité manifest/signature (style V6)',
        f'- manifest build rc: {rc_b}',
        f'- sign rc: {rc_s}',
        f'- verify rc: {rc_v}',
        '',
        '## Fichiers produits',
        '- campaign_runs.csv',
        '- scenario_losses_frequency.csv',
        '- failure_reasons_frequency.csv',
        '- campaign_summary.json',
        '- manifest_forensic_v3.json + .sig',
        '',
        f'- throughput_regression_flag: {throughput_regression}',
        '',
        '## Watchlist scénarios fragiles',
        '- watchlist: 74, 117, 133',
    ]
    md.write_text('\n'.join(lines), encoding='utf-8')
    print(str(out))


if __name__ == '__main__':
    main()
