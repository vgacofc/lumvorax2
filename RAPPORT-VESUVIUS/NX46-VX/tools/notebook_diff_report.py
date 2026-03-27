#!/usr/bin/env python3
"""Generate a line-by-line markdown diff report between two notebooks."""
from __future__ import annotations

import argparse
import difflib
from datetime import datetime, timezone
from pathlib import Path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--before', required=True, type=Path)
    ap.add_argument('--after', required=True, type=Path)
    ap.add_argument('--out-md', required=True, type=Path)
    args = ap.parse_args()

    before_lines = args.before.read_text(encoding='utf-8').splitlines()
    after_lines = args.after.read_text(encoding='utf-8').splitlines()

    diff = list(difflib.unified_diff(
        before_lines,
        after_lines,
        fromfile=str(args.before),
        tofile=str(args.after),
        n=3,
        lineterm=''
    ))

    ts = datetime.now(timezone.utc).isoformat()
    added = sum(1 for l in diff if l.startswith('+') and not l.startswith('+++'))
    removed = sum(1 for l in diff if l.startswith('-') and not l.startswith('---'))

    md = []
    md.append('# RAPPORT DIFF LIGNE À LIGNE')
    md.append('')
    md.append(f'- Généré (UTC): `{ts}`')
    md.append(f'- Notebook avant: `{args.before}`')
    md.append(f'- Notebook après: `{args.after}`')
    md.append(f'- Lignes ajoutées: **{added}**')
    md.append(f'- Lignes supprimées: **{removed}**')
    md.append('')
    md.append('## Diff unifié complet')
    md.append('')
    md.append('```diff')
    md.extend(diff if diff else ['(aucune différence)'])
    md.append('```')
    md.append('')

    args.out_md.parent.mkdir(parents=True, exist_ok=True)
    args.out_md.write_text('\n'.join(md), encoding='utf-8')
    print(f'written: {args.out_md}')
    print(f'added={added} removed={removed} diff_lines={len(diff)}')


if __name__ == '__main__':
    main()
