/**
 * @file c198_test_hybrid_minimal.c
 * @brief Test minimal module hybride OpenCL/Level Zero
 * 
 * OBJECTIF : Valider compilation + init + cleanup
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10D)
 * DATE : 2026-05-09
 */

#include "../src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/src/btc_hybrid_opencl_levelzero.h"
#include <stdio.h>
#include <stdlib.h>

const char* KERNEL_TEST = 
"__kernel void test_minimal(__global float* output) {\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = 42.0f;\n"
"}\n";

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  C198 TEST MODULE HYBRIDE MINIMAL                             ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    /* ========================================================================
     * ÉTAPE 1 : Initialisation
     * ======================================================================== */

    printf("=== ÉTAPE 1 : Initialisation ===\n");

    btc_hybrid_ctx_t* ctx = btc_hybrid_init("./cache_test_minimal", stdout);
    if (!ctx) {
        fprintf(stderr, "❌ ERREUR : Initialisation échouée\n");
        return 1;
    }

    printf("✅ Contexte initialisé\n\n");

    /* ========================================================================
     * ÉTAPE 2 : Warmup DMA
     * ======================================================================== */

    printf("=== ÉTAPE 2 : Warmup DMA ===\n");

    if (!btc_hybrid_warmup_dma(ctx)) {
        fprintf(stderr, "❌ ERREUR : Warmup DMA échoué\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }

    printf("✅ DMA warmed up\n\n");

    /* ========================================================================
     * ÉTAPE 3 : Compilation kernel
     * ======================================================================== */

    printf("=== ÉTAPE 3 : Compilation kernel ===\n");

    btc_hybrid_kernel_t* kernel = btc_hybrid_compile_kernel(
        ctx,
        KERNEL_TEST,
        "test_minimal",
        NULL
    );

    if (!kernel) {
        fprintf(stderr, "❌ ERREUR : Compilation échouée\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }

    printf("✅ Kernel compilé : %zu bytes\n", kernel->isa_size);
    printf("✅ Temps compilation : %.3f ms\n", kernel->compile_time_ns / 1000000.0);
    printf("✅ Cache : %s\n\n", kernel->from_cache ? "HIT" : "MISS");

    /* ========================================================================
     * ÉTAPE 4 : Statistiques
     * ======================================================================== */

    printf("=== ÉTAPE 4 : Statistiques ===\n");

    uint64_t hits, misses, total;
    btc_hybrid_get_cache_stats(ctx, &hits, &misses, &total);

    printf("Total compilations : %lu\n", total);
    printf("Cache hits : %lu\n", hits);
    printf("Cache misses : %lu\n\n", misses);

    /* ========================================================================
     * ÉTAPE 5 : Cleanup
     * ======================================================================== */

    printf("=== ÉTAPE 5 : Cleanup ===\n");

    btc_hybrid_free_kernel(ctx, kernel);
    btc_hybrid_destroy(ctx);

    printf("✅ Cleanup terminé\n\n");

    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ TEST MODULE HYBRIDE MINIMAL RÉUSSI                         ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return 0;
}

// Made with Bob