#!/usr/bin/env python3
"""
Script pour récupérer TOUS les jobs IBM Quantum (147 jobs visibles sur le dashboard)
et créer une analyse forensique complète bit-level
"""

import sys
import json
import time
import base64
import zlib
import struct
from pathlib import Path
from datetime import datetime

try:
    from qiskit_ibm_runtime import QiskitRuntimeService
    import numpy as np
except ImportError as e:
    print(f"Erreur import: {e}")
    sys.exit(1)

# Configuration CRN
CRN_INSTANCE = "crn:v1:bluemix:public:quantum-computing:us-east:a/b893e3b64af743f9860d3ae175578d19:31cb02b4-5010-422a-a999-d6302e4d7b06::"

def decode_numpy_base64(b64_str):
    """Décode un ndarray NumPy encodé en base64+zlib"""
    try:
        compressed = base64.b64decode(b64_str)
        decompressed = zlib.decompress(compressed)
        
        # Parse header NumPy
        magic = decompressed[:6]
        if magic != b'\x93NUMPY':
            return None
            
        version = struct.unpack('BB', decompressed[6:8])
        header_len = struct.unpack('<H', decompressed[8:10])[0]
        header = decompressed[10:10+header_len].decode('latin1')
        
        # Extract dtype et shape
        import re
        dtype_match = re.search(r"'descr':\s*'([^']+)'", header)
        shape_match = re.search(r"'shape':\s*\(([^)]*)\)", header)
        
        dtype_str = dtype_match.group(1) if dtype_match else '<f8'
        shape_str = shape_match.group(1) if shape_match else ''
        
        # Parse data
        data_start = 10 + header_len
        data = decompressed[data_start:]
        
        if dtype_str == '<f8':  # float64 little-endian
            if len(data) == 8:
                value = struct.unpack('<d', data)[0]
                return {'type': 'scalar', 'value': value, 'dtype': 'float64'}
            else:
                count = len(data) // 8
                values = struct.unpack(f'<{count}d', data)
                return {'type': 'array', 'values': list(values), 'dtype': 'float64', 'count': count}
        
        return {'type': 'unknown', 'dtype': dtype_str, 'size': len(data)}
        
    except Exception as e:
        return {'error': str(e)}

def analyze_job_forensic(job, log_dir):
    """Analyse forensique complète d'un job IBM"""
    job_id = job.job_id()
    
    print(f"\n{'='*70}")
    print(f"Analyse job: {job_id}")
    print(f"{'='*70}")
    
    # Créer répertoire
    job_dir = log_dir / f"job-{job_id}"
    job_dir.mkdir(parents=True, exist_ok=True)
    
    # Récupérer métadonnées
    try:
        status = job.status()
        backend_name = job.backend().name if hasattr(job, 'backend') else 'unknown'
        
        # Info basique
        info = {
            'job_id': job_id,
            'status': str(status),
            'backend': backend_name,
            'created': job.creation_date.isoformat() if hasattr(job, 'creation_date') else None,
            'retrieved_at': datetime.utcnow().isoformat() + 'Z'
        }
        
        # Sauvegarder info
        with open(job_dir / f"job-{job_id}-info.json", 'w') as f:
            json.dump(info, f, indent=2)
        
        print(f"  Status: {status}")
        print(f"  Backend: {backend_name}")
        
        # Si complété, récupérer résultats
        if str(status) == 'JobStatus.DONE' or 'Completed' in str(status):
            try:
                result = job.result()
                
                # Extraire métriques
                if hasattr(result, 'metadata'):
                    metadata = result.metadata
                    print(f"  Metadata: {len(metadata)} pubs")
                    
                    with open(job_dir / f"job-{job_id}-metadata.json", 'w') as f:
                        json.dump(metadata, f, indent=2, default=str)
                
                # Extraire valeurs
                if hasattr(result, 'values'):
                    values = result.values
                    print(f"  Values: {len(values)} résultats")
                    
                    # Analyse bit-level
                    forensic_data = []
                    for i, val in enumerate(values):
                        entry = {
                            'pub_index': i,
                            'value': float(val) if hasattr(val, '__float__') else str(val),
                            'type': type(val).__name__
                        }
                        
                        # Décomposition IEEE 754 si float
                        if isinstance(val, (float, np.floating)):
                            bits = struct.pack('>d', float(val))
                            hex_repr = bits.hex()
                            entry['ieee754_hex'] = hex_repr
                            entry['ieee754_bits'] = bin(int(hex_repr, 16))[2:].zfill(64)
                        
                        forensic_data.append(entry)
                    
                    with open(job_dir / f"job-{job_id}-values.json", 'w') as f:
                        json.dump(forensic_data, f, indent=2)
                
                # Extraire std devs
                if hasattr(result, 'stds'):
                    stds = result.stds
                    print(f"  Stds: {len(stds)} valeurs")
                    
                    with open(job_dir / f"job-{job_id}-stds.json", 'w') as f:
                        json.dump([float(s) for s in stds], f, indent=2)
                
                print(f"  ✓ Résultats sauvegardés dans {job_dir}")
                return True
                
            except Exception as e:
                print(f"  ✗ Erreur récupération résultats: {e}")
                return False
        else:
            print(f"  ⚠ Job non complété, skip résultats")
            return False
            
    except Exception as e:
        print(f"  ✗ Erreur analyse: {e}")
        return False

def main():
    print("="*70)
    print("RÉCUPÉRATION COMPLÈTE DES 147 JOBS IBM QUANTUM")
    print("="*70)
    
    # Connexion IBM
    print("\n[1/4] Connexion IBM Quantum (CRN Ubuntu 9)...")
    try:
        service = QiskitRuntimeService(
            channel='ibm_cloud',
            instance=CRN_INSTANCE
        )
        print(f"  ✓ Connecté: {service._account.instance}")
    except Exception as e:
        print(f"  ✗ Erreur connexion: {e}")
        return 1
    
    # Récupérer TOUS les jobs
    print("\n[2/4] Récupération liste complète des jobs...")
    try:
        all_jobs = list(service.jobs(limit=200))  # Récupérer jusqu'à 200 jobs
        print(f"  ✓ {len(all_jobs)} jobs récupérés")
    except Exception as e:
        print(f"  ✗ Erreur: {e}")
        return 1
    
    # Préparer logging
    log_dir = Path("src/advanced_calculations/bitcoin_quantum_mining/logs/log_job_ibm")
    log_dir.mkdir(parents=True, exist_ok=True)
    
    # Analyser chaque job
    print(f"\n[3/4] Analyse forensique de {len(all_jobs)} jobs...")
    
    stats = {
        'total': len(all_jobs),
        'completed': 0,
        'running': 0,
        'failed': 0,
        'analyzed': 0
    }
    
    for i, job in enumerate(all_jobs):
        progress = (i + 1) / len(all_jobs) * 100
        print(f"\n[{i+1}/{len(all_jobs)}] ({progress:.1f}%) ", end='')
        
        try:
            status_str = str(job.status())
            
            if 'DONE' in status_str or 'Completed' in status_str:
                stats['completed'] += 1
                if analyze_job_forensic(job, log_dir):
                    stats['analyzed'] += 1
            elif 'RUNNING' in status_str or 'QUEUED' in status_str:
                stats['running'] += 1
                print(f"Job {job.job_id()}: {status_str} (skip)")
            else:
                stats['failed'] += 1
                print(f"Job {job.job_id()}: {status_str} (failed)")
                
        except Exception as e:
            print(f"Erreur job: {e}")
            stats['failed'] += 1
        
        # Pause pour éviter rate limiting
        if (i + 1) % 10 == 0:
            time.sleep(1)
    
    # Rapport final
    print(f"\n{'='*70}")
    print("[4/4] RAPPORT FINAL")
    print(f"{'='*70}")
    print(f"Total jobs: {stats['total']}")
    print(f"Complétés: {stats['completed']}")
    print(f"En cours: {stats['running']}")
    print(f"Échoués: {stats['failed']}")
    print(f"Analysés avec succès: {stats['analyzed']}")
    
    # Sauvegarder stats
    stats_file = log_dir / "fetch_all_jobs_stats.json"
    with open(stats_file, 'w') as f:
        json.dump({
            'timestamp': datetime.utcnow().isoformat() + 'Z',
            'stats': stats,
            'crn_instance': CRN_INSTANCE
        }, f, indent=2)
    
    print(f"\n✓ Stats sauvegardées: {stats_file}")
    print(f"✓ Logs dans: {log_dir}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
