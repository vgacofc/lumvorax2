#!/usr/bin/env python3
"""Create NX46-VX V3 notebook by copying V2 and injecting runtime patch cells.

Non-destructive: keeps V2 notebook untouched, writes a V3 notebook next to it.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path

PATCH_MD = """## NX46-VX V3 patch (GPU + couverture 100% + convergence dynamique)

Cette cellule est injectée automatiquement pour forcer des réglages V3 sans casser la base V2:
- GPU forcé et vérifié au runtime.
- Couverture train poussée vers 100%.
- Limites de préflight levées pour exécution complète.
- Patience/convergence élargie pour éviter arrêt prématuré.
- Ajout d'événements forensic V3 (garde-fous de traçabilité).
"""

PATCH_CODE = r'''# --- V3 runtime patch injected by tool ---
import os, json, time

# 1) GPU hard-preference
os.environ['CUDA_DEVICE_ORDER'] = 'PCI_BUS_ID'
os.environ['CUDA_VISIBLE_DEVICES'] = os.environ.get('CUDA_VISIBLE_DEVICES', '0')
os.environ['TF_CPP_MIN_LOG_LEVEL'] = os.environ.get('TF_CPP_MIN_LOG_LEVEL', '1')

# 2) Coverage and runtime policy (V3)
os.environ['V1442_PREFLIGHT_TRAIN_PCT'] = '100.0'
os.environ['V1442_PREFLIGHT_TEST_PCT'] = '100.0'
os.environ['V1442_MAX_TRAIN_VOLUMES'] = '0'   # <=0 means uncapped/all in current V2 code-path
os.environ['V1442_MAX_VAL_VOLUMES'] = '0'     # <=0 means uncapped/all in current V2 code-path

# 3) Dynamic/autonomous leaning knobs (safe but less restrictive)
os.environ['V1442_CONVERGENCE_PATIENCE'] = '64'
os.environ['V1442_META_NEURONS'] = '9'
os.environ['V1442_GOLDEN_NONCE_TOPK'] = '64'

# 4) V3 forensic control flags (future-proof; consumed by upgraded kernels)
os.environ['NX46VX_V3_ENABLE_GPU_ALL_PHASES'] = '1'
os.environ['NX46VX_V3_ENABLE_TEACHER_TRANSFER_EVENTS'] = '1'
os.environ['NX46VX_V3_ENABLE_COMPETITOR_TIFF_PHASE_EVENTS'] = '1'
os.environ['NX46VX_V3_ENABLE_PER_NONCE_EVENTS'] = '1'
os.environ['NX46VX_V3_REQUIRE_COVERAGE_100'] = '1'

# 5) Runtime evidence (printed early in notebook output)
try:
    import torch
    torch_cuda = bool(torch.cuda.is_available())
    torch_gpu = torch.cuda.get_device_name(0) if torch_cuda else None
except Exception:
    torch_cuda, torch_gpu = False, None

try:
    import tensorflow as tf
    tf_gpus = [d.name for d in tf.config.list_physical_devices('GPU')]
except Exception:
    tf_gpus = []

v3_runtime_boot = {
    'event': 'V3_RUNTIME_PATCH_BOOT',
    'ts_ns': time.time_ns(),
    'cuda_visible_devices': os.environ.get('CUDA_VISIBLE_DEVICES'),
    'torch_cuda_available': torch_cuda,
    'torch_gpu_name': torch_gpu,
    'tf_visible_gpus': tf_gpus,
    'overrides': {
        'V1442_PREFLIGHT_TRAIN_PCT': os.environ['V1442_PREFLIGHT_TRAIN_PCT'],
        'V1442_PREFLIGHT_TEST_PCT': os.environ['V1442_PREFLIGHT_TEST_PCT'],
        'V1442_MAX_TRAIN_VOLUMES': os.environ['V1442_MAX_TRAIN_VOLUMES'],
        'V1442_MAX_VAL_VOLUMES': os.environ['V1442_MAX_VAL_VOLUMES'],
        'V1442_CONVERGENCE_PATIENCE': os.environ['V1442_CONVERGENCE_PATIENCE'],
        'V1442_META_NEURONS': os.environ['V1442_META_NEURONS'],
        'V1442_GOLDEN_NONCE_TOPK': os.environ['V1442_GOLDEN_NONCE_TOPK'],
    },
}
print(json.dumps(v3_runtime_boot, ensure_ascii=False))
# --- end V3 runtime patch ---
'''


def make_cell(cell_type: str, source: str):
    return {
        'cell_type': cell_type,
        'metadata': {},
        'source': [line + ('\n' if not line.endswith('\n') else '') for line in source.split('\n')],
        **({'outputs': [], 'execution_count': None} if cell_type == 'code' else {}),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--v2-ipynb', required=True, type=Path)
    ap.add_argument('--v3-ipynb', required=True, type=Path)
    args = ap.parse_args()

    nb = json.loads(args.v2_ipynb.read_text(encoding='utf-8'))
    # Inject after first markdown intro cell when possible
    idx = 1 if nb.get('cells') else 0
    nb['cells'][idx:idx] = [make_cell('markdown', PATCH_MD), make_cell('code', PATCH_CODE)]

    args.v3_ipynb.write_text(json.dumps(nb, ensure_ascii=False, indent=1), encoding='utf-8')
    print(f'written: {args.v3_ipynb}')
    print(f'cells_before_patch={len(nb["cells"])-2} cells_after_patch={len(nb["cells"])}')


if __name__ == '__main__':
    main()
