#!/usr/bin/env python3
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--input-dir', required=True)
    p.add_argument('--output', required=True)
    p.add_argument('--exclude', nargs='*', default=[])
    args = p.parse_args()

    input_dir = Path(args.input_dir)
    out = Path(args.output)
    exc = set(args.exclude)
    files = [fp for fp in sorted(input_dir.rglob('*')) if fp.is_file() and fp.name not in exc]
    artifacts = [
        {'file': fp.resolve().as_posix(), 'sha256': sha256_file(fp), 'bytes': fp.stat().st_size}
        for fp in files
    ]
    data = {
        'version': 'v6',
        'generated_at_utc': datetime.now(timezone.utc).isoformat(),
        'source_of_truth': 'manifest_forensic_v6.json (signed)',
        'hash_algo': 'sha256',
        'artifacts': artifacts,
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
    print(f'manifest_written={out}')
    print(f'artifacts={len(artifacts)}')


if __name__ == '__main__':
    main()
