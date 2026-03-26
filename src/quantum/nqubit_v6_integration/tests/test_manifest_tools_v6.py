#!/usr/bin/env python3
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'tools' / 'build_manifest_v6.py'
VERIFY = ROOT / 'tools' / 'verify_manifest_v6.py'


class TestManifestV6(unittest.TestCase):
    def test_build_verify_ok(self):
        with tempfile.TemporaryDirectory() as td:
            d = Path(td)
            (d / 'a.txt').write_text('a', encoding='utf-8')
            (d / 'b.txt').write_text('b', encoding='utf-8')
            mf = d / 'm.json'
            self.assertEqual(subprocess.run(['python', str(BUILD), '--input-dir', str(d), '--output', str(mf)], check=False).returncode, 0)
            data = json.loads(mf.read_text(encoding='utf-8'))
            self.assertEqual(data['version'], 'v6')
            self.assertEqual(subprocess.run(['python', str(VERIFY), '--manifest', str(mf)], check=False).returncode, 0)

    def test_mismatch_detected(self):
        with tempfile.TemporaryDirectory() as td:
            d = Path(td)
            f = d / 'x.bin'
            f.write_bytes(b'111')
            mf = d / 'm.json'
            subprocess.run(['python', str(BUILD), '--input-dir', str(d), '--output', str(mf)], check=True)
            f.write_bytes(b'222')
            self.assertNotEqual(subprocess.run(['python', str(VERIFY), '--manifest', str(mf)], check=False).returncode, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
