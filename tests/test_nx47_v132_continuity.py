import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v132 import NX47V132Kernel




def test_continuity_matrix_contains_all_expected_ranges():
    kernel = NX47V132Kernel.__new__(NX47V132Kernel)
    matrix = NX47V132Kernel._build_continuity_matrix(kernel)
    assert set(matrix.keys()) == {
        'NX-1..NX-10',
        'NX-11..NX-20',
        'NX-21..NX-35',
        'NX-36..NX-47',
        'NX-47 v115..v132',
    }
    assert 'roadmap_realtime' in matrix['NX-36..NX-47']


def test_strict_no_fallback_blocks_missing_supervised_pairs():
    kernel = NX47V132Kernel.__new__(NX47V132Kernel)
    kernel.cfg = SimpleNamespace(supervised_train=True, strict_no_fallback=True)
    kernel.log = lambda *args, **kwargs: None
    kernel.discover_train_pairs = lambda: []

    try:
        NX47V132Kernel.build_supervised_model(kernel)
    except RuntimeError as exc:
        assert 'STRICT_NO_FALLBACK' in str(exc)
    else:
        raise AssertionError('Expected strict mode to block fallback when no supervised pairs are found')
