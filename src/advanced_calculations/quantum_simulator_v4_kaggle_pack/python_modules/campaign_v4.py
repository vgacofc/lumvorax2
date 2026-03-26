from __future__ import annotations

import csv
import json
from collections import Counter
from dataclasses import asdict
from datetime import datetime, timezone
from pathlib import Path

from fusion_benchmark_v4 import RngMode, run_forensic_benchmark


def run_campaign(out_root: Path, runs_per_mode: int = 30, scenarios: int = 360, steps: int = 1400) -> Path:
    run_id = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')
    out = out_root / run_id
    out.mkdir(parents=True, exist_ok=True)

    modes = [RngMode.HARDWARE_PREFERRED, RngMode.DETERMINISTIC_SEEDED]
    rows = []
    loss_counter = Counter()

    for mode in modes:
        for i in range(runs_per_mode):
            seed = 0 if mode == RngMode.HARDWARE_PREFERRED else (i + 1)
            jsonl = out / f'{mode.value}_run_{i:02d}.jsonl'
            summary = run_forensic_benchmark(str(jsonl), scenarios=scenarios, steps=steps, mode=mode, seed=seed)
            rows.append({
                'mode': mode.value,
                'run': i,
                'seed': seed,
                **asdict(summary),
            })
            for line in jsonl.read_text(encoding='utf-8').splitlines():
                d = json.loads(line)
                if d.get('event') == 'scenario_margin' and d.get('margin', 0.0) < 0:
                    loss_counter[d['scenario']] += 1

    with (out / 'campaign_runs.csv').open('w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    with (out / 'scenario_losses_frequency.csv').open('w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['scenario', 'loss_count'])
        for k, v in loss_counter.most_common():
            w.writerow([k, v])

    summary = {
        'run_id': run_id,
        'runs_per_mode': runs_per_mode,
        'scenarios': scenarios,
        'steps': steps,
        'modes': {
            m.value: {
                'win_rate_mean': sum(r['nqubit_win_rate'] for r in rows if r['mode'] == m.value) / runs_per_mode,
                'throughput_mean': sum(r['nqubits_per_sec'] for r in rows if r['mode'] == m.value) / runs_per_mode,
            }
            for m in modes
        }
    }
    (out / 'campaign_summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
    return out


if __name__ == '__main__':
    p = run_campaign(Path('results_v4'))
    print(p)
