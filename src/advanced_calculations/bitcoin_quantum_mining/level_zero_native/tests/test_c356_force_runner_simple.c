/*
 * Test C356 Force Leading Zero - Runner Simple Level Zero
 * 
 * Ce test exécute le kernel sur 1000 nonces et affiche
 * la distribution des leading zeros pour valider count_leading_zeros()
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <level_zero/ze_api.h>

#define CHECK_ZE(call) do { \
    ze_result_t result = (call); \
    if (result != ZE_RESULT_SUCCESS) { \
        fprintf(stderr, "❌ Level Zero error at %s:%d: %d\n", __FILE__, __LINE__, result); \
        exit(1); \
    } \
} while(0)

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C356 - FORCE LEADING ZERO DETECTION                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // Initialiser Level Zero
    CHECK_ZE(zeInit(0));
    
    // Obtenir driver
    uint32_t driver_count = 0;
    CHECK_ZE(zeDriverGet(&driver_count, NULL));
    if (driver_count == 0) {
        printf("❌ Aucun driver Level Zero trouvé\n");
        return 1;
    }
    
    ze_driver_handle_t driver;
    CHECK_ZE(zeDriverGet(&driver_count, &driver));
    
    // Obtenir device
    uint32_t device_count = 0;
    CHECK_ZE(zeDeviceGet(driver, &device_count, NULL));
    if (device_count == 0) {
        printf("❌ Aucun device trouvé\n");
        return 1;
    }
    
    ze_device_handle_t device;
    CHECK_ZE(zeDeviceGet(driver, &device_count, &device));
    
    // Créer contexte
    ze_context_desc_t context_desc = {
        .stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC,
        .pNext = NULL,
        .flags = 0
    };
    ze_context_handle_t context;
    CHECK_ZE(zeContextCreate(driver, &context_desc, &context));
    
    // Créer command queue
    ze_command_queue_desc_t queue_desc = {
        .stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,
        .pNext = NULL,
        .ordinal = 0,
        .index = 0,
        .flags = 0,
        .mode = ZE_COMMAND_QUEUE_MODE_DEFAULT,
        .priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL
    };
    ze_command_queue_handle_t queue;
    CHECK_ZE(zeCommandQueueCreate(context, device, &queue_desc, &queue));
    
    // Créer command list
    ze_command_list_desc_t list_desc = {
        .stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
        .pNext = NULL,
        .commandQueueGroupOrdinal = 0,
        .flags = 0
    };
    ze_command_list_handle_t cmd_list;
    CHECK_ZE(zeCommandListCreate(context, device, &list_desc, &cmd_list));
    
    // Charger kernel SPIR-V
    FILE* f = fopen("tests/test_c356_simple.spv", "rb");
    if (!f) {
        printf("❌ Impossible d'ouvrir tests/test_c356_simple.spv\n");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* kernel_data = malloc(kernel_size);
    fread(kernel_data, 1, kernel_size, f);
    fclose(f);
    
    printf("✓ Kernel chargé: %zu bytes\n", kernel_size);
    
    // Créer module depuis SPIR-V
    ze_module_desc_t module_desc = {
        .stype = ZE_STRUCTURE_TYPE_MODULE_DESC,
        .pNext = NULL,
        .format = ZE_MODULE_FORMAT_IL_SPIRV,
        .inputSize = kernel_size,
        .pInputModule = kernel_data,
        .pBuildFlags = NULL,
        .pConstants = NULL
    };
    ze_module_handle_t module;
    ze_module_build_log_handle_t build_log;
    ze_result_t result = zeModuleCreate(context, device, &module_desc, &module, &build_log);
    
    if (result != ZE_RESULT_SUCCESS) {
        printf("❌ Erreur création module: %d\n", result);
        if (build_log) {
            size_t log_size = 0;
            zeModuleBuildLogGetString(build_log, &log_size, NULL);
            if (log_size > 0) {
                char* log_str = malloc(log_size);
                zeModuleBuildLogGetString(build_log, &log_size, log_str);
                printf("Build log:\n%s\n", log_str);
                free(log_str);
            }
            zeModuleBuildLogDestroy(build_log);
        }
        free(kernel_data);
        return 1;
    }
    free(kernel_data);
    
    // Créer kernel
    ze_kernel_desc_t kernel_desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = NULL,
        .flags = 0,
        .pKernelName = "test_leading_zeros"
    };
    ze_kernel_handle_t kernel;
    CHECK_ZE(zeKernelCreate(module, &kernel_desc, &kernel));
    
    // Configuration test
    uint32_t start_nonce = 0;
    uint32_t count = 1000;  // Tester 1000 nonces
    
    printf("📋 Configuration:\n");
    printf("   Start nonce: %u\n", start_nonce);
    printf("   Count: %u nonces\n", count);
    printf("   Output size: %u uints (10 per nonce)\n", count * 10);
    printf("\n");
    
    // Block header Bitcoin de test (80 bytes)
    uint8_t block_header[80] = {
        0x01, 0x00, 0x00, 0x00, // version
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, // prev_block
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // merkle_root
        0x00, 0x00, 0x00, 0x00, // timestamp
        0x00, 0x00, 0x00, 0x00, // bits
        0x00, 0x00, 0x00, 0x00  // nonce (sera remplacé)
    };
    
    // Allouer buffers GPU
    ze_device_mem_alloc_desc_t alloc_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0
    };
    
    void* d_header;
    void* d_results;
    
    CHECK_ZE(zeMemAllocDevice(context, &alloc_desc, 80, 64, device, &d_header));
    CHECK_ZE(zeMemAllocDevice(context, &alloc_desc, count * 10 * sizeof(uint32_t), 64, device, &d_results));
    
    // Copier header vers GPU
    CHECK_ZE(zeCommandListAppendMemoryCopy(cmd_list, d_header, block_header, 80, NULL, 0, NULL));
    
    // Configurer kernel arguments
    CHECK_ZE(zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &d_header));
    CHECK_ZE(zeKernelSetArgumentValue(kernel, 1, sizeof(uint32_t), &start_nonce));
    CHECK_ZE(zeKernelSetArgumentValue(kernel, 2, sizeof(void*), &d_results));
    
    // Configurer group size
    CHECK_ZE(zeKernelSetGroupSize(kernel, 256, 1, 1));
    
    // Lancer kernel
    uint32_t group_count_x = (count + 255) / 256;
    ze_group_count_t dispatch = { group_count_x, 1, 1 };
    
    CHECK_ZE(zeCommandListAppendLaunchKernel(cmd_list, kernel, &dispatch, NULL, 0, NULL));
    
    // Copier résultats vers CPU
    uint32_t* results = malloc(count * 10 * sizeof(uint32_t));
    CHECK_ZE(zeCommandListAppendMemoryCopy(cmd_list, results, d_results, count * 10 * sizeof(uint32_t), NULL, 0, NULL));
    
    // Exécuter
    CHECK_ZE(zeCommandListClose(cmd_list));
    CHECK_ZE(zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL));
    CHECK_ZE(zeCommandQueueSynchronize(queue, UINT64_MAX));
    
    printf("✓ Kernel exécuté avec succès\n\n");
    
    // Analyser résultats
    printf("📊 Distribution des leading zeros:\n");
    printf("   Leading Zeros | Count\n");
    printf("   --------------|------\n");
    
    uint32_t distribution[256] = {0};
    for (uint32_t i = 0; i < count; i++) {
        uint32_t nonce = results[i * 10 + 0];
        uint32_t leading_zeros = results[i * 10 + 1];
        distribution[leading_zeros]++;
    }
    
    for (uint32_t lz = 0; lz < 32; lz++) {
        if (distribution[lz] > 0) {
            printf("   %13u | %u\n", lz, distribution[lz]);
        }
    }
    
    // Afficher quelques exemples
    printf("\n📋 Exemples (premiers 10 nonces):\n");
    for (uint32_t i = 0; i < 10 && i < count; i++) {
        uint32_t nonce = results[i * 10 + 0];
        uint32_t leading_zeros = results[i * 10 + 1];
        printf("   Nonce %u: %u leading zeros\n", nonce, leading_zeros);
        printf("      Hash: ");
        for (int j = 0; j < 8; j++) {
            printf("%08x ", results[i * 10 + 2 + j]);
        }
        printf("\n");
    }
    
    // Cleanup
    free(results);
    zeMemFree(context, d_header);
    zeMemFree(context, d_results);
    zeKernelDestroy(kernel);
    zeModuleDestroy(module);
    zeCommandListDestroy(cmd_list);
    zeCommandQueueDestroy(queue);
    zeContextDestroy(context);
    
    printf("\n✅ Test terminé avec succès\n");
    return 0;
}

// Made with Bob
