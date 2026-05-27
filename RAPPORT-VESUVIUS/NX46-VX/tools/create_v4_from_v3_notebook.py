#!/usr/bin/env python3
"""Create NX46-VX V4 notebook by copying V3 and injecting a V4 runtime patch.

Goals of V4 patch:
- Explicit GPU utilization target (96%) with runtime telemetry hooks.
- RAM telemetry and persistence in jsonl trace file.
- Unlimited logical golden nonce persistence in realtime jsonl stream.
- Keep V3 notebook immutable.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

PATCH_MD = """## NX46-VX V4 patch (GPU/RAM 96% target + golden nonce persistant temps réel)

Cette cellule est injectée automatiquement pour compléter V3 sans casser le pipeline:
- Cible runtime GPU à 96% (pilotage + télémétrie continue).
- Télémétrie RAM persistante `gpu_ram_trace_v4.jsonl`.
- Persistance temps réel des golden nonces en JSONL (`GOLDEN_NONCE_FOUND`).
- Mode top-k illimité via `V1442_GOLDEN_NONCE_TOPK=0`.
"""

PATCH_CODE = r'''# --- V4 runtime patch injected by tool ---
import os
import json
import time

# 1) Runtime knobs for aggressive utilization tracking
os.environ['NX46VX_V4_ENABLE_RUNTIME_PATCH'] = '1'
os.environ['NX46VX_V4_GPU_UTIL_TARGET_PCT'] = os.environ.get('NX46VX_V4_GPU_UTIL_TARGET_PCT', '96')
os.environ['NX46VX_V4_RAM_UTIL_TARGET_PCT'] = os.environ.get('NX46VX_V4_RAM_UTIL_TARGET_PCT', '96')
os.environ['NX46VX_V4_GPU_RAM_TRACE_PATH'] = os.environ.get('NX46VX_V4_GPU_RAM_TRACE_PATH', 'gpu_ram_trace_v4.jsonl')
os.environ['NX46VX_V4_NONCE_PERSIST_PATH'] = os.environ.get('NX46VX_V4_NONCE_PERSIST_PATH', 'golden_nonce_events_v4.jsonl')

# 2) Lift logical top-k cap for golden nonces (handled in kernel patch below)
os.environ['V1442_GOLDEN_NONCE_TOPK'] = os.environ.get('V1442_GOLDEN_NONCE_TOPK', '0')

# 3) Forensic flags
os.environ['NX46VX_V4_ENABLE_PER_NONCE_EVENTS'] = '1'
os.environ['NX46VX_V4_ENABLE_GPU_RAM_TELEMETRY'] = '1'

v4_runtime_boot = {
    'event': 'V4_RUNTIME_PATCH_BOOT',
    'ts_ns': time.time_ns(),
    'overrides': {
        'NX46VX_V4_GPU_UTIL_TARGET_PCT': os.environ['NX46VX_V4_GPU_UTIL_TARGET_PCT'],
        'NX46VX_V4_RAM_UTIL_TARGET_PCT': os.environ['NX46VX_V4_RAM_UTIL_TARGET_PCT'],
        'NX46VX_V4_GPU_RAM_TRACE_PATH': os.environ['NX46VX_V4_GPU_RAM_TRACE_PATH'],
        'NX46VX_V4_NONCE_PERSIST_PATH': os.environ['NX46VX_V4_NONCE_PERSIST_PATH'],
        'V1442_GOLDEN_NONCE_TOPK': os.environ['V1442_GOLDEN_NONCE_TOPK'],
    },
}
print(json.dumps(v4_runtime_boot, ensure_ascii=False))
# --- end V4 runtime patch ---
'''


def make_cell(cell_type: str, source: str):
    return {
        'cell_type': cell_type,
        'metadata': {},
        'source': [line + ('\n' if not line.endswith('\n') else '') for line in source.split('\n')],
        **({'outputs': [], 'execution_count': None} if cell_type == 'code' else {}),
    }


def patch_notebook_kernel_sources(nb: dict) -> int:
    """Patch golden nonce handling + telemetry directly in code cell sources.

    Returns number of cell sources modified.
    """
    changed = 0
    for cell in nb.get('cells', []):
        if cell.get('cell_type') != 'code':
            continue
        src = ''.join(cell.get('source', []))
        original = src

        # Extend dataclass/config creation with V4 persistence paths.
        src = src.replace(
            "golden_nonce_topk=int(os.environ.get('V1442_GOLDEN_NONCE_TOPK', '11')),",
            "golden_nonce_topk=int(os.environ.get('V1442_GOLDEN_NONCE_TOPK', '11')),\n"
            "        v4_gpu_ram_trace_path=os.environ.get('NX46VX_V4_GPU_RAM_TRACE_PATH', 'gpu_ram_trace_v4.jsonl'),\n"
            "        v4_nonce_persist_path=os.environ.get('NX46VX_V4_NONCE_PERSIST_PATH', 'golden_nonce_events_v4.jsonl'),"
        )

        # Add fields to config dataclass if exact anchor exists.
        src = src.replace(
            "    golden_nonce_topk: int = 11\n",
            "    golden_nonce_topk: int = 11\n"
            "    v4_gpu_ram_trace_path: str = 'gpu_ram_trace_v4.jsonl'\n"
            "    v4_nonce_persist_path: str = 'golden_nonce_events_v4.jsonl'\n"
        )

        # Top-k unlimited mode + per-nonce JSONL persistence.
        src = src.replace(
            "            ord_idx = np.argsort(nonce_values)[::-1][: self.cfg.golden_nonce_topk]\n"
            "            golden_nonce_points = [\n"
            "                {'y': int(yy[k]), 'x': int(xx[k]), 'score': float(nonce_values[k])}\n"
            "                for k in ord_idx\n"
            "            ]\n"
            "        else:\n"
            "            golden_nonce_points = []\n",
            "            topk_cfg = int(getattr(self.cfg, 'golden_nonce_topk', 0))\n"
            "            ord_idx_full = np.argsort(nonce_values)[::-1]\n"
            "            ord_idx = ord_idx_full if topk_cfg <= 0 else ord_idx_full[:topk_cfg]\n"
            "            golden_nonce_points = [\n"
            "                {'y': int(yy[k]), 'x': int(xx[k]), 'score': float(nonce_values[k])}\n"
            "                for k in ord_idx\n"
            "            ]\n"
            "            nonce_path = Path(getattr(self.cfg, 'v4_nonce_persist_path', 'golden_nonce_events_v4.jsonl'))\n"
            "            with nonce_path.open('a', encoding='utf-8') as nf:\n"
            "                for p in golden_nonce_points:\n"
            "                    nf.write(json.dumps({'event': 'GOLDEN_NONCE_FOUND', 'file': file, 'ts_ns': time.time_ns(), **p}, ensure_ascii=False) + '\\n')\n"
            "        else:\n"
            "            golden_nonce_points = []\n"
        )

        # Add lightweight GPU/RAM telemetry write near heartbeat logger.
        src = src.replace(
            "    def _log_heartbeat(self, stage: str):\n"
            "        now = time.time()\n",
            "    def _log_heartbeat(self, stage: str):\n"
            "        now = time.time()\n"
            "        try:\n"
            "            vm = psutil.virtual_memory()\n"
            "            ram_pct = float(vm.percent)\n"
            "        except Exception:\n"
            "            ram_pct = None\n"
            "        try:\n"
            "            gpu_desc = self.probe_hardware_metrics().get('gpu', [])\n"
            "        except Exception:\n"
            "            gpu_desc = []\n"
            "        try:\n"
            "            trace_path = Path(getattr(self.cfg, 'v4_gpu_ram_trace_path', 'gpu_ram_trace_v4.jsonl'))\n"
            "            payload = {\n"
            "                'event': 'GPU_RAM_TRACE',\n"
            "                'ts_ns': time.time_ns(),\n"
            "                'stage': stage,\n"
            "                'gpu_util_target_pct': float(os.environ.get('NX46VX_V4_GPU_UTIL_TARGET_PCT', '96')),\n"
            "                'ram_util_target_pct': float(os.environ.get('NX46VX_V4_RAM_UTIL_TARGET_PCT', '96')),\n"
            "                'ram_used_pct_observed': ram_pct,\n"
            "                'gpu_probe': gpu_desc,\n"
            "            }\n"
            "            with trace_path.open('a', encoding='utf-8') as tf:\n"
            "                tf.write(json.dumps(payload, ensure_ascii=False) + '\\n')\n"
            "        except Exception:\n"
            "            pass\n"
        )

        if src != original:
            cell['source'] = [line + ('\n' if not line.endswith('\n') else '') for line in src.split('\n')]
            changed += 1

    return changed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--v3-ipynb', required=True, type=Path)
    ap.add_argument('--v4-ipynb', required=True, type=Path)
    args = ap.parse_args()

    nb = json.loads(args.v3_ipynb.read_text(encoding='utf-8'))

    # Insert V4 patch right after V3 patch cells when possible.
    idx = 3 if len(nb.get('cells', [])) >= 3 else 1
    nb['cells'][idx:idx] = [make_cell('markdown', PATCH_MD), make_cell('code', PATCH_CODE)]

    modified = patch_notebook_kernel_sources(nb)

    args.v4_ipynb.parent.mkdir(parents=True, exist_ok=True)
    args.v4_ipynb.write_text(json.dumps(nb, ensure_ascii=False, indent=1), encoding='utf-8')

    print(f'written: {args.v4_ipynb}')
    print(f'cells_after_patch={len(nb.get("cells", []))}')
    print(f'code_cells_modified={modified}')


if __name__ == '__main__':
    main()
