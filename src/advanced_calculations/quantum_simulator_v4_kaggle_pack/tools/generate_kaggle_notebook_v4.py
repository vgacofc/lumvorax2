#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[2]

SOURCE_MODULES = [
    REPO / 'src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator.c',
    REPO / 'src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator.h',
    REPO / 'src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator_fusion_v3.c',
    REPO / 'src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator_fusion_v3.h',
    REPO / 'src/advanced_calculations/quantum_simulator_v3_staging/tools/fusion_cli_v3.c',
    REPO / 'src/advanced_calculations/quantum_simulator_v3_staging/tools/run_campaign_v3.py',
]

PY_MODULES = [
    ROOT / 'python_modules/quantum_simulator_py.py',
    ROOT / 'python_modules/fusion_benchmark_v4.py',
    ROOT / 'python_modules/campaign_v4.py',
]


def code_cell(code: str):
    return {
        'cell_type': 'code',
        'execution_count': None,
        'metadata': {},
        'outputs': [],
        'source': [line + '\n' for line in code.splitlines()]
    }


def md_cell(text: str):
    return {
        'cell_type': 'markdown',
        'metadata': {},
        'source': [line + '\n' for line in text.splitlines()]
    }


def build_notebook():
    cells = []
    cells.append(md_cell('# Lumvorax Quantum Simulator V4 Kaggle Notebook\nNotebook auto-généré avec cellules dédiées par module et sous-module.'))

    cells.append(code_cell('''# Cellule 1: imports globaux
import os, json, math, time, random
from pathlib import Path
'''))

    for m in SOURCE_MODULES:
        txt = m.read_text(encoding='utf-8', errors='ignore')
        cells.append(md_cell(f'## Module source original\n- Fichier: `{m.relative_to(REPO)}`\n- Lignes: `{len(txt.splitlines())}`'))
        cells.append(code_cell('''# Snapshot source (lecture seule)
SOURCE_TEXT = r"""{}"""
print("loaded source lines:", len(SOURCE_TEXT.splitlines()))
'''.format(txt.replace('"""', '\"\"\"'))))

    for pm in PY_MODULES:
        code = pm.read_text(encoding='utf-8')
        cells.append(md_cell(f'## Module Python converti\n- Fichier: `{pm.relative_to(REPO)}`\n- Lignes: `{len(code.splitlines())}`'))
        cells.append(code_cell(code))

    cells.append(code_cell('''# Exécution benchmark rapide v4
from pathlib import Path
out = Path('results_v4_quick')
out.mkdir(exist_ok=True)
summary = run_forensic_benchmark(str(out/'fusion_forensic_full.jsonl'), scenarios=120, steps=400)
print(summary)
'''))

    cells.append(code_cell('''# Exécution campagne v4
run_dir = run_campaign(Path('results_v4_campaign'), runs_per_mode=2, scenarios=120, steps=400)
print('campaign_dir=', run_dir)
print((run_dir/'campaign_summary.json').read_text()[:500])
'''))

    nb = {
        'cells': cells,
        'metadata': {
            'kernelspec': {'display_name': 'Python 3', 'language': 'python', 'name': 'python3'},
            'language_info': {'name': 'python', 'version': '3.x'}
        },
        'nbformat': 4,
        'nbformat_minor': 5,
    }
    return nb


if __name__ == '__main__':
    nb = build_notebook()
    out = ROOT / 'notebook' / 'lumvorax_quantum_v4_kaggle.ipynb'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(nb, indent=2), encoding='utf-8')
    print(out)
