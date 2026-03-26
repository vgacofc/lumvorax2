#!/usr/bin/env python3
import difflib
import json
from pathlib import Path

INTEGRATION_ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = Path(__file__).resolve().parents[4]
SOURCE_ROOT = REPO_ROOT / 'src/projetx_NQubit NX/NQubit_v6'

CHECK_FILES = [
    'nqubit_v6_catlin_kaggle_kernel.py',
    'Makefile',
    'README.md',
    'KAGGLE_NOTEBOOK_FIX_V6.md',
    'tools/acquire_real_hardware_v6.py',
    'tools/build_manifest_v6.py',
    'tools/collect_system_metrics_v6.py',
    'tools/generate_v6_post_run_report.py',
    'tools/sign_manifest_v6.sh',
    'tools/stress_harness_v6.py',
    'tools/verify_manifest_v6.py',
    'tests/test_hardware_capture_v6.py',
    'tests/test_kernel_v6.py',
    'tests/test_manifest_tools_v6.py',
    'tests/test_system_metrics_v6.py',
]

report = {
    'source_root': str(SOURCE_ROOT),
    'integration_root': str(INTEGRATION_ROOT),
    'files': [],
}

for rel in CHECK_FILES:
    s = SOURCE_ROOT / rel
    i = INTEGRATION_ROOT / rel
    row = {'file': rel, 'source_exists': s.exists(), 'integration_exists': i.exists()}
    if s.exists() and i.exists():
        s_lines = s.read_text(encoding='utf-8', errors='replace').splitlines()
        i_lines = i.read_text(encoding='utf-8', errors='replace').splitlines()
        row['source_lines'] = len(s_lines)
        row['integration_lines'] = len(i_lines)
        diff = list(difflib.unified_diff(s_lines, i_lines, fromfile=f'source/{rel}', tofile=f'integration/{rel}', lineterm=''))
        row['identical'] = len(diff) == 0
        row['diff_line_count'] = len(diff)
    report['files'].append(row)

report['all_identical'] = all(r.get('identical', False) for r in report['files'] if r['source_exists'] and r['integration_exists'])
out_json = INTEGRATION_ROOT / 'results' / 'line_by_line_comparison_v6.json'
out_md = INTEGRATION_ROOT / 'results' / 'line_by_line_comparison_v6.md'
out_json.parent.mkdir(parents=True, exist_ok=True)
out_json.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

lines = ['# Line-by-line comparison V6 source vs integration', '']
lines.append(f"all_identical={report['all_identical']}")
lines.append('')
lines.append('| file | source_exists | integration_exists | source_lines | integration_lines | identical | diff_line_count |')
lines.append('|---|---:|---:|---:|---:|---:|---:|')
for r in report['files']:
    lines.append(f"| {r['file']} | {r['source_exists']} | {r['integration_exists']} | {r.get('source_lines','-')} | {r.get('integration_lines','-')} | {r.get('identical','-')} | {r.get('diff_line_count','-')} |")
out_md.write_text('\n'.join(lines) + '\n', encoding='utf-8')
print(out_json)
print(out_md)
print('all_identical=', report['all_identical'])
