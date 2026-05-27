#include <stdio.h>
#include "physics/kerr_metric.h"
#include "logging/log_writer.h"

void run_simulation(double mass, double spin, double start_r, const char* test_name) {
    kerr_metric_t m;
    geodesic_state_t s = {0, start_r, 1.57, 0, 1.0, -0.1, 0, 0.01};
    
    log_init_session();
    kerr_metric_init(&m, mass, spin);
    log_writer_entry("TEST_METADATA", test_name, (uint64_t)(spin * 1000));
    
    printf("Running Simulation: %s (Spin: %.3f)...\n", test_name, spin);
    for(int i=0; i<5000; i++) {
        // V15 : Utilisation de Kerr-Schild pour les trajectoires proches de l'horizon
        if (s.r < m.horizon_plus * 1.5) {
            kerr_schild_geodesic_step(&m, &s, 0.005);
        } else {
            kerr_geodesic_step(&m, &s, 0.005);
        }
        
        if (s.r <= 0.1) { // Proche singularité
            log_writer_entry("FINAL_STATE", "SINGULARITY_APPROACH", 1);
            break;
        }
    }
}

int main() {
    // Test 1: Spin Standard
    run_simulation(1.0, 0.5, 10.0, "STANDARD_SPIN");
    
    // Test 2: Spin Critique (Frame-dragging maximal)
    run_simulation(1.0, 0.999, 10.0, "CRITICAL_SPIN");
    
    // Test 3: Approche Ultra-proche
    run_simulation(1.0, 0.998, 2.5, "NEAR_HORIZON_INJECTION");

    printf("\nAll simulations complete. Check trou_noir_sim/logs/ for timestamped sessions.\n");
    return 0;
}
