from nx11_logger_engine import NX14Logger
import time
import os

def run_nx14_execution():
    print("🌐 LANCEMENT NX-14 : SYNCHRONISATION & MÉTRIQUES SYSTÈME")
    unit_a = NX14Logger("NX-14-ALPHA")
    unit_b = NX14Logger("NX-14-BETA")
    
    # 1. Activité parallèle (Simulée)
    print("[1/3] Génération de flux ION Alpha & Beta...")
    for i in range(100):
        unit_a.log_event("TRANS", "ION", "b:0", {}, {}, -1.0, 5000, 0.9, "FUNC", "0x1")
        unit_b.log_event("TRANS", "ION", "b:1", {}, {}, -0.8, 5100, 0.9, "FUNC", "0x1")
        
    # 2. Fusion (Merkle Grafting)
    print("[2/3] Fusion des identités (Merkle Grafting)...")
    merge_hash = unit_a.merge_units(unit_b)
    
    # 3. Collecte Métriques
    print("[3/3] Collecte des métriques hardware...")
    metrics = unit_a.get_hardware_metrics()
    
    log_dir = "logs_AIMO3/nx/NX-14"
    os.makedirs(log_dir, exist_ok=True)
    with open(f"{log_dir}/NX-14_forensic.log", "w") as f:
        f.write(f"MERGE_HASH={merge_hash}\n")
        f.write(f"METRICS={metrics}\n")
        
    print(f"✅ NX-14 Terminé. OPS/s: {metrics['ops_per_second']:.2f}")

if __name__ == "__main__":
    run_nx14_execution()
