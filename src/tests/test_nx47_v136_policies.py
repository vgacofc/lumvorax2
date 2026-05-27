import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v136 import NX47V136Kernel


def test_v136_hardcoded_metric_policy_warn_no_raise():
    k = NX47V136Kernel.__new__(NX47V136Kernel)
    k.cfg = SimpleNamespace(enforce_no_hardcoded_metrics=True, hardcoded_metric_policy='warn')
    k.log = lambda *args, **kwargs: None
    info = {'epoch_history': [{'best_objective': 0.2}, {'best_objective': 0.2}]}
    NX47V136Kernel._assert_no_hardcoded_metric_pattern(k, info)


def test_v136_hardcoded_metric_policy_error_raises():
    k = NX47V136Kernel.__new__(NX47V136Kernel)
    k.cfg = SimpleNamespace(enforce_no_hardcoded_metrics=True, hardcoded_metric_policy='error')
    k.log = lambda *args, **kwargs: None
    info = {'epoch_history': [{'best_objective': 0.2}, {'best_objective': 0.2}]}
    try:
        NX47V136Kernel._assert_no_hardcoded_metric_pattern(k, info)
    except RuntimeError as exc:
        assert 'HARD_METRIC_PATTERN_DETECTED' in str(exc)
    else:
        raise AssertionError('Expected strict policy to raise')


def test_v136_default_min_pair_threshold_is_786():
    cfg = NX47V136Kernel.__init__.__defaults__
    # only sanity check module-level constant via class config
    from nx47_vesu_kernel_v136 import V136Config
    assert V136Config().min_train_pairs_required == 786
