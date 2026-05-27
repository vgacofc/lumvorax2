#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "../core/time_ns.h"
#include "../physics/kerr_metric.h"

void run_hpc_performance_test() {
    printf("--- RUNNING V7 HPC PERFORMANCE TEST ---\n");
    
    kerr_metric_t metric;
    kerr_metric_init(&metric, 1e6, 0.999); // Spin Ultra-Rapide
    
    geodesic_state_t state = { .r = 2.1 * metric.mass, .ut = 1.0, .ur = -0.1 };
    
    uint64_t start = time_ns_get_absolute();
    for(int i=0; i<1000000; i++) {
        kerr_geodesic_step(&metric, &state, 0.001);
    }
    uint64_t end = time_ns_get_absolute();
    
    double total_time_sec = (end - start) / 1e9;
    double ops_per_sec = 1000000 / total_time_sec;
    
    printf("V7 Results:\n");
    printf("Total Time: %.4f s\n", total_time_sec);
    printf("Calculations/sec: %.2e\n", ops_per_sec);
    printf("Avg Latency/step: %.2f ns\n", (double)(end - start) / 1000000.0);
}

int main() {
    run_hpc_performance_test();
    return 0;
}
