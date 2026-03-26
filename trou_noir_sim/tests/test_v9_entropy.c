#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "../physics/kerr_metric.h"
#include "../logging/log_writer.h"
#include "../core/time_ns.h"

/**
 * TEST_V9_ENTROPY_ANALYSIS
 * Mesure la perte d'information computationnelle (Entropie de Calcul)
 * à l'approche de la singularité r=0.
 */
void entropy_test() {
    printf("--- ANALYSE D'ENTROPIE COMPUTATIONNELLE (V9) ---\n");
    kerr_metric_t metric;
    kerr_metric_init(&metric, 1.0, 0.999);
    
    geodesic_state_t state = { .r = 0.5, .ut = 1.0, .ur = -0.1 };
    
    for(int i=0; i<100; i++) {
        double prev_r = state.r;
        kerr_geodesic_step(&metric, &state, 0.005);
        
        // Calcul de l'entropie par la dérive de précision
        double drift = fabs(state.r - (prev_r - 0.0005)); // Simplifié pour le test
        log_writer_entry("ENTROPY", "DRIFT_BIT", (uint64_t)(drift * 1e18));
        
        if (i % 20 == 0) {
            printf("Step %d | Entropy Drift: %.2e | Core Stability: %s\n", 
                   i, drift, drift < 1e-10 ? "STABLE" : "DEGRADED");
        }
    }
    printf("Analyse V9 terminée.\n");
}

int main() {
    entropy_test();
    return 0;
}
