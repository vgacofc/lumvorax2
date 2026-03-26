import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from post_run_full_scope_integrator import find_line, build_triple_matrix


class FullScopeIntegratorTests(unittest.TestCase):
    def test_find_line(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "a.py"
            p.write_text("alpha\nbeta\ngamma\n", encoding="utf-8")
            self.assertEqual(find_line(p, "beta"), 2)
            self.assertEqual(find_line(p, "delta"), -1)

    def test_build_triple_matrix_marks_stability(self):
        snapshots = [
            (1, {"t1": "PASS", "t2": "OBSERVED"}),
            (2, {"t1": "PASS", "t2": "OBSERVED"}),
            (3, {"t1": "PASS", "t2": "FAIL"}),
        ]
        rows = {r["test_id"]: r for r in build_triple_matrix(snapshots)}
        self.assertEqual(rows["t1"]["stability_status"], "PASS")
        self.assertEqual(rows["t2"]["stability_status"], "OBSERVED")


if __name__ == "__main__":
    unittest.main()
