import json
from pathlib import Path


def test_unified_protocol_is_strict_and_well_formed():
    p = Path("src/advanced_calculations/quantum_simulator_v5_competitor_cpu/tools/unified_benchmark_protocol_v5.json")
    data = json.loads(p.read_text())

    assert data["version"] == "v5-strict-unified-1"
    assert data["circuit_family"] == "ghz"

    workloads = data["workloads"]
    assert len(workloads) >= 3

    expected_qubits = [4, 8, 12]
    assert [w["qubits"] for w in workloads] == expected_qubits
    assert all(w["circuit"] == "ghz" for w in workloads)

    shots = {w["shots"] for w in workloads}
    assert shots == {256}
