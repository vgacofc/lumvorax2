import json
import subprocess
import tempfile
import unittest
from pathlib import Path

import numpy as np
import tifffile

ROOT = Path(__file__).resolve().parents[2]


class TestLumvoraxDependencyStack360(unittest.TestCase):
    def test_inventory_builder(self):
        cmd = ["python3", str(ROOT / "RAPPORT-VESUVIUS/validation_lumvorax/build_lumvorax_module_inventory.py")]
        subprocess.check_call(cmd)
        inv = ROOT / "RAPPORT-VESUVIUS/validation_lumvorax/lumvorax_module_inventory_20260220.json"
        self.assertTrue(inv.exists())
        data = json.loads(inv.read_text(encoding="utf-8"))
        self.assertGreater(data["summary"]["headers"], 0)
        self.assertGreater(data["summary"]["functions"], 0)

    def test_competitor_tiff_to_lum_roundtrip(self):
        with tempfile.TemporaryDirectory() as td:
            td = Path(td)
            tif = td / "comp.tif"
            lum = td / "comp.lum"
            log = td / "comp_log.json"
            arr = (np.random.rand(6, 24, 24) > 0.8).astype("uint8")
            tifffile.imwrite(tif, arr, compression="LZW")
            cmd = [
                "python3",
                str(ROOT / "RAPPORT-VESUVIUS/validation_lumvorax/convert_competitor_tiff_to_lum.py"),
                "--input",
                str(tif),
                "--output",
                str(lum),
                "--log",
                str(log),
            ]
            subprocess.check_call(cmd)
            self.assertTrue(lum.exists())
            rep = json.loads(log.read_text(encoding="utf-8"))
            self.assertEqual(rep["shape"], [6, 24, 24])
            self.assertEqual(len(rep["slice_logs"]), 6)


if __name__ == "__main__":
    unittest.main()
