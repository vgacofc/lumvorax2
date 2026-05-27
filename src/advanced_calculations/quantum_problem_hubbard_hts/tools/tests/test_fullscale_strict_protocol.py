import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from fullscale_strict_protocol_runner import compute_consensus, run_proxy_detection


class FullscaleStrictProtocolTests(unittest.TestCase):
    def test_consensus_majority(self):
        signatures = [
            {"cluster_id": "A", "result_hash": "h1", "execution_signature": "s1"},
            {"cluster_id": "B", "result_hash": "h1", "execution_signature": "s2"},
            {"cluster_id": "C", "result_hash": "h2", "execution_signature": "s3"},
        ]
        out = compute_consensus(signatures)
        self.assertTrue(out["validated"])
        self.assertEqual(out["global_consensus_hash"], "h1")

    def test_proxy_detection_finds_placeholder(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            src = root / "a.py"
            src.write_text("# TODO placeholder\nprint('x')\n", encoding="utf-8")
            out_csv = root / "audit" / "proxy_detection_report.csv"
            scanned, violations = run_proxy_detection(root, out_csv)
            self.assertGreaterEqual(scanned, 1)
            self.assertGreaterEqual(violations, 1)
            self.assertTrue(out_csv.exists())


if __name__ == "__main__":
    unittest.main()
