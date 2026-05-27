/**
 * @file levelzero_reproduce_opencl.c
 * @brief Reproduction EXACTE de l'exécution OpenCL dans Level Zero
 * 
 * OBJECTIF : Charger le binaire Gen9 ISA capturé depuis OpenCL et
 *            reproduire EXACTEMENT la même exécution dans Level Zero
 *            pour identifier les différences.
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10B)
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
 * WRAPPERS LEVEL ZERO (comme Phase 10A)
 * ============================================================================ */

static void* g_gmmlib_handle = NULL;
static void* g_driver_handle = NULL;

// Pointeurs fonctions Level Zero
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
    // Charger GMM
    g_gmmlib_handle = dlopen("/home/lvx/gmmlib_build/lib/libigdgmm.so.11", RTLD_NOW | RTLD_GLOBAL);
    if (!g_gmmlib_handle) {
        fprintf(stderr, "ERREUR : Impossible de charger libigdgmm.so.11\n");
        return -1;
    }

    // Charger driver Level Zero
    g_driver_handle = dlopen("/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so", RTLD_NOW | RTLD_GLOBAL);
    if (!g_driver_handle) {
        fprintf(stderr, "ERREUR : Impossible de charger libze_intel_gpu.so\n");
        return -1;
    }

    // Charger symboles
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

    printf("[L0] Driver Level Zero chargé\n");
    return 0;
}

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void* load_binary_file(const char* filename, size_t* size) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "ERREUR : Impossible d'ouvrir '%s'\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* data = malloc(*size);
    if (!data) {
        fclose(f);
        return NULL;
    }

    fread(data, 1, *size, f);
    fclose(f);

    printf("[L0] Fichier chargé : %s (%zu bytes)\n", filename, *size);
    return data;
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  REPRODUCTION LEVEL ZERO — LumVorax C198 Phase 10B            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    ze_result_t res;
    uint64_t t_start, t_end;

    /* ========================================================================
     * ÉTAPE 1 : Chargement driver Level Zero
     * ======================================================================== */

    printf("=== ÉTAPE 1 : Chargement driver Level Zero ===\n");

    if (load_levelzero_driver() != 0) {
        return 1;
    }

    /* ========================================================================
     * ÉTAPE 2 : Initialisation Level Zero
     * ======================================================================== */

    printf("\n=== ÉTAPE 2 : Initialisation Level Zero ===\n");

    t_start = get_time_ns();

    res = g_zeInit(0);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "ERREUR : zeInit = %d\n", res);
        return 1;
    }

    uint32_t driver_count = 0;
    res = g_zeDriverGet(&driver_count, NULL);
    if (res != ZE_RESULT_SUCCESS || driver_count == 0) {
        fprintf(stderr, "ERREUR : zeDriverGet = %d (count=%u)\n", res, driver_count);
        return 1;
    }

    ze_driver_handle_t driver;
    res = g_zeDriverGet(&driver_count, &driver);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "ERREUR : zeDriverGet = %d\n", res);
        return 1;
    }

    uint32_t device_count = 0;
    res = g_zeDeviceGet(driver, &device_count, NULL);
    if (res != ZE_RESULT_SUCCESS || device_count == 0) {
        fprintf(stderr, "ERREUR : zeDeviceGet = %d (count=%u)\n", res, device_count);
        return 1;
    }

    ze_device_handle_t device;
    res = g_zeDeviceGet(driver, &device_count, &device);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "ERREUR : zeDeviceGet = %d\n", res);
        return 1;
    }

    ze_context_desc_t context_desc = {0};
    context_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
    ze_context_handle_t context;
    res = g_zeContextCreate(driver, &context_desc, &context);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "ERREUR : zeContextCreate = %d\n", res);
        return 1;
    }

    ze_command_queue_desc_t queue_desc = {0};
    queue_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    ze_command_queue_handle_t queue;
    res = g_zeCommandQueueCreate(context, device, &queue_desc, &queue);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "ERREUR : zeCommandQueueCreate = %d\n", res);
        return 1;
    }

    ze_command_list_desc_t list_desc = {0};
    list_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
    ze_command_list_handle_t cmd_list;
    res = g_zeCommandListCreate(context, device, &list_desc, &cmd_list);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "ERREUR : zeCommandListCreate = %d\n", res);
        return 1;
    }

    t_end = get_time_ns();
    printf("[L0] Initialisation : %lu ns (%.3f ms)\n", t_end - t_start, (t_end - t_start) / 1e6);

    /* ========================================================================
     * ÉTAPE 3 : Chargement binaire OpenCL capturé
     * ======================================================================== */

    printf("\n=== ÉTAPE 3 : Chargement binaire OpenCL capturé ===\n");

    size_t binary_size;
    uint8_t* binary = load_binary_file("opencl_vector_add_gen9_FULL.bin", &binary_size);
    if (!binary) {
        return 1;
    }

    printf("[L0] Binaire chargé : %zu bytes\n", binary_size);
    printf("[L0] Header : %02X %02X %02X %02X (ELF magic)\n",
           binary[0], binary[1], binary[2], binary[3]);

    /* ========================================================================
     * ÉTAPE 4 : Création module Level Zero depuis binaire OpenCL
     * ======================================================================== */

    printf("\n=== ÉTAPE 4 : Création module Level Zero ===\n");

    ze_module_desc_t module_desc = {0};
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.format = ZE_MODULE_FORMAT_NATIVE;  // Format natif Gen9
    module_desc.inputSize = binary_size;
    module_desc.pInputModule = binary;

    ze_module_handle_t module = NULL;
    res = g_zeModuleCreate(context, device, &module_desc, &module, NULL);

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ❌ ERREUR : zeModuleCreate = %d\n", res);
        fprintf(stderr, "[L0] Le binaire OpenCL n'est PAS compatible Level Zero !\n");
        free(binary);
        return 1;
    }

    printf("[L0] ✅ Module créé depuis binaire OpenCL\n");

    /* ========================================================================
     * ÉTAPE 5 : Création kernel
     * ======================================================================== */

    printf("\n=== ÉTAPE 5 : Création kernel ===\n");

    ze_kernel_desc_t kernel_desc = {0};
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pKernelName = "vector_add";

    ze_kernel_handle_t kernel = NULL;
    res = g_zeKernelCreate(module, &kernel_desc, &kernel);

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ❌ ERREUR : zeKernelCreate = %d\n", res);
        free(binary);
        return 1;
    }

    printf("[L0] ✅ Kernel créé : vector_add\n");

    /* ========================================================================
     * ÉTAPE 6 : Allocation buffers GPU (EXACTEMENT comme OpenCL)
     * ======================================================================== */

    printf("\n=== ÉTAPE 6 : Allocation buffers GPU ===\n");

    const uint32_t n = 1024;
    const size_t buffer_size = n * sizeof(float);

    ze_device_mem_alloc_desc_t alloc_desc = {0};
    alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

    void* gpu_a = NULL;
    void* gpu_b = NULL;
    void* gpu_c = NULL;

    res = g_zeMemAllocDevice(context, &alloc_desc, buffer_size, 64, device, &gpu_a);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeMemAllocDevice (a) = %d\n", res);
        return 1;
    }

    res = g_zeMemAllocDevice(context, &alloc_desc, buffer_size, 64, device, &gpu_b);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeMemAllocDevice (b) = %d\n", res);
        return 1;
    }

    res = g_zeMemAllocDevice(context, &alloc_desc, buffer_size, 64, device, &gpu_c);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeMemAllocDevice (c) = %d\n", res);
        return 1;
    }

    printf("[L0] Buffers GPU alloués : 3×%zu bytes\n", buffer_size);
    printf("[L0] Adresses GPU : a=%p, b=%p, c=%p\n", gpu_a, gpu_b, gpu_c);

    /* ========================================================================
     * ÉTAPE 7 : Chargement données input (depuis fichiers OpenCL)
     * ======================================================================== */

    printf("\n=== ÉTAPE 7 : Chargement données input ===\n");

    size_t input_size_a, input_size_b;
    float* host_a = load_binary_file("opencl_input_a.bin", &input_size_a);
    float* host_b = load_binary_file("opencl_input_b.bin", &input_size_b);
    float* host_c = malloc(buffer_size);

    if (!host_a || !host_b || !host_c) {
        fprintf(stderr, "[L0] ERREUR : Chargement données échoué\n");
        return 1;
    }

    printf("[L0] Données chargées : a[0]=%.2f, a[100]=%.2f, a[1023]=%.2f\n",
           host_a[0], host_a[100], host_a[1023]);
    printf("[L0] Données chargées : b[0]=%.2f, b[100]=%.2f, b[1023]=%.2f\n",
           host_b[0], host_b[100], host_b[1023]);

    /* ========================================================================
     * ÉTAPE 8 : Upload données GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 8 : Upload données GPU ===\n");

    t_start = get_time_ns();

    g_zeCommandListAppendMemoryCopy(cmd_list, gpu_a, host_a, buffer_size, NULL, 0, NULL);
    g_zeCommandListAppendMemoryCopy(cmd_list, gpu_b, host_b, buffer_size, NULL, 0, NULL);
    g_zeCommandListClose(cmd_list);
    g_zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL);
    g_zeCommandQueueSynchronize(queue, UINT64_MAX);

    t_end = get_time_ns();
    printf("[L0] Upload GPU : %lu ns (%.3f ms)\n", t_end - t_start, (t_end - t_start) / 1e6);

    /* ========================================================================
     * ÉTAPE 9 : Configuration arguments kernel (EXACTEMENT comme OpenCL)
     * ======================================================================== */

    printf("\n=== ÉTAPE 9 : Configuration arguments kernel ===\n");

    printf("[L0] Argument 0 : gpu_a (%p, %zu bytes)\n", gpu_a, sizeof(void*));
    res = g_zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &gpu_a);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelSetArgumentValue(0) = %d\n", res);
        return 1;
    }

    printf("[L0] Argument 1 : gpu_b (%p, %zu bytes)\n", gpu_b, sizeof(void*));
    res = g_zeKernelSetArgumentValue(kernel, 1, sizeof(void*), &gpu_b);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelSetArgumentValue(1) = %d\n", res);
        return 1;
    }

    printf("[L0] Argument 2 : gpu_c (%p, %zu bytes)\n", gpu_c, sizeof(void*));
    res = g_zeKernelSetArgumentValue(kernel, 2, sizeof(void*), &gpu_c);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelSetArgumentValue(2) = %d\n", res);
        return 1;
    }

    printf("[L0] Argument 3 : n (%u, %zu bytes)\n", n, sizeof(uint32_t));
    res = g_zeKernelSetArgumentValue(kernel, 3, sizeof(uint32_t), &n);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelSetArgumentValue(3) = %d\n", res);
        return 1;
    }

    res = g_zeKernelSetGroupSize(kernel, 256, 1, 1);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeKernelSetGroupSize = %d\n", res);
        return 1;
    }

    printf("[L0] Configuration : 256 threads/groupe\n");

    /* ========================================================================
     * ÉTAPE 10 : Exécution kernel GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 10 : Exécution kernel GPU ===\n");

    ze_group_count_t dispatch = { 4, 1, 1 };  // 4 work-groups (comme OpenCL)
    printf("[L0] Dispatch : %u work-groups × 256 threads = %u threads\n",
           dispatch.groupCountX, dispatch.groupCountX * 256);

    // Réinitialiser command list
    g_zeCommandListCreate(context, device, &list_desc, &cmd_list);

    t_start = get_time_ns();

    res = g_zeCommandListAppendLaunchKernel(cmd_list, kernel, &dispatch, NULL, 0, NULL);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[L0] ERREUR : zeCommandListAppendLaunchKernel = %d\n", res);
        return 1;
    }

    g_zeCommandListClose(cmd_list);
    g_zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL);
    g_zeCommandQueueSynchronize(queue, UINT64_MAX);

    t_end = get_time_ns();
    printf("[L0] Exécution kernel : %lu ns (%.3f µs)\n", t_end - t_start, (t_end - t_start) / 1e3);

    /* ========================================================================
     * ÉTAPE 11 : Lecture résultats GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 11 : Lecture résultats GPU ===\n");

    g_zeCommandListCreate(context, device, &list_desc, &cmd_list);

    t_start = get_time_ns();

    g_zeCommandListAppendMemoryCopy(cmd_list, host_c, gpu_c, buffer_size, NULL, 0, NULL);
    g_zeCommandListClose(cmd_list);
    g_zeCommandQueueExecuteCommandLists(queue, 1, &cmd_list, NULL);
    g_zeCommandQueueSynchronize(queue, UINT64_MAX);

    t_end = get_time_ns();
    printf("[L0] Download GPU : %lu ns (%.3f ms)\n", t_end - t_start, (t_end - t_start) / 1e6);

    /* ========================================================================
     * ÉTAPE 12 : Vérification résultats
     * ======================================================================== */

    printf("\n=== ÉTAPE 12 : Vérification résultats ===\n");

    bool success = true;
    uint32_t errors = 0;

    for (uint32_t i = 0; i < n && errors < 10; i++) {
        float expected = host_a[i] + host_b[i];
        float actual = host_c[i];
        if (fabsf(actual - expected) > 0.001f) {
            if (errors == 0) {
                fprintf(stderr, "[L0] ❌ ERREURS DÉTECTÉES :\n");
            }
            fprintf(stderr, "  c[%u] = %.2f (attendu %.2f)\n", i, actual, expected);
            errors++;
            success = false;
        }
    }

    if (success) {
        printf("[L0] ✅ TOUS les résultats corrects (%u éléments validés)\n", n);
        printf("[L0] Exemples : c[0]=%.2f, c[100]=%.2f, c[1023]=%.2f\n",
               host_c[0], host_c[100], host_c[1023]);
    } else {
        fprintf(stderr, "[L0] ❌ %u erreurs détectées\n", errors);
    }

    /* ========================================================================
     * ÉTAPE 13 : Cleanup
     * ======================================================================== */

    printf("\n=== ÉTAPE 13 : Cleanup ===\n");

    free(host_a);
    free(host_b);
    free(host_c);
    free(binary);
    g_zeMemFree(context, gpu_a);
    g_zeMemFree(context, gpu_b);
    g_zeMemFree(context, gpu_c);
    g_zeKernelDestroy(kernel);
    g_zeModuleDestroy(module);
    g_zeCommandListDestroy(cmd_list);
    g_zeCommandQueueDestroy(queue);
    g_zeContextDestroy(context);

    printf("[L0] Cleanup terminé\n");

    /* ========================================================================
     * RÉSUMÉ
     * ======================================================================== */

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    if (success) {
        printf("║  ✅ REPRODUCTION LEVEL ZERO RÉUSSIE !                          ║\n");
        printf("║  Le binaire OpenCL fonctionne dans Level Zero !               ║\n");
    } else {
        printf("║  ❌ REPRODUCTION LEVEL ZERO ÉCHOUÉE                            ║\n");
        printf("║  Différences détectées entre OpenCL et Level Zero             ║\n");
    }
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return success ? 0 : 1;
}

// Made with Bob
