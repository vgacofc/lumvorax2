/* LumVorax C278 — Test Solution #4: Batch Size Dynamique
 * 
 * OBJECTIF: Calcul automatique batch size optimal
 * MÉTHODE: batch = (timeout_ms * hashrate * 0.9) / 1000
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Inclure l'implémentation */
#include "../src/btc_gen9_native_runner.c"

int main(void) {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  C278 Solution #4: Batch Size Dynamique\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    /* Configuration */
    btc_gen9_config_t config = {
        .batch_size = 0,  /* Sera calculé automatiquement */
        .work_group_size = 256,
        .kernel_path = "kernels/btc_sha256_mining_gen9.bin",
        .log_path = "logs/c278_tests/solution4_dynamic_batch.log",
        .enable_profiling = true,
        .preempt_timeout_ms = 0,
        .target_hashrate = 408000000,  /* 408 MH/s */
        .auto_batch_size = true  /* Solution #4 */
    };
    
    /* Initialisation */
    btc_gen9_context_t* ctx;
    printf("🔧 Initialisation contexte GPU...\n");
    printf("   Calcul automatique batch size optimal\n");
    if (btc_gen9_init(&ctx, &config) < 0) {
        fprintf(stderr, "❌ Erreur initialisation\n");
        return 1;
    }
    printf("✅ Init OK\n");
    printf("   Batch size calculé: %u nonces\n\n", ctx->batch_size);
    
    /* Test 100 dispatches */
    printf("🧪 Test 100 dispatches (batch dynamique)...\n");
    for (int i = 0; i < 100; i++) {
        if (btc_gen9_dispatch(ctx) < 0) {
            fprintf(stderr, "❌ Dispatch %d échoué\n", i+1);
            btc_gen9_cleanup(ctx);
            return 1;
        }
        
        if ((i+1) % 10 == 0) {
            printf("  ✓ %d/100 dispatches OK\n", i+1);
        }
    }
    
    /* Résultats */
    printf("\n✅ 100 dispatches réussis!\n");
    printf("📊 Hashrate: %.2f MH/s\n", 
           (ctx->total_hashes / 1000000.0) / ctx->total_time_sec);
    printf("⏱️  Temps moyen: %.2f ms/dispatch\n",
           (ctx->total_time_sec * 1000.0) / ctx->total_dispatches);
    printf("📦 Batch size utilisé: %u nonces\n", ctx->batch_size);
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  ✅ TEST RÉUSSI: Solution #4 validée\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}

// Made with Bob
