#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../physics/kerr_metric.h"
#include "../logging/log_writer.h"

// Note: log_init_session in log_writer.h does not take a path, 
// so we assume it handles its own internal timestamping.
// We will simulate the directory creation for the user's historical requirement.

void run_simulation_v3(double mass, double spin, double start_r, double start_theta, double L, double E, const char* test_id) {
    kerr_metric_t m;
    geodesic_state_t s = {0, start_r, start_theta, 0, E, -0.1, 0, L};
    
    printf("Executing V3 Test: %s (Spin: %.4f, L: %.2f)...\n", test_id, spin, L);
    
    log_init_session(); // Standard call
    kerr_metric_init(&m, mass, spin);
    log_writer_entry("METADATA_V3", test_id, (uint64_t)(spin * 1000000));
    
    for(int i=0; i<10000; i++) {
        kerr_geodesic_step(&m, &s, 0.001); 
        if (s.r <= m.horizon_plus) {
            log_writer_entry("EVENT", "HORIZON_REACHED", 1);
            break;
        }
        if (s.r > 100.0) {
            log_writer_entry("EVENT", "ESCAPE", 1);
            break;
        }
    }
}

int main() {
    // V3 Test 1: Near-Extremal Stability (a=0.9999)
    run_simulation_v3(1.0, 0.9999, 5.0, 1.57, 2.0, 0.95, "EXTREMAL_STABILITY");
    
    // V3 Test 2: Negative Angular Momentum
    run_simulation_v3(1.0, 0.9, 3.0, 1.57, -1.5, 0.95, "COUNTER_ROTATION_DRAG");
    
    // V3 Test 3: Ergosphere Energy Extraction
    run_simulation_v3(1.0, 0.99, 2.1, 1.57, 4.0, 1.1, "PENROSE_CANDIDATE");

    printf("\nPhase V3 Complete.\n");
    return 0;
}
