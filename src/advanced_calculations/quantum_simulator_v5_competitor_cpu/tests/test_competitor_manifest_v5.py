import json
from pathlib import Path


def test_manifest_has_five_replit_supported_cpu_competitors():
    p = Path("src/advanced_calculations/quantum_simulator_v5_competitor_cpu/tools/competitors_cpu_manifest.json")
    data = json.loads(p.read_text())
    assert data["version"] == "v5-cpu-1"
    comps = data["competitors"]
    assert len(comps) == 5
    names = {c["name"] for c in comps}
    assert names == {
        "Qiskit Aer",
        "quimb",
        "Qulacs",
        "MQT DDSIM",
        "QuTiP",
    }
    for c in comps:
        assert c["repo"].startswith("https://github.com/")
        assert c["pip_package"]
        assert c["import_name"]
