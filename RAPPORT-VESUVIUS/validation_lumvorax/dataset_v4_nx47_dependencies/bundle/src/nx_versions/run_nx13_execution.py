from nx11_logger_engine import NX13Logger
import time
import random
import os

def run_nx13_stress_test():
    print("🚀 LANCEMENT NX-13 : RÉSILIENCE & PERSISTANCE D'IDENTITÉ")
    logger = NX13Logger("NX-13-RESILIENCE-01")
    
    results = []
    energy_total = 6000.0
    
    # Phase 1: Nominal Execution (50 events)
    print("[1/2] Exécution Nominale & Rotation Merkle...")
    for i in range(75): # Force au moins une rotation
        e_delta = -0.5 * random.random()
        energy_total += e_delta
        line = logger.log_event(
            domain="DISSIPATIVE_STABILITY",
            event_type="ROTATION_TEST",
            bit_trace=f"bit:{i}:1->0",
            state_before={"status": "stable"},
            state_after={"status": "evolving"},
            energy_delta=e_delta,
            energy_total=energy_total,
            invariant_density=0.98,
            regime="FUNCTIONAL_NX",
            phase_flags="0xFF",
            ion_data={"flux": random.gauss(0, 1)}
        )
        results.append(line)

    # Phase 2: Stress Test (30% Structural Destruction)
    print("[2/2] Test de Rupture (30% Destruction Structurelle)...")
    destruct_hash = logger.simulate_destruction(30)
    
    # On valide que l'identité survit
    identity_survivability = "CONFIRMED" if logger.current_merkle_root != "0"*64 else "FAILED"
    
    final_line = f"UTC_NS={int(time.time()*1e9)} EVENT_TYPE=DESTRUCTION_TEST PERCENTAGE=30 DESTRUCT_HASH={destruct_hash} IDENTITY_SURVIVAL={identity_survivability} FINAL_ROOT={logger.current_merkle_root}\n"
    results.append(final_line)

    log_dir = "logs_AIMO3/nx/NX-13"
    os.makedirs(log_dir, exist_ok=True)
    with open(f"{log_dir}/NX-13_forensic.log", "w") as f:
        f.writelines(results)
    
    print(f"✅ NX-13 Terminé. Survie de l'identité : {identity_survivability}")

if __name__ == "__main__":
    run_nx13_stress_test()
