/**
 * @file btc_hybrid_opencl_levelzero.c
 * @brief Module hybride OpenCL/Level Zero pour Bitcoin mining
 * 
 * ARCHITECTURE :
 *   - Compilation : OpenCL (stable, mature)
 *   - Exécution : Level Zero (8-23× plus rapide)
 *   - Cache : Binaires Gen9 ISA (SHA-256 hash)
 *   - Warmup : DMA automatique (résout upload 378× plus lent)
 *   - Logging : Forensique nanoseconde
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10D)
 * DATE : 2026-05-09
 */

#include "btc_hybrid_opencl_levelzero.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dlfcn.h>
#include <openssl/sha.h>

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void compute_sha256(const uint8_t* data, size_t len, uint8_t hash[32]) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(hash, &ctx);
}

static void hash_to_hex(const uint8_t hash[32], char hex[65]) {
    for (int i = 0; i < 32; i++) {
        sprintf(hex + i * 2, "%02x", hash[i]);
    }
    hex[64] = '\0';
}

/* ============================================================================
 * INITIALISATION CONTEXTE HYBRIDE
 * ============================================================================ */

btc_hybrid_ctx_t* btc_hybrid_init(const char* cache_dir, FILE* forensic_log) {
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

    // Créer répertoire cache si nécessaire
    struct stat st;
    if (stat(ctx->cache_dir, &st) != 0) {
        if (mkdir(ctx->cache_dir, 0755) != 0) {
            fprintf(stderr, "[HYBRID] ERREUR : Création cache '%s' échouée : %s\n",
                    ctx->cache_dir, strerror(errno));
            free(ctx);
            return NULL;
        }
    }

    // Configuration logging
    ctx->forensic_log = forensic_log ? forensic_log : stdout;

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Contexte initialisé\n");
    fprintf(ctx->forensic_log, "[HYBRID] Cache : %s\n", ctx->cache_dir);

    // Initialisation OpenCL
    cl_int err;
    err = clGetPlatformIDs(1, &ctx->cl_platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clGetPlatformIDs = %d\n", err);
        free(ctx);
        return NULL;
    }

    err = clGetDeviceIDs(ctx->cl_platform, CL_DEVICE_TYPE_GPU, 1, &ctx->cl_device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clGetDeviceIDs = %d\n", err);
        free(ctx);
        return NULL;
    }

    ctx->cl_context = clCreateContext(NULL, 1, &ctx->cl_device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : clCreateContext = %d\n", err);
        free(ctx);
        return NULL;
    }

    fprintf(ctx->forensic_log, "[HYBRID] ✅ OpenCL initialisé (compilation)\n");

    // Chargement driver Level Zero
    ctx->gmmlib_handle = dlopen("/home/lvx/gmmlib_build/lib/libigdgmm.so.11", RTLD_NOW | RTLD_GLOBAL);
    if (!ctx->gmmlib_handle) {
        fprintf(stderr, "[HYBRID] ERREUR : Chargement libigdgmm.so.11 échoué\n");
        clReleaseContext(ctx->cl_context);
        free(ctx);
        return NULL;
    }

    ctx->ze_driver_handle = dlopen("/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so", RTLD_NOW | RTLD_GLOBAL);
    if (!ctx->ze_driver_handle) {
        fprintf(stderr, "[HYBRID] ERREUR : Chargement libze_intel_gpu.so échoué\n");
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(ctx->cl_context);
        free(ctx);
        return NULL;
    }

    // Charger symboles Level Zero
    #define LOAD_ZE_SYMBOL(name) \
        ctx->ze_##name = dlsym(ctx->ze_driver_handle, "ze" #name); \
        if (!ctx->ze_##name) { \
            fprintf(stderr, "[HYBRID] ERREUR : Symbole 'ze%s' introuvable\n", #name); \
            dlclose(ctx->ze_driver_handle); \
            dlclose(ctx->gmmlib_handle); \
            clReleaseContext(ctx->cl_context); \
            free(ctx); \
            return NULL; \
        }

    LOAD_ZE_SYMBOL(Init);
    LOAD_ZE_SYMBOL(DriverGet);
    LOAD_ZE_SYMBOL(DeviceGet);
    LOAD_ZE_SYMBOL(ContextCreate);
    LOAD_ZE_SYMBOL(CommandQueueCreate);
    LOAD_ZE_SYMBOL(CommandListCreate);
    LOAD_ZE_SYMBOL(MemAllocDevice);
    LOAD_ZE_SYMBOL(CommandListAppendMemoryCopy);
    LOAD_ZE_SYMBOL(CommandListClose);
    LOAD_ZE_SYMBOL(CommandQueueExecuteCommandLists);
    LOAD_ZE_SYMBOL(CommandQueueSynchronize);
    LOAD_ZE_SYMBOL(ModuleCreate);
    LOAD_ZE_SYMBOL(KernelCreate);
    LOAD_ZE_SYMBOL(KernelSetArgumentValue);
    LOAD_ZE_SYMBOL(KernelSetGroupSize);
    LOAD_ZE_SYMBOL(CommandListAppendLaunchKernel);
    LOAD_ZE_SYMBOL(MemFree);
    LOAD_ZE_SYMBOL(KernelDestroy);
    LOAD_ZE_SYMBOL(ModuleDestroy);
    LOAD_ZE_SYMBOL(CommandListDestroy);
    LOAD_ZE_SYMBOL(CommandQueueDestroy);
    LOAD_ZE_SYMBOL(ContextDestroy);

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Driver Level Zero chargé\n");

    // Initialisation Level Zero
    ze_result_t res = ctx->ze_Init(0);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeInit = %d\n", res);
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(ctx->cl_context);
        free(ctx);
        return NULL;
    }

    // Énumération devices
    uint32_t driver_count = 0;
    ctx->ze_DriverGet(&driver_count, NULL);
    if (driver_count == 0) {
        fprintf(stderr, "[HYBRID] ERREUR : Aucun driver Level Zero trouvé\n");
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(ctx->cl_context);
        free(ctx);
        return NULL;
    }

    ctx->ze_DriverGet(&driver_count, &ctx->ze_driver);

    uint32_t device_count = 0;
    ctx->ze_DeviceGet(ctx->ze_driver, &device_count, NULL);
    if (device_count == 0) {
        fprintf(stderr, "[HYBRID] ERREUR : Aucun device Level Zero trouvé\n");
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(ctx->cl_context);
        free(ctx);
        return NULL;
    }

    ctx->ze_DeviceGet(ctx->ze_driver, &device_count, &ctx->ze_device);

    // Création context Level Zero
    ze_context_desc_t ctx_desc;
    memset(&ctx_desc, 0, sizeof(ctx_desc));
    ctx_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
    ctx_desc.pNext = NULL;

    res = ctx->ze_ContextCreate(ctx->ze_driver, &ctx_desc, &ctx->ze_context);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeContextCreate = %d\n", res);
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(ctx->cl_context);
        free(ctx);
        return NULL;
    }

    // Création command queue
    ze_command_queue_desc_t queue_desc;
    memset(&queue_desc, 0, sizeof(queue_desc));
    queue_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    queue_desc.pNext = NULL;
    queue_desc.ordinal = 0;
    queue_desc.index = 0;
    queue_desc.mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;

    res = ctx->ze_CommandQueueCreate(ctx->ze_context, ctx->ze_device, &queue_desc, &ctx->ze_queue);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandQueueCreate = %d\n", res);
        ctx->ze_ContextDestroy(ctx->ze_context);
        dlclose(ctx->ze_driver_handle);
        dlclose(ctx->gmmlib_handle);
        clReleaseContext(ctx->cl_context);
        free(ctx);
        return NULL;
    }

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Level Zero initialisé (exécution)\n");

    // Statistiques
    ctx->stats.cache_hits = 0;
    ctx->stats.cache_misses = 0;
    ctx->stats.total_compilations = 0;
    ctx->stats.total_executions = 0;

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Contexte hybride prêt\n");

    return ctx;
}

/* ============================================================================
 * COMPILATION KERNEL DYNAMIQUE
 * ============================================================================ */

btc_hybrid_kernel_t* btc_hybrid_compile_kernel(
    btc_hybrid_ctx_t* ctx,
    const char* source,
    const char* kernel_name,
    const char* options)
{
    if (!ctx || !source || !kernel_name) {
        fprintf(stderr, "[HYBRID] ERREUR : Paramètres invalides\n");
        return NULL;
    }

    uint64_t t_start = get_time_ns();

    fprintf(ctx->forensic_log, "[HYBRID] Compilation kernel '%s'...\n", kernel_name);

    // Calculer hash SHA-256 du source
    uint8_t hash[32];
    compute_sha256((const uint8_t*)source, strlen(source), hash);
    
    char hash_hex[65];
    hash_to_hex(hash, hash_hex);

    fprintf(ctx->forensic_log, "[HYBRID] Hash source : %s\n", hash_hex);

    // Vérifier cache
    char cache_path[1024];
    snprintf(cache_path, sizeof(cache_path), "%s/%s.bin", ctx->cache_dir, hash_hex);

    struct stat st;
    bool cached = (stat(cache_path, &st) == 0);

    uint8_t* binary = NULL;
    size_t binary_size = 0;

    if (cached) {
        // Charger depuis cache
        fprintf(ctx->forensic_log, "[HYBRID] ✅ Cache HIT : %s\n", cache_path);
        ctx->stats.cache_hits++;

        FILE* f = fopen(cache_path, "rb");
        if (!f) {
            fprintf(stderr, "[HYBRID] ERREUR : Ouverture cache échouée\n");
            return NULL;
        }

        fseek(f, 0, SEEK_END);
        binary_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        binary = malloc(binary_size);
        fread(binary, 1, binary_size, f);
        fclose(f);

        fprintf(ctx->forensic_log, "[HYBRID] Binaire chargé : %zu bytes\n", binary_size);

    } else {
        // Compiler avec OpenCL
        fprintf(ctx->forensic_log, "[HYBRID] ❌ Cache MISS : Compilation OpenCL...\n");
        ctx->stats.cache_misses++;
        ctx->stats.total_compilations++;

        cl_int err;
        size_t source_len = strlen(source);
        cl_program program = clCreateProgramWithSource(ctx->cl_context, 1, &source, &source_len, &err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "[HYBRID] ERREUR : clCreateProgramWithSource = %d\n", err);
            return NULL;
        }

        err = clBuildProgram(program, 1, &ctx->cl_device, options, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "[HYBRID] ERREUR : clBuildProgram = %d\n", err);
            
            size_t log_size;
            clGetProgramBuildInfo(program, ctx->cl_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
            char* log = malloc(log_size);
            clGetProgramBuildInfo(program, ctx->cl_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            fprintf(stderr, "[HYBRID] Build log :\n%s\n", log);
            free(log);
            
            clReleaseProgram(program);
            return NULL;
        }

        // Extraire binaire Gen9 ISA
        err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "[HYBRID] ERREUR : clGetProgramInfo(BINARY_SIZES) = %d\n", err);
            clReleaseProgram(program);
            return NULL;
        }

        binary = malloc(binary_size);
        unsigned char* binaries[1] = { binary };

        err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "[HYBRID] ERREUR : clGetProgramInfo(BINARIES) = %d\n", err);
            free(binary);
            clReleaseProgram(program);
            return NULL;
        }

        clReleaseProgram(program);

        fprintf(ctx->forensic_log, "[HYBRID] ✅ Binaire Gen9 ISA : %zu bytes\n", binary_size);

        // Sauvegarder dans cache
        FILE* f = fopen(cache_path, "wb");
        if (f) {
            fwrite(binary, 1, binary_size, f);
            fclose(f);
            fprintf(ctx->forensic_log, "[HYBRID] ✅ Cache sauvegardé : %s\n", cache_path);
        } else {
            fprintf(stderr, "[HYBRID] AVERTISSEMENT : Sauvegarde cache échouée\n");
        }
    }

    // Créer module Level Zero
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
    ze_result_t res = ctx->ze_ModuleCreate(ctx->ze_context, ctx->ze_device, &module_desc, &module, &build_log);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeModuleCreate = %d\n", res);
        free(binary);
        return NULL;
    }

    // Créer kernel
    ze_kernel_desc_t kernel_desc;
    memset(&kernel_desc, 0, sizeof(kernel_desc));
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pNext = NULL;
    kernel_desc.flags = 0;
    kernel_desc.pKernelName = kernel_name;

    ze_kernel_handle_t kernel;
    res = ctx->ze_KernelCreate(module, &kernel_desc, &kernel);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeKernelCreate = %d\n", res);
        ctx->ze_ModuleDestroy(module);
        free(binary);
        return NULL;
    }

    free(binary);

    // Créer structure kernel
    btc_hybrid_kernel_t* hybrid_kernel = calloc(1, sizeof(btc_hybrid_kernel_t));
    if (!hybrid_kernel) {
        fprintf(stderr, "[HYBRID] ERREUR : Allocation kernel échouée\n");
        ctx->ze_KernelDestroy(kernel);
        ctx->ze_ModuleDestroy(module);
        return NULL;
    }

    hybrid_kernel->ze_module = module;
    hybrid_kernel->ze_kernel = kernel;
    strncpy(hybrid_kernel->name, kernel_name, sizeof(hybrid_kernel->name) - 1);
    memcpy(hybrid_kernel->source_hash, hash, 32);

    uint64_t t_end = get_time_ns();
    fprintf(ctx->forensic_log, "[HYBRID] ✅ Kernel '%s' prêt : %lu ns (%.3f ms)\n",
            kernel_name, t_end - t_start, (t_end - t_start) / 1e6);

    return hybrid_kernel;
}

/* ============================================================================
 * WARMUP DMA (résout upload 378× plus lent)
 * ============================================================================ */

bool btc_hybrid_warmup_dma(btc_hybrid_ctx_t* ctx) {
    if (!ctx) {
        return false;
    }

    if (ctx->dma_warmed_up) {
        fprintf(ctx->forensic_log, "[HYBRID] DMA déjà warmed up\n");
        return true;
    }

    fprintf(ctx->forensic_log, "[HYBRID] Warmup DMA...\n");

    uint64_t t_start = get_time_ns();

    // Allocation dummy 4KB
    ze_device_mem_alloc_desc_t alloc_desc;
    memset(&alloc_desc, 0, sizeof(alloc_desc));
    alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    alloc_desc.pNext = NULL;
    alloc_desc.flags = 0;
    alloc_desc.ordinal = 0;

    void* dummy_gpu = NULL;
    ze_result_t res = ctx->ze_MemAllocDevice(ctx->ze_context, &alloc_desc, 4096, 64, ctx->ze_device, &dummy_gpu);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : Warmup allocation échouée = %d\n", res);
        return false;
    }

    // Upload dummy
    uint8_t* dummy_host = calloc(1, 4096);
    
    ze_command_list_desc_t list_desc;
    memset(&list_desc, 0, sizeof(list_desc));
    list_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
    list_desc.pNext = NULL;

    ze_command_list_handle_t cmd_list;
    ctx->ze_CommandListCreate(ctx->ze_context, ctx->ze_device, &list_desc, &cmd_list);
    ctx->ze_CommandListAppendMemoryCopy(cmd_list, dummy_gpu, dummy_host, 4096, NULL, 0, NULL);
    ctx->ze_CommandListClose(cmd_list);
    ctx->ze_CommandQueueExecuteCommandLists(ctx->ze_queue, 1, &cmd_list, NULL);
    ctx->ze_CommandQueueSynchronize(ctx->ze_queue, UINT64_MAX);

    // Cleanup
    ctx->ze_CommandListDestroy(cmd_list);
    ctx->ze_MemFree(ctx->ze_context, dummy_gpu);
    free(dummy_host);

    ctx->dma_warmed_up = true;

    uint64_t t_end = get_time_ns();
    fprintf(ctx->forensic_log, "[HYBRID] ✅ DMA warmed up : %lu ns (%.3f ms)\n",
            t_end - t_start, (t_end - t_start) / 1e6);

    return true;
}

/* ============================================================================
 * DESTRUCTION CONTEXTE
 * ============================================================================ */

void btc_hybrid_destroy(btc_hybrid_ctx_t* ctx) {
    if (!ctx) {
        return;
    }

    fprintf(ctx->forensic_log, "[HYBRID] Destruction contexte...\n");

    // Statistiques finales
    fprintf(ctx->forensic_log, "[HYBRID] Statistiques :\n");
    fprintf(ctx->forensic_log, "[HYBRID]   Cache hits : %u\n", ctx->stats.cache_hits);
    fprintf(ctx->forensic_log, "[HYBRID]   Cache misses : %u\n", ctx->stats.cache_misses);
    fprintf(ctx->forensic_log, "[HYBRID]   Compilations : %u\n", ctx->stats.total_compilations);
    fprintf(ctx->forensic_log, "[HYBRID]   Exécutions : %u\n", ctx->stats.total_executions);

    // Cleanup Level Zero
    if (ctx->ze_queue) {
        ctx->ze_CommandQueueDestroy(ctx->ze_queue);
    }
    if (ctx->ze_context) {
        ctx->ze_ContextDestroy(ctx->ze_context);
    }

    // Cleanup OpenCL
    if (ctx->cl_context) {
        clReleaseContext(ctx->cl_context);
    }

    // Cleanup drivers
    if (ctx->ze_driver_handle) {
        dlclose(ctx->ze_driver_handle);
    }
    if (ctx->gmmlib_handle) {
        dlclose(ctx->gmmlib_handle);
    }

    free(ctx);

    fprintf(stdout, "[HYBRID] ✅ Contexte détruit\n");
}

/* ============================================================================
 * DESTRUCTION KERNEL
 * ============================================================================ */

void btc_hybrid_kernel_destroy(btc_hybrid_ctx_t* ctx, btc_hybrid_kernel_t* kernel) {
    if (!ctx || !kernel) {
        return;
    }

    fprintf(ctx->forensic_log, "[HYBRID] Destruction kernel '%s'...\n", kernel->name);

    if (kernel->ze_kernel) {
        ctx->ze_KernelDestroy(kernel->ze_kernel);
    }
    if (kernel->ze_module) {
        ctx->ze_ModuleDestroy(kernel->ze_module);
    }

    free(kernel);

    fprintf(ctx->forensic_log, "[HYBRID] ✅ Kernel détruit\n");
}

// Made with Bob
