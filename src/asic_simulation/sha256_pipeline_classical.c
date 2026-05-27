/* sha256_pipeline_classical.c — Modèle comportemental ASIC SHA-256.
 *
 * Cycle C95. Reproduit l'estimation TH/s d'un ASIC type Antminer S19 XP :
 *   throughput ≈ engines × clock_GHz × 1e9 / 64  (1 hash = 64 rounds)
 */
#include "asic_simulation.h"
#include <string.h>

void asic_sha256_init(asic_sha256_classical_t *a, int engines,
                      double clock_GHz, double w_per_th) {
    if (!a) return;
    memset(a, 0, sizeof(*a));
    a->pipeline_depth = 64;
    a->parallel_engines = engines > 0 ? engines : 1;
    a->clock_GHz = clock_GHz > 0.0 ? clock_GHz : 0.5;
    a->watts_per_TH = w_per_th > 0.0 ? w_per_th : 21.5;
}

double asic_sha256_estimated_th_s(const asic_sha256_classical_t *a) {
    if (!a) return 0.0;
    /* hash/s = engines × clock_Hz / pipeline_depth — pipelinage idéal */
    const double hash_per_s = (double)a->parallel_engines
                              * (a->clock_GHz * 1.0e9)
                              / (double)a->pipeline_depth;
    return hash_per_s / 1.0e12;   /* TH/s */
}

double asic_sha256_power_watts(const asic_sha256_classical_t *a) {
    if (!a) return 0.0;
    return asic_sha256_estimated_th_s(a) * a->watts_per_TH;
}
