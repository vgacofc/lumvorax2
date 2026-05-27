from nx11_logger_engine import NX18Logger
import time
import os
import random

def run_nx18_stress_test():
    print("🚀 LANCEMENT NX-18 : STRESS TEST 80% & ANTICIPATION IONIQUE")
    
    # Tentative d'atteindre la limite physique
    TARGET_NEURONS = 250000 
    LOAD_STEP = 50000
    
    logger = NX18Logger("NX-18-STRESS-GIGA")
    
    print(f"[1/3] Test d'Anticipation Ionique...")
    logger.anticipate_ion_load(TARGET_NEURONS)
    
    print(f"[2/3] Montée en charge progressive jusqu'à saturation...")
    current_neurons = 0
    start_time = time.time()
    
    try:
        while current_neurons < TARGET_NEURONS:
            current_neurons += LOAD_STEP
            print(f"   -> Population : {current_neurons} neurones...")
            for i in range(5000):
                logger.log_event("STRESS", "LOAD", f"n:{i}", {}, {}, -1.0, 50000, 0.99, "FUNC", "0xBB")
            
            metrics = logger.get_real_hardware_metrics()
            print(f"      CPU: {metrics['cpu_usage_actual']}% | RAM: {metrics['ram_usage_mb']:.2f} MB")
            
            if metrics['cpu_usage_actual'] > 85 or metrics['ram_usage_mb'] > 3500: # Seuil 80% Replit approx
                print("⚠️ SEUIL 80% ATTEINT. Stabilisation...")
                break
    except Exception as e:
        print(f"❌ Erreur lors du stress test: {e}")

    duration = time.time() - start_time
    final_metrics = logger.get_real_hardware_metrics()
    
    log_dir = "logs_AIMO3/nx/NX-18"
    os.makedirs(log_dir, exist_ok=True)
    with open(f"{log_dir}/NX-18_forensic.log", "w") as f:
        f.write(f"MAX_NEURONS_REACHED={current_neurons}\n")
        f.write(f"FINAL_METRICS={final_metrics}\n")
        f.write(f"DURATION={duration}\n")
        
    print(f"✅ NX-18 Terminé. Neurones Max : {current_neurons}. OPS/s: {final_metrics['ops_per_second']:.2f}")

if __name__ == "__main__":
    run_nx18_stress_test()
