#!/usr/bin/env python3
"""
Récupérer résultats job IBM Quantum
"""

import sys
import json
from qiskit_ibm_runtime import QiskitRuntimeService

def retrieve_job_results(job_id: str):
    """Récupérer résultats d'un job IBM Quantum"""
    
    print(f"\n{'='*60}")
    print(f"RÉCUPÉRATION RÉSULTATS JOB IBM QUANTUM")
    print(f"{'='*60}\n")
    
    # Connexion
    print(f"[1/4] Connexion IBM Quantum...")
    service = QiskitRuntimeService()
    
    # Récupérer job
    print(f"[2/4] Récupération job {job_id}...")
    job = service.job(job_id)
    
    # Statut
    status = job.status()
    print(f"  Statut: {status}")
    
    # Résultats
    print(f"[3/4] Extraction résultats...")
    result = job.result()
    
    # Analyser
    pub_result = result[0]
    data_dict = pub_result.data.__dict__
    meas_key = list(data_dict.keys())[0]
    counts = getattr(pub_result.data, meas_key).get_counts()
    
    # Trouver état dominant
    max_state = max(counts, key=counts.get)
    max_count = counts[max_state]
    total_shots = sum(counts.values())
    max_prob = max_count / total_shots
    
    print(f"\n[4/4] Résultats:")
    print(f"  Total shots: {total_shots}")
    print(f"  États mesurés: {len(counts)}")
    print(f"  État dominant: {max_state} ({max_count} counts, {max_prob*100:.2f}%)")
    
    # Top 10 états
    print(f"\n  Top 10 états:")
    sorted_counts = sorted(counts.items(), key=lambda x: x[1], reverse=True)[:10]
    for state, count in sorted_counts:
        prob = count / total_shots * 100
        print(f"    {state}: {count} ({prob:.2f}%)")
    
    # Sauvegarder
    output = {
        'job_id': job_id,
        'status': str(status),
        'total_shots': total_shots,
        'num_states': len(counts),
        'dominant_state': max_state,
        'dominant_count': max_count,
        'dominant_probability': max_prob,
        'all_counts': counts,
        'top_10': sorted_counts
    }
    
    output_file = f'test_results_c154/ibm_job_{job_id}_results.json'
    with open(output_file, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"\n  Résultats sauvegardés: {output_file}")
    print(f"\n{'='*60}\n")
    
    return output

if __name__ == '__main__':
    job_id = sys.argv[1] if len(sys.argv) > 1 else 'd7r56vvljm6s73b9v5hg'
    retrieve_job_results(job_id)

# Made with Bob
