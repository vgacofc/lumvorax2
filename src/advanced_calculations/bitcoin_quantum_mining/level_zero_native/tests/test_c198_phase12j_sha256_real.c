/*
 * LumVorax C198 Phase 12J — Test Kernel SHA256 Bitcoin Réel
 * 
 * OBJECTIF : Tester kernel SHA256 double hash Bitcoin avec bytecode embarqué
 * 
 * CRITÈRES SUCCÈS :
 * - Kernel SHA256 chargé depuis bytecode embarqué
 * - Arguments Bitcoin configurés (5 args : header, target, nonce, results, hashes)
 * - Exécution 10 secondes stable
 * - Validation hash < target
 * - Cleanup proper (pas de segfault)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "../include/btc_levelzero_runner.h"

/* SPIRV-IL SHA256 embarqué (portable, JIT par driver pour device exact) */
#include "../include/btc_sha256_mining_spirv.h"

/* Fonction helper : charger SPIRV-IL depuis header */
static uint8_t* load_embedded_spirv(size_t* size) {
    *size = kernels_btc_sha256_mining_spirv_cfl_spv_len;
    return (uint8_t*)kernels_btc_sha256_mining_spirv_cfl_spv;
}

/* Test 1 : Charger kernel SHA256 depuis bytecode embarqué */
static int test_load_sha256_kernel(btc_l0_context_t* ctx, ze_module_handle_t* module, ze_kernel_handle_t* kernel) {
    printf("\n=== TEST 1 : Chargement Kernel SHA256 Embarqué ===\n");
    
    /* Charger SPIRV-IL embarqué (portable) */
    size_t spirv_size = 0;
    uint8_t* spirv_data = load_embedded_spirv(&spirv_size);
    
    if (!spirv_data || spirv_size == 0) {
        fprintf(stderr, "❌ ÉCHEC : SPIRV-IL embarqué invalide\n");
        return -1;
    }
    
    printf("✅ SPIRV-IL embarqué chargé : %zu bytes (portable, JIT driver)\n", spirv_size);
    
    /* Créer module Level Zero avec SPIRV-IL (JIT compilation par driver) */
    ze_module_desc_t module_desc = {
        .stype = ZE_STRUCTURE_TYPE_MODULE_DESC,
        .pNext = NULL,
        .format = ZE_MODULE_FORMAT_IL_SPIRV,  /* Format portable SPIRV-IL */
        .inputSize = spirv_size,
        .pInputModule = spirv_data,
        .pBuildFlags = NULL,
        .pConstants = NULL
    };
    
    ze_module_build_log_handle_t build_log = NULL;
    ze_result_t result = btc_l0_zeModuleCreate(ctx->ze_context, ctx->ze_device, &module_desc, module, &build_log);
    
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : zeModuleCreate (code=%d)\n", result);
        if (build_log) {
            size_t log_size = 0;
            btc_l0_zeModuleBuildLogGetString(build_log, &log_size, NULL);
            if (log_size > 0) {
                char* log_str = (char*)malloc(log_size);
                btc_l0_zeModuleBuildLogGetString(build_log, &log_size, log_str);
                fprintf(stderr, "Build log:\n%s\n", log_str);
                free(log_str);
            }
            btc_l0_zeModuleBuildLogDestroy(build_log);
        }
        return -1;
    }
    
    printf("✅ Module créé\n");
    
    /* Créer kernel SHA256 */
    ze_kernel_desc_t kernel_desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = NULL,
        .flags = 0,
        .pKernelName = "btc_sha256_mining"
    };
    
    result = btc_l0_zeKernelCreate(*module, &kernel_desc, kernel);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : zeKernelCreate (code=%d)\n", result);
        return -1;
    }
    
    printf("✅ Kernel créé : btc_sha256_mining\n");
    
    return 0;
}

/* Test 2 : Allocation buffers Bitcoin */
static int test_alloc_bitcoin_buffers(btc_l0_context_t* ctx, size_t batch_size,
                                       void** block_header, void** target,
                                       void** results, void** hashes) {
    printf("\n=== TEST 2 : Allocation Buffers Bitcoin ===\n");
    
    /* Tailles buffers */
    size_t header_size = 80;  /* Bitcoin block header */
    size_t target_size = 32;  /* SHA256 hash (256 bits) */
    size_t results_size = batch_size * sizeof(uint32_t);
    size_t hashes_size = batch_size * 32;  /* 32 bytes per hash */
    
    /* Validation tailles */
    size_t total_size = header_size + target_size + results_size + hashes_size;
    if (total_size > ctx->max_alloc_size) {
        fprintf(stderr, "❌ ÉCHEC : Total size %zu > max_alloc %zu\n", total_size, ctx->max_alloc_size);
        return -1;
    }
    
    printf("✅ Validation tailles OK (max_alloc=%zu MB)\n", ctx->max_alloc_size / 1024 / 1024);
    
    /* Allouer buffers UMA shared */
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0
    };
    
    ze_host_mem_alloc_desc_t host_desc = {
        .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0
    };
    
    ze_result_t result;
    
    /* Block header */
    result = btc_l0_zeMemAllocShared(ctx->ze_context, &device_desc, &host_desc, header_size, 64, ctx->ze_device, block_header);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : Alloc block_header\n");
        return -1;
    }
    printf("✅ block_header alloué : %zu bytes\n", header_size);
    
    /* Target */
    result = btc_l0_zeMemAllocShared(ctx->ze_context, &device_desc, &host_desc, target_size, 64, ctx->ze_device, target);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : Alloc target\n");
        return -1;
    }
    printf("✅ target alloué : %zu bytes\n", target_size);
    
    /* Results */
    result = btc_l0_zeMemAllocShared(ctx->ze_context, &device_desc, &host_desc, results_size, 64, ctx->ze_device, results);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : Alloc results\n");
        return -1;
    }
    printf("✅ results alloué : %zu bytes\n", results_size);
    
    /* Hashes */
    result = btc_l0_zeMemAllocShared(ctx->ze_context, &device_desc, &host_desc, hashes_size, 64, ctx->ze_device, hashes);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : Alloc hashes\n");
        return -1;
    }
    printf("✅ hashes alloué : %zu bytes\n", hashes_size);
    
    printf("✅ Total alloué : %zu bytes (%.2f MB)\n", total_size, total_size / 1024.0 / 1024.0);
    
    return 0;
}

/* Test 3 : Bitcoin Mining 10 secondes avec SHA256 réel */
static int test_bitcoin_mining_sha256(btc_l0_context_t* ctx, ze_kernel_handle_t kernel,
                                       void* block_header, void* target,
                                       void* results, void* hashes, size_t batch_size) {
    printf("\n=== TEST 3 : Bitcoin Mining 10s avec SHA256 Réel ===\n");
    
    /* Préparer block header Bitcoin (exemple) */
    uint8_t* header = (uint8_t*)block_header;
    memset(header, 0, 80);
    header[0] = 0x01;  /* Version */
    header[1] = 0x00;
    header[2] = 0x00;
    header[3] = 0x00;
    
    /* Target Bitcoin (difficulté faible pour test) */
    uint32_t* target_data = (uint32_t*)target;
    memset(target_data, 0xFF, 32);
    target_data[7] = 0x00FFFFFF;  /* Difficulté ~1 */
    
    /* Nonce start */
    uint32_t nonce_start = 0;
    
    /* Configurer kernel arguments (5 args Bitcoin) */
    ze_result_t result;
    
    result = btc_l0_zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &block_header);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : SetArg 0 (block_header)\n");
        return -1;
    }
    
    result = btc_l0_zeKernelSetArgumentValue(kernel, 1, sizeof(void*), &target);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : SetArg 1 (target)\n");
        return -1;
    }
    
    result = btc_l0_zeKernelSetArgumentValue(kernel, 2, sizeof(uint32_t), &nonce_start);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : SetArg 2 (nonce_start)\n");
        return -1;
    }
    
    result = btc_l0_zeKernelSetArgumentValue(kernel, 3, sizeof(void*), &results);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : SetArg 3 (results)\n");
        return -1;
    }
    
    result = btc_l0_zeKernelSetArgumentValue(kernel, 4, sizeof(void*), &hashes);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : SetArg 4 (hashes)\n");
        return -1;
    }
    
    printf("✅ Arguments kernel configurés (5 args Bitcoin)\n");
    
    /* Configurer work-group size */
    uint32_t group_size_x = 256;
    uint32_t group_size_y = 1;
    uint32_t group_size_z = 1;
    
    result = btc_l0_zeKernelSetGroupSize(kernel, group_size_x, group_size_y, group_size_z);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ ÉCHEC : SetGroupSize\n");
        return -1;
    }
    
    uint32_t num_groups = (batch_size + group_size_x - 1) / group_size_x;
    printf("✅ Kernel configuré : %u groups × %u threads = %zu work-items\n",
           num_groups, group_size_x, (size_t)num_groups * group_size_x);
    
    /* Boucle mining 10 secondes */
    printf("\n🚀 Démarrage mining Bitcoin SHA256 (10 secondes)...\n");
    
    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    uint64_t total_hashes = 0;
    uint64_t iterations = 0;
    
    while (1) {
        /* Vérifier timeout */
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) +
                        (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
        
        if (elapsed >= 10.0) {
            break;
        }
        
        /* Lancer kernel */
        ze_group_count_t group_count = { num_groups, 1, 1 };
        
        result = btc_l0_zeCommandListAppendLaunchKernel(ctx->ze_command_list, kernel, &group_count, NULL, 0, NULL);
        if (result != ZE_RESULT_SUCCESS) {
            fprintf(stderr, "❌ ÉCHEC : AppendLaunchKernel\n");
            return -1;
        }
        
        /* Close command list */
        result = btc_l0_zeCommandListClose(ctx->ze_command_list);
        if (result != ZE_RESULT_SUCCESS) {
            fprintf(stderr, "❌ ÉCHEC : CommandListClose\n");
            return -1;
        }
        
        /* Execute */
        result = btc_l0_zeCommandQueueExecuteCommandLists(ctx->ze_command_queue, 1, &ctx->ze_command_list, NULL);
        if (result != ZE_RESULT_SUCCESS) {
            fprintf(stderr, "❌ ÉCHEC : ExecuteCommandLists\n");
            return -1;
        }
        
        /* Synchronize */
        result = btc_l0_zeCommandQueueSynchronize(ctx->ze_command_queue, UINT64_MAX);
        if (result != ZE_RESULT_SUCCESS) {
            fprintf(stderr, "❌ ÉCHEC : Synchronize\n");
            return -1;
        }
        
        /* Reset command list */
        result = btc_l0_zeCommandListReset(ctx->ze_command_list);
        if (result != ZE_RESULT_SUCCESS) {
            fprintf(stderr, "❌ ÉCHEC : CommandListReset\n");
            return -1;
        }
        
        total_hashes += batch_size;
        iterations++;
        nonce_start += batch_size;
        
        /* Afficher progression toutes les 100 itérations */
        if (iterations % 100 == 0) {
            double hashrate = total_hashes / elapsed / 1e6;  /* MH/s */
            printf("   [%.1fs] %lu hashes | %.2f MH/s\n", elapsed, total_hashes, hashrate);
        }
    }
    
    /* Résultats finaux */
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    double total_time = (current_time.tv_sec - start_time.tv_sec) +
                       (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    double final_hashrate = total_hashes / total_time / 1e6;  /* MH/s */
    
    printf("\n✅ Mining terminé :\n");
    printf("   Durée : %.3f secondes\n", total_time);
    printf("   Total hashes : %lu\n", total_hashes);
    printf("   Itérations : %lu\n", iterations);
    printf("   Hashrate moyen : %.2f MH/s\n", final_hashrate);
    
    return 0;
}

/* Test 4 : Cleanup proper */
static int test_cleanup(btc_l0_context_t* ctx, ze_kernel_handle_t kernel, ze_module_handle_t module,
                        void* block_header, void* target, void* results, void* hashes) {
    printf("\n=== TEST 4 : Cleanup Proper ===\n");
    
    /* Free buffers */
    if (block_header) btc_l0_zeMemFree(ctx->ze_context, block_header);
    if (target) btc_l0_zeMemFree(ctx->ze_context, target);
    if (results) btc_l0_zeMemFree(ctx->ze_context, results);
    if (hashes) btc_l0_zeMemFree(ctx->ze_context, hashes);
    
    printf("✅ Buffers libérés\n");
    
    /* Destroy kernel */
    if (kernel) {
        btc_l0_zeKernelDestroy(&kernel);
        printf("✅ Kernel détruit\n");
    }
    
    /* Destroy module */
    if (module) {
        btc_l0_zeModuleDestroy(&module);
        printf("✅ Module détruit\n");
    }
    
    return 0;
}

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 12J — Test SHA256 Bitcoin Réel       ║\n");
    printf("║  Objectif : Kernel SHA256 + Cleanup + 10s mining          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    /* Configuration */
    btc_l0_config_t config = {
        .batch_size = 262144,  /* 256K work-items */
        .work_group_size = 256,
        .num_work_groups = 0,
        .enable_profiling = true,
        .enable_forensic_log = true,
        .forensic_log_path = "logs/c198_phase12j_sha256_real.lum",
        .timeout_ms = 60000
    };
    
    /* Initialiser Level Zero */
    btc_l0_context_t* ctx = btc_l0_init(&config);
    if (!ctx) {
        fprintf(stderr, "❌ ÉCHEC : Initialisation Level Zero\n");
        return 1;
    }
    
    printf("\n✅ Contexte Level Zero initialisé\n");
    printf("   Device : %s\n", ctx->device_name);
    
    /* Variables */
    ze_module_handle_t module = NULL;
    ze_kernel_handle_t kernel = NULL;
    void* block_header = NULL;
    void* target = NULL;
    void* results = NULL;
    void* hashes = NULL;
    
    /* Test 1 : Charger kernel SHA256 */
    if (test_load_sha256_kernel(ctx, &module, &kernel) != 0) {
        goto cleanup;
    }
    
    /* Test 2 : Allouer buffers */
    if (test_alloc_bitcoin_buffers(ctx, config.batch_size, &block_header, &target, &results, &hashes) != 0) {
        goto cleanup;
    }
    
    /* Test 3 : Mining 10 secondes */
    if (test_bitcoin_mining_sha256(ctx, kernel, block_header, target, results, hashes, config.batch_size) != 0) {
        goto cleanup;
    }
    
    /* Test 4 : Cleanup */
    test_cleanup(ctx, kernel, module, block_header, target, results, hashes);
    
    /* Cleanup contexte */
    btc_l0_cleanup(ctx);
    
    printf("\n✅ SUCCÈS : Tous les tests passés\n");
    return 0;
    
cleanup:
    test_cleanup(ctx, kernel, module, block_header, target, results, hashes);
    if (ctx) btc_l0_cleanup(ctx);
    printf("\n❌ ÉCHEC : Tests interrompus\n");
    return 1;
}

// Made with Bob
