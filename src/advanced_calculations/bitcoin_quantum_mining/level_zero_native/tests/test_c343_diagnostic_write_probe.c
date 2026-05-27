/*
 * LumVorax C343 — TEST DIAGNOSTICS WRITE PROBE
 * Objectif : Identifier ROOT CAUSE du bug output[]=0
 * 
 * TESTS DIAGNOSTICS :
 * 1. Memory Write Probe : Détecter si GPU écrit réellement
 * 2. Surface States Validation : Vérifier adresses non-nulles
 * 3. Binding Table Check : Vérifier alignement 64 bytes
 * 4. Interface Descriptor Dump : Détecter kernel_start=0
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "btc_gen9_native_runner.h"

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C343 — DIAGNOSTICS WRITE PROBE                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    btc_gen9_context_t* ctx = NULL;
    
    /* Configuration avec logs forensiques activés */
    btc_gen9_config_t config = {
        .batch_size = 256,  /* Petit batch pour test rapide */
        .work_group_size = 256,
        .kernel_path = "kernels/test_diagnostic_c342_pure_gen9.bin_kbl.bin",
        .log_path = "logs/forensic/test_gen9_native.log",
        .enable_profiling = true
    };
    
    printf("[C343_INIT] Initializing runner with diagnostics...\n");
    int ret = btc_gen9_init(&ctx, &config);
    if (ret != 0) {
        fprintf(stderr, "[C343_ERROR] Init failed: %d\n", ret);
        return 1;
    }
    
    printf("[C343_EXECUTE] Running single dispatch...\n");
    ret = btc_gen9_execute(ctx);
    if (ret != 0) {
        fprintf(stderr, "[C343_ERROR] Execute failed: %d\n", ret);
        btc_gen9_cleanup(ctx);
        return 1;
    }
    
    printf("[C343_RESULT] Reading output buffer...\n");
    void* output_map = btc_gen9_get_output_map(ctx);
    size_t output_size = btc_gen9_get_output_size(ctx);
    
    if (!output_map) {
        fprintf(stderr, "[C343_ERROR] Output buffer not mapped\n");
        btc_gen9_cleanup(ctx);
        return 1;
    }
    
    uint32_t* output = (uint32_t*)output_map;
    uint32_t result = output[0];
    
    /* Compter combien de valeurs ont changé depuis le pattern BEFORE */
    int memory_changed = 0;
    for (size_t i = 0; i < output_size / sizeof(uint32_t); i++) {
        if (output[i] != 0xDEADBEEF) {
            memory_changed++;
        }
    }
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C343 DIAGNOSTIC RESULTS                                  ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Output[0]       = 0x%08X                             ║\n", result);
    printf("║  Memory Changed  = %d / %zu values                     ║\n", 
           memory_changed, output_size / sizeof(uint32_t));
    printf("║  Expected        = NON-ZERO if GPU writes                ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    if (result == 0 && memory_changed == 0) {
        printf("[C343_ANALYSIS] ❌ GPU did NOT write to output buffer\n");
        printf("[C343_ANALYSIS] Check forensic logs for:\n");
        printf("  - C343_MEMORY_PROBE: memory_changed count\n");
        printf("  - C343_SURFACE_CHECK: Surface States addresses\n");
        printf("  - C343_BINDING_TABLE: Binding Table entries\n");
        printf("  - C343_IDD_DUMP: Interface Descriptor kernel_start\n");
    } else if (memory_changed > 0) {
        printf("[C343_ANALYSIS] ✅ GPU wrote to memory!\n");
        printf("[C343_ANALYSIS] Changed values: %d\n", memory_changed);
        if (result != 0) {
            printf("[C343_ANALYSIS] First non-zero nonce found: 0x%08X\n", result);
        }
    } else {
        printf("[C343_ANALYSIS] ⚠️  Partial write detected\n");
    }
    
    btc_gen9_cleanup(ctx);
    
    printf("\n[C343_COMPLETE] Test finished. Check logs/forensic/test_gen9_native.log\n");
    return 0;
}

// Made with Bob
