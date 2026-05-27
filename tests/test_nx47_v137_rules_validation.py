import sys
import zipfile
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v137 import NX47V137Kernel


def test_v137_rules_validation_ok(tmp_path):
    k = NX47V137Kernel.__new__(NX47V137Kernel)
    k.cfg = SimpleNamespace(
        enforce_competition_rules=True,
        competition_rules_path=str(tmp_path / 'rules.md'),
        metric_demo_notebook_path=str(tmp_path / 'demo.ipynb'),
    )
    Path(k.cfg.competition_rules_path).write_text('rules')
    Path(k.cfg.metric_demo_notebook_path).write_text('{}')
    k.log = lambda *args, **kwargs: None
    k.submission_path = tmp_path / 'submission.zip'
    with zipfile.ZipFile(k.submission_path, 'w') as zf:
        zf.writestr('a.tif', b'data')
    expected = [tmp_path / 'a.tif']
    res = NX47V137Kernel._validate_submission_competition_rules(k, expected)
    assert res['status'] == 'ok'
    assert res['submission_tif_files'] == 1


def test_v137_rules_validation_mismatch_raises(tmp_path):
    k = NX47V137Kernel.__new__(NX47V137Kernel)
    k.cfg = SimpleNamespace(enforce_competition_rules=True, competition_rules_path='x', metric_demo_notebook_path='y')
    k.log = lambda *args, **kwargs: None
    k.submission_path = tmp_path / 'submission.zip'
    with zipfile.ZipFile(k.submission_path, 'w') as zf:
        zf.writestr('a.tif', b'data')
    expected = [tmp_path / 'b.tif']
    try:
        NX47V137Kernel._validate_submission_competition_rules(k, expected)
    except RuntimeError as exc:
        assert 'RULES_VALIDATION_FAILED' in str(exc)
    else:
        raise AssertionError('Expected rules validation failure')
