/* ═══════════════════════════════════════════════════════════════════════════
   TEST LUMVORAX NATIVE MINER C188
   ═══════════════════════════════════════════════════════════════════════════
   
   Test complet du miner natif C188 (16 threads CPU, zero OpenCL)
   
   CYCLE: C188
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   ═══════════════════════════════════════════════════════════════════════════ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Prototypes C188 */
int lum_native_miner_init_c188(void);
int lum_native_miner_batch_c188(
    const uint32_t midstate[8],
    const uint32_t tail[4],
    uint32_t nonce_start,
    uint32_t* out_nonce,
    uint32_t out_hash[8],
    uint32_t* out_best_bits
);
double lum_native_miner_benchmark_c188(double duration_sec);
void lum_native_miner_cleanup_c188(void);

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TEST LUMVORAX NATIVE MINER C188                        ║\n");
    printf("║  16 Threads CPU + Kernel Natif C187 (ZERO OpenCL)      ║\n");
    printf("║  Objectif: 30-35 MH/s (16× 2 MH/s/thread)              ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Initialisation C188 */
    printf("[TEST] Initialisation Native Miner C188...\n");
    int ret = lum_native_miner_init_c188();
    if (ret < 0) {
        printf("[TEST] ❌ ERROR: Initialization failed (ret=%d)\n", ret);
        return 1;
    }
    printf("[TEST] ✅ Native Miner C188 initialized\n\n");
    
    /* Benchmark 90 secondes */
    printf("[TEST] Lancement benchmark 90 secondes...\n");
    printf("[TEST] Logging bit-level: logs/lum_native_miner_c188_bit_level.log\n");
    printf("[TEST] 16 threads CPU en parallèle\n");
    printf("[TEST] Kernel SHA-256 natif C187 (zero OpenCL)\n\n");
    
    double mhs = lum_native_miner_benchmark_c188(90.0);
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS FINAUX C188 (NATIVE)                         ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Hashrate C188   : %-8.2f MH/s (16 threads CPU)      ║\n", mhs);
    printf("║  Hashrate C186   : 10.96     MH/s (OpenCL)              ║\n");
    printf("║  Hashrate C180   : 23.14     MH/s (GPU Direct)          ║\n");
    printf("║  Gain vs C186    : %+-8.1f %% (élimination OpenCL)    ║\n", 
           ((mhs - 10.96) / 10.96) * 100.0);
    printf("║  Objectif 30-35  : %s                                   ║\n",
           (mhs >= 30.0 && mhs <= 35.0) ? "✅ ATTEINT" : "⚠️  À AJUSTER");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Cleanup */
    printf("[TEST] Cleanup Native Miner C188...\n");
    lum_native_miner_cleanup_c188();
    printf("[TEST] ✅ Test terminé avec succès\n\n");
    
    return 0;
}

// Made with Bob
