/* ═══════════════════════════════════════════════════════════════════════════
   TEST LUMVORAX GPU DIRECT V2 - C181-C185
   ═══════════════════════════════════════════════════════════════════════════ */

#include "src/lum_gpu_direct_v2.c"
#include <stdio.h>

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TEST LUMVORAX GPU DIRECT V2 - CYCLES C181-C185         ║\n");
    printf("║  Objectif: 30-35 MH/s (vs 23.14 MH/s C180)              ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Initialisation complète C181-C185 */
    if (lum_gpu_v2_init_complete_c185() < 0) {
        fprintf(stderr, "[TEST] ERROR: Initialisation failed\n");
        return 1;
    }
    
    /* Benchmark 10 secondes */
    printf("[TEST] Lancement benchmark 10 secondes...\n\n");
    double mhs = lum_gpu_v2_benchmark_c185(10.0);
    
    /* Résultats finaux */
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS FINAUX C181-C185                              ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Hashrate C185   : %-10.2f MH/s                    ║\n", mhs);
    printf("║  Hashrate C180   : 23.14      MH/s                    ║\n");
    printf("║  Gain vs C180    : +%-9.1f%%                        ║\n", 
           ((mhs - 23.14) / 23.14) * 100);
    printf("║  Objectif 30-35  : %s                              ║\n",
           (mhs >= 30.0 && mhs <= 35.0) ? "✅ ATTEINT    " : "⚠️  À AJUSTER ");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Cleanup */
    lum_gpu_v2_cleanup_c185();
    
    printf("[TEST] ✅ Test terminé avec succès\n\n");
    
    return 0;
}

// Made with ❤️ by Bob - LumVorax C181-C185

// Made with Bob
