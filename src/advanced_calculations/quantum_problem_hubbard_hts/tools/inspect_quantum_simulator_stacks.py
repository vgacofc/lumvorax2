#!/usr/bin/env python3
import re
import subprocess
import sys
from pathlib import Path

TARGETS = [
    'src/advanced_calculations/quantum_simulator_v4_staging_next',
    'src/advanced_calculations/quantum_simulator_v5_competitor_cpu',
    'src/advanced_calculations/quantum_simulator_v4_kaggle_pack',
    'src/advanced_calculations/quantum_simulator_v3_staging',
    'src/advanced_calculations/quantum_simulator_v2_staging',
]

SCAN_EXT = ('.c', '.h', '.cpp', '.hpp', '.py', '.rs', '.md', '.txt', '.json', '.toml', '.yaml', '.yml', '.sh', '.cmake', 'Makefile')
PATTERNS = {
    'stub_or_placeholder': re.compile(r'\b(stub|placeholder|todo|fixme|mock)\b', re.IGNORECASE),
    'hardcoding_risk': re.compile(r'\b(hardcod|magic number|temporary|test-only)\b', re.IGNORECASE),
    'version_markers': re.compile(r'\b(v\d+\b|version|simulator|backend|engine)\b', re.IGNORECASE),
}


def text_files(root: Path):
    for p in root.rglob('*'):
        if p.is_file() and (p.suffix.lower() in SCAN_EXT or p.name in {'Makefile', 'README', 'README.md'}):
            yield p


def count_lines(path: Path):
    try:
        return len(path.read_text(errors='ignore').splitlines())
    except Exception:
        return 0


def main():
    if len(sys.argv) != 2:
        print('Usage: inspect_quantum_simulator_stacks.py <hubbard_root>', file=sys.stderr)
        return 2
    hubbard_root = Path(sys.argv[1]).resolve()
    repo_root = hubbard_root.parents[2]

    lines = ['# RAPPORT_INSPECTION_SIMULATEURS_V4NEXT_FULL_CYCLE33', '']
    lines.append('Inspection statique exhaustive (sans modification des répertoires simulateur).')
    lines.append('')

    for rel in TARGETS:
        root = repo_root / rel
        lines.append(f'## {rel}')
        if not root.exists():
            lines.append('- status: missing')
            lines.append('')
            continue

        files = list(text_files(root))
        total_lines = sum(count_lines(f) for f in files)
        counts = {k: 0 for k in PATTERNS}
        version_samples = []

        for f in files:
            t = f.read_text(errors='ignore')
            for k, rgx in PATTERNS.items():
                m = rgx.findall(t)
                counts[k] += len(m)
            if len(version_samples) < 12:
                for ln in t.splitlines():
                    if PATTERNS['version_markers'].search(ln):
                        version_samples.append(f'{f.relative_to(repo_root)}: {ln[:140]}')
                        if len(version_samples) >= 12:
                            break

        lines.append(f'- file_count: {len(files)}')
        lines.append(f'- line_count: {total_lines}')
        lines.append(f"- stub_or_placeholder_hits: {counts['stub_or_placeholder']}")
        lines.append(f"- hardcoding_risk_hits: {counts['hardcoding_risk']}")
        lines.append(f"- version_marker_hits: {counts['version_markers']}")

        # Primary simulator entry hints
        candidates = [p for p in files if p.name in {'README.md', 'main.c', 'main.py', 'run.sh', 'Makefile'}]
        lines.append('- entrypoint_candidates:')
        for c in candidates[:8]:
            lines.append(f'  - {c.relative_to(repo_root)}')

        lines.append('- sample_version_lines:')
        for s in version_samples[:10]:
            lines.append(f'  - {s}')
        lines.append('')

    lines.append('## Commandes reproductibles')
    lines.append('```bash')
    lines.append('python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/inspect_quantum_simulator_stacks.py src/advanced_calculations/quantum_problem_hubbard_hts')
    lines.append('```')

    out = hubbard_root / 'RAPPORT_INSPECTION_SIMULATEURS_V4NEXT_FULL_CYCLE33.md'
    out.write_text('\n'.join(lines) + '\n')
    print(f'[simulator-inspection] generated: {out}')

    # summarize via ripgrep for traceability
    cmd = ['rg', '-n', '-i', 'stub|placeholder|todo|fixme|hardcod', *TARGETS]
    subprocess.run(cmd, cwd=repo_root)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
