#!/usr/bin/env python3
"""
Script pour lancer jobs IBM Quantum avec clés Bitcoin mainnet
SUR LE BON CRN: crn:v1:bluemix:public:quantum-computing:us-east:a/b893e3b64af743f9860d3ae175578d19:31cb02b4-5010-422a-a999-d6302e4d7b06::
"""

import sys
import json
import hashlib
import requests
import time
from pathlib import Path
from qiskit import QuantumCircuit, transpile
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2
import numpy as np

# Configuration CRN Ubuntu 9
CRN_INSTANCE = "crn:v1:bluemix:public:quantum-computing:us-east:a/b893e3b64af743f9860d3ae175578d19:31cb02b4-5010-422a-a999-d6302e4d7b06::"
ACCOUNT_ID = "b893e3b64af743f9860d3ae175578d19"

class BitcoinMainnetFetcher:
    """Récupère des adresses Bitcoin réelles de la mainnet"""
    
    def __init__(self):
        self.api_url = "https://blockchain.info"
    
    def fetch_recent_addresses(self, limit=10):
        """Récupère les adresses du dernier bloc"""
        print(f"\n[1/6] Récupération {limit} adresses Bitcoin mainnet...")
        try:
            # Récupérer le dernier bloc
            response = requests.get(f"{self.api_url}/latestblock", timeout=10)
            latest_block = response.json()
            block_hash = latest_block['hash']
            block_height = latest_block['height']
            print(f"  ✓ Dernier bloc: #{block_height} - {block_hash[:16]}...")
            
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
                            'tx_hash': tx['hash'],
                            'block_height': block_height,
                            'block_hash': block_hash
                        })
                        if len(addresses) >= limit:
                            break
                if len(addresses) >= limit:
                    break
            
            print(f"  ✓ {len(addresses)} adresses récupérées")
            for i, addr in enumerate(addresses[:3]):
                print(f"    [{i+1}] {addr['address'][:30]}... ({addr['value_btc']:.8f} BTC)")
            
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

def log_forensic(job_id, address_data, circuit, observable, backend, log_dir):
    """Log forensique complet"""
    job_dir = log_dir / f"job-{job_id}"
    job_dir.mkdir(parents=True, exist_ok=True)
    
    # Info job
    info = {
        "job_id": job_id,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "crn_instance": CRN_INSTANCE,
        "account_id": ACCOUNT_ID,
        "address": address_data['address'],
        "value_btc": address_data['value_btc'],
        "block_height": address_data['block_height'],
        "block_hash": address_data['block_hash'],
        "tx_hash": address_data['tx_hash'],
        "backend": backend.name,
        "num_qubits": circuit.num_qubits,
        "circuit_depth": circuit.depth(),
        "circuit_size": circuit.size(),
        "observable": str(observable),
        "optimization_level": 1
    }
    
    with open(job_dir / f"job-{job_id}-info.json", 'w') as f:
        json.dump(info, f, indent=2)
    
    # Circuit QASM (Qiskit 1.x utilise qasm3)
    try:
        from qiskit import qasm3
        with open(job_dir / f"job-{job_id}-circuit.qasm", 'w') as f:
            f.write(qasm3.dumps(circuit))
    except Exception as e:
        # Fallback: sauvegarder la représentation string
        with open(job_dir / f"job-{job_id}-circuit.txt", 'w') as f:
            f.write(str(circuit))
    
    print(f"  ✓ Log forensique: {job_dir}")
    return job_dir

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--dry-run', action='store_true', help='Simulation locale')
    parser.add_argument('--addresses', type=int, default=2, help='Nombre d\'adresses')
    parser.add_argument('--token', help='Token IBM (ou depuis Doppler)')
    args = parser.parse_args()
    
    print("="*70)
    print("IBM QUANTUM - BITCOIN MAINNET RUNNER (CRN UBUNTU 9)")
    print("="*70)
    print(f"CRN: {CRN_INSTANCE[:50]}...")
    print(f"Account ID: {ACCOUNT_ID}")
    print("="*70)
    
    # Récupérer adresses mainnet
    fetcher = BitcoinMainnetFetcher()
    addresses = fetcher.fetch_recent_addresses(limit=args.addresses)
    
    if not addresses:
        print("\n✗ Impossible de récupérer les adresses")
        return 1
    
    # Créer circuits
    print(f"\n[2/6] Création circuits quantiques...")
    circuits = []
    for i, addr_data in enumerate(addresses):
        qc = create_circuit_from_address(addr_data['address'], nonce=i)
        circuits.append((qc, addr_data))
        print(f"  ✓ Circuit {i+1}: {qc.num_qubits}q, {qc.size()} gates, depth={qc.depth()}")
    
    # Créer observable
    print(f"\n[3/6] Création observable...")
    obs = create_observable(num_qubits=2)
    print(f"  ✓ Observable: {obs.num_qubits}q, pauli_str='ZZ'")
    
    if args.dry_run:
        print(f"\n[4/6] Mode DRY-RUN - Simulation locale")
        print(f"  ✓ {len(circuits)} circuits prêts")
        print(f"  ✓ Observable prêt")
        print(f"  ✓ CRN configuré: {CRN_INSTANCE[:50]}...")
        print(f"\n✓ DRY-RUN réussi - Prêt pour soumission IBM sur CRN Ubuntu 9")
        return 0
    
    # Récupérer token
    token = args.token
    if not token:
        import subprocess
        try:
            result = subprocess.run(
                ["doppler", "secrets", "get", "IBM_QUANTUM_TOKEN", "--plain", "--config", "dev_lumvorax"],
                capture_output=True,
                text=True,
                check=True
            )
            token = result.stdout.strip()
            print(f"\n[4/6] Token récupéré depuis Doppler")
        except Exception as e:
            print(f"\n✗ Erreur récupération token Doppler: {e}")
            return 1
    
    # Connexion IBM avec CRN
    print(f"\n[5/6] Connexion IBM Quantum (CRN Ubuntu 9)...")
    try:
        # Sauvegarder compte avec CRN
        QiskitRuntimeService.save_account(
            channel='ibm_cloud',
            token=token,
            instance=CRN_INSTANCE,
            overwrite=True
        )
        print(f"  ✓ Compte sauvegardé avec CRN")
        
        # Se connecter
        service = QiskitRuntimeService(
            channel='ibm_cloud',
            instance=CRN_INSTANCE
        )
        print(f"  ✓ Service connecté")
        print(f"  ✓ Instance: {service._account.instance}")
        
        backend = service.backend("ibm_fez")
        print(f"  ✓ Backend: {backend.name} ({backend.num_qubits}q)")
    except Exception as e:
        print(f"  ✗ Erreur connexion: {e}")
        return 1
    
    # Préparer logging
    log_dir = Path("src/advanced_calculations/bitcoin_quantum_mining/logs/log_job_ibm")
    log_dir.mkdir(parents=True, exist_ok=True)
    
    # Transpilation et soumission
    print(f"\n[6/6] Transpilation et soumission sur CRN Ubuntu 9...")
    jobs = []
    
    for i, (qc, addr_data) in enumerate(circuits):
        try:
            # Transpiler
            qc_t = transpile(qc, backend=backend, optimization_level=1)
            print(f"  ✓ Circuit {i+1} transpiré: {qc_t.size()} gates, depth={qc_t.depth()}")
            
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
            
            # Log forensique
            job_dir = log_forensic(job_id, addr_data, qc_t, obs_extended, backend, log_dir)
            
            jobs.append({
                'job_id': job_id,
                'address': addr_data['address'],
                'value_btc': addr_data['value_btc'],
                'block_height': addr_data['block_height'],
                'block_hash': addr_data['block_hash'],
                'log_dir': str(job_dir)
            })
            
        except Exception as e:
            print(f"  ✗ Erreur job {i+1}: {e}")
    
    # Sauvegarder job IDs
    if jobs:
        log_file = log_dir / "mainnet_jobs_crn_ubuntu9.json"
        with open(log_file, 'w') as f:
            json.dump({
                'timestamp': time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                'crn_instance': CRN_INSTANCE,
                'account_id': ACCOUNT_ID,
                'jobs': jobs
            }, f, indent=2)
        
        print(f"\n{'='*70}")
        print(f"✓ {len(jobs)} jobs soumis avec succès sur CRN Ubuntu 9")
        print(f"✓ Job IDs sauvegardés: {log_file}")
        print(f"{'='*70}")
        
        for job in jobs:
            print(f"\n  Job: {job['job_id']}")
            print(f"  Adresse: {job['address'][:40]}...")
            print(f"  Valeur: {job['value_btc']:.8f} BTC")
            print(f"  Bloc: #{job['block_height']}")
            print(f"  Log: {job['log_dir']}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
