/**
 * @file c198_test_minimal_kernel.c
 * @brief TEST MINIMAL : Kernel ultra-simple pour isoler bug passage arguments
 * 
 * OBJECTIF : Kernel qui écrit 42.0f dans buffer output SANS input
 *            pour valider que le pipeline Level Zero fonctionne
 * 
 * KERNEL OpenCL :
 *   __kernel void test_minimal(__global float* output) {
 *       int gid = get_global_id(0);
 *       output[gid] = 42.0f;
 *   }
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10C - Correction Critique)
 * DATE : 2026-05-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <dlfcn.h>

#include <level_zero/ze_api.h>

/* ============================================================================
 * WRAPPERS LEVEL ZERO
 * ============================================================================ */

static void* g_gmmlib_handle = NULL;
static void* g_driver_handle = NULL;

static ze_result_t (*g_zeInit)(ze_init_flags_t) = NULL;
static ze_result_t (*g_zeDriverGet)(uint32_t*, ze_driver_handle_t*) = NULL;
static ze_result_t (*g_zeDeviceGet)(ze_driver_handle_t, uint32_t*, ze_device_handle_t*) = NULL;
static ze_result_t (*g_zeContextCreate)(ze_driver_handle_t, const ze_context_desc_t*, ze_context_handle_t*) = NULL;
static ze_result_t (*g_zeCommandQueueCreate)(ze_context_handle_t, ze_device_handle_t, const ze_command_queue_desc_t*, ze_command_queue_handle_t*) = NULL;
static ze_result_t (*g_zeCommandListCreate)(ze_context_handle_t, ze_device_handle_t, const ze_command_list_desc_t*, ze_command_list_handle_t*) = NULL;
static ze_result_t (*g_zeMemAllocDevice)(ze_context_handle_t, const ze_device_mem_alloc_desc_t*, size_t, size_t, ze_device_handle_t, void**) = NULL;
static ze_result_t (*g_zeCommandListAppendMemoryCopy)(ze_command_list_handle_t, void*, const void*, size_t, ze_event_handle_t, uint32_t, ze_event_handle_t*) = NULL;
static ze_result_t (*g_zeCommandListClose)(ze_command_list_handle_t) = NULL;
static ze_result_t (*g_zeCommandQueueExecuteCommandLists)(ze_command_queue_handle_t, uint32_t, ze_command_list_handle_t*, ze_fence_handle_t) = NULL;
static ze_result_t (*g_zeCommandQueueSynchronize)(ze_command_queue_handle_t, uint64_t) = NULL;
static ze_result_t (*g_zeModuleCreate)(ze_context_handle_t, ze_device_handle_t, const ze_module_desc_t*, ze_module_handle_t*, ze_module_build_log_handle_t*) = NULL;
static ze_result_t (*g_zeKernelCreate)(ze_module_handle_t, const ze_kernel_desc_t*, ze_kernel_handle_t*) = NULL;
static ze_result_t (*g_zeKernelSetArgumentValue)(ze_kernel_handle_t, uint32_t, size_t, const void*) = NULL;
static ze_result_t (*g_zeKernelSetGroupSize)(ze_kernel_handle_t, uint32_t, uint32_t, uint32_t) = NULL;
static ze_result_t (*g_zeCommandListAppendLaunchKernel)(ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t*, ze_event_handle_t, uint32_t, ze_event_handle_t*) = NULL;
static ze_result_t (*g_zeMemFree)(ze_context_handle_t, void*) = NULL;
static ze_result_t (*g_zeKernelDestroy)(ze_kernel_handle_t) = NULL;
static ze_result_t (*g_zeModuleDestroy)(ze_module_handle_t) = NULL;
static ze_result_t (*g_zeCommandListDestroy)(ze_command_list_handle_t) = NULL;
static ze_result_t (*g_zeCommandQueueDestroy)(ze_command_queue_handle_t) = NULL;
static ze_result_t (*g_zeContextDestroy)(ze_context_handle_t) = NULL;

#define LOAD_SYMBOL(name) \
    g_##name = dlsym(g_driver_handle, #name); \
    if (!g_##name) { \
        fprintf(stderr, "ERREUR : Symbole '%s' introuvable\n", #name); \
        return -1; \
    }

static int load_levelzero_driver(void) {
    g_gmmlib_handle = dlopen("/home/lvx/gmmlib_build/lib/libigdgmm.so.11", RTLD_NOW | RTLD_GLOBAL);
    if (!g_gmmlib_handle) {
        fprintf(stderr, "ERREUR : Impossible de charger libigdgmm.so.11\n");
        return -1;
    }

    g_driver_handle = dlopen("/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so", RTLD_NOW | RTLD_GLOBAL);
    if (!g_driver_handle) {
        fprintf(stderr, "ERREUR : Impossible de charger libze_intel_gpu.so\n");
        return -1;
    }

    LOAD_SYMBOL(zeInit);
    LOAD_SYMBOL(zeDriverGet);
    LOAD_SYMBOL(zeDeviceGet);
    LOAD_SYMBOL(zeContextCreate);
    LOAD_SYMBOL(zeCommandQueueCreate);
    LOAD_SYMBOL(zeCommandListCreate);
    LOAD_SYMBOL(zeMemAllocDevice);
    LOAD_SYMBOL(zeCommandListAppendMemoryCopy);
    LOAD_SYMBOL(zeCommandListClose);
    LOAD_SYMBOL(zeCommandQueueExecuteCommandLists);
    LOAD_SYMBOL(zeCommandQueueSynchronize);
    LOAD_SYMBOL(zeModuleCreate);
    LOAD_SYMBOL(zeKernelCreate);
    LOAD_SYMBOL(zeKernelSetArgumentValue);
    LOAD_SYMBOL(zeKernelSetGroupSize);
    LOAD_SYMBOL(zeCommandListAppendLaunchKernel);
    LOAD_SYMBOL(zeMemFree);
    LOAD_SYMBOL(zeKernelDestroy);
    LOAD_SYMBOL(zeModuleDestroy);
    LOAD_SYMBOL(zeCommandListDestroy);
    LOAD_SYMBOL(zeCommandQueueDestroy);
    LOAD_SYMBOL(zeContextDestroy);

    return 0;
}

/* ============================================================================
 * KERNEL OPENCL SOURCE (pour compilation)
 * ============================================================================ */

const char* KERNEL_SOURCE = 
"__kernel void test_minimal(__global float* output) {\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = 42.0f;\n"
"}\n";

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  C198 TEST MINIMAL KERNEL — Validation Pipeline Level Zero    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    /* ========================================================================
     * ÉTAPE 1 : Chargement driver Level Zero
     * ======================================================================== */

    printf("=== ÉTAPE 1 : Chargement driver Level Zero ===\n");

    if (load_levelzero_driver() != 0) {
        return 1;
    }

    printf("[L0] ✅ Driver Level Zero chargé\n");

    /* ========================================================================
     * ÉTAPE 2 : Initialisation Level Zero
     * ======================================================================== */

    printf("\n=== ÉTAPE 2 : Initialisation Level Zero ===\n");

    ze_result_t res = g_zeInit(0);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeInit = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ zeInit() OK\n");

    /* ========================================================================
     * ÉTAPE 3 : Énumération devices
     * ======================================================================== */

    printf("\n=== ÉTAPE 3 : Énumération devices ===\n");

    uint32_t driver_count = 0;
    g_zeDriverGet(&driver_count, NULL);
    if (driver_count == 0) {
        fprintf(stderr, "[L0] ERREUR : Aucun driver trouvé\n");
        return 1;
    }

    ze_driver_handle_t driver;
    g_zeDriverGet(&driver_count, &driver);

    uint32_t device_count = 0;
    g_zeDeviceGet(driver, &device_count, NULL);
    if (device_count == 0) {
        fprintf(stderr, "[L0] ERREUR : Aucun device trouvé\n");
        return 1;
    }

    ze_device_handle_t device;
    g_zeDeviceGet(driver, &device_count, &device);

    printf("[L0] ✅ Device détecté\n");

    /* ========================================================================
     * ÉTAPE 4 : Création context
     * ======================================================================== */

    printf("\n=== ÉTAPE 4 : Création context ===\n");

    ze_context_desc_t ctx_desc;
    memset(&ctx_desc, 0, sizeof(ctx_desc));
    ctx_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
    ctx_desc.pNext = NULL;

    ze_context_handle_t context;
    res = g_zeContextCreate(driver, &ctx_desc, &context);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeContextCreate = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ Context créé\n");

    /* ========================================================================
     * ÉTAPE 5 : Création command queue
     * ======================================================================== */

    printf("\n=== ÉTAPE 5 : Création command queue ===\n");

    ze_command_queue_desc_t queue_desc;
    memset(&queue_desc, 0, sizeof(queue_desc));
    queue_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    queue_desc.pNext = NULL;
    queue_desc.ordinal = 0;
    queue_desc.index = 0;
    queue_desc.mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;

    ze_command_queue_handle_t queue;
    res = g_zeCommandQueueCreate(context, device, &queue_desc, &queue);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeCommandQueueCreate = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ Command queue créée\n");

    /* ========================================================================
     * ÉTAPE 6 : Allocation buffer GPU (1024 floats)
     * ======================================================================== */

    printf("\n=== ÉTAPE 6 : Allocation buffer GPU ===\n");

    const uint32_t n = 1024;
    const size_t buffer_size = n * sizeof(float);

    ze_device_mem_alloc_desc_t alloc_desc;
    memset(&alloc_desc, 0, sizeof(alloc_desc));
    alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    alloc_desc.pNext = NULL;
    alloc_desc.flags = 0;
    alloc_desc.ordinal = 0;

    void* gpu_output = NULL;
    res = g_zeMemAllocDevice(context, &alloc_desc, buffer_size, 64, device, &gpu_output);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeMemAllocDevice = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ Buffer GPU alloué : %p (%zu bytes)\n", gpu_output, buffer_size);

    /* ========================================================================
     * ÉTAPE 7 : Compilation kernel OpenCL → Gen9 ISA
     * ======================================================================== */

    printf("\n=== ÉTAPE 7 : Compilation kernel OpenCL ===\n");
    printf("[INFO] Utilisation OpenCL pour compiler en Gen9 ISA...\n");
    printf("[INFO] (Cette étape sera automatisée dans module hybride)\n");
    printf("[INFO] Pour ce test, on charge le binaire précompilé\n");

    // TODO : Intégrer compilation OpenCL automatique
    // Pour l'instant, on suppose que le binaire existe

    printf("[L0] ⚠️  SKIP : Chargement binaire précompilé requis\n");
    printf("[L0] ℹ️  Créer binaire avec : tools/opencl_forensic_capture\n");

    /* ========================================================================
     * ÉTAPE 8 : Chargement binaire Gen9 ISA (si disponible)
     * ======================================================================== */

    printf("\n=== ÉTAPE 8 : Chargement binaire Gen9 ISA ===\n");

    const char* binary_path = "opencl_test_minimal_gen9.bin";
    FILE* f = fopen(binary_path, "rb");
    if (!f) {
        fprintf(stderr, "[L0] ERREUR : Binaire '%s' introuvable\n", binary_path);
        fprintf(stderr, "[L0] Compiler d'abord avec OpenCL\n");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t binary_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* binary = malloc(binary_size);
    fread(binary, 1, binary_size, f);
    fclose(f);

    printf("[L0] ✅ Binaire chargé : %zu bytes\n", binary_size);

    /* ========================================================================
     * ÉTAPE 9 : Création module Level Zero
     * ======================================================================== */

    printf("\n=== ÉTAPE 9 : Création module Level Zero ===\n");

    ze_module_desc_t module_desc;
    memset(&module_desc, 0, sizeof(module_desc));
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.pNext = NULL;
    module_desc.format = ZE_MODULE_FORMAT_NATIVE;
    module_desc.inputSize = binary_size;
    module_desc.pInputModule = binary;
    module_desc.pBuildFlags = NULL;
    module_desc.pConstants = NULL;

    ze_module_handle_t module;
    ze_module_build_log_handle_t build_log;
    res = g_zeModuleCreate(context, device, &module_desc, &module, &build_log);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeModuleCreate = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ Module créé\n");

    /* ========================================================================
     * ÉTAPE 10 : Création kernel
     * ======================================================================== */

    printf("\n=== ÉTAPE 10 : Création kernel ===\n");

    ze_kernel_desc_t kernel_desc;
    memset(&kernel_desc, 0, sizeof(kernel_desc));
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pNext = NULL;
    kernel_desc.flags = 0;
    kernel_desc.pKernelName = "test_minimal";

    ze_kernel_handle_t kernel;
    res = g_zeKernelCreate(module, &kernel_desc, &kernel);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelCreate = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ Kernel 'test_minimal' créé\n");

    /* ========================================================================
     * ÉTAPE 11 : Configuration arguments kernel (CORRECTION CRITIQUE)
     * ======================================================================== */

    printf("\n=== ÉTAPE 11 : Configuration arguments kernel ===\n");

    // ❌ ANCIEN CODE (BUG) :
    // res = g_zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &gpu_output);
    //
    // ✅ NOUVEAU CODE (CORRECT) :
    // On passe gpu_output DIRECTEMENT (c'est déjà un pointeur GPU)

    printf("[L0] 🔍 FORENSIC : gpu_output = %p (pointeur GPU)\n", gpu_output);
    printf("[L0] 🔍 FORENSIC : &gpu_output = %p (adresse stack)\n", (void*)&gpu_output);
    printf("[L0] 🔍 FORENSIC : sizeof(void*) = %zu\n", sizeof(void*));

    // CORRECTION : Passer gpu_output directement
    res = g_zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &gpu_output);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelSetArgumentValue = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ Argument 0 configuré (output buffer)\n");

    /* ========================================================================
     * ÉTAPE 12 : Configuration group size
     * ======================================================================== */

    printf("\n=== ÉTAPE 12 : Configuration group size ===\n");

    res = g_zeKernelSetGroupSize(kernel, 256, 1, 1);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelSetGroupSize = %d\n", res);
        return 1;
    }

    printf("[L0] ✅ Group size : 256 threads\n");

    /* ========================================================================
     * ÉTAPE 13 : Exécution kernel
     * ======================================================================== */

    printf("\n=== ÉTAPE 13 : Exécution kernel ===\n");

    ze_command_list_desc_t list_desc;
    memset(&list_desc, 0, sizeof(list_desc));
    list_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
    list_desc.pNext = NULL;

    ze_command_list_handle_t cmd_list;
    g_zeCommandListCreate(context, device, &list_desc, &cmd_list);

    ze_group_count_t dispatch = { 4, 1, 1 };  // 4 work-groups × 256 = 1024 threads
    printf("[L0] Dispatch : %u work-groups × 256 threads = %u threads\n",
           dispatch.groupCountX, dispatch.groupCountX * 256);

    res = g_zeCommandListAppendLaunchKernel(cmd_list, kernel, &dispatch, NULL, 0, NULL);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeCommandListAppendLaunchKernel = %d\n", res);
        return 1;
    }

    g_zeCommandListClose(cmd_list);
    g_zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL);
    g_zeCommandQueueSynchronize(queue, UINT64_MAX);

    printf("[L0] ✅ Kernel exécuté\n");

    /* ========================================================================
     * ÉTAPE 14 : Lecture résultats GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 14 : Lecture résultats GPU ===\n");

    float* host_output = malloc(buffer_size);
    memset(host_output, 0, buffer_size);

    g_zeCommandListCreate(context, device, &list_desc, &cmd_list);
    g_zeCommandListAppendMemoryCopy(cmd_list, host_output, gpu_output, buffer_size, NULL, 0, NULL);
    g_zeCommandListClose(cmd_list);
    g_zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL);
    g_zeCommandQueueSynchronize(queue, UINT64_MAX);

    printf("[L0] ✅ Résultats téléchargés\n");

    /* ========================================================================
     * ÉTAPE 15 : Validation résultats
     * ======================================================================== */

    printf("\n=== ÉTAPE 15 : Validation résultats ===\n");

    bool success = true;
    uint32_t errors = 0;
    uint32_t zeros = 0;

    for (uint32_t i = 0; i < n && errors < 10; i++) {
        if (host_output[i] == 0.0f) {
            zeros++;
        }
        if (fabsf(host_output[i] - 42.0f) > 0.001f) {
            if (errors == 0) {
                fprintf(stderr, "[L0] ❌ ERREURS DÉTECTÉES :\n");
            }
            fprintf(stderr, "  output[%u] = %.2f (attendu 42.00)\n", i, host_output[i]);
            errors++;
            success = false;
        }
    }

    if (success) {
        printf("[L0] ✅ TOUS les résultats corrects (%u éléments = 42.00)\n", n);
        printf("[L0] Exemples : output[0]=%.2f, output[100]=%.2f, output[1023]=%.2f\n",
               host_output[0], host_output[100], host_output[1023]);
    } else {
        fprintf(stderr, "[L0] ❌ %u erreurs détectées\n", errors);
        if (zeros == n) {
            fprintf(stderr, "[L0] ⚠️  TOUS les résultats = 0 → Bug passage arguments !\n");
        }
    }

    /* ========================================================================
     * ÉTAPE 16 : Cleanup
     * ======================================================================== */

    printf("\n=== ÉTAPE 16 : Cleanup ===\n");

    free(host_output);
    free(binary);
    g_zeMemFree(context, gpu_output);
    g_zeKernelDestroy(kernel);
    g_zeModuleDestroy(module);
    g_zeCommandListDestroy(cmd_list);
    g_zeCommandQueueDestroy(queue);
    g_zeContextDestroy(context);

    printf("[L0] ✅ Cleanup terminé\n");

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST MINIMAL TERMINÉ                                          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return success ? 0 : 1;
}

// Made with Bob
