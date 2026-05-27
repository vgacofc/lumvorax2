import json
import sys
from pathlib import Path

import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from nx47_vesu_kernel_v2 import FatalPipelineError, NX47_VESU_Production


def test_fail_fast_on_missing_test_directory(tmp_path):
    model = NX47_VESU_Production(input_dir=str(tmp_path / "missing_input"), output_dir=str(tmp_path / "out"))
    try:
        model.process_fragments()
    except FatalPipelineError as exc:
        assert "INPUT_STRUCTURE_INVALID" in str(exc)
    else:
        raise AssertionError("Expected FatalPipelineError when /test directory is missing")


def test_pipeline_outputs_submission_and_roadmap(tmp_path):
    input_dir = tmp_path / "input"
    test_dir = input_dir / "test"
    test_dir.mkdir(parents=True)
    (test_dir / "frag_001").mkdir()
    (test_dir / "frag_002").mkdir()

    output_dir = tmp_path / "out"
    model = NX47_VESU_Production(input_dir=str(input_dir), output_dir=str(output_dir))
    stats = model.process_fragments()

    submission_path = output_dir / "submission.parquet"
    roadmap_path = output_dir / "v132_roadmap_realtime.json"
    logs_path = output_dir / "v132_execution_logs.json"
    metadata_path = output_dir / "v132_execution_metadata.json"

    assert submission_path.exists()
    assert roadmap_path.exists()
    assert logs_path.exists()
    assert metadata_path.exists()

    df = pd.read_parquet(submission_path)
    assert set(df.columns) == {"id", "target"}
    assert len(df) == 2

    roadmap = json.loads(roadmap_path.read_text(encoding="utf-8"))
    assert roadmap["current_step"] == "finalize"
    assert roadmap["overall_progress_percent"] == 100.0

    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    assert metadata["fallback_disabled"] is True
    assert metadata["merkle_root"]

    assert stats["files_processed"] == 2
    assert stats["files_autonomous_fallback"] == 0
