#!/usr/bin/env python3
"""
Test unitaire minimal IBM Quantum pour identifier la cause de l'erreur 1500
"""

import sys
from qiskit import QuantumCircuit
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2

def test_1_simple_circuit():
    """Test 1: Circuit minimal (1 qubit, 1 gate)"""
    print("\n=== TEST 1: Circuit Minimal ===")
    try:
        qc = QuantumCircuit(1, 1)
        qc.h(0)
        qc.measure(0, 0)
        print(f"✓ Circuit créé: {qc.num_qubits}q, {qc.size()} gates")
        return qc
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_2_observable_simple():
    """Test 2: Observable minimal (1 qubit)"""
    print("\n=== TEST 2: Observable Minimal ===")
    try:
        obs = SparsePauliOp(["Z"])
        print(f"✓ Observable créé: {obs.num_qubits}q")
        return obs
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_3_connection():
    """Test 3: Connexion IBM"""
    print("\n=== TEST 3: Connexion IBM ===")
    try:
        service = QiskitRuntimeService(channel="ibm_quantum_platform")
        print(f"✓ Service connecté")
        
        backends = service.backends()
        print(f"✓ Backends disponibles: {len(backends)}")
        for b in backends[:3]:
            print(f"  - {b.name}: {b.num_qubits}q")
        
        return service
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_4_backend_selection(service):
    """Test 4: Sélection backend"""
    print("\n=== TEST 4: Sélection Backend ===")
    try:
        backend = service.backend("ibm_fez")
        print(f"✓ Backend: {backend.name}")
        print(f"  Qubits: {backend.num_qubits}")
        print(f"  Status: {backend.status().status_msg}")
        return backend
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_5_transpilation(qc, backend):
    """Test 5: Transpilation"""
    print("\n=== TEST 5: Transpilation ===")
    try:
        from qiskit import transpile
        qc_t = transpile(qc, backend=backend, optimization_level=1)
        print(f"✓ Circuit transpiré: {qc_t.num_qubits}q, {qc_t.size()} gates")
        return qc_t
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_6_pub_creation(qc, obs):
    """Test 6: Création pub avec extension observable"""
    print("\n=== TEST 6: Création Pub ===")
    try:
        # CORRECTION: Étendre l'observable au layout du circuit transpiré
        if hasattr(qc, 'layout') and qc.layout is not None:
            obs_extended = obs.apply_layout(qc.layout)
            print(f"✓ Observable étendu: {obs.num_qubits}q → {obs_extended.num_qubits}q")
        else:
            obs_extended = obs
            print(f"⚠ Pas de layout, observable non étendu: {obs.num_qubits}q")
        
        pub = (qc, obs_extended)
        print(f"✓ Pub créé: circuit {qc.num_qubits}q + observable {obs_extended.num_qubits}q")
        return [pub]
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_7_estimator_creation(backend):
    """Test 7: Création Estimator (pour observables)"""
    print("\n=== TEST 7: Création Estimator ===")
    try:
        estimator = EstimatorV2(backend)
        print(f"✓ Estimator créé (pour mesure observables)")
        return estimator
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_8_job_submission(estimator, pubs):
    """Test 8: Soumission job"""
    print("\n=== TEST 8: Soumission Job ===")
    try:
        job = estimator.run(pubs, precision=0.01)
        print(f"✓ Job soumis: {job.job_id()}")
        return job
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def test_9_job_status(job):
    """Test 9: Statut job"""
    print("\n=== TEST 9: Statut Job ===")
    try:
        import time
        for i in range(10):
            status = job.status()
            print(f"  [{i+1}/10] Status: {status}")
            
            if status.name == "DONE":
                print(f"✓ Job terminé avec succès")
                return True
            elif status.name == "ERROR":
                print(f"✗ Job en erreur")
                print(f"  Message: {job.error_message()}")
                return False
            
            time.sleep(5)
        
        print(f"⚠ Timeout (50s)")
        return None
    except Exception as e:
        print(f"✗ Erreur: {e}")
        return None

def main():
    print("="*70)
    print("TEST UNITAIRE IBM QUANTUM - Diagnostic Erreur 1500")
    print("="*70)
    
    # Test 1: Circuit minimal
    qc = test_1_simple_circuit()
    if not qc:
        print("\n❌ ÉCHEC: Impossible de créer circuit")
        return 1
    
    # Test 2: Observable minimal
    obs = test_2_observable_simple()
    if not obs:
        print("\n❌ ÉCHEC: Impossible de créer observable")
        return 1
    
    # Test 3: Connexion
    service = test_3_connection()
    if not service:
        print("\n❌ ÉCHEC: Impossible de se connecter à IBM")
        return 1
    
    # Test 4: Backend
    backend = test_4_backend_selection(service)
    if not backend:
        print("\n❌ ÉCHEC: Impossible de sélectionner backend")
        return 1
    
    # Test 5: Transpilation
    qc_t = test_5_transpilation(qc, backend)
    if not qc_t:
        print("\n❌ ÉCHEC: Impossible de transpiler")
        return 1
    
    # Test 6: Pub
    pubs = test_6_pub_creation(qc_t, obs)
    if not pubs:
        print("\n❌ ÉCHEC: Impossible de créer pub")
        return 1
    
    # Test 7: Estimator
    estimator = test_7_estimator_creation(backend)
    if not estimator:
        print("\n❌ ÉCHEC: Impossible de créer estimator")
        return 1
    
    # Test 8: Soumission
    job = test_8_job_submission(estimator, pubs)
    if not job:
        print("\n❌ ÉCHEC: Impossible de soumettre job")
        return 1
    
    # Test 9: Statut
    success = test_9_job_status(job)
    
    print("\n" + "="*70)
    if success:
        print("✅ TOUS LES TESTS RÉUSSIS")
        print(f"Job ID: {job.job_id()}")
        print("="*70)
        return 0
    elif success is False:
        print("❌ JOB EN ERREUR")
        print(f"Job ID: {job.job_id()}")
        print("Vérifier les logs IBM pour plus de détails")
        print("="*70)
        return 1
    else:
        print("⚠ TIMEOUT - Job toujours en cours")
        print(f"Job ID: {job.job_id()}")
        print("Vérifier manuellement le statut")
        print("="*70)
        return 2

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
