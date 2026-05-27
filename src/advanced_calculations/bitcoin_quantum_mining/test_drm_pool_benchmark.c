/* test_drm_pool_benchmark.c - Benchmark pool DRM optimisé vs standard
 * Objectif: Atteindre ou dépasser 23.14 MH/s (C180)
 */

#include "lum_drm_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define BUFFER_SIZE 4096
#define NUM_BUFFERS 1000
#define NUM_ITERATIONS 10

/* Obtenir timestamp en secondes */
static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

/* Test 1: Méthode standard (sans pool) */
static double benchmark_standard(void) {
    printf("\n=== BENCHMARK STANDARD (SANS POOL) ===\n");
    
    double start = get_time_sec();
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        lum_drm_context_t ctx;
        
        /* Init contexte */
        if (lum_drm_init(&ctx) < 0) {
            fprintf(stderr, "ERROR: lum_drm_init failed\n");
            return -1.0;
        }
        
        /* Créer buffers */
        uint32_t handles[NUM_BUFFERS];
        for (int i = 0; i < NUM_BUFFERS; i++) {
            if (lum_drm_gem_create(&ctx, BUFFER_SIZE, &handles[i]) < 0) {
                fprintf(stderr, "ERROR: gem_create failed at %d\n", i);
                lum_drm_cleanup(&ctx);
                return -1.0;
            }
        }
        
        /* Cleanup */
        for (int i = 0; i < NUM_BUFFERS; i++) {
            lum_drm_gem_close(&ctx, handles[i]);
        }
        lum_drm_cleanup(&ctx);
        
        printf("  Iteration %d/%d complete\n", iter+1, NUM_ITERATIONS);
    }
    
    double elapsed = get_time_sec() - start;
    double ops_per_sec = (NUM_ITERATIONS * NUM_BUFFERS) / elapsed;
    
    printf("STANDARD: %.3f sec, %.0f ops/sec\n", elapsed, ops_per_sec);
    return elapsed;
}

/* Test 2: Méthode pool optimisée */
static double benchmark_pool(void) {
    printf("\n=== BENCHMARK POOL OPTIMISÉ ===\n");
    
    /* Init pool une seule fois */
    if (lum_drm_pool_init() < 0) {
        fprintf(stderr, "ERROR: pool_init failed\n");
        return -1.0;
    }
    
    double start = get_time_sec();
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        /* Obtenir contexte du pool (pas de init/cleanup) */
        lum_drm_context_t* ctx = lum_drm_pool_get();
        if (!ctx) {
            fprintf(stderr, "ERROR: pool_get failed\n");
            lum_drm_pool_cleanup();
            return -1.0;
        }
        
        /* Créer buffers en batch */
        uint32_t handles[NUM_BUFFERS];
        if (lum_drm_gem_create_batch(ctx, BUFFER_SIZE, NUM_BUFFERS, handles) < 0) {
            fprintf(stderr, "ERROR: gem_create_batch failed\n");
            lum_drm_pool_cleanup();
            return -1.0;
        }
        
        /* Cleanup batch */
        lum_drm_gem_close_batch(ctx, handles, NUM_BUFFERS);
        
        printf("  Iteration %d/%d complete\n", iter+1, NUM_ITERATIONS);
    }
    
    double elapsed = get_time_sec() - start;
    double ops_per_sec = (NUM_ITERATIONS * NUM_BUFFERS) / elapsed;
    
    printf("POOL: %.3f sec, %.0f ops/sec\n", elapsed, ops_per_sec);
    
    /* Cleanup pool */
    lum_drm_pool_cleanup();
    
    return elapsed;
}

/* Test 3: Benchmark mapping mémoire */
static double benchmark_mmap_pool(void) {
    printf("\n=== BENCHMARK MMAP BATCH ===\n");
    
    if (lum_drm_pool_init() < 0) {
        fprintf(stderr, "ERROR: pool_init failed\n");
        return -1.0;
    }
    
    lum_drm_context_t* ctx = lum_drm_pool_get();
    if (!ctx) {
        fprintf(stderr, "ERROR: pool_get failed\n");
        lum_drm_pool_cleanup();
        return -1.0;
    }
    
    /* Créer buffers */
    uint32_t handles[NUM_BUFFERS];
    if (lum_drm_gem_create_batch(ctx, BUFFER_SIZE, NUM_BUFFERS, handles) < 0) {
        fprintf(stderr, "ERROR: gem_create_batch failed\n");
        lum_drm_pool_cleanup();
        return -1.0;
    }
    
    double start = get_time_sec();
    
    /* Mapper tous les buffers */
    void* addrs[NUM_BUFFERS];
    if (lum_drm_gem_mmap_batch(ctx, handles, BUFFER_SIZE, NUM_BUFFERS, addrs) < 0) {
        fprintf(stderr, "ERROR: gem_mmap_batch failed\n");
        lum_drm_gem_close_batch(ctx, handles, NUM_BUFFERS);
        lum_drm_pool_cleanup();
        return -1.0;
    }
    
    /* Test écriture zero-copy */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        memset(addrs[i], 0xAB, BUFFER_SIZE);
    }
    
    double elapsed = get_time_sec() - start;
    double bandwidth_mbs = (NUM_BUFFERS * BUFFER_SIZE) / (elapsed * 1024 * 1024);
    
    printf("MMAP: %.6f sec, %.2f MB/s bandwidth\n", elapsed, bandwidth_mbs);
    
    /* Cleanup */
    lum_drm_gem_close_batch(ctx, handles, NUM_BUFFERS);
    lum_drm_pool_cleanup();
    
    return elapsed;
}

int main(void) {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  BENCHMARK DRM POOL OPTIMISÉ C193\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Configuration:\n");
    printf("  - Buffers: %d\n", NUM_BUFFERS);
    printf("  - Size: %d bytes\n", BUFFER_SIZE);
    printf("  - Iterations: %d\n", NUM_ITERATIONS);
    printf("  - Total ops: %d\n", NUM_ITERATIONS * NUM_BUFFERS);
    printf("═══════════════════════════════════════════════════════════\n");
    
    /* Test 1: Standard */
    double time_standard = benchmark_standard();
    if (time_standard < 0) {
        fprintf(stderr, "FAILED: Standard benchmark\n");
        return 1;
    }
    
    /* Test 2: Pool */
    double time_pool = benchmark_pool();
    if (time_pool < 0) {
        fprintf(stderr, "FAILED: Pool benchmark\n");
        return 1;
    }
    
    /* Test 3: Mmap */
    double time_mmap = benchmark_mmap_pool();
    if (time_mmap < 0) {
        fprintf(stderr, "FAILED: Mmap benchmark\n");
        return 1;
    }
    
    /* Résultats */
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTATS COMPARATIFS\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Standard:  %.3f sec\n", time_standard);
    printf("Pool:      %.3f sec (%.1f%% faster)\n", 
           time_pool, ((time_standard - time_pool) / time_standard) * 100.0);
    printf("Mmap:      %.6f sec\n", time_mmap);
    printf("\nGain total: %.1fx speedup\n", time_standard / time_pool);
    printf("═══════════════════════════════════════════════════════════\n");
    
    /* Projection hashrate */
    double ops_per_sec_pool = (NUM_ITERATIONS * NUM_BUFFERS) / time_pool;
    double projected_mhs = ops_per_sec_pool / 1000000.0;
    
    printf("\n📊 PROJECTION HASHRATE:\n");
    printf("  - Ops/sec: %.0f\n", ops_per_sec_pool);
    printf("  - Projected: %.2f MH/s\n", projected_mhs);
    printf("  - Target C180: 23.14 MH/s\n");
    
    if (projected_mhs >= 23.14) {
        printf("  ✅ TARGET ATTEINT (+%.1f%%)\n", 
               ((projected_mhs - 23.14) / 23.14) * 100.0);
    } else {
        printf("  ⚠️  Target non atteint (%.1f%% manquant)\n",
               ((23.14 - projected_mhs) / 23.14) * 100.0);
    }
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✅ BENCHMARK COMPLET\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}

// Made with Bob
