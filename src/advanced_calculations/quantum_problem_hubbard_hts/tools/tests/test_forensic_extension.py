import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from post_run_forensic_extension_tests import linreg, rmse, bootstrap_ci_alpha


class ForensicExtensionTests(unittest.TestCase):
    def test_linreg_exact_line(self):
        a, b = linreg([1, 2, 3, 4], [3, 5, 7, 9])
        self.assertAlmostEqual(a, 1.0, places=9)
        self.assertAlmostEqual(b, 2.0, places=9)

    def test_rmse_zero_when_equal(self):
        self.assertAlmostEqual(rmse([1.0, 2.0], [1.0, 2.0]), 0.0, places=12)

    def test_bootstrap_ci_returns_ordered_interval(self):
        rows = [
            {"energy": str(v), "pairing": str(2 * v + 3), "step": str(i), "problem": "p"}
            for i, v in enumerate([1, 2, 3, 4, 5, 6, 7, 8])
        ]
        est, lo, hi = bootstrap_ci_alpha(rows, n_boot=40, seed=42)
        self.assertLessEqual(lo, est)
        self.assertLessEqual(est, hi)


if __name__ == "__main__":
    unittest.main()
