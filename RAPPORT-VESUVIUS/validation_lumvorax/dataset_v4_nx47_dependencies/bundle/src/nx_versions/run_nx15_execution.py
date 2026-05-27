from nx11_logger_engine import NX15Logger
import time
import os
import random

def run_nx15_execution():
    print("🎭 LANCEMENT NX-15 : RÉSONANCE & ÉLAGAGE")
    # Simulation d'une population de 1000 neurones (logique groupee)
    population_size = 1000
    group_logger = NX15Logger("NX-15-GROUP-COLLECTIVE")
    
    print(f"[1/3] Simulation de {population_size} neurones sous stimuli divergents...")
    for i in range(500):
        conflict = random.random()
        group_logger.log_event("RESONANCE", "STIMULUS", f"n:{i}", {}, {}, -1.2, 10000, 0.95, "FUNC", "0xEE")
        if conflict > 0.8:
            group_logger.resolve_resonance(conflict)
            
    # 2. Pruning
    print("[2/3] Élagage Causal (Pruning 50% des logs)...")
    nodes_left = group_logger.prune_logs(50)
    
    # 3. Métriques Finales
    print("[3/3] Analyse des métriques hardware et population...")
    metrics = group_logger.get_hardware_metrics()
    
    log_dir = "logs_AIMO3/nx/NX-15"
    os.makedirs(log_dir, exist_ok=True)
    with open(f"{log_dir}/NX-15_forensic.log", "w") as f:
        f.write(f"POPULATION_SIZE={population_size}\n")
        f.write(f"NODES_POST_PRUNING={nodes_left}\n")
        f.write(f"RESONANCE_FINAL={group_logger.resonance_index}\n")
        f.write(f"METRICS={metrics}\n")
        
    print(f"✅ NX-15 Terminé. OPS/s Post-Pruning: {metrics['ops_per_second']:.2f}")

if __name__ == "__main__":
    run_nx15_execution()
