/* LumVorax C278 — Test Solution #1: Batch Size Réduit (32K)
 * 
 * OBJECTIF: Valider 100 dispatches sans GPU hang
 * MÉTHODE: Réduire batch_size de 262K → 32K (700ms → 87ms)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Configuration forcée */
#define BTC_BATCH_SIZE 32768
#define BTC_CTX_MAX_REUSE 2147483647

/* Inclure l'implémentation */
#include "../src/btc_gen9_native_runner.c"

int main(void) {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  C278 Solution #1: Batch Size Réduit (32K nonces)\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    /* Configuration */
    btc_gen9_config_t config = {
        .batch_size = 32768,  /* Solution #1: 32K nonces */
        .work_group_size = 256,
        .kernel_path = "kernels/btc_sha256_mining_gen9.bin",
        .log_path = "logs/c278_tests/solution1_batch32k.log",
        .enable_profiling = true,
        .preempt_timeout_ms = 0,  /* Pas de modification timeout */
        .target_hashrate = 0,
        .auto_batch_size = false
    };
    
    /* Initialisation */
    btc_gen9_context_t* ctx;
    printf("🔧 Initialisation contexte GPU...\n");
    if (btc_gen9_init(&ctx, &config) < 0) {
        fprintf(stderr, "❌ Erreur initialisation\n");
        return 1;
    }
    printf("✅ Init OK\n\n");
    
    /* Test 100 dispatches */
    printf("🧪 Test 100 dispatches (batch=32K, ~87ms/dispatch)...\n");
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
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  ✅ TEST RÉUSSI: Solution #1 validée\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}

// Made with Bob
