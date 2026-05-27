#!/usr/bin/env python3
import csv
import json
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


def to_float(value, default=0.0):
    try:
        return float(value)
    except Exception:
        return default


def main():
    if len(sys.argv) != 3:
        print('Usage: v4next_rollout_controller.py <run_dir> <mode:shadow|canary|full>', file=sys.stderr)
        return 2

    run_dir = Path(sys.argv[1]).resolve()
    mode = sys.argv[2].strip().lower()
    if mode not in {'shadow', 'canary', 'full'}:
        print(f'Invalid rollout mode: {mode}', file=sys.stderr)
        return 3

    tests = run_dir / 'tests'
    gate = read_csv(tests / 'integration_gate_summary.csv')
    physics = read_csv(tests / 'integration_physics_gate_summary.csv')
    readiness = read_csv(tests / 'integration_v4next_connection_readiness.csv')

    gate_pass = all(r.get('status') == 'PASS' for r in gate)
    physics_pass = all(r.get('status') == 'PASS' for r in physics)

    readiness_map = {r.get('dimension'): to_float(r.get('percent')) for r in readiness}
    conn = readiness_map.get('v4next_connection_readiness', 0.0)
    shadow_safety = readiness_map.get('shadow_mode_safety', 0.0)
    realism = readiness_map.get('realistic_simulation_level', 0.0)

    status = 'HOLD'
    rollback = 'ENABLED'
    reason = ''
    activated = False

    if mode == 'shadow':
        activated = gate_pass and physics_pass
        status = 'SHADOW_ACTIVE' if activated else 'SHADOW_BLOCKED'
        reason = 'Shadow mode always first stage; auto rollback armed.'

    elif mode == 'canary':
        cond = gate_pass and physics_pass and conn >= 65.0 and shadow_safety >= 80.0
        if cond:
            activated = True
            status = 'CANARY_ACTIVE'
            reason = 'Canary thresholds satisfied.'
        else:
            status = 'ROLLBACK_TRIGGERED'
            reason = 'Canary thresholds not satisfied -> automatic rollback to shadow.'

    elif mode == 'full':
        cond = gate_pass and physics_pass and conn >= 80.0 and realism >= 55.0 and shadow_safety >= 85.0
        if cond:
            activated = True
            status = 'FULL_ACTIVE'
            reason = 'Full rollout thresholds satisfied.'
        else:
            status = 'ROLLBACK_TRIGGERED'
            reason = 'Full thresholds not satisfied -> automatic rollback to canary/shadow.'

    out_rows = [
        ['mode_requested', mode, 'Rollout mode requested by operator'],
        ['gate_pass', str(gate_pass), 'All technical integration gates'],
        ['physics_gate_pass', str(physics_pass), 'All physics readiness gates'],
        ['v4next_connection_readiness_pct', f'{conn:.2f}', 'Connection readiness percentage'],
        ['shadow_mode_safety_pct', f'{shadow_safety:.2f}', 'Shadow safety percentage'],
        ['realistic_simulation_level_pct', f'{realism:.2f}', 'Realism percentage'],
        ['activated', str(activated), 'Whether requested stage is active'],
        ['rollout_status', status, 'Current rollout status'],
        ['rollback_state', rollback, 'Automatic rollback is always enabled'],
        ['decision_reason', reason, 'Decision explanation'],
    ]

    csv_out = tests / 'integration_v4next_rollout_status.csv'
    write_csv(csv_out, ['key', 'value', 'description'], out_rows)

    md_out = tests / 'integration_v4next_rollback_plan.md'
    md_out.write_text(
        '# V4 NEXT Rollout/Rollback Plan\n\n'
        f'- Mode demandé: `{mode}`\n'
        f'- Statut: `{status}`\n'
        f'- Activated: `{activated}`\n'
        '- Rollback automatique: `ENABLED`\n\n'
        '## Procédure automatique\n'
        '1. Si seuils non respectés, revenir à `shadow` immédiatement.\n'
        '2. Désactiver activation globale (`full`).\n'
        '3. Conserver logs/checksums et artefacts de drift pour diagnostic.\n'
        '4. Réexécuter après correction.\n'
    )

    print(f'[rollout] generated: {csv_out}')
    print(f'[rollout] generated: {md_out}')
    print(f'[rollout] status={status}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
