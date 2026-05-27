import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from post_run_cycle35_exhaustive_report import load_analysis_text, read_csv


class Cycle35ExhaustiveReportTests(unittest.TestCase):
    def test_load_analysis_text_prefers_chat_analyse2(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "CHAT").mkdir()
            (root / "CHAT" / "analysechatgpt2.md").write_text("A2", encoding="utf-8")
            (root / "CHAT" / "analysechatgpt3.md").write_text("A3", encoding="utf-8")
            self.assertEqual(load_analysis_text(root), "A2")

    def test_read_csv_missing_file_returns_empty(self):
        with tempfile.TemporaryDirectory() as td:
            missing = Path(td) / "missing.csv"
            self.assertEqual(read_csv(missing), [])


if __name__ == "__main__":
    unittest.main()
