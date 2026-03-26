#!/usr/bin/env python3
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAP = ROOT / 'tools' / 'acquire_real_hardware_v6.py'


class TestHardwareCaptureV6(unittest.TestCase):
    def test_capture_generates_report_and_jitter(self):
        with tempfile.TemporaryDirectory() as td:
            out = Path(td)
            rc = subprocess.run(['python', str(CAP), '--output-dir', str(out), '--bytes', '2048', '--jitter-samples', '2000'], check=False).returncode
            self.assertEqual(rc, 0)
            rpt = out / 'ADC_REAL_HARDWARE_CAPTURE_V6.json'
            self.assertTrue(rpt.exists())
            data = json.loads(rpt.read_text(encoding='utf-8'))
            self.assertEqual(data['version'], 'v6')
            self.assertTrue((out / 'hardware_jitter_samples_v6.csv').exists())
            self.assertGreaterEqual(len(data.get('sources', [])), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
