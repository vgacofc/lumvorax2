/* C279: Test validation des 3 heaps GPU (SSH, DSH, IOH)
 * Objectif: Vérifier que les heaps éliminent les GPU hangs
 * Attendu: 100 dispatches sans errno=5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../include/btc_gen9_native_runner.h"

int main(void) {
    printf("=== C279: Test 3 Heaps GPU (SSH + DSH + IOH) ===\n\n");
    
    /* Configuration */
    btc_gen9_config_t config = {
        .kernel_path = "opencl_vector_add_gen9_FULL.bin",
        .input_size = 1024 * 1024,
        .output_size = 1024 * 1024,
        .batch_size = 32768,
        .work_group_size = 256,
        .log_path = "test_c279_heaps.log",
        .preempt_timeout_ms = 0,
        .auto_batch_size = 0,
        .target_hashrate = 0
    };
    
    /* Initialiser */
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0) {
        fprintf(stderr, "ERREUR: btc_gen9_init failed\n");
        return 1;
    }
    
    printf("✅ Initialisation réussie\n");
    printf("   - SSH: handle=%u size=%zu\n", ctx->ssh_bo, ctx->ssh_size);
    printf("   - DSH: handle=%u size=%zu\n", ctx->dsh_bo, ctx->dsh_size);
    printf("   - IOH: handle=%u size=%zu\n", ctx->ioh_bo, ctx->ioh_size);
    printf("\n");
    
    /* Préparer données input */
    uint32_t* input = (uint32_t*)ctx->input_map;
    for (size_t i = 0; i < config.input_size / sizeof(uint32_t); i++) {
        input[i] = i;
    }
    
    /* Test: 100 dispatches */
    printf("Test: 100 dispatches GPU...\n");
    int success_count = 0;
    int fail_count = 0;
    
    for (int i = 0; i < 100; i++) {
        ret = btc_gen9_dispatch(ctx);
        
        if (ret == 0) {
            success_count++;
            if ((i + 1) % 10 == 0) {
                printf("  [%3d/100] ✅ OK (total: %d succès, %d échecs)\n", 
                       i + 1, success_count, fail_count);
            }
        } else {
            fail_count++;
            printf("  [%3d/100] ❌ ÉCHEC errno=%d (%s)\n", 
                   i + 1, errno, strerror(errno));
            
            if (errno == 5) {
                printf("\n❌ ÉCHEC: errno=5 (EIO) détecté au dispatch %d\n", i + 1);
                printf("   Cause: Contexte banni après GPU hangs\n");
                break;
            }
        }
    }
    
    printf("\n=== RÉSULTATS ===\n");
    printf("Dispatches réussis: %d/100\n", success_count);
    printf("Dispatches échoués: %d/100\n", fail_count);
    
    if (success_count == 100) {
        printf("\n✅ SUCCÈS COMPLET: 100 dispatches sans GPU hang !\n");
        printf("   Les 3 heaps GPU (SSH + DSH + IOH) ont résolu le problème.\n");
    } else if (success_count > 27) {
        printf("\n✅ AMÉLIORATION: %d dispatches (vs 27 avant)\n", success_count);
        printf("   Les heaps ont partiellement résolu le problème.\n");
    } else {
        printf("\n❌ ÉCHEC: Toujours limité à %d dispatches\n", success_count);
        printf("   Les heaps n'ont pas résolu le problème.\n");
    }
    
    /* Statistiques */
    printf("\n=== STATISTIQUES ===\n");
    printf("Total dispatches: %lu\n", ctx->total_dispatches);
    printf("Temps total: %.3f sec\n", ctx->total_time_sec);
    if (ctx->total_time_sec > 0) {
        printf("Hashrate moyen: %.2f MH/s\n", 
               (ctx->total_dispatches * config.batch_size) / ctx->total_time_sec / 1000000.0);
    }
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    return (success_count == 100) ? 0 : 1;
}

// Made with Bob
