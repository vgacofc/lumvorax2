/* ═══════════════════════════════════════════════════════════════════════════
   TEST LUMVORAX GPU DIRECT - Programme de test C180
   ═══════════════════════════════════════════════════════════════════════════ */

#include "src/lum_gpu_direct.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TEST LUMVORAX GPU DIRECT - CYCLE C180                   ║\n");
    printf("║  Validation overheads transferts CPU↔GPU                 ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Configuration */
    lum_gpu_direct_config_t config = {
        .batch_size = 262144,      /* 262k nonces */
        .workgroup_size = 64,
        .target_bits = 28
    };
    
    /* Initialisation */
    printf("[C180-TEST] Initialisation GPU Direct...\n");
    if (lum_gpu_direct_init(&config) < 0) {
        fprintf(stderr, "[C180-TEST] ERROR: Initialisation failed\n");
        return 1;
    }
    
    /* Benchmark 10 secondes */
    printf("\n[C180-TEST] Lancement benchmark 10 secondes...\n\n");
    double mhs = lum_gpu_direct_benchmark_mhs(262144, 10.0);
    
    /* Statistiques finales */
    lum_gpu_direct_stats_t stats;
    lum_gpu_direct_get_stats(&stats);
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS FINAUX C180                                   ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Batches traités : %-10lu                          ║\n", stats.total_batches);
    printf("║  Hashes calculés : %-10lu                          ║\n", stats.total_hashes);
    printf("║  Hashrate moyen  : %-10.2f MH/s                    ║\n", stats.average_mhs);
    printf("║  Hashrate final  : %-10.2f MH/s                    ║\n", mhs);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Cleanup */
    lum_gpu_direct_cleanup();
    
    printf("[C180-TEST] ✅ Test terminé avec succès\n\n");
    
    return 0;
}

// Made with  by Bob - LumVorax C180

// Made with Bob
