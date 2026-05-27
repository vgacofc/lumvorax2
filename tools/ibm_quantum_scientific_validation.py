#!/usr/bin/env python3
"""
Script de Validation Scientifique Rigoureuse - Cycle C167
Objectif : Prouver (ou réfuter) les patterns SHA-256 avec quota IBM limité
Budget : 2min20s (~140s) de QR usage
Tests : Random vs SHA256, Bit-flip sensitivity, Statistical validation
"""

import sys
import json
import hashlib
import time
import random
from pathlib import Path
from qiskit import QuantumCircuit, transpile
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2
import numpy as np

class ScientificValidator:
    """Validateur scientifique pour hypothèses SHA-256"""
    
    def __init__(self, budget_seconds=140):
        self.budget_seconds = budget_seconds
        self.results = []
        self.start_time = None
        
    def create_circuit_from_bytes(self, data_bytes, label=""):
        """Crée circuit quantique depuis bytes"""
        qc = QuantumCircuit(2, name=label)
        
        # Angles basés sur bytes
        angle1 = (data_bytes[0] / 255.0) * np.pi
        angle2 = (data_bytes[1] / 255.0) * np.pi
        
        qc.ry(angle1, 0)
        qc.cx(0, 1)
        qc.ry(angle2, 1)
        
        return qc
    
    def test_1_sha256_vs_random(self, service, backend, n_samples=5):
        """
        TEST CRITIQUE 1 : SHA-256 vs Random
        Hypothèse : Distribution EV(SHA256) ≠ Distribution EV(random)
        """
        print("\n" + "="*70)
        print("TEST 1 : SHA-256 vs RANDOM (n={})".format(n_samples))
        print("="*70)
        
        results_sha = []
        results_rnd = []
        
        # Test SHA-256
        print("\n[1/2] Testing SHA-256 inputs...")
        for i in range(n_samples):
            # Input déterministe
            input_str = f"test_sha256_{i}"
            hash_bytes = hashlib.sha256(input_str.encode()).digest()
            
            qc = self.create_circuit_from_bytes(hash_bytes, f"SHA256_{i}")
            ev, snr = self._run_single_job(service, backend, qc, f"SHA256_{i}")
            
            results_sha.append({
                'type': 'SHA256',
                'input': input_str,
                'ev': ev,
                'snr': snr
            })
            
            print(f"  [{i+1}/{n_samples}] SHA256: EV={ev:.6f}, SNR={snr:.2f}")
        
        # Test Random
        print("\n[2/2] Testing RANDOM inputs...")
        for i in range(n_samples):
            # Bytes aléatoires
            random_bytes = random.randbytes(32)
            
            qc = self.create_circuit_from_bytes(random_bytes, f"RANDOM_{i}")
            ev, snr = self._run_single_job(service, backend, qc, f"RANDOM_{i}")
            
            results_rnd.append({
                'type': 'RANDOM',
                'ev': ev,
                'snr': snr
            })
            
            print(f"  [{i+1}/{n_samples}] RANDOM: EV={ev:.6f}, SNR={snr:.2f}")
        
        # Analyse statistique
        evs_sha = [r['ev'] for r in results_sha]
        evs_rnd = [r['ev'] for r in results_rnd]
        
        mean_sha = np.mean(evs_sha)
        mean_rnd = np.mean(evs_rnd)
        std_sha = np.std(evs_sha)
        std_rnd = np.std(evs_rnd)
        
        print("\n" + "-"*70)
        print("RÉSULTATS TEST 1:")
        print(f"  SHA-256 : mean={mean_sha:.6f}, std={std_sha:.6f}")
        print(f"  RANDOM  : mean={mean_rnd:.6f}, std={std_rnd:.6f}")
        print(f"  Δmean   : {abs(mean_sha - mean_rnd):.6f}")
        
        # Test t simple (sans scipy pour éviter erreurs de type)
        print(f"  Δmean/std : {abs(mean_sha - mean_rnd) / max(std_sha, std_rnd, 0.001):.4f}")
        
        if abs(mean_sha - mean_rnd) > 2 * max(std_sha, std_rnd):
            print("  ✓ DIFFÉRENCE SIGNIFICATIVE (Δ > 2σ)")
        else:
            print("  ✗ PAS DE DIFFÉRENCE SIGNIFICATIVE")
        
        return {
            'test': 'SHA256_vs_RANDOM',
            'sha256': results_sha,
            'random': results_rnd,
            'statistics': {
                'mean_sha': mean_sha,
                'mean_rnd': mean_rnd,
                'std_sha': std_sha,
                'std_rnd': std_rnd
            }
        }
    
    def test_2_bit_flip_sensitivity(self, service, backend, n_samples=3):
        """
        TEST CRITIQUE 2 : Sensibilité Bit-Flip
        Hypothèse : Flip 1 bit → ΔEV mesurable (vs effet avalanche SHA-256)
        """
        print("\n" + "="*70)
        print("TEST 2 : BIT-FLIP SENSITIVITY (n={})".format(n_samples))
        print("="*70)
        
        results = []
        
        for i in range(n_samples):
            # Input original
            input_str = f"bitflip_test_{i}"
            hash_orig = hashlib.sha256(input_str.encode()).digest()
            
            # Flip 1 bit (position aléatoire)
            bit_pos = random.randint(0, 255)
            byte_idx = bit_pos // 8
            bit_idx = bit_pos % 8
            
            hash_flip = bytearray(hash_orig)
            hash_flip[byte_idx] ^= (1 << bit_idx)
            hash_flip = bytes(hash_flip)
            
            # Test original
            qc_orig = self.create_circuit_from_bytes(hash_orig, f"ORIG_{i}")
            ev_orig, snr_orig = self._run_single_job(service, backend, qc_orig, f"ORIG_{i}")
            
            # Test flipped
            qc_flip = self.create_circuit_from_bytes(hash_flip, f"FLIP_{i}")
            ev_flip, snr_flip = self._run_single_job(service, backend, qc_flip, f"FLIP_{i}")
            
            delta_ev = abs(ev_orig - ev_flip)
            
            results.append({
                'input': input_str,
                'bit_flipped': bit_pos,
                'ev_original': ev_orig,
                'ev_flipped': ev_flip,
                'delta_ev': delta_ev,
                'snr_original': snr_orig,
                'snr_flipped': snr_flip
            })
            
            print(f"\n  [{i+1}/{n_samples}] Input: {input_str}")
            print(f"    Bit flipped: {bit_pos}")
            print(f"    EV original: {ev_orig:.6f}")
            print(f"    EV flipped : {ev_flip:.6f}")
            print(f"    ΔEV        : {delta_ev:.6f}")
        
        # Analyse
        deltas = [r['delta_ev'] for r in results]
        mean_delta = np.mean(deltas)
        
        print("\n" + "-"*70)
        print("RÉSULTATS TEST 2:")
        print(f"  ΔEV moyen : {mean_delta:.6f}")
        print(f"  ΔEV range : [{min(deltas):.6f}, {max(deltas):.6f}]")
        
        if mean_delta > 0.5:
            print("  ✓ SENSIBILITÉ ÉLEVÉE (effet avalanche confirmé)")
        else:
            print("  ⚠ SENSIBILITÉ FAIBLE (structure possible?)")
        
        return {
            'test': 'BIT_FLIP_SENSITIVITY',
            'results': results,
            'statistics': {
                'mean_delta': mean_delta,
                'min_delta': min(deltas),
                'max_delta': max(deltas)
            }
        }
    
    def test_3_repeatability(self, service, backend, n_repeats=3):
        """
        TEST CRITIQUE 3 : Répétabilité
        Hypothèse : Même input → même output (modulo bruit quantique)
        """
        print("\n" + "="*70)
        print("TEST 3 : REPEATABILITY (n={})".format(n_repeats))
        print("="*70)
        
        # Input fixe
        input_str = "repeatability_test"
        hash_bytes = hashlib.sha256(input_str.encode()).digest()
        
        results = []
        
        print(f"\nInput: {input_str}")
        print(f"Testing {n_repeats} independent runs...")
        
        for i in range(n_repeats):
            qc = self.create_circuit_from_bytes(hash_bytes, f"REP_{i}")
            ev, snr = self._run_single_job(service, backend, qc, f"REP_{i}")
            
            results.append({
                'run': i+1,
                'ev': ev,
                'snr': snr
            })
            
            print(f"  [{i+1}/{n_repeats}] EV={ev:.6f}, SNR={snr:.2f}")
        
        # Analyse variance
        evs = [r['ev'] for r in results]
        mean_ev = np.mean(evs)
        std_ev = np.std(evs)
        cv = (std_ev / abs(mean_ev)) * 100 if mean_ev != 0 else 0
        
        print("\n" + "-"*70)
        print("RÉSULTATS TEST 3:")
        print(f"  EV mean : {mean_ev:.6f}")
        print(f"  EV std  : {std_ev:.6f}")
        print(f"  CV      : {cv:.2f}%")
        
        if cv < 5:
            print("  ✓ RÉPÉTABILITÉ EXCELLENTE (CV<5%)")
        elif cv < 10:
            print("  ✓ RÉPÉTABILITÉ BONNE (CV<10%)")
        else:
            print("  ⚠ RÉPÉTABILITÉ FAIBLE (bruit quantique dominant)")
        
        return {
            'test': 'REPEATABILITY',
            'input': input_str,
            'results': results,
            'statistics': {
                'mean': mean_ev,
                'std': std_ev,
                'cv_percent': cv
            }
        }
    
    def _run_single_job(self, service, backend, qc, label):
        """Exécute un job et retourne EV, SNR"""
        try:
            # Transpile
            qc_t = transpile(qc, backend=backend, optimization_level=1)
            
            # Observable
            obs = SparsePauliOp(["ZZ"])
            if hasattr(qc_t, 'layout') and qc_t.layout is not None:
                obs_ext = obs.apply_layout(qc_t.layout)
            else:
                obs_ext = obs
            
            # Run
            estimator = EstimatorV2(backend)
            job = estimator.run([(qc_t, obs_ext)], precision=0.01)
            
            # Wait
            result = job.result()
            
            # Extract
            ev = result[0].data.evs[0]
            std = result[0].data.stds[0]
            snr = abs(ev / std) if std > 0 else 0
            
            return ev, snr
            
        except Exception as e:
            print(f"  ✗ Erreur job {label}: {e}")
            return 0.0, 0.0
    
    def run_all_tests(self):
        """Exécute tous les tests avec budget temps"""
        print("="*70)
        print("VALIDATION SCIENTIFIQUE RIGOUREUSE - CYCLE C167")
        print("="*70)
        print(f"Budget : {self.budget_seconds}s de QR usage")
        print(f"Objectif : Prouver/réfuter patterns SHA-256")
        
        self.start_time = time.time()
        
        # Connexion IBM
        print("\n[INIT] Connexion IBM Quantum...")
        try:
            service = QiskitRuntimeService(channel="ibm_quantum_platform")
            backend = service.backend("ibm_fez")
            print(f"  ✓ Backend: {backend.name} ({backend.num_qubits}q)")
        except Exception as e:
            print(f"  ✗ Erreur connexion: {e}")
            return None
        
        # Test 1 : SHA256 vs Random (5 samples = ~70s)
        test1 = self.test_1_sha256_vs_random(service, backend, n_samples=5)
        self.results.append(test1)
        
        # Test 2 : Bit-flip (3 samples = ~42s)
        test2 = self.test_2_bit_flip_sensitivity(service, backend, n_samples=3)
        self.results.append(test2)
        
        # Test 3 : Repeatability (3 repeats = ~21s)
        test3 = self.test_3_repeatability(service, backend, n_repeats=3)
        self.results.append(test3)
        
        # Temps total
        elapsed = time.time() - self.start_time
        
        print("\n" + "="*70)
        print("VALIDATION TERMINÉE")
        print("="*70)
        print(f"Temps total : {elapsed:.1f}s")
        print(f"Budget utilisé : ~{elapsed:.0f}s / {self.budget_seconds}s")
        
        # Sauvegarder
        self._save_results()
        
        return self.results
    
    def _save_results(self):
        """Sauvegarde résultats"""
        log_dir = Path("src/advanced_calculations/bitcoin_quantum_mining/logs/scientific_validation")
        log_dir.mkdir(parents=True, exist_ok=True)
        
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        log_file = log_dir / f"validation_c167_{timestamp}.json"
        
        with open(log_file, 'w') as f:
            json.dump({
                'timestamp': timestamp,
                'budget_seconds': self.budget_seconds,
                'tests': self.results
            }, f, indent=2)
        
        print(f"\n✓ Résultats sauvegardés: {log_file}")

def main():
    validator = ScientificValidator(budget_seconds=140)
    results = validator.run_all_tests()
    
    if results:
        print("\n" + "="*70)
        print("CONCLUSION SCIENTIFIQUE")
        print("="*70)
        print("\nCes tests permettront de :")
        print("  1. Valider/réfuter patterns SHA-256")
        print("  2. Fournir baseline pour CRS Willow")
        print("  3. Contribuer à QDAYPRIZE")
        print("  4. Optimiser mining classique")
        print("\nProchaine étape : Analyse statistique complète")
        return 0
    else:
        print("\n✗ Validation échouée")
        return 1

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob - Cycle C167