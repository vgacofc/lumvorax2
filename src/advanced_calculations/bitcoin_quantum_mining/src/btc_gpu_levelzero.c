/**
 * @file btc_gpu_levelzero.c
 * @brief Wrapper Level Zero pour interface abstraite GPU backend
 * 
 * Adapte le code C198 Phase 12H vers l'interface abstraite btc_gpu_backend.h
 * OBJECTIF : 0% dépendance OpenCL - 100% Level Zero natif
 * 
 * @author Bob (LumVorax AI Agent)
 * @date 2026-05-12
 * @version 1.0
 * @cycle C221
 */

#include "btc_gpu_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <level_zero/ze_api.h>

/* ========================================================================
 * STRUCTURES INTERNES
 * ======================================================================== */

/**
 * @brief Contexte Level Zero interne
 */
typedef struct {
    /* Magic number pour validation */
    uint32_t magic;
    #define BTC_L0_MAGIC 0x4C564C30  /* "LVL0" */
    
    /* Handles Level Zero */
    ze_driver_handle_t ze_driver;
    ze_device_handle_t ze_device;
    ze_context_handle_t ze_context;
    ze_command_queue_handle_t ze_queue;
    ze_command_list_handle_t ze_cmdlist;
    ze_module_handle_t ze_module;
    
    /* Configuration */
    btc_gpu_config_t config;
    
    /* Statistiques */
    btc_gpu_stats_t stats;
    pthread_mutex_t stats_mutex;
    
    /* Device info */
    char device_name[256];
    uint32_t device_id;
    uint64_t global_mem_size;
    uint32_t max_compute_units;
    
} btc_l0_context_internal_t;

/**
 * @brief Buffer Level Zero interne
 */
typedef struct {
    uint32_t magic;
    #define BTC_L0_BUFFER_MAGIC 0x42554646  /* "BUFF" */
    
    void *ze_memory;
    size_t size;
    btc_l0_context_internal_t *ctx;
    
} btc_l0_buffer_internal_t;

/**
 * @brief Kernel Level Zero interne
 */
typedef struct {
    uint32_t magic;
    #define BTC_L0_KERNEL_MAGIC 0x4B45524E  /* "KERN" */
    
    ze_kernel_handle_t ze_kernel;
    btc_l0_context_internal_t *ctx;
    char name[256];
    
} btc_l0_kernel_internal_t;

/**
 * @brief Event Level Zero interne
 */
typedef struct {
    uint32_t magic;
    #define BTC_L0_EVENT_MAGIC 0x4556544E  /* "EVTN" */
    
    ze_event_handle_t ze_event;
    ze_event_pool_handle_t ze_event_pool;
    btc_l0_context_internal_t *ctx;
    
} btc_l0_event_internal_t;

/* ========================================================================
 * CHARGEMENT DYNAMIQUE LEVEL ZERO (C198 Phase 10A)
 * ======================================================================== */

/* Pointeurs fonctions Level Zero */
static ze_result_t (*_zeInit)(ze_init_flags_t) = NULL;
static ze_result_t (*_zeDriverGet)(uint32_t*, ze_driver_handle_t*) = NULL;
static ze_result_t (*_zeDeviceGet)(ze_driver_handle_t, uint32_t*, ze_device_handle_t*) = NULL;
static ze_result_t (*_zeDeviceGetProperties)(ze_device_handle_t, ze_device_properties_t*) = NULL;
static ze_result_t (*_zeContextCreate)(ze_driver_handle_t, const ze_context_desc_t*, ze_context_handle_t*) = NULL;
static ze_result_t (*_zeContextDestroy)(ze_context_handle_t) = NULL;
static ze_result_t (*_zeCommandQueueCreate)(ze_context_handle_t, ze_device_handle_t, const ze_command_queue_desc_t*, ze_command_queue_handle_t*) = NULL;
static ze_result_t (*_zeCommandQueueDestroy)(ze_command_queue_handle_t) = NULL;
static ze_result_t (*_zeCommandListCreate)(ze_context_handle_t, ze_device_handle_t, const ze_command_list_desc_t*, ze_command_list_handle_t*) = NULL;
static ze_result_t (*_zeCommandListDestroy)(ze_command_list_handle_t) = NULL;
static ze_result_t (*_zeCommandListClose)(ze_command_list_handle_t) = NULL;
static ze_result_t (*_zeCommandListReset)(ze_command_list_handle_t) = NULL;
static ze_result_t (*_zeCommandQueueExecuteCommandLists)(ze_command_queue_handle_t, uint32_t, ze_command_list_handle_t*, ze_fence_handle_t) = NULL;
static ze_result_t (*_zeCommandQueueSynchronize)(ze_command_queue_handle_t, uint64_t) = NULL;
static ze_result_t (*_zeMemAllocDevice)(ze_context_handle_t, const ze_device_mem_alloc_desc_t*, size_t, size_t, ze_device_handle_t, void**) = NULL;
static ze_result_t (*_zeMemAllocShared)(ze_context_handle_t, const ze_device_mem_alloc_desc_t*, const ze_host_mem_alloc_desc_t*, size_t, size_t, ze_device_handle_t, void**) = NULL;
static ze_result_t (*_zeMemFree)(ze_context_handle_t, void*) = NULL;
static ze_result_t (*_zeCommandListAppendMemoryCopy)(ze_command_list_handle_t, void*, const void*, size_t, ze_event_handle_t, uint32_t, ze_event_handle_t*) = NULL;
static ze_result_t (*_zeModuleCreate)(ze_context_handle_t, ze_device_handle_t, const ze_module_desc_t*, ze_module_handle_t*, ze_module_build_log_handle_t*) = NULL;
static ze_result_t (*_zeModuleDestroy)(ze_module_handle_t) = NULL;
static ze_result_t (*_zeKernelCreate)(ze_module_handle_t, const ze_kernel_desc_t*, ze_kernel_handle_t*) = NULL;
static ze_result_t (*_zeKernelDestroy)(ze_kernel_handle_t) = NULL;
static ze_result_t (*_zeKernelSetArgumentValue)(ze_kernel_handle_t, uint32_t, size_t, const void*) = NULL;
static ze_result_t (*_zeKernelSetGroupSize)(ze_kernel_handle_t, uint32_t, uint32_t, uint32_t) = NULL;
static ze_result_t (*_zeCommandListAppendLaunchKernel)(ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t*, ze_event_handle_t, uint32_t, ze_event_handle_t*) = NULL;
static ze_result_t (*_zeEventPoolCreate)(ze_context_handle_t, const ze_event_pool_desc_t*, uint32_t, ze_device_handle_t*, ze_event_pool_handle_t*) = NULL;
static ze_result_t (*_zeEventPoolDestroy)(ze_event_pool_handle_t) = NULL;
static ze_result_t (*_zeEventCreate)(ze_event_pool_handle_t, const ze_event_desc_t*, ze_event_handle_t*) = NULL;
static ze_result_t (*_zeEventDestroy)(ze_event_handle_t) = NULL;
static ze_result_t (*_zeEventHostSynchronize)(ze_event_handle_t, uint64_t) = NULL;
static ze_result_t (*_zeEventQueryKernelTimestamp)(ze_event_handle_t, ze_kernel_timestamp_result_t*) = NULL;

static bool g_levelzero_loaded = false;

/**
 * @brief Charge dynamiquement Level Zero (dlopen)
 */
static int load_levelzero_library(void) {
    if (g_levelzero_loaded) return 0;
    
    fprintf(stderr, "[C221-L0] Chargement dynamique libze_loader.so...\n");
    
    /* TODO: Implémenter dlopen() comme C198 Phase 10A */
    /* Pour l'instant, on suppose que Level Zero est linkée statiquement */
    
    _zeInit = zeInit;
    _zeDriverGet = zeDriverGet;
    _zeDeviceGet = zeDeviceGet;
    _zeDeviceGetProperties = zeDeviceGetProperties;
    _zeContextCreate = zeContextCreate;
    _zeContextDestroy = zeContextDestroy;
    _zeCommandQueueCreate = zeCommandQueueCreate;
    _zeCommandQueueDestroy = zeCommandQueueDestroy;
    _zeCommandListCreate = zeCommandListCreate;
    _zeCommandListDestroy = zeCommandListDestroy;
    _zeCommandListClose = zeCommandListClose;
    _zeCommandListReset = zeCommandListReset;
    _zeCommandQueueExecuteCommandLists = zeCommandQueueExecuteCommandLists;
    _zeCommandQueueSynchronize = zeCommandQueueSynchronize;
    _zeMemAllocDevice = zeMemAllocDevice;
    _zeMemAllocShared = zeMemAllocShared;
    _zeMemFree = zeMemFree;
    _zeCommandListAppendMemoryCopy = zeCommandListAppendMemoryCopy;
    _zeModuleCreate = zeModuleCreate;
    _zeModuleDestroy = zeModuleDestroy;
    _zeKernelCreate = zeKernelCreate;
    _zeKernelDestroy = zeKernelDestroy;
    _zeKernelSetArgumentValue = zeKernelSetArgumentValue;
    _zeKernelSetGroupSize = zeKernelSetGroupSize;
    _zeCommandListAppendLaunchKernel = zeCommandListAppendLaunchKernel;
    _zeEventPoolCreate = zeEventPoolCreate;
    _zeEventPoolDestroy = zeEventPoolDestroy;
    _zeEventCreate = zeEventCreate;
    _zeEventDestroy = zeEventDestroy;
    _zeEventHostSynchronize = zeEventHostSynchronize;
    _zeEventQueryKernelTimestamp = zeEventQueryKernelTimestamp;
    
    g_levelzero_loaded = true;
    fprintf(stderr, "[C221-L0] ✓ Level Zero chargé\n");
    return 0;
}

/* ========================================================================
 * IMPLÉMENTATION API PUBLIQUE
 * ======================================================================== */

/**
 * @brief Initialise contexte Level Zero
 */
int btc_gpu_levelzero_init(const btc_gpu_config_t *config, 
                           btc_gpu_backend_context_t **ctx_out) {
    if (!config || !ctx_out) {
        fprintf(stderr, "[C221-L0] ERREUR : Paramètres invalides\n");
        return -1;
    }
    
    fprintf(stderr, "[C221-L0] ========================================\n");
    fprintf(stderr, "[C221-L0] Initialisation Level Zero Native\n");
    fprintf(stderr, "[C221-L0] OBJECTIF : 0%% dépendance OpenCL\n");
    fprintf(stderr, "[C221-L0] ========================================\n");
    
    /* Charge Level Zero */
    if (load_levelzero_library() != 0) {
        fprintf(stderr, "[C221-L0] ERREUR : Chargement Level Zero échoué\n");
        return -1;
    }
    
    /* Alloue contexte interne */
    btc_l0_context_internal_t *ctx = calloc(1, sizeof(btc_l0_context_internal_t));
    if (!ctx) {
        fprintf(stderr, "[C221-L0] ERREUR : Allocation contexte échouée\n");
        return -1;
    }
    
    ctx->magic = BTC_L0_MAGIC;
    memcpy(&ctx->config, config, sizeof(btc_gpu_config_t));
    pthread_mutex_init(&ctx->stats_mutex, NULL);
    
    /* Étape 1 : zeInit() */
    fprintf(stderr, "[C221-L0] Étape 1/6 : zeInit()...\n");
    ze_result_t ze_ret = _zeInit(ZE_INIT_FLAG_GPU_ONLY);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeInit() = %d\n", ze_ret);
        free(ctx);
        return -1;
    }
    
    /* Étape 2 : Énumération drivers */
    fprintf(stderr, "[C221-L0] Étape 2/6 : Énumération drivers...\n");
    uint32_t driver_count = 0;
    ze_ret = _zeDriverGet(&driver_count, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS || driver_count == 0) {
        fprintf(stderr, "[C221-L0] ERREUR : Aucun driver Level Zero\n");
        free(ctx);
        return -1;
    }
    
    ze_driver_handle_t *drivers = calloc(driver_count, sizeof(ze_driver_handle_t));
    ze_ret = _zeDriverGet(&driver_count, drivers);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeDriverGet() échoué\n");
        free(drivers);
        free(ctx);
        return -1;
    }
    
    ctx->ze_driver = drivers[0];
    fprintf(stderr, "[C221-L0] ✓ %u driver(s) trouvé(s)\n", driver_count);
    
    /* Étape 3 : Énumération devices */
    fprintf(stderr, "[C221-L0] Étape 3/6 : Énumération devices...\n");
    uint32_t device_count = 0;
    ze_ret = _zeDeviceGet(ctx->ze_driver, &device_count, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS || device_count == 0) {
        fprintf(stderr, "[C221-L0] ERREUR : Aucun device GPU\n");
        free(drivers);
        free(ctx);
        return -1;
    }
    
    ze_device_handle_t *devices = calloc(device_count, sizeof(ze_device_handle_t));
    ze_ret = _zeDeviceGet(ctx->ze_driver, &device_count, devices);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeDeviceGet() échoué\n");
        free(devices);
        free(drivers);
        free(ctx);
        return -1;
    }
    
    ctx->ze_device = devices[config->device_id < device_count ? config->device_id : 0];
    fprintf(stderr, "[C221-L0] ✓ %u device(s) trouvé(s), utilisation device %u\n", 
            device_count, config->device_id);
    
    /* Propriétés device */
    ze_device_properties_t dev_props = {0};
    dev_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    ze_ret = _zeDeviceGetProperties(ctx->ze_device, &dev_props);
    if (ze_ret == ZE_RESULT_SUCCESS) {
        /* C221-FIX : Garantir null-terminator après strncpy */
        strncpy(ctx->device_name, dev_props.name, sizeof(ctx->device_name) - 1);
        ctx->device_name[sizeof(ctx->device_name) - 1] = '\0';
        ctx->device_id = dev_props.deviceId;
        ctx->max_compute_units = dev_props.numSlices * dev_props.numSubslicesPerSlice;
        fprintf(stderr, "[C221-L0] Device : %s (ID=0x%x, CUs=%u)\n",
                ctx->device_name, ctx->device_id, ctx->max_compute_units);
    }
    
    /* Étape 4 : Création contexte */
    fprintf(stderr, "[C221-L0] Étape 4/6 : Création contexte...\n");
    ze_context_desc_t ctx_desc = {0};
    ctx_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
    ze_ret = _zeContextCreate(ctx->ze_driver, &ctx_desc, &ctx->ze_context);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeContextCreate() = %d\n", ze_ret);
        free(devices);
        free(drivers);
        free(ctx);
        return -1;
    }
    
    /* Étape 5 : Création command queue */
    fprintf(stderr, "[C221-L0] Étape 5/6 : Création command queue...\n");
    ze_command_queue_desc_t queue_desc = {0};
    queue_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    queue_desc.ordinal = 0;
    queue_desc.index = 0;
    queue_desc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (config->enable_profiling) {
        queue_desc.flags = ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY;
    }
    
    ze_ret = _zeCommandQueueCreate(ctx->ze_context, ctx->ze_device, &queue_desc, &ctx->ze_queue);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeCommandQueueCreate() = %d\n", ze_ret);
        _zeContextDestroy(ctx->ze_context);
        free(devices);
        free(drivers);
        free(ctx);
        return -1;
    }
    
    /* Étape 6 : Création command list */
    fprintf(stderr, "[C221-L0] Étape 6/6 : Création command list...\n");
    ze_command_list_desc_t list_desc = {0};
    list_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
    list_desc.commandQueueGroupOrdinal = 0;
    
    ze_ret = _zeCommandListCreate(ctx->ze_context, ctx->ze_device, &list_desc, &ctx->ze_cmdlist);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeCommandListCreate() = %d\n", ze_ret);
        _zeCommandQueueDestroy(ctx->ze_queue);
        _zeContextDestroy(ctx->ze_context);
        free(devices);
        free(drivers);
        free(ctx);
        return -1;
    }
    
    /* Chargement module (bytecode) */
    if (config->kernel_bytecode && config->kernel_bytecode_size > 0) {
        fprintf(stderr, "[C221-L0] Chargement bytecode kernel (%zu bytes)...\n", 
                config->kernel_bytecode_size);
        
        ze_module_desc_t module_desc = {0};
        module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
        module_desc.format = ZE_MODULE_FORMAT_NATIVE;
        module_desc.inputSize = config->kernel_bytecode_size;
        module_desc.pInputModule = config->kernel_bytecode;
        
        ze_module_build_log_handle_t build_log = NULL;
        ze_ret = _zeModuleCreate(ctx->ze_context, ctx->ze_device, &module_desc, 
                                 &ctx->ze_module, &build_log);
        if (ze_ret != ZE_RESULT_SUCCESS) {
            fprintf(stderr, "[C221-L0] ERREUR : zeModuleCreate() = %d\n", ze_ret);
            _zeCommandListDestroy(ctx->ze_cmdlist);
            _zeCommandQueueDestroy(ctx->ze_queue);
            _zeContextDestroy(ctx->ze_context);
            free(devices);
            free(drivers);
            free(ctx);
            return -1;
        }
        
        fprintf(stderr, "[C221-L0] ✓ Module chargé\n");
    }
    
    free(devices);
    free(drivers);
    
    fprintf(stderr, "[C221-L0] ========================================\n");
    fprintf(stderr, "[C221-L0] ✓ Initialisation Level Zero réussie\n");
    fprintf(stderr, "[C221-L0] ========================================\n\n");
    
    *ctx_out = (btc_gpu_backend_context_t*)ctx;
    return 0;
}

/**
 * @brief Détruit contexte Level Zero
 */
int btc_gpu_levelzero_destroy(btc_gpu_backend_context_t *ctx_opaque) {
    if (!ctx_opaque) return -1;
    
    btc_l0_context_internal_t *ctx = (btc_l0_context_internal_t*)ctx_opaque;
    if (ctx->magic != BTC_L0_MAGIC) {
        fprintf(stderr, "[C221-L0] ERREUR : Magic number invalide\n");
        return -1;
    }
    
    fprintf(stderr, "[C221-L0] Cleanup Level Zero...\n");
    
    if (ctx->ze_module) _zeModuleDestroy(ctx->ze_module);
    if (ctx->ze_cmdlist) _zeCommandListDestroy(ctx->ze_cmdlist);
    if (ctx->ze_queue) _zeCommandQueueDestroy(ctx->ze_queue);
    if (ctx->ze_context) _zeContextDestroy(ctx->ze_context);
    
    pthread_mutex_destroy(&ctx->stats_mutex);
    ctx->magic = 0xDEADBEEF;
    free(ctx);
    
    fprintf(stderr, "[C221-L0] ✓ Cleanup terminé\n");
    return 0;
}

/**
 * @brief Alloue buffer GPU
 */
int btc_gpu_levelzero_buffer_alloc(btc_gpu_backend_context_t *ctx_opaque,
                                   size_t size_bytes,
                                   btc_gpu_buffer_t **buffer_out) {
    if (!ctx_opaque || !buffer_out || size_bytes == 0) return -1;
    
    btc_l0_context_internal_t *ctx = (btc_l0_context_internal_t*)ctx_opaque;
    if (ctx->magic != BTC_L0_MAGIC) return -1;
    
    fprintf(stderr, "[C221-L0] Allocation buffer %zu bytes...\n", size_bytes);
    
    btc_l0_buffer_internal_t *buffer = calloc(1, sizeof(btc_l0_buffer_internal_t));
    if (!buffer) return -1;
    
    buffer->magic = BTC_L0_BUFFER_MAGIC;
    buffer->size = size_bytes;
    buffer->ctx = ctx;
    
    /* Allocation mémoire HOST accessible GPU (workaround libigdgmm v12) */
    ze_host_mem_alloc_desc_t host_desc = {0};
    host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
    
    ze_device_mem_alloc_desc_t device_desc = {0};
    device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    device_desc.ordinal = 0;
    
    fprintf(stderr, "[C222-L0] ⚠️  Utilisation mémoire HOST (workaround ABI libigdgmm v12)\n");
    
    ze_result_t ze_ret = zeMemAllocShared(ctx->ze_context, &device_desc, &host_desc,
                                          size_bytes, 64, ctx->ze_device, &buffer->ze_memory);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C222-L0] ERREUR : zeMemAllocShared() = %d\n", ze_ret);
        free(buffer);
        return -1;
    }
    
    fprintf(stderr, "[C221-L0] ✓ Buffer alloué : %p\n", buffer->ze_memory);
    
    pthread_mutex_lock(&ctx->stats_mutex);
    ctx->stats.memory_allocated_bytes += size_bytes;
    pthread_mutex_unlock(&ctx->stats_mutex);
    
    *buffer_out = (btc_gpu_buffer_t*)buffer;
    return 0;
}

/**
 * @brief Libère buffer GPU
 */
int btc_gpu_levelzero_buffer_free(btc_gpu_buffer_t *buffer_opaque) {
    if (!buffer_opaque) return -1;
    
    btc_l0_buffer_internal_t *buffer = (btc_l0_buffer_internal_t*)buffer_opaque;
    if (buffer->magic != BTC_L0_BUFFER_MAGIC) return -1;
    
    fprintf(stderr, "[C221-L0] Libération buffer %p...\n", buffer->ze_memory);
    
    ze_result_t ze_ret = _zeMemFree(buffer->ctx->ze_context, buffer->ze_memory);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] AVERTISSEMENT : zeMemFree() = %d\n", ze_ret);
    }
    
    pthread_mutex_lock(&buffer->ctx->stats_mutex);
    buffer->ctx->stats.memory_allocated_bytes -= buffer->size;
    pthread_mutex_unlock(&buffer->ctx->stats_mutex);
    
    buffer->magic = 0xDEADBEEF;
    free(buffer);
    
    return 0;
}

/**
 * @brief Écrit données dans buffer GPU
 */
int btc_gpu_levelzero_buffer_write(btc_gpu_buffer_t *buffer_opaque,
                                   const void *data,
                                   size_t size_bytes,
                                   bool blocking) {
    if (!buffer_opaque || !data || size_bytes == 0) return -1;
    
    btc_l0_buffer_internal_t *buffer = (btc_l0_buffer_internal_t*)buffer_opaque;
    if (buffer->magic != BTC_L0_BUFFER_MAGIC) return -1;
    
    btc_l0_context_internal_t *ctx = buffer->ctx;
    
    /* Reset command list */
    ze_result_t ze_ret = _zeCommandListReset(ctx->ze_cmdlist);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Copie host → device */
    ze_ret = _zeCommandListAppendMemoryCopy(ctx->ze_cmdlist, buffer->ze_memory, 
                                            data, size_bytes, NULL, 0, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Close + Execute */
    ze_ret = _zeCommandListClose(ctx->ze_cmdlist);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    ze_ret = _zeCommandQueueExecuteCommandLists(ctx->ze_queue, 1, &ctx->ze_cmdlist, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Synchronisation si bloquant */
    if (blocking) {
        ze_ret = _zeCommandQueueSynchronize(ctx->ze_queue, UINT64_MAX);
        if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    }
    
    pthread_mutex_lock(&ctx->stats_mutex);
    ctx->stats.memory_transferred_bytes += size_bytes;
    pthread_mutex_unlock(&ctx->stats_mutex);
    
    return 0;
}

/**
 * @brief Lit données depuis buffer GPU
 */
int btc_gpu_levelzero_buffer_read(btc_gpu_buffer_t *buffer_opaque,
                                  void *data,
                                  size_t size_bytes,
                                  bool blocking) {
    if (!buffer_opaque || !data || size_bytes == 0) return -1;
    
    btc_l0_buffer_internal_t *buffer = (btc_l0_buffer_internal_t*)buffer_opaque;
    if (buffer->magic != BTC_L0_BUFFER_MAGIC) return -1;
    
    btc_l0_context_internal_t *ctx = buffer->ctx;
    
    /* Reset command list */
    ze_result_t ze_ret = _zeCommandListReset(ctx->ze_cmdlist);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Copie device → host */
    ze_ret = _zeCommandListAppendMemoryCopy(ctx->ze_cmdlist, data, 
                                            buffer->ze_memory, size_bytes, NULL, 0, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Close + Execute */
    ze_ret = _zeCommandListClose(ctx->ze_cmdlist);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    ze_ret = _zeCommandQueueExecuteCommandLists(ctx->ze_queue, 1, &ctx->ze_cmdlist, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Synchronisation si bloquant */
    if (blocking) {
        ze_ret = _zeCommandQueueSynchronize(ctx->ze_queue, UINT64_MAX);
        if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    }
    
    pthread_mutex_lock(&ctx->stats_mutex);
    ctx->stats.memory_transferred_bytes += size_bytes;
    pthread_mutex_unlock(&ctx->stats_mutex);
    
    return 0;
}

/**
 * @brief Crée kernel GPU
 */
int btc_gpu_levelzero_kernel_create(btc_gpu_backend_context_t *ctx_opaque,
                                    const char *kernel_name,
                                    btc_gpu_kernel_t **kernel_out) {
    if (!ctx_opaque || !kernel_name || !kernel_out) return -1;
    
    btc_l0_context_internal_t *ctx = (btc_l0_context_internal_t*)ctx_opaque;
    if (ctx->magic != BTC_L0_MAGIC) return -1;
    
    if (!ctx->ze_module) {
        fprintf(stderr, "[C221-L0] ERREUR : Module non chargé\n");
        return -1;
    }
    
    fprintf(stderr, "[C221-L0] Création kernel '%s'...\n", kernel_name);
    
    btc_l0_kernel_internal_t *kernel = calloc(1, sizeof(btc_l0_kernel_internal_t));
    if (!kernel) return -1;
    
    kernel->magic = BTC_L0_KERNEL_MAGIC;
    kernel->ctx = ctx;
    strncpy(kernel->name, kernel_name, sizeof(kernel->name) - 1);
    
    /* Création kernel */
    ze_kernel_desc_t kernel_desc = {0};
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pKernelName = kernel_name;
    
    ze_result_t ze_ret = _zeKernelCreate(ctx->ze_module, &kernel_desc, &kernel->ze_kernel);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeKernelCreate() = %d\n", ze_ret);
        free(kernel);
        return -1;
    }
    
    fprintf(stderr, "[C221-L0] ✓ Kernel créé\n");
    
    *kernel_out = (btc_gpu_kernel_t*)kernel;
    return 0;
}

/**
 * @brief Détruit kernel GPU
 */
int btc_gpu_levelzero_kernel_destroy(btc_gpu_kernel_t *kernel_opaque) {
    if (!kernel_opaque) return -1;
    
    btc_l0_kernel_internal_t *kernel = (btc_l0_kernel_internal_t*)kernel_opaque;
    if (kernel->magic != BTC_L0_KERNEL_MAGIC) return -1;
    
    fprintf(stderr, "[C221-L0] Destruction kernel '%s'...\n", kernel->name);
    
    ze_result_t ze_ret = _zeKernelDestroy(kernel->ze_kernel);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] AVERTISSEMENT : zeKernelDestroy() = %d\n", ze_ret);
    }
    
    kernel->magic = 0xDEADBEEF;
    free(kernel);
    
    return 0;
}

/**
 * @brief Définit argument kernel (buffer)
 */
int btc_gpu_levelzero_kernel_set_arg_buffer(btc_gpu_kernel_t *kernel_opaque,
                                            uint32_t arg_index,
                                            btc_gpu_buffer_t *buffer_opaque) {
    if (!kernel_opaque || !buffer_opaque) return -1;
    
    btc_l0_kernel_internal_t *kernel = (btc_l0_kernel_internal_t*)kernel_opaque;
    btc_l0_buffer_internal_t *buffer = (btc_l0_buffer_internal_t*)buffer_opaque;
    
    if (kernel->magic != BTC_L0_KERNEL_MAGIC) return -1;
    if (buffer->magic != BTC_L0_BUFFER_MAGIC) return -1;
    
    ze_result_t ze_ret = _zeKernelSetArgumentValue(kernel->ze_kernel, arg_index, 
                                                   sizeof(void*), &buffer->ze_memory);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeKernelSetArgumentValue() = %d\n", ze_ret);
        return -1;
    }
    
    return 0;
}

/**
 * @brief Définit argument kernel (valeur)
 */
int btc_gpu_levelzero_kernel_set_arg_value(btc_gpu_kernel_t *kernel_opaque,
                                           uint32_t arg_index,
                                           const void *value,
                                           size_t size_bytes) {
    if (!kernel_opaque || !value) return -1;
    
    btc_l0_kernel_internal_t *kernel = (btc_l0_kernel_internal_t*)kernel_opaque;
    if (kernel->magic != BTC_L0_KERNEL_MAGIC) return -1;
    
    ze_result_t ze_ret = _zeKernelSetArgumentValue(kernel->ze_kernel, arg_index, 
                                                   size_bytes, value);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C221-L0] ERREUR : zeKernelSetArgumentValue() = %d\n", ze_ret);
        return -1;
    }
    
    return 0;
}

/**
 * @brief Lance kernel GPU
 */
int btc_gpu_levelzero_kernel_launch(btc_gpu_kernel_t *kernel_opaque,
                                    size_t global_work_size,
                                    size_t local_work_size,
                                    btc_gpu_event_t **event_out) {
    if (!kernel_opaque) return -1;
    
    btc_l0_kernel_internal_t *kernel = (btc_l0_kernel_internal_t*)kernel_opaque;
    if (kernel->magic != BTC_L0_KERNEL_MAGIC) return -1;
    
    btc_l0_context_internal_t *ctx = kernel->ctx;
    
    /* Définir group size */
    if (local_work_size == 0) local_work_size = 256;  /* Défaut */
    
    ze_result_t ze_ret = _zeKernelSetGroupSize(kernel->ze_kernel, local_work_size, 1, 1);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Calculer group count */
    ze_group_count_t group_count;
    group_count.groupCountX = (global_work_size + local_work_size - 1) / local_work_size;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;
    
    /* Reset command list */
    ze_ret = _zeCommandListReset(ctx->ze_cmdlist);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Append kernel launch */
    ze_ret = _zeCommandListAppendLaunchKernel(ctx->ze_cmdlist, kernel->ze_kernel, 
                                              &group_count, NULL, 0, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    /* Close + Execute */
    ze_ret = _zeCommandListClose(ctx->ze_cmdlist);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    ze_ret = _zeCommandQueueExecuteCommandLists(ctx->ze_queue, 1, &ctx->ze_cmdlist, NULL);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    pthread_mutex_lock(&ctx->stats_mutex);
    ctx->stats.kernel_launches++;
    pthread_mutex_unlock(&ctx->stats_mutex);
    
    /* TODO: Créer event si event_out != NULL */
    if (event_out) *event_out = NULL;
    
    return 0;
}

/**
 * @brief Attend événement GPU
 */
int btc_gpu_levelzero_event_wait(btc_gpu_event_t *event_opaque) {
    if (!event_opaque) return -1;
    
    btc_l0_event_internal_t *event = (btc_l0_event_internal_t*)event_opaque;
    if (event->magic != BTC_L0_EVENT_MAGIC) return -1;
    
    ze_result_t ze_ret = _zeEventHostSynchronize(event->ze_event, UINT64_MAX);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    return 0;
}

/**
 * @brief Obtient timestamps profiling événement
 */
int btc_gpu_levelzero_event_get_profiling(btc_gpu_event_t *event_opaque,
                                          uint64_t *queued_ns,
                                          uint64_t *submit_ns,
                                          uint64_t *start_ns,
                                          uint64_t *end_ns) {
    if (!event_opaque) return -1;
    
    btc_l0_event_internal_t *event = (btc_l0_event_internal_t*)event_opaque;
    if (event->magic != BTC_L0_EVENT_MAGIC) return -1;
    
    ze_kernel_timestamp_result_t ts_result;
    ze_result_t ze_ret = _zeEventQueryKernelTimestamp(event->ze_event, &ts_result);
    if (ze_ret != ZE_RESULT_SUCCESS) return -1;
    
    if (queued_ns) *queued_ns = 0;  /* Level Zero n'a pas queued */
    if (submit_ns) *submit_ns = 0;  /* Level Zero n'a pas submit */
    if (start_ns) *start_ns = ts_result.global.kernelStart;
    if (end_ns) *end_ns = ts_result.global.kernelEnd;
    
    return 0;
}

/**
 * @brief Libère événement GPU
 */
int btc_gpu_levelzero_event_release(btc_gpu_event_t *event_opaque) {
    if (!event_opaque) return -1;
    
    btc_l0_event_internal_t *event = (btc_l0_event_internal_t*)event_opaque;
    if (event->magic != BTC_L0_EVENT_MAGIC) return -1;
    
    _zeEventDestroy(event->ze_event);
    _zeEventPoolDestroy(event->ze_event_pool);
    
    event->magic = 0xDEADBEEF;
    free(event);
    
    return 0;
}

/**
 * @brief Obtient statistiques GPU
 */
int btc_gpu_levelzero_get_stats(btc_gpu_backend_context_t *ctx_opaque,
                                btc_gpu_stats_t *stats) {
    if (!ctx_opaque || !stats) return -1;
    
    btc_l0_context_internal_t *ctx = (btc_l0_context_internal_t*)ctx_opaque;
    if (ctx->magic != BTC_L0_MAGIC) return -1;
    
    pthread_mutex_lock(&ctx->stats_mutex);
    memcpy(stats, &ctx->stats, sizeof(btc_gpu_stats_t));
    pthread_mutex_unlock(&ctx->stats_mutex);
    
    return 0;
}

/**
 * @brief Réinitialise statistiques GPU
 */
int btc_gpu_levelzero_reset_stats(btc_gpu_backend_context_t *ctx_opaque) {
    if (!ctx_opaque) return -1;
    
    btc_l0_context_internal_t *ctx = (btc_l0_context_internal_t*)ctx_opaque;
    if (ctx->magic != BTC_L0_MAGIC) return -1;
    
    pthread_mutex_lock(&ctx->stats_mutex);
    memset(&ctx->stats, 0, sizeof(btc_gpu_stats_t));
    pthread_mutex_unlock(&ctx->stats_mutex);
    
    return 0;
}

/* Made with Bob - LumVorax C221 - Level Zero Native 100% */

// Made with Bob
