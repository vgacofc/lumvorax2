#!/usr/bin/env python3
"""Create NX46-VX V6 notebook from V5 without shrinking scientific workload.

V6 changes:
- strict copy-first workflow from V5 source notebook;
- explicit GPU target selection policy (prefer H100, fallback P100, else any CUDA GPU);
- runtime tags switched to V6 outputs;
- forensic/progress/checkpoint intent environment flags for V6 continuity.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

PATCH_MD = """## NX46-VX V6 patch (base V5 intégrale + sélection GPU H100→P100 + continuité)

Cette V6 est créée par **copie directe de la V5** (`nx46-vx-unified-kaggle-v5.ipynb`) puis patch ciblé, sans réduction des boucles scientifiques.

Ajouts V6:
- politique GPU explicite: **H100 prioritaire**, sinon **P100**, sinon GPU CUDA disponible;
- variables runtime pour continuité/checkpoint/reprise;
- identifiants runtime/log versionnés en V6.
"""

PATCH_CODE = r'''# --- V6 runtime patch injected by tool ---
import json
import os
import subprocess
import time

os.environ['NX46VX_V6_RUNTIME_PATCH'] = '1'
os.environ['NX46VX_GPU_POLICY'] = os.environ.get('NX46VX_GPU_POLICY', 'H100_THEN_P100')
os.environ['NX46VX_FORENSIC_MODE'] = os.environ.get('NX46VX_FORENSIC_MODE', 'FULL')
os.environ['NX46VX_RESUME_STRICT'] = os.environ.get('NX46VX_RESUME_STRICT', '1')
os.environ['NX46VX_CHECKPOINT_MODE'] = os.environ.get('NX46VX_CHECKPOINT_MODE', 'REALTIME')

def _nx46vx_select_gpu_target():
    gpus = []
    try:
        out = subprocess.check_output(
            ['bash', '-lc', 'nvidia-smi --query-gpu=name --format=csv,noheader'],
            stderr=subprocess.DEVNULL,
            timeout=3,
        ).decode().strip()
        gpus = [x.strip() for x in out.splitlines() if x.strip()]
    except Exception:
        gpus = []

    target = 'CPU'
    reason = 'no_gpu_detected'
    for name in gpus:
        u = name.upper()
        if 'H100' in u:
            target = 'H100'
            reason = f'detected:{name}'
            break
    if target == 'CPU':
        for name in gpus:
            u = name.upper()
            if 'P100' in u:
                target = 'P100'
                reason = f'detected:{name}'
                break
    if target == 'CPU' and gpus:
        target = 'CUDA_GENERIC'
        reason = f'detected:{gpus[0]}'

    os.environ['NX46VX_GPU_TARGET'] = target
    os.environ['NX46VX_GPU_DETECTED'] = '|'.join(gpus) if gpus else ''
    print(json.dumps({
        'event': 'V6_GPU_POLICY',
        'ts_ns': time.time_ns(),
        'policy': os.environ['NX46VX_GPU_POLICY'],
        'detected_gpus': gpus,
        'selected_target': target,
        'reason': reason,
    }, ensure_ascii=False))

_nx46vx_select_gpu_target()
# --- end V6 runtime patch ---
'''


def make_cell(cell_type: str, source: str):
    return {
        'cell_type': cell_type,
        'metadata': {},
        'source': [line + ('\n' if not line.endswith('\n') else '') for line in source.split('\n')],
        **({'outputs': [], 'execution_count': None} if cell_type == 'code' else {}),
    }


def patch_notebook(nb: dict) -> int:
    changed = 0
    for cell in nb.get('cells', []):
        if cell.get('cell_type') != 'code':
            continue
        src = ''.join(cell.get('source', []))
        original = src

        src = src.replace('NX46-VX V5', 'NX46-VX V6')
        src = src.replace("self.log_version_tag = 'nx46vx_v2'", "self.log_version_tag = 'nx46vx_v6'")

        if src != original:
            cell['source'] = [line + ('\n' if not line.endswith('\n') else '') for line in src.split('\n')]
            changed += 1

    return changed


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('--v5-ipynb', required=True, type=Path)
    ap.add_argument('--v6-ipynb', required=True, type=Path)
    args = ap.parse_args()

    nb = json.loads(args.v5_ipynb.read_text(encoding='utf-8'))

    idx = 5 if len(nb.get('cells', [])) >= 5 else 1
    nb['cells'][idx:idx] = [make_cell('markdown', PATCH_MD), make_cell('code', PATCH_CODE)]

    modified = patch_notebook(nb)

    args.v6_ipynb.parent.mkdir(parents=True, exist_ok=True)
    args.v6_ipynb.write_text(json.dumps(nb, ensure_ascii=False, indent=1), encoding='utf-8')

    print(f'written: {args.v6_ipynb}')
    print(f'cells_after_patch={len(nb.get("cells", []))}')
    print(f'code_cells_modified={modified}')


if __name__ == '__main__':
    main()
