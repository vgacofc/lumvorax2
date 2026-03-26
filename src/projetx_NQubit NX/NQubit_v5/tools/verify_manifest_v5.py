#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    p = argparse.ArgumentParser(description='Verify forensic manifest V5')
    p.add_argument('--manifest', required=True)
    args = p.parse_args()

    manifest_path = Path(args.manifest)
    data = json.loads(manifest_path.read_text(encoding='utf-8'))

    mismatches = []
    missing = []
    for a in data.get('artifacts', []):
        fp = Path(a['file'])
        if not fp.exists():
            missing.append(a['file'])
            continue
        actual = sha256_file(fp)
        if actual != a['sha256']:
            mismatches.append((a['file'], a['sha256'], actual))

    print(f"artifact_count={len(data.get('artifacts', []))}")
    print(f'missing={len(missing)}')
    print(f'mismatches={len(mismatches)}')

    for m in missing[:20]:
        print(f'MISSING {m}')
    for f, expected, actual in mismatches[:20]:
        print(f'MISMATCH {f} expected={expected} actual={actual}')

    return 1 if missing or mismatches else 0


if __name__ == '__main__':
    raise SystemExit(main())
