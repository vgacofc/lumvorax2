/*
 * Test C396 - Runner pour kernel manuel avec opcodes OpenCL réels
 * Objectif: Valider que output[0-15] = 0xDEADBEEF
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
    printf("║  TEST C396 - VALIDATION KERNEL MANUEL                     ║\n");
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
    
    // Afficher info device
    ze_device_properties_t device_props = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES
    };
    CHECK_ZE(zeDeviceGetProperties(device, &device_props));
    printf("📱 Device: %s\n", device_props.name);
    printf("   Type: %s\n", device_props.type == ZE_DEVICE_TYPE_GPU ? "GPU" : "Other");
    printf("\n");
    
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
    
    // Charger kernel binaire natif
    const char* kernel_path = "kernels/test_c396_from_opencl.bin";
    FILE* f = fopen(kernel_path, "rb");
    if (!f) {
        printf("❌ Impossible d'ouvrir %s\n", kernel_path);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* kernel_data = malloc(kernel_size);
    fread(kernel_data, 1, kernel_size, f);
    fclose(f);
    
    printf("✅ Kernel chargé: %zu bytes\n", kernel_size);
    
    // Créer module depuis binaire natif
    ze_module_desc_t module_desc = {
        .stype = ZE_STRUCTURE_TYPE_MODULE_DESC,
        .pNext = NULL,
        .format = ZE_MODULE_FORMAT_NATIVE,  // Format natif (pas SPIR-V)
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
        
        // Afficher build log
        size_t log_size = 0;
        zeModuleBuildLogGetString(build_log, &log_size, NULL);
        if (log_size > 0) {
            char* log_str = malloc(log_size);
            zeModuleBuildLogGetString(build_log, &log_size, log_str);
            printf("Build log:\n%s\n", log_str);
            free(log_str);
        }
        
        return 1;
    }
    
    printf("✅ Module créé\n");
    
    // Créer kernel
    ze_kernel_desc_t kernel_desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = NULL,
        .flags = 0,
        .pKernelName = "test_magic_values"  // Nom du kernel OpenCL
    };
    
    ze_kernel_handle_t kernel;
    CHECK_ZE(zeKernelCreate(module, &kernel_desc, &kernel));
    printf("✅ Kernel créé: %s\n", kernel_desc.pKernelName);
    
    // Allouer buffer output (16 uint32_t pour SIMD16)
    const size_t output_size = 16 * sizeof(uint32_t);
    
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0
    };
    
    void* output_buffer;
    CHECK_ZE(zeMemAllocDevice(context, &device_desc, output_size, 64, device, &output_buffer));
    
    // Initialiser à zéro
    CHECK_ZE(zeCommandListAppendMemoryFill(cmd_list, output_buffer, &(uint32_t){0}, sizeof(uint32_t), output_size, NULL, 0, NULL));
    
    printf("✅ Buffer alloué: %zu bytes\n", output_size);
    
    // Configurer argument kernel
    CHECK_ZE(zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &output_buffer));
    
    // Configurer group size
    CHECK_ZE(zeKernelSetGroupSize(kernel, 16, 1, 1));  // SIMD16
    
    // Lancer kernel
    ze_group_count_t group_count = {1, 1, 1};
    CHECK_ZE(zeCommandListAppendLaunchKernel(cmd_list, kernel, &group_count, NULL, 0, NULL));
    
    // Barrier
    CHECK_ZE(zeCommandListAppendBarrier(cmd_list, NULL, 0, NULL));
    
    // Fermer command list
    CHECK_ZE(zeCommandListClose(cmd_list));
    
    printf("🚀 Exécution kernel...\n");
    
    // Exécuter
    CHECK_ZE(zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL));
    CHECK_ZE(zeCommandQueueSynchronize(queue, UINT64_MAX));
    
    printf("✅ Exécution terminée\n\n");
    
    // Lire résultats
    uint32_t output_host[16];
    CHECK_ZE(zeCommandListReset(cmd_list));
    CHECK_ZE(zeCommandListAppendMemoryCopy(cmd_list, output_host, output_buffer, output_size, NULL, 0, NULL));
    CHECK_ZE(zeCommandListClose(cmd_list));
    CHECK_ZE(zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL));
    CHECK_ZE(zeCommandQueueSynchronize(queue, UINT64_MAX));
    
    // Afficher résultats
    printf("📊 Résultats:\n");
    int success_count = 0;
    for (int i = 0; i < 16; i++) {
        printf("   output[%2d] = 0x%08X", i, output_host[i]);
        if (output_host[i] == 0xDEADBEEF) {
            printf(" ✅\n");
            success_count++;
        } else {
            printf(" ❌ (attendu: 0xDEADBEEF)\n");
        }
    }
    
    printf("\n");
    printf("📈 Résumé: %d/16 valeurs correctes\n", success_count);
    
    if (success_count == 16) {
        printf("\n");
        printf("🎉 SUCCÈS TOTAL!\n");
        printf("   - Kernel manuel fonctionne\n");
        printf("   - Opcodes OpenCL validés\n");
        printf("   - Indépendance OpenCL confirmée\n");
        printf("\n");
    } else {
        printf("\n❌ ÉCHEC: Certaines valeurs incorrectes\n\n");
    }
    
    // Cleanup
    zeMemFree(context, output_buffer);
    zeKernelDestroy(kernel);
    zeModuleDestroy(module);
    zeCommandListDestroy(cmd_list);
    zeCommandQueueDestroy(queue);
    zeContextDestroy(context);
    free(kernel_data);
    
    return (success_count == 16) ? 0 : 1;
}

// Made with Bob
