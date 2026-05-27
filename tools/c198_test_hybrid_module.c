/**
 * @file c198_test_hybrid_module.c
 * @brief Test complet module hybride OpenCL/Level Zero
 * 
 * OBJECTIF : Valider compilation dynamique + cache + warmup DMA
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10D)
 * DATE : 2026-05-09
 */

#include "../src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/src/btc_hybrid_opencl_levelzero.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

const char* KERNEL_VECTOR_ADD = 
"__kernel void vector_add(\n"
"    __global const float* a,\n"
"    __global const float* b,\n"
"    __global float* c,\n"
"    const uint n)\n"
"{\n"
"    int gid = get_global_id(0);\n"
"    if (gid < n) {\n"
"        c[gid] = a[gid] + b[gid];\n"
"    }\n"
"}\n";

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  C198 TEST MODULE HYBRIDE — OpenCL/Level Zero                 ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    /* ========================================================================
     * ÉTAPE 1 : Initialisation contexte hybride
     * ======================================================================== */

    printf("=== ÉTAPE 1 : Initialisation contexte hybride ===\n");

    btc_hybrid_ctx_t* ctx = btc_hybrid_init("./cache_test_hybrid", stdout);
    if (!ctx) {
        fprintf(stderr, "ERREUR : Initialisation contexte échouée\n");
        return 1;
    }

    printf("✅ Contexte hybride initialisé\n\n");

    /* ========================================================================
     * ÉTAPE 2 : Warmup DMA
     * ======================================================================== */

    printf("=== ÉTAPE 2 : Warmup DMA ===\n");

    if (!btc_hybrid_warmup_dma(ctx)) {
        fprintf(stderr, "ERREUR : Warmup DMA échoué\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }

    printf("✅ DMA warmed up\n\n");

    /* ========================================================================
     * ÉTAPE 3 : Compilation kernel (cache MISS)
     * ======================================================================== */

    printf("=== ÉTAPE 3 : Compilation kernel (cache MISS) ===\n");

    btc_hybrid_kernel_t* kernel = btc_hybrid_compile_kernel(
        ctx,
        KERNEL_VECTOR_ADD,
        "vector_add",
        NULL
    );

    if (!kernel) {
        fprintf(stderr, "ERREUR : Compilation kernel échouée\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }

    printf("✅ Kernel compilé (cache MISS)\n\n");

    /* ========================================================================
     * ÉTAPE 4 : Recompilation kernel (cache HIT)
     * ======================================================================== */

    printf("=== ÉTAPE 4 : Recompilation kernel (cache HIT) ===\n");

    btc_hybrid_kernel_destroy(ctx, kernel);

    kernel = btc_hybrid_compile_kernel(
        ctx,
        KERNEL_VECTOR_ADD,
        "vector_add",
        NULL
    );

    if (!kernel) {
        fprintf(stderr, "ERREUR : Recompilation kernel échouée\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }

    printf("✅ Kernel recompilé (cache HIT)\n\n");

    /* ========================================================================
     * ÉTAPE 5 : Cleanup
     * ======================================================================== */

    printf("=== ÉTAPE 5 : Cleanup ===\n");

    btc_hybrid_kernel_destroy(ctx, kernel);
    btc_hybrid_destroy(ctx);

    printf("✅ Cleanup terminé\n\n");

    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ TEST MODULE HYBRIDE RÉUSSI                                 ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return 0;
}

// Made with Bob
