#!/usr/bin/env python3
import csv
import sys
from pathlib import Path


def read_csv(path: Path):
    with path.open(newline='') as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w', newline='') as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def as_float(v):
    try:
        return float(v)
    except Exception:
        return 0.0


def stage_status(stage, gate_ok, physics_ok, conn, shadow, realism):
    if stage == 'shadow':
        ok = gate_ok and physics_ok
        return ('READY' if ok else 'BLOCKED', 'rollback enabled')
    if stage == 'canary':
        ok = gate_ok and physics_ok and conn >= 65.0 and shadow >= 80.0
        return ('READY' if ok else 'ROLLBACK_REQUIRED', 'thresholds conn>=65 shadow>=80')
    ok = gate_ok and physics_ok and conn >= 80.0 and realism >= 55.0 and shadow >= 85.0
    return ('READY' if ok else 'ROLLBACK_REQUIRED', 'thresholds conn>=80 realism>=55 shadow>=85')


def main():
    if len(sys.argv) != 2:
        print('Usage: post_run_v4next_rollout_progress.py <run_dir>', file=sys.stderr)
        return 2
    run_dir = Path(sys.argv[1]).resolve()
    tests = run_dir / 'tests'
    gates = read_csv(tests / 'integration_gate_summary.csv')
    physics = read_csv(tests / 'integration_physics_gate_summary.csv')
    readiness = read_csv(tests / 'integration_v4next_connection_readiness.csv')

    gate_ok = all(r.get('status') == 'PASS' for r in gates)
    physics_ok = all(r.get('status') == 'PASS' for r in physics)
    rm = {r['dimension']: as_float(r['percent']) for r in readiness}
    conn = rm.get('v4next_connection_readiness', 0.0)
    shadow = rm.get('shadow_mode_safety', 0.0)
    realism = rm.get('realistic_simulation_level', 0.0)

    rows = []
    for stage in ['shadow', 'canary', 'full']:
        st, rule = stage_status(stage, gate_ok, physics_ok, conn, shadow, realism)
        rows.append([stage, st, f'{conn:.2f}', f'{shadow:.2f}', f'{realism:.2f}', rule])

    out = tests / 'integration_v4next_rollout_progress.csv'
    write_csv(out, ['stage', 'status', 'connection_readiness_pct', 'shadow_safety_pct', 'realism_pct', 'rule'], rows)
    print(f'[rollout-progress] generated: {out}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
