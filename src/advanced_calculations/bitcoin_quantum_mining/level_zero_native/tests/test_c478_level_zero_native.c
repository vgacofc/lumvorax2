/*
 * TEST C478 - LEVEL ZERO NATIVE (API INTEL OFFICIELLE)
 *
 * RÉVOLUTION: Utilisation de Level Zero au lieu de i915 DRM direct
 * Level Zero = API Intel qui gère automatiquement batch buffers + relocations
 * 
 * Avantages:
 * - Gestion automatique batch buffers (comme OpenCL)
 * - Gestion automatique relocations
 * - Gestion automatique binding tables
 * - API stable et supportée par Intel
 * - SANS OpenCL (natif pur)
 *
 * Objectif: Premier 0xDEADBEEF GPU natif via Level Zero ✅
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <level_zero/ze_api.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

#define ZE_CHECK(call) do { \
    ze_result_t result = call; \
    if (result != ZE_RESULT_SUCCESS) { \
        FORENSIC_LOG("❌ Level Zero error: %s returned %d", #call, result); \
        exit(1); \
    } \
} while(0)

/* Kernel ISA Gen9 - Section .text extraite */
static const unsigned char kernel_text[] = {
    0x01,0x00,0x60,0x00,0x0c,0x02,0x40,0x20,0x00,0x00,0x20,0x00,
    0x00,0x00,0x00,0x00,0x06,0x80,0x00,0x00,0x04,0x00,0x00,0x30,
    0x00,0x10,0x00,0x16,0xc0,0x04,0xc0,0x04,0x01,0x00,0x00,0x00,
    0x0c,0x47,0x80,0x20,0xef,0xbe,0xad,0xde,0xbe,0xba,0xfe,0xca,
    0x01,0x00,0x60,0x00,0x6c,0x26,0x70,0x20,0x00,0x00,0x00,0x00,
    0x40,0xc8,0x00,0x00,0x01,0x00,0x00,0x00,0x0c,0x47,0x88,0x20,
    0x78,0x56,0x34,0x12,0x00,0xef,0xcd,0xab,0x01,0x4d,0x00,0x20,
    0x07,0x7f,0x02,0x00,0x01,0xcd,0x02,0x20,0x00,0x08,0x04,0x00,
    0x40,0x00,0x60,0x00,0x0c,0x43,0xc0,0x20,0x60,0x00,0x00,0x1a,
    0x70,0x00,0x8d,0x00,0x01,0xc3,0x02,0x60,0x05,0x08,0x04,0x00,
    0x33,0x00,0x60,0x0c,0x14,0x80,0x00,0x00,0xc1,0x00,0x00,0x00,
    0xff,0x81,0x06,0x04,0x31,0x00,0x00,0x07,0x04,0x02,0x00,0x20,
    0xe0,0x0f,0x00,0x06,0x10,0x00,0x00,0x82,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
#define KERNEL_TEXT_SIZE 320

int main() {
    FORENSIC_LOG("🚀 TEST C478 - LEVEL ZERO NATIVE (API INTEL OFFICIELLE)");
    FORENSIC_LOG("📊 État: 10%% - Initialisation Level Zero");
    
    /* 1. Initialiser Level Zero */
    ZE_CHECK(zeInit(0));  // Pas de flag GPU_ONLY
    FORENSIC_LOG("✅ Level Zero initialisé");
    FORENSIC_LOG("📊 État: 20%%");
    
    /* 2. Découvrir drivers */
    uint32_t driver_count = 0;
    ZE_CHECK(zeDriverGet(&driver_count, NULL));
    if (driver_count == 0) {
        FORENSIC_LOG("❌ Aucun driver Level Zero trouvé");
        return 1;
    }
    
    ze_driver_handle_t* drivers = malloc(driver_count * sizeof(ze_driver_handle_t));
    ZE_CHECK(zeDriverGet(&driver_count, drivers));
    FORENSIC_LOG("✅ %u driver(s) Level Zero trouvé(s)", driver_count);
    
    ze_driver_handle_t driver = drivers[0];
    free(drivers);
    FORENSIC_LOG("📊 État: 30%%");
    
    /* 3. Découvrir devices */
    uint32_t device_count = 0;
    ZE_CHECK(zeDeviceGet(driver, &device_count, NULL));
    if (device_count == 0) {
        FORENSIC_LOG("❌ Aucun device GPU trouvé");
        return 1;
    }
    
    ze_device_handle_t* devices = malloc(device_count * sizeof(ze_device_handle_t));
    ZE_CHECK(zeDeviceGet(driver, &device_count, devices));
    
    ze_device_handle_t device = devices[0];
    free(devices);
    
    /* Afficher propriétés device */
    ze_device_properties_t device_props = {.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    ZE_CHECK(zeDeviceGetProperties(device, &device_props));
    FORENSIC_LOG("✅ GPU: %s", device_props.name);
    FORENSIC_LOG("   Compute Units: %u", device_props.numSlices * device_props.numSubslicesPerSlice * device_props.numEUsPerSubslice);
    FORENSIC_LOG("📊 État: 40%%");
    
    /* 4. Créer contexte */
    ze_context_desc_t context_desc = {.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC};
    ze_context_handle_t context;
    ZE_CHECK(zeContextCreate(driver, &context_desc, &context));
    FORENSIC_LOG("✅ Contexte Level Zero créé");
    FORENSIC_LOG("📊 État: 50%%");
    
    /* 5. Créer command queue */
    ze_command_queue_desc_t queue_desc = {
        .stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,
        .ordinal = 0,
        .index = 0,
        .flags = 0,
        .mode = ZE_COMMAND_QUEUE_MODE_DEFAULT,
        .priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL
    };
    ze_command_queue_handle_t queue;
    ZE_CHECK(zeCommandQueueCreate(context, device, &queue_desc, &queue));
    FORENSIC_LOG("✅ Command queue créée");
    
    /* 6. Créer command list */
    ze_command_list_desc_t list_desc = {
        .stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
        .commandQueueGroupOrdinal = 0,
        .flags = 0
    };
    ze_command_list_handle_t list;
    ZE_CHECK(zeCommandListCreate(context, device, &list_desc, &list));
    FORENSIC_LOG("✅ Command list créée");
    FORENSIC_LOG("📊 État: 60%%");
    
    /* 7. Créer module depuis ISA binaire */
    ze_module_desc_t module_desc = {
        .stype = ZE_STRUCTURE_TYPE_MODULE_DESC,
        .format = ZE_MODULE_FORMAT_NATIVE,  // ISA binaire Gen9
        .inputSize = KERNEL_TEXT_SIZE,
        .pInputModule = kernel_text,
        .pBuildFlags = "",
        .pConstants = NULL
    };
    ze_module_handle_t module;
    ze_module_build_log_handle_t build_log;
    ze_result_t result = zeModuleCreate(context, device, &module_desc, &module, &build_log);
    
    if (result != ZE_RESULT_SUCCESS) {
        size_t log_size = 0;
        zeModuleBuildLogGetString(build_log, &log_size, NULL);
        char* log = malloc(log_size);
        zeModuleBuildLogGetString(build_log, &log_size, log);
        FORENSIC_LOG("❌ Module creation failed: %s", log);
        free(log);
        zeModuleBuildLogDestroy(build_log);
        return 1;
    }
    
    FORENSIC_LOG("✅ Module Level Zero créé depuis ISA Gen9");
    FORENSIC_LOG("📊 État: 70%%");
    
    /* 8. Créer kernel */
    ze_kernel_desc_t kernel_desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .flags = 0,
        .pKernelName = "minimal_compute"  // Nom du kernel dans l'ISA
    };
    ze_kernel_handle_t kernel;
    ZE_CHECK(zeKernelCreate(module, &kernel_desc, &kernel));
    FORENSIC_LOG("✅ Kernel créé");
    
    /* 9. Allouer buffer output (device memory) */
    ze_device_mem_alloc_desc_t alloc_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .flags = 0,
        .ordinal = 0
    };
    void* output_device;
    ZE_CHECK(zeMemAllocDevice(context, &alloc_desc, 4096, 64, device, &output_device));
    FORENSIC_LOG("✅ Buffer output alloué (device): %p", output_device);
    FORENSIC_LOG("📊 État: 80%%");
    
    /* 10. Configurer kernel arguments */
    ZE_CHECK(zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &output_device));
    FORENSIC_LOG("✅ Argument kernel configuré");
    
    /* 11. Configurer group size */
    ZE_CHECK(zeKernelSetGroupSize(kernel, 1, 1, 1));
    
    /* 12. Lancer kernel */
    ze_group_count_t group_count = {1, 1, 1};
    ZE_CHECK(zeCommandListAppendLaunchKernel(list, kernel, &group_count, NULL, 0, NULL));
    FORENSIC_LOG("✅ Kernel ajouté à command list");
    
    /* 13. Fermer command list */
    ZE_CHECK(zeCommandListClose(list));
    
    /* 14. Exécuter */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    ZE_CHECK(zeCommandQueueExecuteCommandLists(queue, 1, &list, NULL));
    ZE_CHECK(zeCommandQueueSynchronize(queue, UINT64_MAX));
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double exec_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    FORENSIC_LOG("✅ Kernel exécuté: %.3f ms", exec_time * 1000);
    FORENSIC_LOG("📊 État: 90%%");
    
    /* 15. Lire résultat */
    uint32_t* output_host = malloc(4096);
    ZE_CHECK(zeCommandListReset(list));
    ZE_CHECK(zeCommandListAppendMemoryCopy(list, output_host, output_device, 4096, NULL, 0, NULL));
    ZE_CHECK(zeCommandListClose(list));
    ZE_CHECK(zeCommandQueueExecuteCommandLists(queue, 1, &list, NULL));
    ZE_CHECK(zeCommandQueueSynchronize(queue, UINT64_MAX));
    
    FORENSIC_LOG("📊 État: 100%%");
    FORENSIC_LOG("🎯 RÉSULTAT GPU LEVEL ZERO:");
    FORENSIC_LOG("   output[0] = 0x%08X %s", output_host[0], output_host[0] == 0xDEADBEEF ? "✅ SUCCÈS!" : "");
    FORENSIC_LOG("   output[1] = 0x%08X %s", output_host[1], output_host[1] == 0xCAFEBABE ? "✅" : "");
    FORENSIC_LOG("   output[2] = 0x%08X %s", output_host[2], output_host[2] == 0x12345678 ? "✅" : "");
    FORENSIC_LOG("   output[3] = 0x%08X %s", output_host[3], output_host[3] == 0xABCDEF00 ? "✅" : "");
    
    if (output_host[0] == 0xDEADBEEF) {
        FORENSIC_LOG("🎉 PREMIER KERNEL GPU NATIF LEVEL ZERO RÉUSSI!");
        FORENSIC_LOG("🎉 ÉLIMINATION OPENCL: 100%% ✅");
    }
    
    /* Cleanup */
    free(output_host);
    zeMemFree(context, output_device);
    zeKernelDestroy(kernel);
    zeModuleDestroy(module);
    zeCommandListDestroy(list);
    zeCommandQueueDestroy(queue);
    zeContextDestroy(context);
    
    return output_host[0] == 0xDEADBEEF ? 0 : 1;
}

// Made with Bob
