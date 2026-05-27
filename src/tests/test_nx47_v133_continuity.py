import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v133 import NX47V133Kernel


def test_v133_continuity_matrix_ranges():
    kernel = NX47V133Kernel.__new__(NX47V133Kernel)
    matrix = NX47V133Kernel._build_continuity_matrix(kernel)
    assert set(matrix.keys()) == {
        'NX-1..NX-10',
        'NX-11..NX-20',
        'NX-21..NX-35',
        'NX-36..NX-47',
        'NX-47 v115..v133',
    }


def test_v133_strict_no_fallback_raises_when_no_pairs():
    kernel = NX47V133Kernel.__new__(NX47V133Kernel)
    kernel.cfg = SimpleNamespace(supervised_train=True, strict_no_fallback=True)
    kernel.log = lambda *args, **kwargs: None
    kernel.discover_train_pairs = lambda: []

    try:
        NX47V133Kernel.build_supervised_model(kernel)
    except RuntimeError as exc:
        assert 'STRICT_NO_FALLBACK' in str(exc)
    else:
        raise AssertionError('Expected STRICT_NO_FALLBACK when no supervised pairs are found')
