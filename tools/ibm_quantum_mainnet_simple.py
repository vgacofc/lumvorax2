#!/usr/bin/env python3
"""
Script simplifié pour lancer jobs IBM Quantum avec clés Bitcoin mainnet
Basé sur la méthode qui fonctionne (ibm_quantum_unit_test.py)
"""

import sys
import json
import hashlib
import requests
from pathlib import Path
from qiskit import QuantumCircuit, transpile
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2
import numpy as np

class BitcoinMainnetFetcher:
    """Récupère des adresses Bitcoin réelles de la mainnet"""
    
    def __init__(self):
        self.api_url = "https://blockchain.info"
    
    def fetch_recent_addresses(self, limit=10):
        """Récupère les adresses du dernier bloc"""
        print(f"\n[1/5] Récupération {limit} adresses Bitcoin mainnet...")
        try:
            # Récupérer le dernier bloc
            response = requests.get(f"{self.api_url}/latestblock", timeout=10)
            latest_block = response.json()
            block_hash = latest_block['hash']
            print(f"  ✓ Dernier bloc: {block_hash[:16]}...")
            
            # Récupérer les transactions du bloc
            response = requests.get(f"{self.api_url}/rawblock/{block_hash}", timeout=10)
            block_data = response.json()
            
            addresses = []
            for tx in block_data['tx'][:limit]:
                for out in tx['out']:
                    if 'addr' in out and out['value'] > 0:
                        addresses.append({
                            'address': out['addr'],
                            'value_btc': out['value'] / 100000000,
                            'tx_hash': tx['hash']
                        })
                        if len(addresses) >= limit:
                            break
                if len(addresses) >= limit:
                    break
            
            print(f"  ✓ {len(addresses)} adresses récupérées")
            for i, addr in enumerate(addresses[:3]):
                print(f"    [{i+1}] {addr['address'][:20]}... ({addr['value_btc']:.8f} BTC)")
            
            return addresses
        
        except Exception as e:
            print(f"  ✗ Erreur: {e}")
            return []

def create_circuit_from_address(address, nonce=0):
    """Crée un circuit quantique basé sur une adresse Bitcoin"""
    # Hash de l'adresse + nonce
    data = f"{address}{nonce}".encode()
    hash_bytes = hashlib.sha256(data).digest()
    
    # Utiliser 2 qubits (simple pour test)
    num_qubits = 2
    qc = QuantumCircuit(num_qubits)
    
    # État initial basé sur hash
    angle = (hash_bytes[0] / 255.0) * np.pi
    qc.ry(angle, 0)
    
    # Entanglement
    qc.cx(0, 1)
    
    # Rotation finale
    angle2 = (hash_bytes[1] / 255.0) * np.pi
    qc.ry(angle2, 1)
    
    return qc

def create_observable(num_qubits=2):
    """Crée un observable simple"""
    # Observable Z sur tous les qubits
    pauli_str = "Z" * num_qubits
    obs = SparsePauliOp([pauli_str], coeffs=[1.0])
    return obs

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--dry-run', action='store_true', help='Simulation locale')
    parser.add_argument('--addresses', type=int, default=2, help='Nombre d\'adresses')
    args = parser.parse_args()
    
    print("="*70)
    print("IBM QUANTUM - BITCOIN MAINNET RUNNER")
    print("="*70)
    
    # Récupérer adresses mainnet
    fetcher = BitcoinMainnetFetcher()
    addresses = fetcher.fetch_recent_addresses(limit=args.addresses)
    
    if not addresses:
        print("\n✗ Impossible de récupérer les adresses")
        return 1
    
    # Créer circuits
    print(f"\n[2/5] Création circuits quantiques...")
    circuits = []
    for i, addr_data in enumerate(addresses):
        qc = create_circuit_from_address(addr_data['address'], nonce=i)
        circuits.append((qc, addr_data))
        print(f"  ✓ Circuit {i+1}: {qc.num_qubits}q, {qc.size()} gates")
    
    # Créer observable
    print(f"\n[3/5] Création observable...")
    obs = create_observable(num_qubits=2)
    print(f"  ✓ Observable: {obs.num_qubits}q")
    
    if args.dry_run:
        print(f"\n[4/5] Mode DRY-RUN - Simulation locale")
        print(f"  ✓ {len(circuits)} circuits prêts")
        print(f"  ✓ Observable prêt")
        print(f"\n✓ DRY-RUN réussi - Prêt pour soumission IBM")
        return 0
    
    # Connexion IBM
    print(f"\n[4/5] Connexion IBM Quantum...")
    try:
        service = QiskitRuntimeService(channel="ibm_quantum_platform")
        print(f"  ✓ Service connecté")
        
        backend = service.backend("ibm_fez")
        print(f"  ✓ Backend: {backend.name} ({backend.num_qubits}q)")
    except Exception as e:
        print(f"  ✗ Erreur connexion: {e}")
        return 1
    
    # Transpilation et soumission
    print(f"\n[5/5] Transpilation et soumission...")
    jobs = []
    
    for i, (qc, addr_data) in enumerate(circuits):
        try:
            # Transpiler
            qc_t = transpile(qc, backend=backend, optimization_level=1)
            print(f"  ✓ Circuit {i+1} transpiré: {qc_t.size()} gates")
            
            # Étendre observable
            if hasattr(qc_t, 'layout') and qc_t.layout is not None:
                obs_extended = obs.apply_layout(qc_t.layout)
            else:
                obs_extended = obs
            
            # Créer pub
            pub = (qc_t, obs_extended)
            
            # Soumettre
            estimator = EstimatorV2(backend)
            job = estimator.run([pub], precision=0.01)
            
            job_id = job.job_id()
            print(f"  ✓ Job {i+1} soumis: {job_id}")
            
            jobs.append({
                'job_id': job_id,
                'address': addr_data['address'],
                'value_btc': addr_data['value_btc']
            })
            
        except Exception as e:
            print(f"  ✗ Erreur job {i+1}: {e}")
    
    # Sauvegarder job IDs
    if jobs:
        log_dir = Path("src/advanced_calculations/bitcoin_quantum_mining/logs/log_job_ibm")
        log_dir.mkdir(parents=True, exist_ok=True)
        
        log_file = log_dir / "mainnet_jobs.json"
        with open(log_file, 'w') as f:
            json.dump(jobs, f, indent=2)
        
        print(f"\n✓ {len(jobs)} jobs soumis avec succès")
        print(f"✓ Job IDs sauvegardés: {log_file}")
        
        for job in jobs:
            print(f"\n  Job: {job['job_id']}")
            print(f"  Adresse: {job['address'][:30]}...")
            print(f"  Valeur: {job['value_btc']:.8f} BTC")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
