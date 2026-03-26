import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v137 import NX47V137Kernel, V137Config


def test_v137_defaults_remove_volume_and_epoch_limits():
    cfg = V137Config()
    assert cfg.max_train_volumes == 0
    assert cfg.max_val_volumes == 0
    assert cfg.supervised_epochs == 0


def test_v137_dataset_audit_uses_all_pairs_when_unbounded(tmp_path):
    k = NX47V137Kernel.__new__(NX47V137Kernel)
    k.cfg = SimpleNamespace(
        max_train_volumes=0,
        max_val_volumes=0,
        adapt_train_threshold_to_dataset_size=True,
        train_pair_coverage_target_pct=100.0,
        min_train_pairs_required=786,
    )
    k.log = lambda *args, **kwargs: None
    pairs = []
    for i in range(3):
        img = tmp_path / f"{i}.tif"
        lbl = tmp_path / f"{i}_lbl.tif"
        img.write_bytes(b'1234')
        lbl.write_bytes(b'12')
        pairs.append((img, lbl))
    NX47V137Kernel._audit_train_dataset_size(k, pairs)
    assert k.train_dataset_audit['pair_count_selected_for_training'] == 3
    assert k.train_dataset_audit['coverage_pct_selected_vs_discovered'] == 100.0
