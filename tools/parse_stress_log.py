#!/usr/bin/env python3
# Parser logs avec extraction métriques optimisations avancées

import sys
import re
import json
from datetime import datetime

def parse_optimization_metrics(log_content):
    """Parse métriques d'optimisations avancées"""
    metrics = {
        "performance": {},
        "optimizations": {},
        "system": {},
        "validation": {}
    }
    
    # Performance baseline
    lums_match = re.search(r'(\d+[\d,]*)\s+LUMs/seconde', log_content)
    if lums_match:
        metrics["performance"]["lums_per_second"] = int(lums_match.group(1).replace(',', ''))
    
    gbps_match = re.search(r'([\d.]+)\s+Gbps', log_content)
    if gbps_match:
        metrics["performance"]["gbps"] = float(gbps_match.group(1))
    
    # Optimisations SIMD
    if "AVX-512" in log_content:
        metrics["optimizations"]["simd"] = "AVX-512 16x acceleration"
    elif "AVX2" in log_content:
        metrics["optimizations"]["simd"] = "AVX2 8x acceleration"
    
    # Optimisations mémoire
    cache_match = re.search(r'(\d+)%.*cache.*miss', log_content)
    if cache_match:
        metrics["optimizations"]["cache_reduction"] = f"{cache_match.group(1)}% cache miss reduction"
    
    # Validation modules
    module_match = re.search(r'(\d+)\s+modules?.*opérationnel', log_content)
    if module_match:
        metrics["validation"]["modules_operational"] = int(module_match.group(1))
    
    return metrics

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 parse_stress_log.py <log_file>")
        sys.exit(1)
    
    log_file = sys.argv[1]
    
    try:
        with open(log_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        metrics = parse_optimization_metrics(content)
        
        # Ajout métadonnées
        metrics["metadata"] = {
            "parsed_at": datetime.utcnow().isoformat() + "Z",
            "log_file": log_file,
            "parser_version": "2.0_optimized"
        }
        
        # Sortie JSON formatée
        print(json.dumps(metrics, indent=2, ensure_ascii=False))
        
        # Sauvegarde dans stress_results.json
        with open('stress_results.json', 'w') as f:
            json.dump(metrics, f, indent=2, ensure_ascii=False)
            
    except Exception as e:
        print(f"Erreur parsing: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
