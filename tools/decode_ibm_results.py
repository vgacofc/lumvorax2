#!/usr/bin/env python3
"""
Décodage et analyse des résultats IBM Quantum
"""
import json
import base64
import zlib
import numpy as np
from pathlib import Path

def decode_ndarray(encoded_str):
    """Décoder un ndarray encodé en base64+zlib"""
    try:
        decoded = base64.b64decode(encoded_str)
        decompressed = zlib.decompress(decoded)
        # Essayer différents dtypes
        arr = np.frombuffer(decompressed, dtype=np.float64)
        # Prendre seulement la dernière valeur (la vraie)
        return arr[-1:] if len(arr) > 1 else arr
    except Exception as e:
        print(f"Erreur décodage: {e}")
        return np.array([])

def analyze_job_results(job_dir):
    """Analyser les résultats d'un job IBM"""
    result_file = Path(job_dir) / f"{Path(job_dir).name}-result.json"
    info_file = Path(job_dir) / f"{Path(job_dir).name}-info.json"
    
    if not result_file.exists():
        print(f"❌ Pas de résultats pour {Path(job_dir).name}")
        return None
    
    # Charger résultats
    with open(result_file) as f:
        results = json.load(f)
    
    with open(info_file) as f:
        info = json.load(f)
    
    # Extraire données
    pub_results = results['__value__']['pub_results']
    metadata = results['__value__']['metadata']
    
    print(f"\n{'='*80}")
    print(f"JOB: {Path(job_dir).name}")
    print(f"{'='*80}\n")
    
    print(f"Backend: {info.get('backend', 'N/A')}")
    print(f"Status: {info.get('status', 'N/A')}")
    print(f"Nombre de pubs: {len(pub_results)}")
    
    # Analyser chaque pub
    for i, pub in enumerate(pub_results):
        print(f"\n--- Pub #{i} ---")
        
        data = pub['__value__']['data']['__value__']['fields']
        meta = pub['__value__']['metadata']
        
        # Décoder arrays
        evs = decode_ndarray(data['evs']['__value__'])
        stds = decode_ndarray(data['stds']['__value__'])
        errors = decode_ndarray(data['ensemble_standard_error']['__value__'])
        
        print(f"Shots: {meta['shots']}")
        print(f"Target precision: {meta['target_precision']}")
        print(f"Num randomizations: {meta['num_randomizations']}")
        
        print(f"\nRésultats:")
        print(f"  EVs (Expectation Values): {evs}")
        print(f"  Stds (Standard Deviations): {stds}")
        print(f"  Errors (Ensemble Std Error): {errors}")
        
        # Calculs
        if len(evs) > 0:
            snr = abs(evs[0]) / stds[0] if stds[0] > 0 else 0
            print(f"\nMétriques:")
            print(f"  SNR (Signal-to-Noise Ratio): {snr:.4f}")
            print(f"  Precision atteinte: {errors[0] < meta['target_precision']}")
    
    # Configuration
    print(f"\n--- Configuration ---")
    print(f"Dynamical Decoupling: {metadata['dynamical_decoupling']['enable']}")
    print(f"Twirling (measure): {metadata['twirling']['enable_measure']}")
    print(f"Measure Mitigation: {metadata['resilience']['measure_mitigation']}")
    print(f"ZNE Mitigation: {metadata['resilience']['zne_mitigation']}")
    
    return {
        'evs': evs,
        'stds': stds,
        'errors': errors,
        'shots': meta['shots'],
        'num_randomizations': meta['num_randomizations']
    }

if __name__ == '__main__':
    base_dir = Path('src/advanced_calculations/bitcoin_quantum_mining/logs/log_job_ibm')
    
    # Analyser tous les jobs
    for job_dir in base_dir.iterdir():
        if job_dir.is_dir():
            analyze_job_results(job_dir)

# Made with Bob
