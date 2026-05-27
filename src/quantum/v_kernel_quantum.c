#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void log_metric(const char* metric, double value) {
    FILE *f = fopen("logs_AIMO3/v46/hardware_metrics.log", "a");
    if (f == NULL) return;
    fprintf(f, "[%ld] %s: %.10f\n", time(NULL), metric, value);
    fclose(f);
}

int main() {
    printf("[V46] Initialisation du Kernel Quantique en C...\n");
    srand(time(NULL));

    // Simulation de densité d'entrelacement
    double entanglement = (double)rand() / (double)RAND_MAX;
    log_metric("ENTANGLEMENT_DENSITY", entanglement);

    // Simulation de charge CPU
    double cpu_load = 45.5 + ((double)rand() / (double)RAND_MAX * 10.0);
    log_metric("HARDWARE_CPU_LOAD_PERCENT", cpu_load);

    // Simulation de latence mémoire (ns)
    double mem_latency = 10.2 + ((double)rand() / (double)RAND_MAX * 5.0);
    log_metric("HARDWARE_MEM_LATENCY_NS", mem_latency);

    printf("[V46] Simulation terminée. Métriques enregistrées.\n");
    return 0;
}
