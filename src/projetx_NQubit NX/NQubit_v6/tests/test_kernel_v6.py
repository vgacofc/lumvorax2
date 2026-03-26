#!/usr/bin/env python3
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KERNEL = ROOT / 'nqubit_v6_catlin_kaggle_kernel.py'


class TestKernelV6(unittest.TestCase):
    def test_kernel_runs_in_current_dir_single_cell_mode(self):
        with tempfile.TemporaryDirectory() as td:
            wd = Path(td)
            p = subprocess.run(['python', str(KERNEL)], cwd=wd, check=False)
            self.assertEqual(p.returncode, 0)
            envf = wd / 'results' / 'kernel_env_detect_v6.json'
            sumf = wd / 'results' / 'kernel_run_summary_v6.json'
            statef = wd / 'results' / 'kernel_final_state_v6.json'
            self.assertTrue(envf.exists())
            self.assertTrue(sumf.exists())
            self.assertTrue(statef.exists())
            env = json.loads(envf.read_text(encoding='utf-8'))
            self.assertIn('compute_mode', env)


if __name__ == '__main__':
    unittest.main(verbosity=2)
