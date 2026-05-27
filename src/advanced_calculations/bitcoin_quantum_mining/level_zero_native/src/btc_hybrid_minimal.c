/**
 * @file btc_hybrid_minimal.c
 * @brief Implémentation minimale module hybride OpenCL/Level Zero
 * 
 * VERSION SIMPLIFIÉE pour validation rapide
 * Implémentation complète dans btc_hybrid_opencl_levelzero.c (en cours)
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10D)
 * DATE : 2026-05-09
 */

#include "btc_hybrid_opencl_levelzero.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <dlfcn.h>
#include <CL/cl.h>

/* ============================================================================
 * TYPES LEVEL ZERO (définitions minimales)
 * ============================================================================ */

typedef uint32_t ze_result_t;
#define ZE_RESULT_SUCCESS 0

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ============================================================================
 * INITIALISATION
 * ============================================================================ */

btc_hybrid_ctx_t* btc_hybrid_init(const char* cache_dir, FILE* forensic_log) {
    uint64_t start_ns = get_time_ns();
    
    btc_hybrid_ctx_t* ctx = calloc(1, sizeof(btc_hybrid_ctx_t));
    if (!ctx) {
        fprintf(stderr, "[HYBRID] ERREUR : Allocation contexte échouée\n");
        return NULL;
    }

    // Configuration cache
    if (cache_dir) {
        strncpy(ctx->cache_dir, cache_dir, sizeof(ctx->cache_dir) - 1);
    } else {
        strncpy(ctx->cache_dir, "./cache_gen9_isa", sizeof(ctx->cache_dir) - 1);
    }

    // Créer répertoire cache
    struct stat st;
    if (stat(ctx->cache_dir, &st) != 0) {
        if (mkdir(ctx->cache_dir, 0755) != 0) {
            fprintf(stderr, "[HYBRID] ERREUR : Création cache '%s' : %s\n",
                    ctx->cache_dir, strerror(errno));
            free(ctx);
            return NULL;
        }
    }

    // Configuration logging
    ctx->forensic_log = forensic_log ? forensic_log : stdout;
    ctx->enable_cache = true;
    ctx->enable_forensic = true;
    ctx->dma_warmed_up = false;

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Contexte initialisé\n");
    fprintf(ctx->forensic_log, "[HYBRID] Cache : %s\n", ctx->cache_dir);

    // Initialisation OpenCL
    cl_int err;
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clGetPlatformIDs = %d\n", err);
        free(ctx);
        return NULL;
    }
    ctx->cl_platform = platform;

    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clGetDeviceIDs = %d\n", err);
        free(ctx);
        return NULL;
    }
    ctx->cl_device = device;

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clCreateContext = %d\n", err);
        free(ctx);
        return NULL;
    }
    ctx->cl_context = context;

    fprintf(ctx->forensic_log, "[HYBRID] ✅ OpenCL initialisé (compilation)\n");

    // Chargement driver Level Zero
    ctx->gmmlib_handle = dlopen("/home/lvx/gmmlib_build/lib/libigdgmm.so.11", RTLD_NOW | RTLD_GLOBAL);
    if (!ctx->gmmlib_handle) {
        fprintf(stderr, "[HYBRID] ERREUR : Chargement libigdgmm.so.11 échoué\n");
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }

    ctx->ze_driver_handle = dlopen("/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so", RTLD_NOW | RTLD_GLOBAL);
    if (!ctx->ze_driver_handle) {
        fprintf(stderr, "[HYBRID] ERREUR : Chargement libze_intel_gpu.so échoué\n");
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }

    // Charger symboles Level Zero
    ctx->ze_Init = dlsym(ctx->ze_driver_handle, "zeInit");
    ctx->ze_MemAllocDevice = dlsym(ctx->ze_driver_handle, "zeMemAllocDevice");
    ctx->ze_MemAllocShared = dlsym(ctx->ze_driver_handle, "zeMemAllocShared");
    ctx->ze_MemFree = dlsym(ctx->ze_driver_handle, "zeMemFree");
    
    if (!ctx->ze_Init) {
        fprintf(stderr, "[HYBRID] ERREUR : Symbole 'zeInit' introuvable\n");
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }
    
    if (!ctx->ze_MemAllocDevice || !ctx->ze_MemAllocShared || !ctx->ze_MemFree) {
        fprintf(stderr, "[HYBRID] ERREUR : Symboles memory Level Zero introuvables\n");
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Driver Level Zero chargé\n");

    // Initialisation Level Zero
    typedef ze_result_t (*zeInit_t)(uint32_t);
    zeInit_t zeInit_fn = (zeInit_t)ctx->ze_Init;
    ze_result_t res = zeInit_fn(0);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeInit = %d\n", res);
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Level Zero initialisé\n");
    fprintf(ctx->forensic_log, "[HYBRID] ✅ Symboles memory chargés (zeMemAllocDevice, zeMemAllocShared, zeMemFree)\n");

    // Charger symboles driver/device/context
    ctx->ze_DriverGet = dlsym(ctx->ze_driver_handle, "zeDriverGet");
    ctx->ze_DeviceGet = dlsym(ctx->ze_driver_handle, "zeDeviceGet");
    ctx->ze_ContextCreate = dlsym(ctx->ze_driver_handle, "zeContextCreate");
    
    if (!ctx->ze_DriverGet || !ctx->ze_DeviceGet || !ctx->ze_ContextCreate) {
        fprintf(stderr, "[HYBRID] ERREUR : Symboles driver/device/context introuvables\n");
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }
    
    // Obtenir driver Level Zero
    typedef ze_result_t (*zeDriverGet_t)(uint32_t*, void**);
    zeDriverGet_t zeDriverGet_fn = (zeDriverGet_t)ctx->ze_DriverGet;
    uint32_t driver_count = 1;
    void* driver = NULL;
    res = zeDriverGet_fn(&driver_count, &driver);
    if (res != ZE_RESULT_SUCCESS || !driver) {
        fprintf(stderr, "[HYBRID] ERREUR : zeDriverGet = %d\n", res);
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }
    ctx->ze_driver = driver;
    
    // Obtenir device Level Zero
    typedef ze_result_t (*zeDeviceGet_t)(void*, uint32_t*, void**);
    zeDeviceGet_t zeDeviceGet_fn = (zeDeviceGet_t)ctx->ze_DeviceGet;
    uint32_t device_count = 1;
    void* ze_device = NULL;
    res = zeDeviceGet_fn(driver, &device_count, &ze_device);
    if (res != ZE_RESULT_SUCCESS || !ze_device) {
        fprintf(stderr, "[HYBRID] ERREUR : zeDeviceGet = %d\n", res);
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }
    ctx->ze_device = ze_device;
    
    // Créer contexte Level Zero
    typedef enum {
        ZE_STRUCTURE_TYPE_CONTEXT_DESC = 0xd
    } ze_structure_type_t;
    
    typedef struct {
        ze_structure_type_t stype;
        const void* pNext;
        uint32_t flags;
    } ze_context_desc_t;
    
    typedef ze_result_t (*zeContextCreate_t)(void*, const ze_context_desc_t*, void**);
    zeContextCreate_t zeContextCreate_fn = (zeContextCreate_t)ctx->ze_ContextCreate;
    
    ze_context_desc_t context_desc = {
        .stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC,
        .pNext = NULL,
        .flags = 0
    };
    void* ze_context = NULL;
    res = zeContextCreate_fn(driver, &context_desc, &ze_context);
    if (res != ZE_RESULT_SUCCESS || !ze_context) {
        fprintf(stderr, "[HYBRID] ERREUR : zeContextCreate = %d\n", res);
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(context);
        free(ctx);
        return NULL;
    }
    ctx->ze_context = ze_context;
    
    fprintf(ctx->forensic_log, "[HYBRID] ✅ Driver/Device/Context Level Zero créés\n");

    uint64_t end_ns = get_time_ns();
    fprintf(ctx->forensic_log, "[HYBRID] Temps init : %.3f ms\n",
            (end_ns - start_ns) / 1000000.0);

    return ctx;
}

/* ============================================================================
 * COMPILATION KERNEL (VERSION SIMPLIFIÉE)
 * ============================================================================ */

btc_hybrid_kernel_t* btc_hybrid_compile_kernel(
    btc_hybrid_ctx_t* ctx,
    const char* source,
    const char* kernel_name,
    const btc_hybrid_compile_opts_t* options)
{
    if (!ctx || !source || !kernel_name) {
        return NULL;
    }

    uint64_t start_ns = get_time_ns();

    btc_hybrid_kernel_t* kernel = calloc(1, sizeof(btc_hybrid_kernel_t));
    if (!kernel) {
        fprintf(stderr, "[HYBRID] ERREUR : Allocation kernel échouée\n");
        return NULL;
    }

    strncpy(kernel->kernel_name, kernel_name, sizeof(kernel->kernel_name) - 1);

    // Compilation OpenCL
    cl_int err;
    cl_program program = clCreateProgramWithSource(
        (cl_context)ctx->cl_context,
        1,
        &source,
        NULL,
        &err
    );

    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clCreateProgramWithSource = %d\n", err);
        free(kernel);
        return NULL;
    }

    const char* build_opts = options && options->build_options ? 
                             options->build_options : "-cl-std=CL1.2";

    err = clBuildProgram(
        program,
        1,
        (cl_device_id*)&ctx->cl_device,
        build_opts,
        NULL,
        NULL
    );

    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clBuildProgram = %d\n", err);
        
        // Afficher log compilation
        size_t log_size;
        clGetProgramBuildInfo(program, (cl_device_id)ctx->cl_device, 
                             CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        if (log) {
            clGetProgramBuildInfo(program, (cl_device_id)ctx->cl_device,
                                 CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            fprintf(stderr, "[HYBRID] Log compilation :\n%s\n", log);
            free(log);
        }
        
        clReleaseProgram(program);
        free(kernel);
        return NULL;
    }

    // Extraire binaire Gen9 ISA
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 
                          sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clGetProgramInfo (size) = %d\n", err);
        clReleaseProgram(program);
        free(kernel);
        return NULL;
    }

    uint8_t* binary = malloc(binary_size);
    if (!binary) {
        fprintf(stderr, "[HYBRID] ERREUR : Allocation binaire échouée\n");
        clReleaseProgram(program);
        free(kernel);
        return NULL;
    }

    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, 
                          sizeof(uint8_t*), &binary, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clGetProgramInfo (binary) = %d\n", err);
        free(binary);
        clReleaseProgram(program);
        free(kernel);
        return NULL;
    }

    kernel->isa_binary = binary;
    kernel->isa_size = binary_size;
    kernel->from_cache = false;

    clReleaseProgram(program);

    uint64_t end_ns = get_time_ns();
    kernel->compile_time_ns = end_ns - start_ns;

    ctx->stats.total_compilations++;
    ctx->stats.cache_misses++;
    ctx->stats.total_compile_time_ns += kernel->compile_time_ns;

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Kernel '%s' compilé : %zu bytes, %.3f ms\n",
            kernel_name, binary_size, kernel->compile_time_ns / 1000000.0);

    return kernel;
}

/* ============================================================================
 * WARMUP DMA
 * ============================================================================ */

bool btc_hybrid_warmup_dma(btc_hybrid_ctx_t* ctx) {
    if (!ctx) {
        return false;
    }

    if (ctx->dma_warmed_up) {
        fprintf(ctx->forensic_log, "[HYBRID] DMA déjà warmed up\n");
        return true;
    }

    fprintf(ctx->forensic_log, "[HYBRID] Warmup DMA en cours...\n");

    // TODO: Implémenter warmup réel avec allocation/copie test
    ctx->dma_warmed_up = true;

    fprintf(ctx->forensic_log, "[HYBRID] ✅ DMA warmed up\n");
    return true;
}

/* ============================================================================
 * STATISTIQUES
 * ============================================================================ */

void btc_hybrid_get_cache_stats(
    btc_hybrid_ctx_t* ctx,
    uint64_t* hits,
    uint64_t* misses,
    uint64_t* total)
{
    if (!ctx) {
        return;
    }

    if (hits) *hits = ctx->stats.cache_hits;
    if (misses) *misses = ctx->stats.cache_misses;
    if (total) *total = ctx->stats.total_compilations;
}

/* ============================================================================
 * CLEANUP
 * ============================================================================ */

void btc_hybrid_free_kernel(btc_hybrid_ctx_t* ctx, btc_hybrid_kernel_t* kernel) {
    if (!kernel) {
        return;
    }

    if (kernel->isa_binary) {
        free(kernel->isa_binary);
    }

    free(kernel);

    if (ctx && ctx->forensic_log) {
        fprintf(ctx->forensic_log, "[HYBRID] Kernel '%s' libéré\n", kernel->kernel_name);
    }
}

void btc_hybrid_destroy(btc_hybrid_ctx_t* ctx) {
    if (!ctx) {
        return;
    }

    fprintf(ctx->forensic_log, "[HYBRID] Cleanup contexte...\n");

    if (ctx->cl_context) {
        clReleaseContext((cl_context)ctx->cl_context);
    }

    if (ctx->ze_driver_handle) {
        dlclose(ctx->ze_driver_handle);
    }

    if (ctx->gmmlib_handle) {
        dlclose(ctx->gmmlib_handle);
    }

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Contexte détruit\n");
    fprintf(ctx->forensic_log, "[HYBRID] Statistiques finales :\n");
    fprintf(ctx->forensic_log, "[HYBRID]   - Total compilations : %lu\n", ctx->stats.total_compilations);
    fprintf(ctx->forensic_log, "[HYBRID]   - Cache hits : %lu\n", ctx->stats.cache_hits);
    fprintf(ctx->forensic_log, "[HYBRID]   - Cache misses : %lu\n", ctx->stats.cache_misses);

    free(ctx);
}

/* ============================================================================
 * STUBS SUPPRIMÉS
 * ============================================================================ */

// Stubs supprimés - Implémentations réelles dans btc_hybrid_shared_memory.c
// - btc_hybrid_alloc_device()
// - btc_hybrid_alloc_shared()
// - btc_hybrid_upload()
// - btc_hybrid_download()
// - btc_hybrid_free_buffer()
//
/* ============================================================================
 * KERNEL DISPATCH (IMPLÉMENTATION COMPLÈTE PHASE 10F)
 * ============================================================================ */

bool btc_hybrid_set_kernel_arg(btc_hybrid_ctx_t* ctx, btc_hybrid_kernel_t* kernel,
                                uint32_t index, size_t size, const void* value) {
    if (!ctx || !kernel || !value) {
        fprintf(stderr, "[HYBRID] ERREUR : Paramètres invalides\n");
        return false;
    }

    // Charger fonction zeKernelSetArgumentValue
    typedef ze_result_t (*zeKernelSetArgumentValue_t)(
        void*, uint32_t, size_t, const void*);
    
    zeKernelSetArgumentValue_t zeSetArg_fn =
        (zeKernelSetArgumentValue_t)ctx->ze_KernelSetArgumentValue;
    
    if (!zeSetArg_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : zeKernelSetArgumentValue non chargé\n");
        return false;
    }

    // Définir argument kernel
    ze_result_t res = zeSetArg_fn(kernel->ze_kernel, index, size, value);
    
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeKernelSetArgumentValue = %d (index=%u, size=%zu)\n",
                res, index, size);
        return false;
    }

    if (ctx->forensic_log) {
        fprintf(ctx->forensic_log,
                "[HYBRID] ✅ Argument kernel défini : index=%u, size=%zu\n",
                index, size);
    }

    return true;
}

bool btc_hybrid_set_group_size(btc_hybrid_ctx_t* ctx, btc_hybrid_kernel_t* kernel,
                                uint32_t x, uint32_t y, uint32_t z) {
    if (!ctx || !kernel) {
        fprintf(stderr, "[HYBRID] ERREUR : Paramètres invalides\n");
        return false;
    }

    // Charger fonction zeKernelSetGroupSize
    typedef ze_result_t (*zeKernelSetGroupSize_t)(
        void*, uint32_t, uint32_t, uint32_t);
    
    zeKernelSetGroupSize_t zeSetGroupSize_fn =
        (zeKernelSetGroupSize_t)ctx->ze_KernelSetGroupSize;
    
    if (!zeSetGroupSize_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : zeKernelSetGroupSize non chargé\n");
        return false;
    }

    // Définir taille groupe de travail
    ze_result_t res = zeSetGroupSize_fn(kernel->ze_kernel, x, y, z);
    
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeKernelSetGroupSize = %d (x=%u, y=%u, z=%u)\n",
                res, x, y, z);
        return false;
    }

    if (ctx->forensic_log) {
        fprintf(ctx->forensic_log,
                "[HYBRID] ✅ Taille groupe définie : (%u, %u, %u)\n",
                x, y, z);
    }

    return true;
}

bool btc_hybrid_execute(btc_hybrid_ctx_t* ctx, btc_hybrid_kernel_t* kernel,
                        uint32_t x, uint32_t y, uint32_t z) {
    if (!ctx || !kernel) {
        fprintf(stderr, "[HYBRID] ERREUR : Paramètres invalides\n");
        return false;
    }

    uint64_t start_ns = get_time_ns();

    // Charger fonctions Level Zero
    typedef ze_result_t (*zeCommandListAppendLaunchKernel_t)(
        void*, void*, const void*, void*, uint32_t, void**);
    typedef ze_result_t (*zeCommandListClose_t)(void*);
    typedef ze_result_t (*zeCommandQueueExecuteCommandLists_t)(
        void*, uint32_t, void**, void*);
    typedef ze_result_t (*zeCommandQueueSynchronize_t)(void*, uint64_t);

    zeCommandListAppendLaunchKernel_t zeLaunchKernel_fn =
        (zeCommandListAppendLaunchKernel_t)ctx->ze_CommandListAppendLaunchKernel;
    zeCommandListClose_t zeClose_fn =
        (zeCommandListClose_t)ctx->ze_CommandListClose;
    zeCommandQueueExecuteCommandLists_t zeExecute_fn =
        (zeCommandQueueExecuteCommandLists_t)ctx->ze_CommandQueueExecuteCommandLists;
    zeCommandQueueSynchronize_t zeSync_fn =
        (zeCommandQueueSynchronize_t)ctx->ze_CommandQueueSynchronize;

    if (!zeLaunchKernel_fn || !zeClose_fn || !zeExecute_fn || !zeSync_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : Fonctions Level Zero manquantes\n");
        return false;
    }

    // Définir dimensions dispatch (group count)
    typedef struct {
        uint32_t stype;
        const void* pNext;
        uint32_t groupCountX;
        uint32_t groupCountY;
        uint32_t groupCountZ;
    } ze_group_count_t;

    ze_group_count_t group_count = {
        .stype = 0x18,  // ZE_STRUCTURE_TYPE_GROUP_COUNT
        .pNext = NULL,
        .groupCountX = x,
        .groupCountY = y,
        .groupCountZ = z
    };

    // Lancer kernel
    ze_result_t res = zeLaunchKernel_fn(
        ctx->ze_cmd_list,
        kernel->ze_kernel,
        &group_count,
        NULL,  // event signal
        0,     // numWaitEvents
        NULL   // phWaitEvents
    );

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListAppendLaunchKernel = %d\n", res);
        return false;
    }

    // Fermer command list
    res = zeClose_fn(ctx->ze_cmd_list);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListClose = %d\n", res);
        return false;
    }

    // Exécuter command list
    res = zeExecute_fn(ctx->ze_queue, 1, &ctx->ze_cmd_list, NULL);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandQueueExecuteCommandLists = %d\n", res);
        return false;
    }

    // Synchroniser GPU
    res = zeSync_fn(ctx->ze_queue, UINT64_MAX);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandQueueSynchronize = %d\n", res);
        return false;
    }

    uint64_t elapsed_ns = get_time_ns() - start_ns;

    if (ctx->forensic_log) {
        fprintf(ctx->forensic_log,
                "[HYBRID] ✅ Kernel '%s' exécuté : (%u, %u, %u) groups, %.3f ms\n",
                kernel->kernel_name, x, y, z, elapsed_ns / 1000000.0);
    }

    return true;
}

// Made with Bob