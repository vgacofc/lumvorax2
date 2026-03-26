#!/usr/bin/env python3
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'tools' / 'build_manifest_v5.py'
VERIFY = ROOT / 'tools' / 'verify_manifest_v5.py'


class ManifestToolsTest(unittest.TestCase):
    def test_build_and_verify_success(self):
        with tempfile.TemporaryDirectory() as td:
            d = Path(td)
            (d / 'a.txt').write_text('alpha', encoding='utf-8')
            (d / 'b.txt').write_text('beta', encoding='utf-8')
            manifest = d / 'manifest.json'

            p1 = subprocess.run(['python', str(BUILD), '--input-dir', str(d), '--output', str(manifest)], check=False)
            self.assertEqual(p1.returncode, 0)
            self.assertTrue(manifest.exists())

            data = json.loads(manifest.read_text(encoding='utf-8'))
            self.assertEqual(data['version'], 'v5')
            self.assertGreaterEqual(len(data['artifacts']), 2)

            p2 = subprocess.run(['python', str(VERIFY), '--manifest', str(manifest)], check=False)
            self.assertEqual(p2.returncode, 0)

    def test_verify_detects_mismatch(self):
        with tempfile.TemporaryDirectory() as td:
            d = Path(td)
            (d / 'x.bin').write_bytes(b'123')
            manifest = d / 'manifest.json'
            subprocess.run(['python', str(BUILD), '--input-dir', str(d), '--output', str(manifest)], check=True)

            (d / 'x.bin').write_bytes(b'456')
            p = subprocess.run(['python', str(VERIFY), '--manifest', str(manifest)], check=False)
            self.assertNotEqual(p.returncode, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
