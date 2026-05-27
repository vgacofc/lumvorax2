#!/usr/bin/env python3
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERIFY = ROOT / 'tools' / 'verify_manifest_v5.py'
MANIFEST = ROOT / 'results' / 'manifest_forensic_v5.json'


class IntegrationTest(unittest.TestCase):
    def test_v5_manifest_matches_artifacts(self):
        p = subprocess.run(['python', str(VERIFY), '--manifest', str(MANIFEST)], check=False)
        self.assertEqual(p.returncode, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
