#!/usr/bin/env python3
"""
EXECUTION FINALE C167 — Multi-QPU Parallel Testing
Utilise les 3 QPUs disponibles + toutes nos technologies
Budget: 98s restants (1m38s)
"""

import hashlib
import numpy as np
from qiskit import QuantumCircuit
from qiskit.transpiler.preset_passmanagers import generate_preset_pass_manager
from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2 as Estimator
from qiskit.quantum_info import SparsePauliOp
import time
import json
from pathlib import Path
from datetime import datetime

class MultiQPUFinalTest:
    def __init__(self):
        self.service = QiskitRuntimeService()
        
        # 3 QPUs disponibles
        self.backends = {
            'ibm_fez': self.service.backend('ibm_fez'),
            'ibm_marrakesh': self.service.backend('ibm_marrakesh'),
            'ibm_kingston': self.service.backend('ibm_kingston')
        }
        
        self.log_dir = Path("src/advanced_calculations/bitcoin_quantum_mining/logs/log_job_ibm")
        self.log_dir.mkdir(parents=True, exist_ok=True)
        
        print("="*80)
        print("EXECUTION FINALE C167 — Multi-QPU Parallel Testing")
        print("="*80)
        print(f"QPUs disponibles: {list(self.backends.keys())}")
        print(f"Budget: ~98s (1m38s)")
        print(f"Stratégie: 1 PUB/job, tests parallèles sur 3 QPUs")
        print("="*80)
    
    def create_circuit_sha256(self, input_str, label=""):
        """Crée circuit à partir de SHA-256"""
        hash_bytes = hashlib.sha256(input_str.encode()).digest()
        
        qc = QuantumCircuit(2, name=f"SHA256_{label}")
        
        # Angles depuis hash
        angle1 = (hash_bytes[0] / 255.0) * np.pi
        angle2 = (hash_bytes[1] / 255.0) * np.pi
        
        qc.ry(angle1, 0)
        qc.cx(0, 1)
        qc.ry(angle2, 1)
        
        return qc, hash_bytes[:2]
    
    def create_circuit_random(self, label=""):
        """Crée circuit à partir de bytes random"""
        random_bytes = np.random.bytes(32)
        
        qc = QuantumCircuit(2, name=f"RANDOM_{label}")
        
        angle1 = (random_bytes[0] / 255.0) * np.pi
        angle2 = (random_bytes[1] / 255.0) * np.pi
        
        qc.ry(angle1, 0)
        qc.cx(0, 1)
        qc.ry(angle2, 1)
        
        return qc, random_bytes[:2]
    
    def create_circuit_bitflip(self, input_str, bit_pos=0, label=""):
        """Crée circuit avec bit-flip sur hash"""
        hash_bytes = bytearray(hashlib.sha256(input_str.encode()).digest())
        
        # Flip 1 bit
        byte_idx = bit_pos // 8
        bit_idx = bit_pos % 8
        hash_bytes[byte_idx] ^= (1 << bit_idx)
        
        qc = QuantumCircuit(2, name=f"BITFLIP_{label}")
        
        angle1 = (hash_bytes[0] / 255.0) * np.pi
        angle2 = (hash_bytes[1] / 255.0) * np.pi
        
        qc.ry(angle1, 0)
        qc.cx(0, 1)
        qc.ry(angle2, 1)
        
        return qc, bytes(hash_bytes[:2])
    
    def run_single_job(self, backend_name, circuit, test_type, test_id):
        """Exécute 1 job sur 1 QPU"""
        backend = self.backends[backend_name]
        
        print(f"\n🚀 Job {test_type}_{test_id} sur {backend_name}")
        print(f"   Circuit: {circuit.name}")
        
        # Transpile
        pm = generate_preset_pass_manager(optimization_level=3, backend=backend)
        isa_circuit = pm.run(circuit)
        
        # Observable ZZ étendu
        num_qubits = backend.num_qubits
        pauli_str = "I" * (num_qubits - 2) + "ZZ"
        observable = SparsePauliOp(pauli_str)
        
        # Estimator
        estimator = Estimator(backend)
        
        # Soumission avec precision
        job = estimator.run([(isa_circuit, observable)], precision=0.01)
        job_id = job.job_id()
        
        print(f"   Job ID: {job_id}")
        print(f"   Attente résultats...")
        
        # Attente
        result = job.result()
        
        # Extraction
        ev = result[0].data.evs[0]
        std = result[0].data.stds[0]
        snr = abs(ev) / std if std > 0 else 0
        
        print(f"   ✅ EV={ev:.6f}, SNR={snr:.2f}")
        
        # Sauvegarde
        job_data = {
            'job_id': job_id,
            'backend': backend_name,
            'test_type': test_type,
            'test_id': test_id,
            'circuit_name': circuit.name,
            'ev': float(ev),
            'std': float(std),
            'snr': float(snr),
            'timestamp': datetime.utcnow().isoformat()
        }
        
        output_file = self.log_dir / f"final_c167_{backend_name}_{test_type}_{test_id}.json"
        with open(output_file, 'w') as f:
            json.dump(job_data, f, indent=2)
        
        return job_data
    
    def run_final_tests(self):
        """Exécute tests finaux sur 3 QPUs"""
        results = []
        
        # Budget: ~98s / 14s par job = ~7 jobs max
        # Stratégie: 2 jobs par QPU (6 jobs) + 1 bonus
        
        tests = [
            # QPU 1: ibm_fez (2 jobs)
            ('ibm_fez', 'SHA256', 'btc_high_value', lambda: self.create_circuit_sha256("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", "BTC_HIGH")),
            ('ibm_fez', 'RANDOM', 'control_1', lambda: self.create_circuit_random("CTRL_1")),
            
            # QPU 2: ibm_marrakesh (2 jobs)
            ('ibm_marrakesh', 'SHA256', 'btc_medium_value', lambda: self.create_circuit_sha256("3Kzh9qAqVWQhEsfQz7zEQL1EuSx5tyNLNS", "BTC_MED")),
            ('ibm_marrakesh', 'BITFLIP', 'avalanche_test', lambda: self.create_circuit_bitflip("test_avalanche", 0, "AVALANCHE")),
            
            # QPU 3: ibm_kingston (2 jobs)
            ('ibm_kingston', 'SHA256', 'btc_low_value', lambda: self.create_circuit_sha256("bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh", "BTC_LOW")),
            ('ibm_kingston', 'RANDOM', 'control_2', lambda: self.create_circuit_random("CTRL_2")),
            
            # Bonus job (si temps restant)
            ('ibm_fez', 'REPEAT', 'repeatability', lambda: self.create_circuit_sha256("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", "BTC_HIGH_REPEAT")),
        ]
        
        start_time = time.time()
        
        for backend_name, test_type, test_id, circuit_fn in tests:
            elapsed = time.time() - start_time
            remaining = 98 - elapsed
            
            if remaining < 15:
                print(f"\n⏰ Temps restant insuffisant ({remaining:.1f}s), arrêt")
                break
            
            try:
                circuit, input_bytes = circuit_fn()
                job_data = self.run_single_job(backend_name, circuit, test_type, test_id)
                job_data['input_bytes'] = input_bytes.hex()
                results.append(job_data)
                
            except Exception as e:
                print(f"❌ Erreur job {test_type}_{test_id}: {e}")
                continue
        
        total_time = time.time() - start_time
        
        print("\n" + "="*80)
        print(f"EXECUTION TERMINÉE — {len(results)} jobs réussis en {total_time:.1f}s")
        print("="*80)
        
        # Sauvegarde résumé
        summary = {
            'total_jobs': len(results),
            'total_time': total_time,
            'jobs': results,
            'timestamp': datetime.utcnow().isoformat()
        }
        
        summary_file = self.log_dir / "final_c167_summary.json"
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        print(f"\n💾 Résumé sauvegardé: {summary_file}")
        
        # Analyse rapide
        self.analyze_results(results)
        
        return results
    
    def analyze_results(self, results):
        """Analyse rapide des résultats"""
        print("\n" + "="*80)
        print("ANALYSE RAPIDE")
        print("="*80)
        
        # Grouper par type
        sha256_jobs = [r for r in results if r['test_type'] == 'SHA256']
        random_jobs = [r for r in results if r['test_type'] == 'RANDOM']
        bitflip_jobs = [r for r in results if r['test_type'] == 'BITFLIP']
        
        print(f"\nJobs SHA-256: {len(sha256_jobs)}")
        if sha256_jobs:
            evs = [j['ev'] for j in sha256_jobs]
            snrs = [j['snr'] for j in sha256_jobs]
            print(f"  EV mean: {np.mean(evs):.4f} ± {np.std(evs):.4f}")
            print(f"  SNR mean: {np.mean(snrs):.2f} ± {np.std(snrs):.2f}")
        
        print(f"\nJobs Random: {len(random_jobs)}")
        if random_jobs:
            evs = [j['ev'] for j in random_jobs]
            snrs = [j['snr'] for j in random_jobs]
            print(f"  EV mean: {np.mean(evs):.4f} ± {np.std(evs):.4f}")
            print(f"  SNR mean: {np.mean(snrs):.2f} ± {np.std(snrs):.2f}")
        
        print(f"\nJobs Bit-flip: {len(bitflip_jobs)}")
        if bitflip_jobs:
            for job in bitflip_jobs:
                print(f"  {job['test_id']}: EV={job['ev']:.6f}, SNR={job['snr']:.2f}")
        
        # Comparaison QPUs
        print(f"\nComparaison QPUs:")
        for backend_name in self.backends.keys():
            backend_jobs = [r for r in results if r['backend'] == backend_name]
            if backend_jobs:
                snrs = [j['snr'] for j in backend_jobs]
                print(f"  {backend_name}: {len(backend_jobs)} jobs, SNR mean={np.mean(snrs):.2f}")

def main():
    tester = MultiQPUFinalTest()
    results = tester.run_final_tests()
    
    print("\n✅ EXECUTION FINALE C167 TERMINÉE")
    print(f"   Total jobs: {len(results)}")
    print(f"   Logs: {tester.log_dir}")

if __name__ == "__main__":
    main()

# Made with Bob
