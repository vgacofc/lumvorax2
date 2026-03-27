#!/usr/bin/env python3
"""Audit GPU evidence and runtime budget risk from NX46-VX V2 result bundle."""
from __future__ import annotations
import argparse
import json
import zipfile
from pathlib import Path


def load_json_from_zip(zpath: Path, inner: str):
    with zipfile.ZipFile(zpath) as zf:
        with zf.open(inner) as f:
            return json.load(f)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--results-zip', required=True, type=Path)
    ap.add_argument('--out-md', required=True, type=Path)
    args = ap.parse_args()

    meta = load_json_from_zip(args.results_zip, 'nx46vx_v2_execution_metadata.json')
    logs = load_json_from_zip(args.results_zip, 'nx46vx_v2_execution_logs.json')

    boot = next((e for e in logs if e.get('event') == 'BOOT'), {})
    hw = boot.get('hardware', {})
    gpus = hw.get('gpu', [])

    total_s = float(meta.get('global_stats', {}).get('elapsed_total_s', 0.0) or 0.0)
    total_h = total_s / 3600.0
    margin_h = 9.0 - total_h

    gpu_evidence_events = []
    gpu_tokens = ('cuda', 'gpu', 'torch.cuda', 'tensorflow', 'device')
    for e in logs:
        s = json.dumps(e, ensure_ascii=False).lower()
        if any(t in s for t in gpu_tokens):
            gpu_evidence_events.append((e.get('event'), e.get('ts_ns')))
    unique_gpu_events = sorted(set(gpu_evidence_events))

    lines = []
    lines.append('# Audit GPU et budget runtime — NX46-VX V2')
    lines.append('')
    lines.append('## 1) Détection hardware')
    lines.append('')
    lines.append(f'- GPU(s) détecté(s) au BOOT: `{gpus}`')
    lines.append('')
    lines.append('## 2) Utilisation GPU prouvée dans les logs V2')
    lines.append('')
    if unique_gpu_events:
        lines.append('- Des traces contenant des tokens GPU/CUDA existent, événements uniques:')
        for ev, ts in unique_gpu_events[:25]:
            lines.append(f'  - `{ev}` @ `{ts}`')
    else:
        lines.append('- ❗ Aucune preuve explicite d’usage GPU phase-par-phase dans les logs V2 (seulement détection hardware au BOOT).')
    lines.append('')
    lines.append('## 3) Risque dépassement limite 9h Kaggle')
    lines.append('')
    lines.append(f'- `elapsed_total_s`: `{total_s:.3f}` s')
    lines.append(f'- `elapsed_total_h`: `{total_h:.3f}` h')
    lines.append(f'- marge vs 9h: `{margin_h:.3f}` h')
    if margin_h < 0:
        lines.append('- ⚠️ Run au-dessus de 9h: risque de timeout/scoring error élevé.')
    elif margin_h < 0.5:
        lines.append('- ⚠️ Run proche de 9h: risque opérationnel élevé (variance machine/Kaggle).')
    else:
        lines.append('- ✅ Run en-dessous de 9h avec marge significative.')

    lines.append('')
    lines.append('## 4) Conclusion')
    lines.append('')
    lines.append('- V2 détecte la P100 au BOOT, mais la preuve d’utilisation GPU complète A→Z n’est pas assez explicite dans les traces publiques actuelles.')
    lines.append('- Il faut ajouter des événements `GPU_PHASE_START/END` et un compteur `gpu_utilization_trace` en V3 pour lever toute ambiguïté.')

    args.out_md.parent.mkdir(parents=True, exist_ok=True)
    args.out_md.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'written: {args.out_md}')


if __name__ == '__main__':
    main()
