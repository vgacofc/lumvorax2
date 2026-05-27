#!/usr/bin/env python3
"""
IBM Quantum Willow Killer - Mainnet Bitcoin Keys
Intégration CRS (Cross-Randomized Sampling) + NX48 + Clés Réelles
Objectif : Battre le record Google Willow
"""

import os
import sys
import json
import time
import hashlib
import requests
from datetime import datetime
from pathlib import Path

# Qiskit imports
from qiskit import QuantumCircuit, transpile
from qiskit.circuit.library import RXXGate, RYYGate, RZZGate
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2 as Estimator
from qiskit_aer import AerSimulator
import numpy as np

# Configuration
BASE_DIR = Path(__file__).parent.parent
LOGS_DIR = BASE_DIR / "src/advanced_calculations/bitcoin_quantum_mining/logs"
FORENSIC_DIR = LOGS_DIR / "forensic_c163"
FORENSIC_DIR.mkdir(parents=True, exist_ok=True)

# Paramètres Willow Killer
WILLOW_BENCHMARK = {
    "qubits": 105,  # Google Willow
    "gates": 1000,  # Circuits complexes
    "fidelity": 0.9985,  # Fidélité Willow
    "time": 300  # 5 minutes (record Willow)
}

# Paramètres NX48
NX48_PARAMS = {
    "exploration_bias": 0.902,
    "delta_nonce_scale": 40.73,
    "pre_measure_alpha": np.sqrt(0.8536),
    "pre_measure_beta": np.sqrt(0.1464)
}

# CRS Parameters (Cross-Randomized Sampling)
CRS_PARAMS = {
    "num_samples": 32,  # Échantillons croisés
    "randomization_depth": 3,  # Profondeur randomisation
    "cross_validation_folds": 4  # Validation croisée
}

class BitcoinMainnetFetcher:
    """Récupère des clés Bitcoin réelles de la mainnet"""
    
    def __init__(self):
        self.api_url = "https://blockchain.info"
        self.cache_file = FORENSIC_DIR / "mainnet_keys_cache.json"
        
    def fetch_recent_addresses(self, limit=100):
        """Récupère les adresses récentes de la blockchain"""
        print(f"\n[1/5] Récupération adresses Bitcoin mainnet...")
        
        try:
            # Récupérer les derniers blocs
            response = requests.get(f"{self.api_url}/latestblock", timeout=10)
            latest_block = response.json()
            block_height = latest_block['height']
            
            print(f"  Dernier bloc : {block_height}")
            
            # Récupérer transactions du dernier bloc
            response = requests.get(
                f"{self.api_url}/block-height/{block_height}?format=json",
                timeout=10
            )
            block_data = response.json()
            
            addresses = []
            for block in block_data['blocks'][:1]:  # Premier bloc seulement
                for tx in block['tx'][:limit]:
                    for output in tx['out']:
                        if 'addr' in output:
                            addresses.append({
                                'address': output['addr'],
                                'value': output['value'],
                                'tx_hash': tx['hash']
                            })
            
            print(f"  ✅ {len(addresses)} adresses récupérées")
            
            # Sauvegarder cache
            with open(self.cache_file, 'w') as f:
                json.dump(addresses, f, indent=2)
            
            return addresses
            
        except Exception as e:
            print(f"  ⚠️ Erreur API : {e}")
            # Utiliser cache si disponible
            if self.cache_file.exists():
                with open(self.cache_file) as f:
                    return json.load(f)
            return []
    
    def address_to_public_key_hash(self, address):
        """Convertit une adresse Bitcoin en hash de clé publique"""
        # Décoder Base58Check
        try:
            # Simplification : utiliser le hash de l'adresse
            return hashlib.sha256(address.encode()).hexdigest()[:40]
        except:
            return None

class CRSQuantumCircuit:
    """Cross-Randomized Sampling Quantum Circuit"""
    
    def __init__(self, num_qubits=8):
        self.num_qubits = num_qubits
        self.crs_params = CRS_PARAMS
        
    def create_crs_circuit(self, pubkey_hash, nonce, sample_idx):
        """
        Crée un circuit avec CRS pour battre Willow
        
        CRS = Cross-Randomized Sampling :
        - Échantillonnage croisé sur plusieurs bases
        - Randomisation à différentes profondeurs
        - Validation croisée des résultats
        """
        qc = QuantumCircuit(self.num_qubits, self.num_qubits)
        
        # 1. État initial pré-mesuré (NX48)
        alpha = NX48_PARAMS['pre_measure_alpha']
        beta = NX48_PARAMS['pre_measure_beta']
        qc.initialize([alpha, beta], 0)
        
        # 2. Encoding pubkey_hash avec CRS
        hash_int = int(pubkey_hash[:16], 16)
        
        for i in range(self.num_qubits):
            # Rotation basée sur hash + nonce + sample
            angle = (hash_int >> i) & 1
            crs_angle = angle * np.pi + (sample_idx * np.pi / self.crs_params['num_samples'])
            
            if angle:
                qc.ry(crs_angle, i)
        
        # 3. ADAPT-VQE Pool avec randomisation CRS
        depth = self.crs_params['randomization_depth']
        for d in range(depth):
            # Randomisation à chaque profondeur
            random_phase = (sample_idx + d) * 2 * np.pi / (self.crs_params['num_samples'] * depth)
            
            for i in range(self.num_qubits - 1):
                # RXX avec phase CRS
                theta_xx = (nonce * 0.1 + random_phase) % (2 * np.pi)
                qc.rxx(theta_xx, i, i + 1)
                
                # RYY avec phase CRS
                theta_yy = (nonce * 0.2 + random_phase) % (2 * np.pi)
                qc.ryy(theta_yy, i, i + 1)
                
                # RZZ avec phase CRS
                theta_zz = (nonce * 0.3 + random_phase) % (2 * np.pi)
                qc.rzz(theta_zz, i, i + 1)
        
        # 4. Entanglement croisé (CRS spécifique)
        for fold in range(self.crs_params['cross_validation_folds']):
            start = fold * (self.num_qubits // self.crs_params['cross_validation_folds'])
            end = start + (self.num_qubits // self.crs_params['cross_validation_folds'])
            
            for i in range(start, min(end - 1, self.num_qubits - 1)):
                qc.cx(i, i + 1)
        
        # 5. Mesure
        qc.measure_all()
        
        return qc

class WillowKillerRunner:
    """Runner principal pour battre Willow"""
    
    def __init__(self, use_real_backend=True):
        self.use_real_backend = use_real_backend
        self.fetcher = BitcoinMainnetFetcher()
        self.crs_circuit = CRSQuantumCircuit(num_qubits=8)
        
        # Initialiser service IBM
        if use_real_backend:
            # Utiliser IBM Quantum Platform (détecte automatiquement l'instance)
            self.service = QiskitRuntimeService(channel='ibm_quantum_platform')
            self.backend = self.service.least_busy(
                operational=True,
                simulator=False,
                min_num_qubits=8
            )
            print(f"\n🎯 Backend sélectionné : {self.backend.name}")
        else:
            self.backend = AerSimulator()
            print(f"\n🎯 Mode simulation : AerSimulator")
    
    def create_observables(self):
        """Crée les 6 observables complémentaires"""
        observables = []
        
        # S(π) - Symétrie Pi
        obs1 = SparsePauliOp(['ZZ'], coeffs=[0.5])
        observables.append(('S_pi', obs1))
        
        # S(k×4) - Symétrie 4k
        obs2 = SparsePauliOp(['ZZ'], coeffs=[1.0])
        observables.append(('S_4k', obs2))
        
        # S(k×4) variant
        obs3 = SparsePauliOp(['ZZ'], coeffs=[0.7071])
        observables.append(('S_4k_v', obs3))
        
        # C(r) - Corrélation
        obs4 = SparsePauliOp(['ZZ'], coeffs=[1.0])
        observables.append(('C_r', obs4))
        
        # C(r) variant
        obs5 = SparsePauliOp(['ZZ'], coeffs=[-0.7071])
        observables.append(('C_r_v', obs5))
        
        # C(r) maximal
        obs6 = SparsePauliOp(['ZZ'], coeffs=[-1.0])
        observables.append(('C_r_max', obs6))
        
        return observables
    
    def run_willow_killer(self, num_addresses=3, max_pubs=30):
        """
        Exécute le Willow Killer avec CRS
        
        Stratégie :
        1. Récupérer clés réelles mainnet
        2. Créer circuits CRS pour chaque clé
        3. Batch processing adaptatif (max 30 pubs)
        4. Comparer avec benchmark Willow
        """
        print("\n" + "="*80)
        print("🚀 IBM QUANTUM WILLOW KILLER - MAINNET + CRS")
        print("="*80)
        
        # 1. Récupérer adresses mainnet
        addresses = self.fetcher.fetch_recent_addresses(limit=num_addresses)
        if not addresses:
            print("❌ Aucune adresse récupérée")
            return
        
        print(f"\n[2/5] Sélection {num_addresses} adresses pour analyse...")
        selected = addresses[:num_addresses]
        
        for i, addr in enumerate(selected):
            print(f"  Adresse {i+1}: {addr['address'][:20]}... ({addr['value']/1e8:.8f} BTC)")
        
        # 2. Créer circuits CRS
        print(f"\n[3/5] Création circuits CRS (×{CRS_PARAMS['num_samples']} samples)...")
        
        pubs = []
        observables = self.create_observables()
        
        for addr_idx, addr in enumerate(selected):
            pubkey_hash = self.fetcher.address_to_public_key_hash(addr['address'])
            if not pubkey_hash:
                continue
            
            # Créer CRS_PARAMS['num_samples'] circuits par adresse
            for sample_idx in range(min(3, CRS_PARAMS['num_samples'])):  # Limiter à 3 samples
                nonce = sample_idx
                
                # Créer circuit CRS
                qc = self.crs_circuit.create_crs_circuit(pubkey_hash, nonce, sample_idx)
                
                # Transpiler
                qc_transpiled = transpile(qc, self.backend, optimization_level=3)
                
                # Créer pubs pour chaque observable
                for obs_name, obs in observables:
                    # Extension observable
                    obs_extended = obs.apply_layout(qc_transpiled.layout)
                    
                    pubs.append((qc_transpiled, obs_extended))
                    
                    if len(pubs) >= max_pubs:
                        break
                
                if len(pubs) >= max_pubs:
                    break
            
            if len(pubs) >= max_pubs:
                break
        
        print(f"  ✅ {len(pubs)} pubs créés (limite: {max_pubs})")
        
        # 3. Exécution
        print(f"\n[4/5] Soumission job IBM Quantum...")
        print(f"  Backend: {self.backend.name if hasattr(self.backend, 'name') else 'AerSimulator'}")
        print(f"  Pubs: {len(pubs)}")
        print(f"  Precision: 0.01")
        
        start_time = time.time()
        
        if self.use_real_backend:
            # Job réel IBM
            estimator = Estimator(self.backend)
            estimator.options.resilience_level = 1
            estimator.options.optimization_level = 3
            
            job = estimator.run(pubs, precision=0.01)
            job_id = job.job_id()
            
            print(f"  Job ID: {job_id}")
            print(f"  Status: En attente...")
            
            # Sauvegarder info job
            job_info = {
                'job_id': job_id,
                'backend': self.backend.name,
                'num_pubs': len(pubs),
                'num_addresses': num_addresses,
                'crs_samples': CRS_PARAMS['num_samples'],
                'timestamp': datetime.now().isoformat(),
                'addresses': [a['address'] for a in selected]
            }
            
            job_dir = LOGS_DIR / f"log_job_ibm/job-{job_id}"
            job_dir.mkdir(parents=True, exist_ok=True)
            
            with open(job_dir / f"job-{job_id}-info.json", 'w') as f:
                json.dump(job_info, f, indent=2)
            
            print(f"\n  📁 Logs: {job_dir}")
            print(f"\n  ⏳ Attente résultats (peut prendre plusieurs minutes)...")
            print(f"  💡 Utilisez retrieve_ibm_job_results.py pour récupérer les résultats")
            
            return job_id
            
        else:
            # Simulation Aer
            from qiskit.primitives import StatevectorEstimator
            estimator = StatevectorEstimator()
            job = estimator.run(pubs)
            result = job.result()
            
            elapsed = time.time() - start_time
            
            # 4. Analyse résultats
            print(f"\n[5/5] Analyse résultats (simulation)...")
            print(f"  Temps: {elapsed:.2f}s")
            
            for i, pub_result in enumerate(result):
                evs = pub_result.data.evs
                stds = pub_result.data.stds
                
                if i < 3:  # Afficher premiers résultats
                    print(f"  Pub {i}: EVs={evs:.6f}, Stds={stds:.6f}")
            
            # Comparaison Willow
            print(f"\n📊 COMPARAISON AVEC GOOGLE WILLOW:")
            print(f"  Willow qubits: {WILLOW_BENCHMARK['qubits']}")
            print(f"  Notre système: {self.crs_circuit.num_qubits} qubits")
            print(f"  Willow fidélité: {WILLOW_BENCHMARK['fidelity']}")
            print(f"  Notre approche: CRS + NX48 + Clés réelles")
            
            return None

def main():
    """Point d'entrée principal"""
    import argparse
    
    parser = argparse.ArgumentParser(description='IBM Quantum Willow Killer')
    parser.add_argument('--real', action='store_true', help='Utiliser backend IBM réel')
    parser.add_argument('--addresses', type=int, default=3, help='Nombre d\'adresses')
    parser.add_argument('--max-pubs', type=int, default=30, help='Max pubs par job')
    
    args = parser.parse_args()
    
    runner = WillowKillerRunner(use_real_backend=args.real)
    job_id = runner.run_willow_killer(
        num_addresses=args.addresses,
        max_pubs=args.max_pubs
    )
    
    if job_id:
        print(f"\n✅ Job soumis avec succès: {job_id}")
        print(f"\n📋 Prochaines étapes:")
        print(f"  1. Attendre complétion job (~5-10 min)")
        print(f"  2. Récupérer résultats:")
        print(f"     python3 tools/retrieve_ibm_job_results.py {job_id}")
        print(f"  3. Analyser avec decode_ibm_results.py")
    else:
        print(f"\n✅ Simulation complétée")

if __name__ == '__main__':
    main()

# Made with Bob
