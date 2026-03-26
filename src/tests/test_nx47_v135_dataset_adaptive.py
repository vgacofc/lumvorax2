import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v135 import NX47V135Kernel


def test_v135_adaptive_requirement_scales_with_dataset():
    k = NX47V135Kernel.__new__(NX47V135Kernel)
    k.cfg = SimpleNamespace(
        adapt_train_threshold_to_dataset_size=True,
        train_pair_coverage_target_pct=90.0,
        min_train_pairs_required=1500,
    )
    req = NX47V135Kernel._derive_train_pair_requirement(k, 786)
    assert req == 708


def test_v135_adaptive_audit_builds_bytes(tmp_path):
    k = NX47V135Kernel.__new__(NX47V135Kernel)
    k.cfg = SimpleNamespace(
        adapt_train_threshold_to_dataset_size=True,
        train_pair_coverage_target_pct=100.0,
        min_train_pairs_required=1500,
        max_train_volumes=24,
        max_val_volumes=8,
    )
    k.log = lambda *args, **kwargs: None
    img = tmp_path / 'a.tif'
    lbl = tmp_path / 'a_lbl.tif'
    img.write_bytes(b'1234')
    lbl.write_bytes(b'12')
    NX47V135Kernel._audit_train_dataset_size(k, [(img, lbl)])
    assert k.train_dataset_audit['pair_count_discovered'] == 1
    assert k.train_dataset_audit['total_train_image_bytes'] == 4
    assert k.train_dataset_audit['total_train_label_bytes'] == 2
