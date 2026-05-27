import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v134 import NX47V134Kernel


def test_v134_min_train_pairs_gate():
    kernel = NX47V134Kernel.__new__(NX47V134Kernel)
    kernel.cfg = SimpleNamespace(supervised_train=True, min_train_pairs_required=1500)
    try:
        NX47V134Kernel._assert_train_pairs_threshold(kernel, pair_count=1499)
    except RuntimeError as exc:
        assert 'TRAIN_PAIRS_BELOW_THRESHOLD' in str(exc)
    else:
        raise AssertionError('Expected training-pair threshold failure')


def test_v134_train_completion_100_gate():
    kernel = NX47V134Kernel.__new__(NX47V134Kernel)
    kernel.cfg = SimpleNamespace(require_train_completion_100=True, supervised_epochs=3)
    kernel.supervised_train_info = {'epoch_history': [{'epoch': 0}, {'epoch': 1}]}
    try:
        NX47V134Kernel._assert_train_completed_100(kernel)
    except RuntimeError as exc:
        assert 'TRAIN_COMPLETION_100_FAILED' in str(exc)
    else:
        raise AssertionError('Expected train-completion gate failure')
