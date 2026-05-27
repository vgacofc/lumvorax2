#!/usr/bin/env python3
import argparse
import json
import zipfile
from pathlib import Path


def load_events(results_zip: Path):
    with zipfile.ZipFile(results_zip) as zf:
        with zf.open('nx46vx_v2_execution_logs.json') as f:
            return json.load(f)


def to_markdown(events):
    boot = next((e for e in events if e.get('event') == 'BOOT'), None)
    nxm = next((e for e in events if e.get('event') == 'NX47_METRICS'), None)
    gstats = next((e for e in events if e.get('event') == 'GLOBAL_STATS'), None)

    lines = []
    lines.append('# Golden Nonce Timeline — NX46-VX V2')
    lines.append('')
    if boot:
        cfg = boot.get('config', {})
        lines.append(f"- BOOT ts_ns: `{boot.get('ts_ns')}`")
        lines.append(f"- `golden_nonce_topk` (config): `{cfg.get('golden_nonce_topk')}`")
    if nxm:
        lines.append(f"- NX47_METRICS ts_ns: `{nxm.get('ts_ns')}`")
        lines.append(f"- `golden_nonce_detected`: `{nxm.get('golden_nonce_detected')}`")
    if gstats:
        lines.append(f"- GLOBAL_STATS ts_ns: `{gstats.get('ts_ns')}`")
        lines.append(f"- `golden_nonce_detected` (global): `{gstats.get('golden_nonce_detected')}`")

    points = (nxm or {}).get('golden_nonce_points_topk', [])
    lines.append('')
    lines.append('## Top-11 points (ranked)')
    lines.append('')
    lines.append('| rank | y | x | score |')
    lines.append('|---:|---:|---:|---:|')
    for i, p in enumerate(points, 1):
        lines.append(f"| {i} | {p.get('y')} | {p.get('x')} | {p.get('score'):.15f} |")

    lines.append('')
    lines.append('## Note')
    lines.append('')
    lines.append('- Les logs ne montrent pas un événement séparé pour chaque nonce.')
    lines.append('- Les 11 nonces sont publiés au moment de `NX47_METRICS` comme top-K final.')
    return '\n'.join(lines) + '\n'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--results-zip', required=True, type=Path)
    ap.add_argument('--out-md', required=True, type=Path)
    args = ap.parse_args()

    events = load_events(args.results_zip)
    md = to_markdown(events)
    args.out_md.parent.mkdir(parents=True, exist_ok=True)
    args.out_md.write_text(md, encoding='utf-8')
    print(f'written: {args.out_md}')


if __name__ == '__main__':
    main()
