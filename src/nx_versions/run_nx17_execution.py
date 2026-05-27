from nx11_logger_engine import NX17Logger
import time
import os
import random

def run_nx17_execution():
    print("🧬 LANCEMENT NX-17 : CLUSTERS ÉLASTIQUES & RÉGÉNÉRATION")
    
    # Capacité augmentée pour le test
    TOTAL_NEURONS = 50000 
    STIMULI_LOAD = 10000
    
    logger = NX17Logger("NX-17-ELASTIC-GIGA")
    
    print(f"[1/3] Simulation de Morphisme sur {TOTAL_NEURONS} neurones...")
    # Mutation dynamique d'un cluster
    morph_hash = logger.morph_cluster("CLUSTER_0", 2000)
    
    print(f"[2/3] Test de Régénération Sémantique (Merkle Repair)...")
    repair_hash = logger.regenerate_log("CORRUPTED_EVENT_X", "ROOT_NEIGHBOR_Y")
    
    print(f"[3/3] Injection de charge maximale {STIMULI_LOAD} stimuli...")
    start = time.time()
    for i in range(STIMULI_LOAD):
        logger.log_event("ELASTIC_LOAD", "MORPH", f"stim:{i}", {}, {}, -1.0, 20000, 0.99, "FUNC", "0xAA")
    duration = time.time() - start
    
    ops_actual = STIMULI_LOAD / duration if duration > 0 else 0
    metrics = logger.get_hardware_metrics()
    
    log_dir = "logs_AIMO3/nx/NX-17"
    os.makedirs(log_dir, exist_ok=True)
    with open(f"{log_dir}/NX-17_forensic.log", "w") as f:
        f.write(f"TOTAL_NEURONS={TOTAL_NEURONS}\n")
        f.write(f"MORPH_HASH={morph_hash}\n")
        f.write(f"REPAIR_HASH={repair_hash}\n")
        f.write(f"OPS_REAL_TIME={ops_actual}\n")
        f.write(f"METRICS={metrics}\n")
        
    print(f"✅ NX-17 Terminé. OPS/s: {ops_actual:.2f}")

if __name__ == "__main__":
    run_nx17_execution()
