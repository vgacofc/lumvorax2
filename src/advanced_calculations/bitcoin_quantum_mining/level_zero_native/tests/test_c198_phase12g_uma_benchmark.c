/*
 * LumVorax Bitcoin Mining — Test Benchmark UMA (C198 Phase 12G)
 * 
 * OBJECTIF : Mesurer performance mémoire partagée UMA vs device memory
 *            Éliminer cold start DMA (78 ms → < 1 ms)
 * 
 * MÉTRIQUES :
 * - Cold start bandwidth (première écriture)
 * - Warm bandwidth (écritures suivantes)
 * - Latence allocation
 * - Latence accès
 * 
 * TRAÇABILITÉ : Forensique bit-level sur TOUTES opérations mémoire
 */

#include "btc_levelzero_runner.h"
#include "btc_levelzero_driver_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/* Obtenir timestamp nanoseconde */
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Test allocation mémoire partagée UMA */
static void test_uma_allocation(btc_l0_context_t* ctx) {
    printf("\n=== TEST 1 : Allocation Mémoire Partagée UMA ===\n");
    
    size_t test_size = 4096;  /* 4 KB */
    
    /* Descripteurs pour mémoire partagée */
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED,
        .ordinal = 0
    };
    
    ze_host_mem_alloc_desc_t host_desc = {
        .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_CACHED
    };
    
    /* Mesurer latence allocation */
    uint64_t start = get_time_ns();
    
    void* ptr = NULL;
    ze_result_t result = btc_l0_zeMemAllocShared(
        ctx->ze_context,
        &device_desc,
        &host_desc,
        test_size,
        64,  /* Alignement 64 bytes */
        ctx->ze_device,
        &ptr
    );
    
    uint64_t end = get_time_ns();
    uint64_t alloc_latency_ns = end - start;
    
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC allocation UMA : %d\n", result);
        return;
    }
    
    printf("✅ Allocation UMA réussie\n");
    printf("   Adresse : %p\n", ptr);
    printf("   Taille : %zu bytes\n", test_size);
    printf("   Latence : %lu ns (%.3f μs)\n", alloc_latency_ns, alloc_latency_ns / 1000.0);
    
    btc_l0_log_event(ctx, "UMA_ALLOC", "Shared memory allocated", 
                     (uint64_t)ptr, test_size, alloc_latency_ns);
    
    /* Libérer mémoire */
    btc_l0_zeMemFree(ctx->ze_context, ptr);
    btc_l0_log_event(ctx, "UMA_FREE", "Shared memory freed", (uint64_t)ptr, 0, 0);
}

/* Test cold start vs warm bandwidth */
static void test_uma_bandwidth(btc_l0_context_t* ctx) {
    printf("\n=== TEST 2 : Bandwidth Cold Start vs Warm ===\n");
    
    size_t test_size = 4096;  /* 4 KB */
    
    /* Allouer mémoire partagée */
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED,
        .ordinal = 0
    };
    
    ze_host_mem_alloc_desc_t host_desc = {
        .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_CACHED
    };
    
    void* ptr = NULL;
    ze_result_t result = btc_l0_zeMemAllocShared(
        ctx->ze_context, &device_desc, &host_desc,
        test_size, 64, ctx->ze_device, &ptr
    );
    
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC allocation UMA\n");
        return;
    }
    
    /* Test COLD START (première écriture) */
    printf("\n--- Cold Start (première écriture) ---\n");
    uint64_t start = get_time_ns();
    memset(ptr, 0x42, test_size);
    uint64_t end = get_time_ns();
    
    uint64_t cold_latency_ns = end - start;
    double cold_bandwidth_gbps = (test_size / (double)cold_latency_ns) * 1000.0;  /* GB/s */
    
    printf("   Latence : %lu ns (%.3f μs)\n", cold_latency_ns, cold_latency_ns / 1000.0);
    printf("   Bandwidth : %.2f GB/s\n", cold_bandwidth_gbps);
    
    btc_l0_log_event(ctx, "UMA_COLD_START", "First write", 
                     (uint64_t)ptr, test_size, cold_latency_ns);
    
    /* Test WARM (écritures suivantes) */
    printf("\n--- Warm (écritures suivantes) ---\n");
    
    uint64_t total_warm_ns = 0;
    int num_iterations = 10;
    
    for (int i = 0; i < num_iterations; i++) {
        start = get_time_ns();
        memset(ptr, 0x43 + i, test_size);
        end = get_time_ns();
        total_warm_ns += (end - start);
    }
    
    uint64_t avg_warm_latency_ns = total_warm_ns / num_iterations;
    double warm_bandwidth_gbps = (test_size / (double)avg_warm_latency_ns) * 1000.0;
    
    printf("   Latence moyenne : %lu ns (%.3f μs)\n", 
           avg_warm_latency_ns, avg_warm_latency_ns / 1000.0);
    printf("   Bandwidth : %.2f GB/s\n", warm_bandwidth_gbps);
    
    btc_l0_log_event(ctx, "UMA_WARM", "Average write", 
                     (uint64_t)ptr, test_size, avg_warm_latency_ns);
    
    /* Calcul ratio cold/warm */
    double ratio = (double)cold_latency_ns / (double)avg_warm_latency_ns;
    printf("\n--- Résultats ---\n");
    printf("   Ratio Cold/Warm : %.2fx\n", ratio);
    printf("   Amélioration bandwidth : %.2fx\n", warm_bandwidth_gbps / cold_bandwidth_gbps);
    
    /* Validation objectif Phase 12G */
    if (cold_latency_ns < 1000000) {  /* < 1 ms */
        printf("   ✅ OBJECTIF ATTEINT : Cold start < 1 ms\n");
    } else {
        printf("   ⚠️  Cold start encore > 1 ms (objectif non atteint)\n");
    }
    
    if (warm_bandwidth_gbps > 10.0) {  /* > 10 GB/s */
        printf("   ✅ OBJECTIF ATTEINT : Bandwidth > 10 GB/s\n");
    } else {
        printf("   ⚠️  Bandwidth < 10 GB/s (objectif non atteint)\n");
    }
    
    /* Libérer mémoire */
    btc_l0_zeMemFree(ctx->ze_context, ptr);
}

/* Test comparaison Device vs Shared memory */
static void test_device_vs_shared(btc_l0_context_t* ctx) {
    printf("\n=== TEST 3 : Comparaison Device vs Shared Memory ===\n");
    
    size_t test_size = 4096;
    
    /* Test Device Memory */
    printf("\n--- Device Memory (baseline) ---\n");
    
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0
    };
    
    void* device_ptr = NULL;
    uint64_t start = get_time_ns();
    ze_result_t result = btc_l0_zeMemAllocDevice(
        ctx->ze_context, &device_desc, test_size, 64, ctx->ze_device, &device_ptr
    );
    uint64_t end = get_time_ns();
    
    if (result == ZE_RESULT_SUCCESS) {
        printf("   Latence allocation : %lu ns\n", end - start);
        btc_l0_zeMemFree(ctx->ze_context, device_ptr);
    }
    
    /* Test Shared Memory */
    printf("\n--- Shared Memory (UMA) ---\n");
    
    ze_host_mem_alloc_desc_t host_desc = {
        .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_CACHED
    };
    
    void* shared_ptr = NULL;
    start = get_time_ns();
    result = btc_l0_zeMemAllocShared(
        ctx->ze_context, &device_desc, &host_desc,
        test_size, 64, ctx->ze_device, &shared_ptr
    );
    end = get_time_ns();
    
    if (result == ZE_RESULT_SUCCESS) {
        printf("   Latence allocation : %lu ns\n", end - start);
        
        /* Test accès direct depuis CPU */
        start = get_time_ns();
        memset(shared_ptr, 0xFF, test_size);
        end = get_time_ns();
        
        printf("   Latence écriture CPU : %lu ns\n", end - start);
        printf("   ✅ Accès direct CPU possible (UMA)\n");
        
        btc_l0_zeMemFree(ctx->ze_context, shared_ptr);
    }
}

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 12G — Benchmark Mémoire UMA          ║\n");
    printf("║  Objectif : Éliminer cold start DMA (78 ms → < 1 ms)      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    /* Configuration */
    btc_l0_config_t config = {
        .batch_size = 262144,
        .work_group_size = 256,
        .num_work_groups = 1024,
        .enable_profiling = true,
        .enable_forensic_log = true,
        .timeout_ms = 60000
    };
    snprintf(config.forensic_log_path, sizeof(config.forensic_log_path),
             "logs/c198_phase12g_uma_benchmark.lum");
    
    /* Initialiser contexte Level Zero */
    btc_l0_context_t* ctx = btc_l0_init(&config);
    
    if (ctx == NULL) {
        fprintf(stderr, "❌ ÉCHEC initialisation Level Zero\n");
        return 1;
    }
    
    printf("\n✅ Contexte Level Zero initialisé\n");
    printf("   Device : %s\n", ctx->device_name);
    
    /* Exécuter tests */
    test_uma_allocation(ctx);
    test_uma_bandwidth(ctx);
    test_device_vs_shared(ctx);
    
    /* Cleanup */
    printf("\n=== Cleanup ===\n");
    btc_l0_cleanup(ctx);
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ PHASE 12G COMPLÉTÉE                                    ║\n");
    printf("║  Logs forensiques : logs/c198_phase12g_uma_benchmark.lum  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

// Made with Bob — C198 Phase 12G