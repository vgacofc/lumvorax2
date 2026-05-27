/**
 * @file btc_hybrid_shared_memory.c
 * @brief Implémentation shared memory zeMemAllocShared (UMA)
 * 
 * PRIORITÉ 1 : Sur iGPU Intel UHD 620 (UMA), shared memory est CRITIQUE
 * 
 * ARCHITECTURE UMA :
 *   CPU et GPU partagent RAM système
 *   → Copies CPU↔GPU peuvent être artificielles
 *   → zeMemAllocShared() évite migrations inutiles
 * 
 * BENCHMARK :
 *   Comparer zeMemAllocDevice vs zeMemAllocShared
 *   Datasets : 4MB, 16MB, 64MB, 256MB
 *   Timestamps GPU natifs (events Level Zero)
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10E Priorité 1)
 * DATE : 2026-05-09
 */

#include "btc_hybrid_opencl_levelzero.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

/* ============================================================================
 * TYPES LEVEL ZERO
 * ============================================================================ */

typedef uint32_t ze_result_t;
typedef void* ze_context_handle_t;
typedef void* ze_device_handle_t;
typedef void* ze_event_pool_handle_t;
typedef void* ze_event_handle_t;
typedef void* ze_command_list_handle_t;
typedef void* ze_command_queue_handle_t;

#define ZE_RESULT_SUCCESS 0

typedef enum {
    ZE_MEMORY_TYPE_UNKNOWN = 0,
    ZE_MEMORY_TYPE_HOST = 1,
    ZE_MEMORY_TYPE_DEVICE = 2,
    ZE_MEMORY_TYPE_SHARED = 3
} ze_memory_type_t;

typedef enum {
    ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC = 0x15,
    ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC = 0x16
} ze_structure_type_mem_t;

typedef struct {
    ze_structure_type_mem_t stype;
    const void* pNext;
    uint32_t flags;
    uint32_t ordinal;
} ze_device_mem_alloc_desc_t;

typedef struct {
    ze_structure_type_mem_t stype;
    const void* pNext;
    uint32_t flags;
} ze_host_mem_alloc_desc_t;

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ============================================================================
 * ALLOCATION SHARED MEMORY (UMA)
 * ============================================================================ */

btc_hybrid_buffer_t* btc_hybrid_alloc_shared(
    btc_hybrid_ctx_t* ctx,
    size_t size)
{
    if (!ctx || size == 0) {
        return NULL;
    }

    uint64_t start_ns = get_time_ns();

    btc_hybrid_buffer_t* buffer = calloc(1, sizeof(btc_hybrid_buffer_t));
    if (!buffer) {
        fprintf(stderr, "[HYBRID] ERREUR : Allocation buffer échouée\n");
        return NULL;
    }

    // Charger fonction zeMemAllocShared
    typedef ze_result_t (*zeMemAllocShared_t)(
        ze_context_handle_t,
        const ze_device_mem_alloc_desc_t*,
        const ze_host_mem_alloc_desc_t*,
        size_t,
        size_t,
        ze_device_handle_t,
        void**
    );

    zeMemAllocShared_t zeMemAllocShared_fn = 
        (zeMemAllocShared_t)ctx->ze_MemAllocShared;

    if (!zeMemAllocShared_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : zeMemAllocShared non chargé\n");
        free(buffer);
        return NULL;
    }

    // Descripteurs allocation
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

    // Allocation shared memory
    void* ptr = NULL;
    ze_result_t res = zeMemAllocShared_fn(
        (ze_context_handle_t)ctx->ze_context,
        &device_desc,
        &host_desc,
        size,
        64,  // Alignement 64 bytes (cache line)
        (ze_device_handle_t)ctx->ze_device,
        &ptr
    );

    if (res != ZE_RESULT_SUCCESS || !ptr) {
        fprintf(stderr, "[HYBRID] ERREUR : zeMemAllocShared = %d\n", res);
        free(buffer);
        return NULL;
    }

    buffer->ze_memory = ptr;
    buffer->size = size;
    buffer->is_device = false;  // Shared memory
    buffer->alloc_time_ns = get_time_ns() - start_ns;

    if (ctx->forensic_log) {
        fprintf(ctx->forensic_log, 
                "[HYBRID] ✅ Shared memory allouée : %p (%zu bytes, %.3f µs)\n",
                ptr, size, buffer->alloc_time_ns / 1000.0);
    }

    return buffer;
}

/* ============================================================================
 * ALLOCATION DEVICE MEMORY
 * ============================================================================ */

btc_hybrid_buffer_t* btc_hybrid_alloc_device(
    btc_hybrid_ctx_t* ctx,
    size_t size)
{
    if (!ctx || size == 0) {
        return NULL;
    }

    uint64_t start_ns = get_time_ns();

    btc_hybrid_buffer_t* buffer = calloc(1, sizeof(btc_hybrid_buffer_t));
    if (!buffer) {
        fprintf(stderr, "[HYBRID] ERREUR : Allocation buffer échouée\n");
        return NULL;
    }

    // Charger fonction zeMemAllocDevice
    typedef ze_result_t (*zeMemAllocDevice_t)(
        ze_context_handle_t,
        const ze_device_mem_alloc_desc_t*,
        size_t,
        size_t,
        ze_device_handle_t,
        void**
    );

    zeMemAllocDevice_t zeMemAllocDevice_fn = 
        (zeMemAllocDevice_t)ctx->ze_MemAllocDevice;

    if (!zeMemAllocDevice_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : zeMemAllocDevice non chargé\n");
        free(buffer);
        return NULL;
    }

    // Descripteur allocation
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0
    };

    // Allocation device memory
    void* ptr = NULL;
    ze_result_t res = zeMemAllocDevice_fn(
        (ze_context_handle_t)ctx->ze_context,
        &device_desc,
        size,
        64,  // Alignement 64 bytes
        (ze_device_handle_t)ctx->ze_device,
        &ptr
    );

    if (res != ZE_RESULT_SUCCESS || !ptr) {
        fprintf(stderr, "[HYBRID] ERREUR : zeMemAllocDevice = %d\n", res);
        free(buffer);
        return NULL;
    }

    buffer->ze_memory = ptr;
    buffer->size = size;
    buffer->is_device = true;  // Device memory
    buffer->alloc_time_ns = get_time_ns() - start_ns;

    if (ctx->forensic_log) {
        fprintf(ctx->forensic_log, 
                "[HYBRID] ✅ Device memory allouée : %p (%zu bytes, %.3f µs)\n",
                ptr, size, buffer->alloc_time_ns / 1000.0);
    }

    return buffer;
}

/* ============================================================================
 * UPLOAD/DOWNLOAD (DEVICE MEMORY UNIQUEMENT)
 * ============================================================================ */

bool btc_hybrid_upload(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_buffer_t* buffer,
    const void* host_data,
    size_t size)
{
    if (!ctx || !buffer || !host_data || size == 0) {
        return false;
    }

    // Shared memory : copie directe (pas de transfert GPU)
    if (!buffer->is_device) {
        uint64_t start_ns = get_time_ns();
        memcpy(buffer->ze_memory, host_data, size);
        uint64_t elapsed_ns = get_time_ns() - start_ns;
        
        if (ctx->forensic_log) {
            fprintf(ctx->forensic_log,
                    "[HYBRID] ✅ Shared memory copie : %zu bytes, %.3f µs (%.2f GB/s)\n",
                    size, elapsed_ns / 1000.0,
                    (size / (double)elapsed_ns) * 1000.0);
        }
        return true;
    }

    // Device memory : transfert via command list
    uint64_t start_ns = get_time_ns();

    typedef ze_result_t (*zeCommandListAppendMemoryCopy_t)(
        ze_command_list_handle_t,
        void*,
        const void*,
        size_t,
        ze_event_handle_t,
        uint32_t,
        ze_event_handle_t*
    );

    zeCommandListAppendMemoryCopy_t zeMemoryCopy_fn =
        (zeCommandListAppendMemoryCopy_t)ctx->ze_CommandListAppendMemoryCopy;

    if (!zeMemoryCopy_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListAppendMemoryCopy non chargé\n");
        return false;
    }

    ze_result_t res = zeMemoryCopy_fn(
        (ze_command_list_handle_t)ctx->ze_cmd_list,
        buffer->ze_memory,
        host_data,
        size,
        NULL,
        0,
        NULL
    );

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListAppendMemoryCopy = %d\n", res);
        return false;
    }

    // Close + Execute + Synchronize
    typedef ze_result_t (*zeCommandListClose_t)(ze_command_list_handle_t);
    typedef ze_result_t (*zeCommandQueueExecuteCommandLists_t)(
        ze_command_queue_handle_t, uint32_t, ze_command_list_handle_t*, ze_event_handle_t*);
    typedef ze_result_t (*zeCommandQueueSynchronize_t)(ze_command_queue_handle_t, uint64_t);

    zeCommandListClose_t zeClose_fn = (zeCommandListClose_t)ctx->ze_CommandListClose;
    zeCommandQueueExecuteCommandLists_t zeExecute_fn = 
        (zeCommandQueueExecuteCommandLists_t)ctx->ze_CommandQueueExecuteCommandLists;
    zeCommandQueueSynchronize_t zeSync_fn = 
        (zeCommandQueueSynchronize_t)ctx->ze_CommandQueueSynchronize;

    if (!zeClose_fn || !zeExecute_fn || !zeSync_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : Fonctions Level Zero manquantes\n");
        return false;
    }

    res = zeClose_fn((ze_command_list_handle_t)ctx->ze_cmd_list);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListClose = %d\n", res);
        return false;
    }

    res = zeExecute_fn(
        (ze_command_queue_handle_t)ctx->ze_queue,
        1,
        (ze_command_list_handle_t*)&ctx->ze_cmd_list,
        NULL
    );

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandQueueExecuteCommandLists = %d\n", res);
        return false;
    }

    res = zeSync_fn((ze_command_queue_handle_t)ctx->ze_queue, UINT64_MAX);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandQueueSynchronize = %d\n", res);
        return false;
    }

    uint64_t elapsed_ns = get_time_ns() - start_ns;

    if (ctx->forensic_log) {
        fprintf(ctx->forensic_log,
                "[HYBRID] ✅ Device memory upload : %zu bytes, %.3f µs (%.2f GB/s)\n",
                size, elapsed_ns / 1000.0,
                (size / (double)elapsed_ns) * 1000.0);
    }

    return true;
}

bool btc_hybrid_download(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_buffer_t* buffer,
    void* host_data,
    size_t size)
{
    if (!ctx || !buffer || !host_data || size == 0) {
        return false;
    }

    // Shared memory : copie directe
    if (!buffer->is_device) {
        uint64_t start_ns = get_time_ns();
        memcpy(host_data, buffer->ze_memory, size);
        uint64_t elapsed_ns = get_time_ns() - start_ns;
        
        if (ctx->forensic_log) {
            fprintf(ctx->forensic_log,
                    "[HYBRID] ✅ Shared memory copie : %zu bytes, %.3f µs (%.2f GB/s)\n",
                    size, elapsed_ns / 1000.0,
                    (size / (double)elapsed_ns) * 1000.0);
        }
        return true;
    }

    // Device memory : transfert via command list (GPU → Host)
    uint64_t start_ns = get_time_ns();

    typedef ze_result_t (*zeCommandListAppendMemoryCopy_t)(
        ze_command_list_handle_t,
        void*,
        const void*,
        size_t,
        ze_event_handle_t,
        uint32_t,
        ze_event_handle_t*
    );

    zeCommandListAppendMemoryCopy_t zeMemoryCopy_fn =
        (zeCommandListAppendMemoryCopy_t)ctx->ze_CommandListAppendMemoryCopy;

    if (!zeMemoryCopy_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListAppendMemoryCopy non chargé\n");
        return false;
    }

    // Download GPU → Host (inversé de upload)
    ze_result_t res = zeMemoryCopy_fn(
        (ze_command_list_handle_t)ctx->ze_cmd_list,
        host_data,           // ← Destination (host)
        buffer->ze_memory,   // ← Source (GPU)
        size,
        NULL,
        0,
        NULL
    );

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListAppendMemoryCopy = %d\n", res);
        return false;
    }

    // Close + Execute + Synchronize (identique à upload)
    typedef ze_result_t (*zeCommandListClose_t)(ze_command_list_handle_t);
    typedef ze_result_t (*zeCommandQueueExecuteCommandLists_t)(
        ze_command_queue_handle_t, uint32_t, ze_command_list_handle_t*, ze_event_handle_t*);
    typedef ze_result_t (*zeCommandQueueSynchronize_t)(ze_command_queue_handle_t, uint64_t);

    zeCommandListClose_t zeClose_fn = (zeCommandListClose_t)ctx->ze_CommandListClose;
    zeCommandQueueExecuteCommandLists_t zeExecute_fn =
        (zeCommandQueueExecuteCommandLists_t)ctx->ze_CommandQueueExecuteCommandLists;
    zeCommandQueueSynchronize_t zeSync_fn =
        (zeCommandQueueSynchronize_t)ctx->ze_CommandQueueSynchronize;

    if (!zeClose_fn || !zeExecute_fn || !zeSync_fn) {
        fprintf(stderr, "[HYBRID] ERREUR : Fonctions Level Zero manquantes\n");
        return false;
    }

    res = zeClose_fn((ze_command_list_handle_t)ctx->ze_cmd_list);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandListClose = %d\n", res);
        return false;
    }

    res = zeExecute_fn(
        (ze_command_queue_handle_t)ctx->ze_queue,
        1,
        (ze_command_list_handle_t*)&ctx->ze_cmd_list,
        NULL
    );

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandQueueExecuteCommandLists = %d\n", res);
        return false;
    }

    res = zeSync_fn((ze_command_queue_handle_t)ctx->ze_queue, UINT64_MAX);
    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[HYBRID] ERREUR : zeCommandQueueSynchronize = %d\n", res);
        return false;
    }

    uint64_t elapsed_ns = get_time_ns() - start_ns;

    if (ctx->forensic_log) {
        fprintf(ctx->forensic_log,
                "[HYBRID] ✅ Device memory download : %zu bytes, %.3f µs (%.2f GB/s)\n",
                size, elapsed_ns / 1000.0,
                (size / (double)elapsed_ns) * 1000.0);
    }

    return true;
}

/* ============================================================================
 * LIBÉRATION MÉMOIRE
 * ============================================================================ */

void btc_hybrid_free_buffer(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_buffer_t* buffer)
{
    if (!buffer) {
        return;
    }

    if (buffer->ze_memory) {
        typedef ze_result_t (*zeMemFree_t)(ze_context_handle_t, void*);
        zeMemFree_t zeMemFree_fn = (zeMemFree_t)ctx->ze_MemFree;

        if (zeMemFree_fn) {
            zeMemFree_fn((ze_context_handle_t)ctx->ze_context, buffer->ze_memory);
        }

        if (ctx && ctx->forensic_log) {
            fprintf(ctx->forensic_log,
                    "[HYBRID] Buffer %s libéré : %p (%zu bytes)\n",
                    buffer->is_device ? "device" : "shared",
                    buffer->ze_memory, buffer->size);
        }
    }

    free(buffer);
}

// Made with Bob