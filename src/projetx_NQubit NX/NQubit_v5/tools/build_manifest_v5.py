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


def main() -> int:
    p = argparse.ArgumentParser(description='Build canonical forensic manifest V5')
    p.add_argument('--input-dir', required=True, help='Directory containing forensic artifacts')
    p.add_argument('--output', required=True, help='Output manifest JSON path')
    p.add_argument('--exclude', nargs='*', default=[], help='Basenames to exclude')
    args = p.parse_args()

    input_dir = Path(args.input_dir).resolve()
    output = Path(args.output).resolve()
    exclude = set(args.exclude)

    files = []
    for fp in sorted(input_dir.rglob('*')):
        if fp.is_file() and fp.name not in exclude:
            files.append(fp)

    artifacts = []
    for fp in files:
        artifacts.append({
            'file': fp.as_posix(),
            'sha256': sha256_file(fp),
            'bytes': fp.stat().st_size,
        })

    manifest = {
        'version': 'v5',
        'generated_at_utc': datetime.now(timezone.utc).isoformat(),
        'source_of_truth': 'manifest_forensic_v5.json (signed)',
        'hash_algo': 'sha256',
        'artifacts': artifacts,
    }

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'manifest_written={output}')
    print(f'artifacts={len(artifacts)}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
