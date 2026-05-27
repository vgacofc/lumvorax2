#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "../physics/kerr_metric.h"
#include "../logging/log_writer.h"

/**
 * TEST_SINGULARITY_PROBE (V8 - Centre du Trou Noir)
 * Objectif : Sonder le point 0 (r -> 0) pour identifier la structure de la singularité.
 */
void singularity_probe() {
    printf("--- SONDE DE SINGULARITÉ (V8 - POINT 0) ---\n");
    kerr_metric_t metric;
    kerr_metric_init(&metric, 1.0, 0.99); // Masse unité, spin élevé
    
    geodesic_state_t state = { .r = 0.1, .ut = 1.0, .ur = -0.5 };
    
    printf("Entrée dans la zone de singularité (r < 0.1)...\n");
    
    for(int i=0; i<50; i++) {
        double ds = 0.001;
        kerr_geodesic_step(&metric, &state, ds);
        
        // Surveillance critique du point 0
        if (isnan(state.r) || isinf(state.r) || state.r < 1e-15) {
            printf("[CRITICAL] Singularité atteinte à l'itération %d | r: %.2e\n", i, state.r);
            log_writer_entry("SINGULARITY", "ZERO_POINT_REACHED", (uint64_t)i);
            break;
        }
        
        if (i % 10 == 0) {
            printf("Probe r: %.6f | Potential: %.2e\n", state.r, 1.0/state.r);
        }
    }
}

int main() {
    singularity_probe();
    return 0;
}
