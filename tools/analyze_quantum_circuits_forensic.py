#!/usr/bin/env python3
"""
Analyse Forensique Bit-Level des Circuits Quantiques IBM
Décode les circuits, extrait les angles, identifie SHA-256 vs Random
"""

import json
import base64
import zlib
import pickle
import hashlib
import numpy as np
from pathlib import Path
import sys

def decode_base64_circuit(encoded_str):
    """Décode un circuit encodé en base64"""
    try:
        decoded = base64.b64decode(encoded_str)
        decompressed = zlib.decompress(decoded)
        circuit = pickle.loads(decompressed)
        return circuit
    except Exception as e:
        print(f"❌ Erreur décodage circuit: {e}")
        return None

def extract_angles_from_circuit(circuit):
    """Extrait les angles RY du circuit"""
    angles = []
    try:
        for instruction in circuit.data:
            if instruction[0].name == 'ry':
                angle = float(instruction[0].params[0])
                angles.append(angle)
        return angles
    except Exception as e:
        print(f"❌ Erreur extraction angles: {e}")
        return []

def decode_ndarray(encoded_str):
    """Décode un ndarray encodé en base64"""
    try:
        decoded = base64.b64decode(encoded_str)
        decompressed = zlib.decompress(decoded)
        arr = pickle.loads(decompressed)
        return arr
    except Exception as e:
        return None

def analyze_job_forensic(job_dir):
    """Analyse forensique complète d'un job"""
    job_id = job_dir.name
    info_file = job_dir / f"{job_id}-info.json"
    result_file = job_dir / f"{job_id}-result.json"
    
    if not info_file.exists() or not result_file.exists():
        return None
    
    # Charger les fichiers
    with open(info_file, 'r') as f:
        info = json.load(f)
    
    with open(result_file, 'r') as f:
        result = json.load(f)
    
    # Extraire le circuit
    circuit_encoded = info['params']['pubs'][0][0]['__value__']
    circuit = decode_base64_circuit(circuit_encoded)
    
    if circuit is None:
        return None
    
    # Extraire les angles
    angles = extract_angles_from_circuit(circuit)
    
    # Extraire les résultats
    evs_encoded = result['__value__']['pub_results'][0]['__value__']['data']['__value__']['fields']['evs']['__value__']
    stds_encoded = result['__value__']['pub_results'][0]['__value__']['data']['__value__']['fields']['stds']['__value__']
    
    evs = decode_ndarray(evs_encoded)
    stds = decode_ndarray(stds_encoded)
    
    ev = float(evs[0]) if evs is not None else 0.0
    std = float(stds[0]) if stds is not None else 0.0
    snr = abs(ev) / std if std > 0 else 0.0
    
    # Analyser les angles pour identifier le type
    angle_bytes = []
    for angle in angles:
        # Reconvertir l'angle en byte original
        byte_val = int((angle / np.pi) * 255)
        angle_bytes.append(byte_val)
    
    # Tester si c'est un hash SHA-256
    is_sha256 = False
    input_str = None
    
    # Tester plusieurs inputs possibles
    test_inputs = [
        f"test_sha256_{i}" for i in range(10)
    ] + [
        f"test_random_{i}" for i in range(10)
    ] + [
        f"SHA256_{i}" for i in range(10)
    ] + [
        f"RANDOM_{i}" for i in range(10)
    ]
    
    for test_input in test_inputs:
        hash_bytes = hashlib.sha256(test_input.encode()).digest()
        if len(angle_bytes) >= 2:
            if hash_bytes[0] == angle_bytes[0] and hash_bytes[1] == angle_bytes[1]:
                is_sha256 = True
                input_str = test_input
                break
    
    return {
        'job_id': job_id,
        'created': info.get('created', ''),
        'angles': angles,
        'angle_bytes': angle_bytes,
        'ev': ev,
        'std': std,
        'snr': snr,
        'is_sha256': is_sha256,
        'input_str': input_str,
        'num_gates': len(circuit.data) if circuit else 0
    }

def main():
    log_dir = Path("src/advanced_calculations/bitcoin_quantum_mining/logs/log_job_ibm")
    
    if not log_dir.exists():
        print(f"❌ Répertoire {log_dir} introuvable")
        return
    
    # Analyser tous les jobs
    jobs_data = []
    
    for job_dir in sorted(log_dir.iterdir()):
        if job_dir.is_dir() and job_dir.name.startswith('job-d7s8'):
            print(f"🔍 Analyse {job_dir.name}...")
            data = analyze_job_forensic(job_dir)
            if data:
                jobs_data.append(data)
    
    # Trier par timestamp
    jobs_data.sort(key=lambda x: x['created'])
    
    print("\n" + "="*100)
    print("ANALYSE FORENSIQUE BIT-LEVEL — 11 JOBS VALIDATION SCIENTIFIQUE")
    print("="*100)
    
    # Grouper par type
    sha256_jobs = [j for j in jobs_data if j['is_sha256']]
    random_jobs = [j for j in jobs_data if not j['is_sha256']]
    
    print(f"\n📊 RÉSUMÉ:")
    print(f"  Total jobs analysés: {len(jobs_data)}")
    print(f"  Jobs SHA-256: {len(sha256_jobs)}")
    print(f"  Jobs Random: {len(random_jobs)}")
    
    # Afficher détails SHA-256
    if sha256_jobs:
        print(f"\n🔐 JOBS SHA-256:")
        for job in sha256_jobs:
            print(f"\n  Job: {job['job_id']}")
            print(f"    Input: {job['input_str']}")
            print(f"    Angles: {[f'{a:.4f}' for a in job['angles']]}")
            print(f"    Bytes: {job['angle_bytes']}")
            print(f"    EV: {job['ev']:.6f}")
            print(f"    SNR: {job['snr']:.2f}")
    
    # Afficher détails Random
    if random_jobs:
        print(f"\n🎲 JOBS RANDOM:")
        for job in random_jobs:
            print(f"\n  Job: {job['job_id']}")
            print(f"    Angles: {[f'{a:.4f}' for a in job['angles']]}")
            print(f"    Bytes: {job['angle_bytes']}")
            print(f"    EV: {job['ev']:.6f}")
            print(f"    SNR: {job['snr']:.2f}")
    
    # Analyse statistique
    if sha256_jobs and random_jobs:
        sha_evs = [j['ev'] for j in sha256_jobs]
        rnd_evs = [j['ev'] for j in random_jobs]
        
        sha_mean = np.mean(sha_evs)
        sha_std = np.std(sha_evs)
        rnd_mean = np.mean(rnd_evs)
        rnd_std = np.std(rnd_evs)
        
        delta = abs(sha_mean - rnd_mean)
        max_std = max(sha_std, rnd_std)
        significance = delta / max_std if max_std > 0 else 0
        
        print(f"\n📈 ANALYSE STATISTIQUE:")
        print(f"  SHA-256 EV: {sha_mean:.4f} ± {sha_std:.4f}")
        print(f"  Random EV:  {rnd_mean:.4f} ± {rnd_std:.4f}")
        print(f"  Δmean:      {delta:.4f}")
        print(f"  Δ/σ:        {significance:.2f} {'✅ SIGNIFICATIF' if significance > 2 else '❌ NON SIGNIFICATIF'}")
    
    # Sauvegarder les résultats
    output_file = "src/advanced_calculations/bitcoin_quantum_mining/CHAT/ANALYSE_FORENSIQUE_CIRCUITS_C167.json"
    with open(output_file, 'w') as f:
        json.dump(jobs_data, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {output_file}")

if __name__ == "__main__":
    main()

# Made with Bob
