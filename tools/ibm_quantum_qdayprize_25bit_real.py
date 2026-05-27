#!/usr/bin/env python3
"""
QDAYPRIZE 25-bit Real - IBM Quantum Hardware
Exécution réelle sur IBM Quantum (156 qubits)
Casse clés Bitcoin 25-bit avec Grover Search

Usage:
    python3 ibm_quantum_qdayprize_25bit_real.py --keys 5000 --shots 1024
"""

import argparse
import json
import time
from datetime import datetime
from pathlib import Path

from qiskit import QuantumCircuit, transpile
from qiskit_ibm_runtime import QiskitRuntimeService, Sampler, Options

def create_grover_circuit_25bit(target_key: int, num_qubits: int = 25) -> QuantumCircuit:
    """
    Créer circuit Grover pour recherche clé 25-bit
    
    Args:
        target_key: Clé cible (0-33554431)
        num_qubits: Nombre de qubits (25 pour 25-bit)
    
    Returns:
        Circuit quantique Grover
    """
    qc = QuantumCircuit(num_qubits, num_qubits)
    
    # Initialisation superposition
    qc.h(range(num_qubits))
    
    # Oracle pour target_key
    target_binary = format(target_key, f'0{num_qubits}b')
    
    # Marquer état cible
    for i, bit in enumerate(target_binary):
        if bit == '0':
            qc.x(i)
    
    # Multi-controlled Z
    qc.h(num_qubits - 1)
    qc.mcx(list(range(num_qubits - 1)), num_qubits - 1)
    qc.h(num_qubits - 1)
    
    # Défaire X
    for i, bit in enumerate(target_binary):
        if bit == '0':
            qc.x(i)
    
    # Diffusion
    qc.h(range(num_qubits))
    qc.x(range(num_qubits))
    qc.h(num_qubits - 1)
    qc.mcx(list(range(num_qubits - 1)), num_qubits - 1)
    qc.h(num_qubits - 1)
    qc.x(range(num_qubits))
    qc.h(range(num_qubits))
    
    # Mesure
    qc.measure(range(num_qubits), range(num_qubits))
    
    return qc

def run_qdayprize_real(num_keys: int = 5000, shots: int = 1024, output_file: str | None = None):
    """
    Exécuter QDAYPRIZE 25-bit sur IBM Quantum réel
    
    Args:
        num_keys: Nombre de clés à tester
        shots: Nombre de mesures par circuit
        output_file: Fichier de sortie JSON
    """
    print(f"\n{'='*60}")
    print(f"QDAYPRIZE 25-BIT - IBM QUANTUM REAL")
    print(f"{'='*60}\n")
    
    # Connexion IBM Quantum
    print("[1/5] Connexion IBM Quantum Platform...")
    try:
        service = QiskitRuntimeService(channel='ibm_quantum_platform')
        backends = service.backends(operational=True, simulator=False)
        
        # Sélectionner backend avec le plus de qubits
        backend = max(backends, key=lambda b: b.configuration().n_qubits)
        print(f"✓ Backend sélectionné: {backend.name} ({backend.configuration().n_qubits} qubits)")
    except Exception as e:
        print(f"✗ Erreur connexion: {e}")
        return
    
    # Configuration
    print(f"\n[2/5] Configuration...")
    print(f"  Clés à tester: {num_keys}")
    print(f"  Shots par clé: {shots}")
    print(f"  Qubits: 25 (25-bit)")
    print(f"  Temps estimé: {num_keys * 2 / 3600:.1f}h (2s/clé)")
    
    # Résultats
    results = {
        'timestamp': datetime.now().isoformat(),
        'backend': backend.name,
        'num_qubits': 25,
        'num_keys': num_keys,
        'shots': shots,
        'keys_tested': [],
        'keys_cracked': [],
        'success_rate': 0.0,
        'total_time_seconds': 0.0
    }
    
    start_time = time.time()
    
    # Tester clés
    print(f"\n[3/5] Test clés (0-{num_keys-1})...")
    
    for key_idx in range(num_keys):
        if key_idx % 100 == 0:
            elapsed = time.time() - start_time
            rate = key_idx / elapsed if elapsed > 0 else 0
            eta = (num_keys - key_idx) / rate if rate > 0 else 0
            print(f"  Progression: {key_idx}/{num_keys} ({key_idx*100/num_keys:.1f}%) - "
                  f"ETA: {eta/3600:.1f}h")
        
        # Créer circuit Grover
        target_key = key_idx % (2**25)  # 0-33554431
        qc = create_grover_circuit_25bit(target_key, num_qubits=25)
        
        # Transpiler
        qc_transpiled = transpile(qc, backend=backend, optimization_level=3)
        
        # Exécuter
        try:
            # SamplerV2 : mode session avec backend
            sampler = Sampler(mode=backend)
            job = sampler.run([qc_transpiled], shots=shots)
            print(f"  ✓ Job soumis: {job.job_id()}")
            result = job.result()
            print(f"  ✓ Résultats reçus")
            
            # Analyser résultats (SamplerV2 API)
            pub_result = result[0]
            # Obtenir les counts depuis les attributs du DataBin
            data_dict = pub_result.data.__dict__
            # Le premier attribut contient les mesures
            meas_key = list(data_dict.keys())[0]
            counts = getattr(pub_result.data, meas_key).get_counts()
            
            # Trouver état dominant
            max_state = max(counts, key=counts.get)
            max_prob = counts[max_state] / shots  # Normaliser
            
            # Vérifier si clé cassée (probabilité >50%)
            if max_prob > 0.5:
                results['keys_cracked'].append({
                    'key': target_key,
                    'found_state': max_state,
                    'probability': max_prob,
                    'job_id': job.job_id()
                })
            
            results['keys_tested'].append({
                'key': target_key,
                'success': max_prob > 0.5,
                'probability': max_prob
            })
            
        except Exception as e:
            print(f"  ✗ Erreur clé {key_idx}: {e}")
            results['keys_tested'].append({
                'key': target_key,
                'success': False,
                'error': str(e)
            })
    
    # Statistiques finales
    elapsed_time = time.time() - start_time
    results['total_time_seconds'] = elapsed_time
    results['success_rate'] = len(results['keys_cracked']) / num_keys * 100
    
    print(f"\n[4/5] Résultats...")
    print(f"  Clés testées: {len(results['keys_tested'])}")
    print(f"  Clés cassées: {len(results['keys_cracked'])}")
    print(f"  Taux succès: {results['success_rate']:.1f}%")
    print(f"  Temps total: {elapsed_time/3600:.2f}h")
    
    # Sauvegarder résultats
    if output_file is None:
        output_file = f"../test_results_c154/qdayprize_c159_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    
    output_path = Path(output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_path, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n[5/5] Résultats sauvegardés: {output_path}")
    print(f"\n{'='*60}")
    print(f"QDAYPRIZE 25-BIT TERMINÉ")
    print(f"{'='*60}\n")

def main():
    parser = argparse.ArgumentParser(description='QDAYPRIZE 25-bit Real - IBM Quantum')
    parser.add_argument('--keys', type=int, default=100, help='Nombre de clés à tester (default: 100)')
    parser.add_argument('--shots', type=int, default=1024, help='Shots par circuit (default: 1024)')
    parser.add_argument('--output', type=str, help='Fichier de sortie JSON')
    
    args = parser.parse_args()
    
    run_qdayprize_real(
        num_keys=args.keys,
        shots=args.shots,
        output_file=args.output
    )

if __name__ == '__main__':
    main()

# Made with Bob
