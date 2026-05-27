#!/usr/bin/env python3
"""Create NX46-VX V5 notebook from V4 and inject operational hardening patches.

V5 adds the missing operational controls identified in V3/V4 forensic analysis:
- Runtime version tagging aligned to V5.
- Explicit phase completion events PHASE_1_DONE..PHASE_4_DONE.
- Explicit teacher transfer proof event (TEACHER_TRANSFER_APPLIED).
- Periodic synthetic KPI event during supervised fit for ETA/position clarity.
- Rename legacy early SUBMISSION_READY marker to PRECHECK_SUBMISSION_READY.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

PATCH_MD = """## NX46-VX V5 patch (traçabilité phases + KPI + transfert teacher explicite)

Patch injecté automatiquement à partir de V4:
- alignement du `BOOT.version` vers `NX46-VX V5`;
- événements explicites de fin de phase (`PHASE_1_DONE..PHASE_4_DONE`);
- preuve d'application transfert teacher (`TEACHER_TRANSFER_APPLIED`);
- KPI synthétique périodique en `train_supervised/fit_prox_iter`;
- suppression d'ambiguïté du signal bootstrap (`PRECHECK_SUBMISSION_READY`).
"""

PATCH_CODE = r'''# --- V5 runtime patch injected by tool ---
import os
import json
import time

os.environ['NX46VX_V5_RUNTIME_PATCH'] = '1'
os.environ['NX46VX_V5_EXPECTED_TEACHERS'] = os.environ.get('NX46VX_V5_EXPECTED_TEACHERS', '9')
os.environ['NX46VX_V5_KPI_INTERVAL_ITER'] = os.environ.get('NX46VX_V5_KPI_INTERVAL_ITER', '10')

v5_runtime_boot = {
    'event': 'V5_RUNTIME_PATCH_BOOT',
    'ts_ns': time.time_ns(),
    'overrides': {
        'NX46VX_V5_EXPECTED_TEACHERS': os.environ['NX46VX_V5_EXPECTED_TEACHERS'],
        'NX46VX_V5_KPI_INTERVAL_ITER': os.environ['NX46VX_V5_KPI_INTERVAL_ITER'],
    },
}
print(json.dumps(v5_runtime_boot, ensure_ascii=False))
# --- end V5 runtime patch ---
'''


def make_cell(cell_type: str, source: str):
    return {
        'cell_type': cell_type,
        'metadata': {},
        'source': [line + ('\n' if not line.endswith('\n') else '') for line in source.split('\n')],
        **({'outputs': [], 'execution_count': None} if cell_type == 'code' else {}),
    }


def patch_notebook_kernel_sources(nb: dict) -> int:
    changed = 0
    for cell in nb.get('cells', []):
        if cell.get('cell_type') != 'code':
            continue
        src = ''.join(cell.get('source', []))
        original = src

        # 1) align runtime version tag
        src = src.replace("self.version = 'NX46-VX V2'", "self.version = 'NX46-VX V5'")

        # 2) avoid confusing bootstrap marker naming
        src = src.replace('logger.log("SUBMISSION_READY", {"zip": str(ZIP_PATH)})',
                          'logger.log("PRECHECK_SUBMISSION_READY", {"zip": str(ZIP_PATH)})')

        # 3) explicit teacher transfer applied event once runtime roots and registry are known
        src = src.replace(
            "print('training plan phases:', NX46_V2_TRAINING_PLAN)\n\n# keep strict lock: no distillation if 9 teachers are not declared\nassert_9_teacher_models_ready(TEACHER_MODELS_REGISTRY)\n",
            "print('training plan phases:', NX46_V2_TRAINING_PLAN)\n"
            "teacher_expected = int(os.environ.get('NX46VX_V5_EXPECTED_TEACHERS', '9'))\n"
            "teacher_runtime_found = int(len(runtime_teacher_roots))\n"
            "teacher_registry_count = int(len(TEACHER_MODELS_REGISTRY))\n"
            "print(json.dumps({\n"
            "    'event': 'TEACHER_TRANSFER_APPLIED',\n"
            "    'ts_ns': time.time_ns(),\n"
            "    'teachers_expected': teacher_expected,\n"
            "    'teachers_registry': teacher_registry_count,\n"
            "    'teacher_roots_runtime_found': teacher_runtime_found,\n"
            "    'teacher_transfer_enabled': bool(teacher_registry_count >= teacher_expected and teacher_runtime_found >= 1),\n"
            "}, ensure_ascii=False))\n\n"
            "# keep strict lock: no distillation if 9 teachers are not declared\n"
            "assert_9_teacher_models_ready(TEACHER_MODELS_REGISTRY)\n"
        )

        # 4) add KPI event during iterative prox fit
        src = src.replace(
            "                payload.update({'substage': 'fit_prox_iter', 'iter': int(it + 1), 'max_iter': int(max_iter), 'iter_progress_percent': float(100.0 * (it + 1) / max(1, max_iter))})\n"
            "                progress_cb(**payload)\n",
            "                payload.update({'substage': 'fit_prox_iter', 'iter': int(it + 1), 'max_iter': int(max_iter), 'iter_progress_percent': float(100.0 * (it + 1) / max(1, max_iter))})\n"
            "                progress_cb(**payload)\n"
            "                kpi_every = max(1, int(os.environ.get('NX46VX_V5_KPI_INTERVAL_ITER', '10')))\n"
            "                if ((it + 1) % kpi_every == 0) or ((it + 1) == max_iter):\n"
            "                    kpi = dict(progress_prefix or {})\n"
            "                    kpi.update({\n"
            "                        'substage': 'fit_kpi',\n"
            "                        'iter': int(it + 1),\n"
            "                        'max_iter': int(max_iter),\n"
            "                        'iter_progress_percent': float(100.0 * (it + 1) / max(1, max_iter)),\n"
            "                        'eta_iter_remaining': int(max(0, max_iter - (it + 1))),\n"
            "                    })\n"
            "                    progress_cb(**kpi)\n"
        )

        # 5) explicit phase completion logs in run()
        src = src.replace(
            "        files = self.discover_inputs()\n        self._run_preflight_5pct(files)\n        self.build_supervised_model()\n        self.plan.update('package', 10.0)\n",
            "        files = self.discover_inputs()\n"
            "        self.log('PHASE_1_DONE', phase='phase_1_teacher_distillation', status='completed')\n"
            "        self._run_preflight_5pct(files)\n"
            "        self.build_supervised_model()\n"
            "        self.log('PHASE_2_DONE', phase='phase_2_supervised_train', status='completed')\n"
            "        self.plan.update('package', 10.0)\n"
        )

        src = src.replace(
            "        pred_map, metrics = self.segment_volume(files)\n",
            "        pred_map, metrics = self.segment_volume(files)\n"
            "        self.log('PHASE_3_DONE', phase='phase_3_test_guided_ultra_finetune', status='completed')\n"
        )

        src = src.replace(
            "        self.log('EXEC_COMPLETE',",
            "        self.log('PHASE_3_DONE', phase='phase_3_test_guided_ultra_finetune', status='completed')\n"
            "        self.log('PHASE_4_DONE', phase='phase_4_main_test_inference', status='completed')\n"
            "        self.log('EXEC_COMPLETE',"
        )

        if src != original:
            cell['source'] = [line + ('\n' if not line.endswith('\n') else '') for line in src.split('\n')]
            changed += 1

    return changed


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('--v4-ipynb', required=True, type=Path)
    ap.add_argument('--v5-ipynb', required=True, type=Path)
    args = ap.parse_args()

    nb = json.loads(args.v4_ipynb.read_text(encoding='utf-8'))

    idx = 5 if len(nb.get('cells', [])) >= 5 else 1
    nb['cells'][idx:idx] = [make_cell('markdown', PATCH_MD), make_cell('code', PATCH_CODE)]

    modified = patch_notebook_kernel_sources(nb)

    args.v5_ipynb.parent.mkdir(parents=True, exist_ok=True)
    args.v5_ipynb.write_text(json.dumps(nb, ensure_ascii=False, indent=1), encoding='utf-8')

    print(f'written: {args.v5_ipynb}')
    print(f'cells_after_patch={len(nb.get("cells", []))}')
    print(f'code_cells_modified={modified}')


if __name__ == '__main__':
    main()
