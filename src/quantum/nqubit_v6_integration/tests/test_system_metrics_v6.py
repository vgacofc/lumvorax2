#!/usr/bin/env python3
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'tools' / 'collect_system_metrics_v6.py'


class TestSystemMetricsV6(unittest.TestCase):
    def test_collect_metrics_json(self):
        with tempfile.TemporaryDirectory() as td:
            wd = Path(td)
            results = wd / 'results'
            results.mkdir(parents=True, exist_ok=True)
            rc = subprocess.run(['python', str(SCRIPT)], cwd=wd, check=False).returncode
            self.assertEqual(rc, 0)
            out = results / 'system_metrics_v6.json'
            self.assertTrue(out.exists())
            data = json.loads(out.read_text(encoding='utf-8'))
            self.assertEqual(data['version'], 'v6')
            self.assertIn('system', data)
            self.assertIn('memory', data)


if __name__ == '__main__':
    unittest.main(verbosity=2)
