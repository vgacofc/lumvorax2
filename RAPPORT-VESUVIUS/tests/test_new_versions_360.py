import ast
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

FILES = {
    'v2_5': ROOT / 'RAPPORT-VESUVIUS/src_vesuvius/nx47_vesu_kernel_v2/nx47_vesu_kernel_v2.5.py',
    'v7_7': ROOT / 'RAPPORT-VESUVIUS/src_vesuvius/v7.7/nx46-vesuvius-core-kaggle-ready-v7.7.py',
    'v61_5': ROOT / 'RAPPORT-VESUVIUS/notebook-version-NX47-V61.5/nx47-vesu-kernel-new-v61-5.py',
    'v144_3': ROOT / 'RAPPORT-VESUVIUS/notebook-version-NX47-V144.3/nx47-vesu-kernel-new-v144-3.py',
}

class TestNewVersions360(unittest.TestCase):
    def test_files_exist(self):
        for name, path in FILES.items():
            self.assertTrue(path.exists(), f"missing file: {name}")

    def test_python_parseable(self):
        for name, path in FILES.items():
            txt = path.read_text(encoding='utf-8')
            ast.parse(txt)

    def test_forensic_markers_present(self):
        markers = {
            'v2_5': ['DEPENDENCY_RESOLUTION_REPORT', 'NATIVE_BRIDGE_STATUS', 'MEMORY_SNAPSHOT', 'LUM_ROUNDTRIP_OK'],
            'v7_7': ['dependency_manifest_v77.json', 'merkle_chain.log', 'forensic_ultra.log'],
            'v61_5': ['dependency_manifest_v615.json', 'EXEC_COMPLETE'],
            'v144_3': ['NATIVE_BRIDGE_STATUS', 'UltraAuthentic360Merkle', 'EXEC_COMPLETE'],
        }
        for name, expected in markers.items():
            txt = FILES[name].read_text(encoding='utf-8')
            for token in expected:
                self.assertIn(token, txt, f"{name} missing {token}")

if __name__ == '__main__':
    unittest.main()
