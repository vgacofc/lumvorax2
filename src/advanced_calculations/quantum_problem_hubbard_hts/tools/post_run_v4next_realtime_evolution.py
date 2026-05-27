#!/usr/bin/env python3
import csv
import sys
from pathlib import Path


TARGETS = {
    'v4next_connection_readiness': 80.0,
    'shadow_mode_safety': 85.0,
    'realistic_simulation_level': 55.0,
}


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


def readiness_map(run_dir: Path):
    rows = read_csv(run_dir / 'tests' / 'integration_v4next_connection_readiness.csv')
    return {r['dimension']: to_float(r['percent']) for r in rows}


def rollout_mode_status(run_dir: Path):
    p = run_dir / 'tests' / 'integration_v4next_rollout_status.csv'
    if not p.exists():
        return ('unknown', 'unknown')
    rows = read_csv(p)
    m = {r['key']: r['value'] for r in rows}
    return (m.get('mode_requested', 'unknown'), m.get('rollout_status', 'unknown'))


def main():
    if len(sys.argv) != 3:
        print('Usage: post_run_v4next_realtime_evolution.py <root_dir> <run_dir>', file=sys.stderr)
        return 2

    root_dir = Path(sys.argv[1]).resolve()
    run_dir = Path(sys.argv[2]).resolve()

    results_dir = root_dir / 'results'
    runs = sorted([p for p in results_dir.iterdir() if p.is_dir() and p.name.startswith('research_')])
    idx = runs.index(run_dir) if run_dir in runs else len(runs) - 1
    window = runs[max(0, idx - 5):idx + 1]

    timeline_rows = []
    for r in window:
        try:
            rm = readiness_map(r)
        except Exception:
            continue
        mode, status = rollout_mode_status(r)
        timeline_rows.append([
            r.name,
            mode,
            status,
            f"{rm.get('v4next_connection_readiness', 0.0):.2f}",
            f"{rm.get('shadow_mode_safety', 0.0):.2f}",
            f"{rm.get('realistic_simulation_level', 0.0):.2f}",
            f"{rm.get('global_weighted_readiness', 0.0):.2f}",
        ])

    if not timeline_rows:
        raise SystemExit('No readiness timeline rows available')

    latest = timeline_rows[-1]
    conn = to_float(latest[3])
    shadow = to_float(latest[4])
    realism = to_float(latest[5])
    global_w = to_float(latest[6])

    gap_conn = max(0.0, TARGETS['v4next_connection_readiness'] - conn)
    gap_shadow = max(0.0, TARGETS['shadow_mode_safety'] - shadow)
    gap_realism = max(0.0, TARGETS['realistic_simulation_level'] - realism)

    # percent of full readiness based on threshold fulfillment ratio
    ratio_conn = min(1.0, conn / TARGETS['v4next_connection_readiness'])
    ratio_shadow = min(1.0, shadow / TARGETS['shadow_mode_safety'])
    ratio_realism = min(1.0, realism / TARGETS['realistic_simulation_level'])
    full_threshold_progress_pct = (ratio_conn + ratio_shadow + ratio_realism) / 3.0 * 100.0

    blockers = []
    if gap_conn > 0:
        blockers.append(f'connection+{gap_conn:.2f}')
    if gap_shadow > 0:
        blockers.append(f'shadow_safety+{gap_shadow:.2f}')
    if gap_realism > 0:
        blockers.append(f'realism+{gap_realism:.2f}')

    summary_rows = [
        ['run_id', latest[0], 'Latest run id'],
        ['global_weighted_readiness_pct', f'{global_w:.2f}', 'Global weighted readiness from integration status'],
        ['full_threshold_progress_pct', f'{full_threshold_progress_pct:.2f}', 'Progress toward full rollout hard thresholds'],
        ['gap_connection_pct', f'{gap_conn:.2f}', 'Need this improvement to reach full threshold'],
        ['gap_shadow_safety_pct', f'{gap_shadow:.2f}', 'Need this improvement to reach full threshold'],
        ['gap_realism_pct', f'{gap_realism:.2f}', 'Need this improvement to reach full threshold'],
        ['full_rollout_possible_now', 'YES' if not blockers else 'NO', 'Whether all full thresholds are satisfied now'],
        ['active_blockers', ';'.join(blockers) if blockers else 'none', 'Blocking dimensions before full rollout'],
    ]

    out_timeline = run_dir / 'tests' / 'integration_v4next_realtime_evolution_timeline.csv'
    out_summary = run_dir / 'tests' / 'integration_v4next_realtime_evolution_summary.csv'
    write_csv(
        out_timeline,
        ['run_id', 'mode_requested', 'rollout_status', 'connection_readiness_pct', 'shadow_safety_pct', 'realism_pct', 'global_weighted_readiness_pct'],
        timeline_rows,
    )
    write_csv(out_summary, ['key', 'value', 'description'], summary_rows)

    md = run_dir / 'tests' / 'integration_v4next_realtime_evolution.md'
    md.write_text(
        '# V4 NEXT Realtime Evolution\n\n'
        f'- Run: `{latest[0]}`\n'
        f'- Progression vers FULL (seuils): `{full_threshold_progress_pct:.2f}%`\n'
        f'- Full possible maintenant: `{"YES" if not blockers else "NO"}`\n'
        f'- Blockers: `{("none" if not blockers else "; ".join(blockers))}`\n\n'
        '## Lecture rapide\n'
        '- `global_weighted_readiness_pct`: état global agrégé.\n'
        '- `full_threshold_progress_pct`: progression stricte vers seuils FULL.\n'
        '- `gap_*`: manque restant avant FULL.\n'
    )

    print(f'[realtime-evolution] generated: {out_timeline}')
    print(f'[realtime-evolution] generated: {out_summary}')
    print(f'[realtime-evolution] generated: {md}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
