import json
import subprocess
from pathlib import Path


def test_fusion_cli_supports_baseline_neutralized_and_qubit_width(tmp_path: Path):
    out = tmp_path / "forensic.jsonl"
    summary = tmp_path / "summary.json"

    bin_path = tmp_path / "fusion_cli_v3"
    compile_cmd = [
        "gcc", "-I./src", "-I./src/advanced_calculations", "-I./src/common", "-I./src/lum",
        "src/advanced_calculations/quantum_simulator_v4_staging_next/tools/fusion_cli_v3.c",
        "src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_simulator_fusion_v3.c",
        "-lm", "-o", str(bin_path)
    ]
    subprocess.run(compile_cmd, check=True)

    subprocess.run([
        str(bin_path), str(out), "baseline_neutralized", "1", "10", "20", "42", str(summary)
    ], check=True)

    j = json.loads(summary.read_text(encoding="utf-8"))
    assert j["mode"] == "baseline_neutralized"
    assert j["max_qubits_width"] == 42
    assert j["nqubits_simulated"] == 200
