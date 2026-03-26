import json
from pathlib import Path


def test_competitor_protocol_has_top10_and_required_fields():
    p = Path('src/advanced_calculations/quantum_simulator_v3_staging/competitor_benchmark_protocol_v1.json')
    data = json.loads(p.read_text(encoding='utf-8'))

    competitors = data['competitors']
    assert len(competitors) == 10, 'Le protocole doit couvrir exactement 10 concurrents'

    required_metrics = set(data['required_metrics'])
    must_have = {
        'throughput_nqubits_per_sec',
        'latency_p95_ns',
        'latency_p99_ns',
        'max_memory_mb',
        'fidelity_proxy',
        'success_rate',
    }
    assert must_have.issubset(required_metrics)

    for c in competitors:
        assert {'name', 'method', 'target_qubits'}.issubset(c.keys())
        assert c['name'].strip()
        assert c['method'].strip()
        assert c['target_qubits'].strip()
