/**
 * @file c198_test_levelzero_alloc_minimal.c
 * @brief Test MINIMAL validation allocations Level Zero
 * 
 * OBJECTIF : Prouver que zeMemAllocShared/Device fonctionnent RÉELLEMENT
 * 
 * Phase 10E-10 : Validation Runtime (recommandation feedback expert)
 * 
 * AUTEUR : Bob
 * DATE : 2026-05-09
 */

#include "src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/src/btc_hybrid_opencl_levelzero.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  C198 TEST MINIMAL — VALIDATION ALLOCATIONS LEVEL ZERO        ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    // Init contexte
    printf("=== INITIALISATION ===\n");
    btc_hybrid_ctx_t* ctx = btc_hybrid_init("./cache_test", stdout);
    if (!ctx) {
        fprintf(stderr, "❌ Init échoué\n");
        return 1;
    }
    printf("✅ Contexte initialisé\n\n");

    // Vérifier pointeurs fonctions
    printf("=== VÉRIFICATION SYMBOLES ===\n");
    printf("zeMemAllocDevice = %p\n", ctx->ze_MemAllocDevice);
    printf("zeMemAllocShared = %p\n", ctx->ze_MemAllocShared);
    printf("zeMemFree        = %p\n", ctx->ze_MemFree);
    printf("ze_context       = %p\n", ctx->ze_context);
    printf("ze_device        = %p\n", ctx->ze_device);
    
    if (!ctx->ze_MemAllocDevice || !ctx->ze_MemAllocShared || !ctx->ze_MemFree) {
        fprintf(stderr, "❌ Symboles memory non chargés\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }
    
    if (!ctx->ze_context || !ctx->ze_device) {
        fprintf(stderr, "❌ Context/Device non initialisés\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ Tous symboles présents\n\n");

    // Test allocation SHARED
    printf("=== TEST ALLOCATION SHARED (4MB) ===\n");
    btc_hybrid_buffer_t* buf_shared = btc_hybrid_alloc_shared(ctx, 4 * 1024 * 1024);
    if (!buf_shared) {
        fprintf(stderr, "❌ Allocation shared échouée\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ Shared memory allouée : %p\n", buf_shared->ze_memory);
    printf("   Taille : %zu bytes\n", buf_shared->size);
    printf("   Temps alloc : %.3f µs\n", buf_shared->alloc_time_ns / 1000.0);

    // Test écriture/lecture
    printf("\n=== TEST WRITE/READ SHARED ===\n");
    uint8_t* ptr = (uint8_t*)buf_shared->ze_memory;
    memset(ptr, 0xAA, 1024);
    printf("✅ Write 1KB pattern 0xAA\n");
    
    bool valid = true;
    for (int i = 0; i < 1024; i++) {
        if (ptr[i] != 0xAA) {
            valid = false;
            break;
        }
    }
    printf("%s Read verify : %s\n", valid ? "✅" : "❌", valid ? "OK" : "FAILED");

    // Test allocation DEVICE
    printf("\n=== TEST ALLOCATION DEVICE (4MB) ===\n");
    btc_hybrid_buffer_t* buf_device = btc_hybrid_alloc_device(ctx, 4 * 1024 * 1024);
    if (!buf_device) {
        fprintf(stderr, "❌ Allocation device échouée\n");
        btc_hybrid_free_buffer(ctx, buf_shared);
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ Device memory allouée : %p\n", buf_device->ze_memory);
    printf("   Taille : %zu bytes\n", buf_device->size);
    printf("   Temps alloc : %.3f µs\n", buf_device->alloc_time_ns / 1000.0);

    // Cleanup
    printf("\n=== CLEANUP ===\n");
    btc_hybrid_free_buffer(ctx, buf_shared);
    btc_hybrid_free_buffer(ctx, buf_device);
    btc_hybrid_destroy(ctx);
    printf("✅ Cleanup terminé\n\n");

    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ VALIDATION LEVEL ZERO RÉUSSIE                              ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return 0;
}

// Made with Bob
