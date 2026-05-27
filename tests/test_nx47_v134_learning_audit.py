import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v134 import NX47V134Kernel


def test_learning_percent_real_computation():
    kernel = NX47V134Kernel.__new__(NX47V134Kernel)
    kernel.cfg = SimpleNamespace(supervised_epochs=4)
    pct = NX47V134Kernel._compute_learning_percent_real(
        kernel,
        {
            'epoch_history': [{'epoch': 0}, {'epoch': 1}, {'epoch': 2}, {'epoch': 3}],
            'selected_hyperparams': {'val_f1': 0.5, 'val_iou': 0.25},
        },
    )
    assert pct > 80.0


def test_hardcoded_metric_pattern_detection():
    kernel = NX47V134Kernel.__new__(NX47V134Kernel)
    kernel.cfg = SimpleNamespace(enforce_no_hardcoded_metrics=True)
    info = {
        'epoch_history': [
            {'best_objective': 0.123},
            {'best_objective': 0.123},
            {'best_objective': 0.123},
        ]
    }
    try:
        NX47V134Kernel._assert_no_hardcoded_metric_pattern(kernel, info)
    except RuntimeError as exc:
        assert 'HARD_METRIC_PATTERN_DETECTED' in str(exc)
    else:
        raise AssertionError('Expected hardcoded metric pattern detection to fail')
