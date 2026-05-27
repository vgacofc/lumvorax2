import json
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MATRIX = json.loads((ROOT / 'campaign_run_matrix_v61_3_v7_5.json').read_text(encoding='utf-8'))

v61_script = ROOT / 'notebook-version-NX47-V61.3' / 'nx47-vesuvius-challenge-surface-detection-v61.3.py'
v75_script = ROOT / 'src_vesuvius' / 'v7.5' / 'nx46-vesuvius-core-kaggle-ready-v7.5.py'

results = {'v61_3': [], 'v7_5': []}

for cfg in MATRIX['v61_3']:
    env = os.environ.copy()
    env['NX47_DRY_RUN'] = '1'
    env['NX47_RUN_TAG'] = cfg['run_id']
    for k,v in cfg.items():
        if k != 'run_id':
            env[k] = str(v)
    proc = subprocess.run(['python', str(v61_script)], env=env, text=True, capture_output=True)
    results['v61_3'].append({
        'run_id': cfg['run_id'],
        'returncode': proc.returncode,
        'stdout_tail': proc.stdout.splitlines()[-5:],
        'stderr_tail': proc.stderr.splitlines()[-5:],
        'params': cfg,
    })

for cfg in MATRIX['v7_5']:
    env = os.environ.copy()
    env['NX46_DRY_RUN'] = '1'
    env['NX46_RUN_TAG'] = cfg['run_id']
    for k,v in cfg.items():
        if k != 'run_id':
            env[k] = str(v)
    proc = subprocess.run(['python', str(v75_script)], env=env, text=True, capture_output=True)
    results['v7_5'].append({
        'run_id': cfg['run_id'],
        'returncode': proc.returncode,
        'stdout_tail': proc.stdout.splitlines()[-8:],
        'stderr_tail': proc.stderr.splitlines()[-8:],
        'params': cfg,
    })

(ROOT / 'campaign_execution_local_preflight_v61_3_v7_5.json').write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding='utf-8')
print('saved', ROOT / 'campaign_execution_local_preflight_v61_3_v7_5.json')
