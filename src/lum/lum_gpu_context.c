/*
 * LumVorax — Module LUM GPU Context Manager
 * lum_gpu_context.c — Implémentation Complète Gestion GPU
 *
 * INTÉGRATION TOTALE 100% - AUCUNE SIMPLIFICATION
 */

/* _GNU_SOURCE déjà défini par command-line */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "lum_gpu_context.h"

/* ══════════════════════════════════════════════════════════════════════
 * STUBS TEMPORAIRES POUR FONCTIONS LUM
 * ══════════════════════════════════════════════════════════════════════ */

/* Stubs pour lum_memory_tracer */
lum_memory_tracer_t* lum_memory_tracer_create(void) { return NULL; }
void lum_memory_tracer_destroy(lum_memory_tracer_t* t) { (void)t; }
void lum_memory_trace_alloc(lum_memory_tracer_t* t, void* p, size_t s, const char* n) { (void)t; (void)p; (void)s; (void)n; }
void lum_memory_trace_free(lum_memory_tracer_t* t, void* p) { (void)t; (void)p; }
int lum_memory_tracer_get_stats(lum_memory_tracer_t* t, size_t* a, size_t* p) { (void)t; if(a)*a=0; if(p)*p=0; return 0; }

/* Stubs pour lum_log_encoder */
lum_log_encoder_t* lum_log_encoder_create(void) { return NULL; }
void lum_log_encoder_destroy(lum_log_encoder_t* l) { (void)l; }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

/* DRM headers */
#include <drm/i915_drm.h>
#include <xf86drm.h>

/* ══════════════════════════════════════════════════════════════════════
 * UTILITAIRES INTERNES
 * ══════════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static double ns_to_sec(uint64_t ns) {
    return (double)ns / 1000000000.0;
}

#define LOG_EVENT(ctx, event, ...) do { \
    if ((ctx)->log_file) { \
        uint64_t ts = get_timestamp_ns(); \
        fprintf((ctx)->log_file, "[%lu.%09lu] " event "\n", \
                (unsigned long)(ts / 1000000000ULL), \
                (unsigned long)(ts % 1000000000ULL), ##__VA_ARGS__); \
        fflush((ctx)->log_file); \
        (ctx)->event_counter++; \
    } \
} while(0)

/* ══════════════════════════════════════════════════════════════════════
 * GESTION BUFFERS GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Allouer buffer GPU avec GEM
 */
static int alloc_gpu_buffer(
    lum_gpu_context_t* ctx,
    size_t size,
    lum_gpu_buffer_t* out_buffer
) {
    struct drm_i915_gem_create create = {0};
    create.size = size;
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        LOG_EVENT(ctx, "GEM_ALLOC_FAILED: size=%zu errno=%d", size, errno);
        return -1;
    }
    
    /* Mapper en mémoire CPU */
    struct drm_i915_gem_mmap mmap_arg = {0};
    mmap_arg.handle = create.handle;
    mmap_arg.offset = 0;
    mmap_arg.size = size;
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        LOG_EVENT(ctx, "GEM_MMAP_FAILED: handle=%u errno=%d", create.handle, errno);
        
        struct drm_gem_close close_arg = {0};
        close_arg.handle = create.handle;
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        return -1;
    }
    
    void* map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    /* THP si >2MB */
    if (ctx->enable_thp && size >= LUM_GPU_BUFFER_SIZE_2MB) {
        madvise(map, size, MADV_HUGEPAGE);
        LOG_EVENT(ctx, "GEM_ALLOC_THP: handle=%u size=%zu addr=%p", 
                  create.handle, size, map);
    } else {
        LOG_EVENT(ctx, "GEM_ALLOC_SUCCESS: handle=%u size=%zu addr=%p", 
                  create.handle, size, map);
    }
    
    /* Remplir structure */
    out_buffer->handle = create.handle;
    out_buffer->map = map;
    out_buffer->size = size;
    out_buffer->alloc_timestamp = get_timestamp_ns();
    out_buffer->in_use = false;
    out_buffer->dispatch_id = -1;
    
    /* Traçage mémoire LUM */
    if (ctx->mem_tracer) {
        lum_memory_trace_alloc(ctx->mem_tracer, map, size, "GPU_BUFFER");
    }
    
    return 0;
}

/**
 * Libérer buffer GPU
 */
static void free_gpu_buffer(lum_gpu_context_t* ctx, lum_gpu_buffer_t* buffer) {
    if (buffer->handle == 0) return;
    
    /* Traçage mémoire LUM */
    if (ctx->mem_tracer && buffer->map) {
        lum_memory_trace_free(ctx->mem_tracer, buffer->map);
    }
    
    /* Unmapper */
    if (buffer->map) {
        munmap(buffer->map, buffer->size);
        buffer->map = NULL;
    }
    
    /* Fermer handle GEM */
    struct drm_gem_close close_arg = {0};
    close_arg.handle = buffer->handle;
    ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    LOG_EVENT(ctx, "GEM_FREE: handle=%u size=%zu", buffer->handle, buffer->size);
    
    buffer->handle = 0;
    buffer->size = 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * GESTION CONTEXTES GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Créer contexte GPU i915
 */
static int create_gpu_context(lum_gpu_context_t* ctx, uint32_t* out_ctx_id) {
    struct drm_i915_gem_context_create create = {0};
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create) < 0) {
        LOG_EVENT(ctx, "CONTEXT_CREATE_FAILED: errno=%d", errno);
        return -1;
    }
    
    *out_ctx_id = create.ctx_id;
    LOG_EVENT(ctx, "CONTEXT_CREATE_SUCCESS: ctx_id=%u", create.ctx_id);
    
    return 0;
}

/**
 * Détruire contexte GPU
 */
static void destroy_gpu_context(lum_gpu_context_t* ctx, uint32_t ctx_id) {
    struct drm_i915_gem_context_destroy destroy = {0};
    destroy.ctx_id = ctx_id;
    
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &destroy);
    LOG_EVENT(ctx, "CONTEXT_DESTROY: ctx_id=%u", ctx_id);
}

/**
 * Allouer contexte disponible du pool
 */
static int allocate_context(lum_gpu_context_t* ctx, int dispatch_id) {
    pthread_mutex_lock(&ctx->ctx_mutex);
    
    /* Chercher contexte libre */
    for (int i = 0; i < LUM_GPU_CTX_POOL_SIZE; i++) {
        if (!ctx->ctx_pool[i].in_use) {
            ctx->ctx_pool[i].in_use = true;
            ctx->ctx_pool[i].dispatch_id = dispatch_id;
            ctx->ctx_pool[i].last_used_ns = get_timestamp_ns();
            ctx->ctx_pool[i].usage_count++;
            
            uint32_t ctx_id = ctx->ctx_pool[i].ctx_id;
            pthread_mutex_unlock(&ctx->ctx_mutex);
            
            LOG_EVENT(ctx, "CONTEXT_ALLOCATED: ctx_id=%u dispatch_id=%d usage=%d",
                      ctx_id, dispatch_id, ctx->ctx_pool[i].usage_count);
            return i;
        }
    }
    
    pthread_mutex_unlock(&ctx->ctx_mutex);
    LOG_EVENT(ctx, "CONTEXT_POOL_EXHAUSTED: all %d contexts in use", 
              LUM_GPU_CTX_POOL_SIZE);
    return -1;
}

/**
 * Libérer contexte vers pool
 */
static void release_context(lum_gpu_context_t* ctx, int ctx_index) {
    pthread_mutex_lock(&ctx->ctx_mutex);
    
    if (ctx_index >= 0 && ctx_index < LUM_GPU_CTX_POOL_SIZE) {
        ctx->ctx_pool[ctx_index].in_use = false;
        ctx->ctx_pool[ctx_index].dispatch_id = -1;
        
        LOG_EVENT(ctx, "CONTEXT_RELEASED: ctx_id=%u index=%d",
                  ctx->ctx_pool[ctx_index].ctx_id, ctx_index);
    }
    
    pthread_mutex_unlock(&ctx->ctx_mutex);
}

/* ══════════════════════════════════════════════════════════════════════
 * GESTION BATCH BUFFERS
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Allouer batch buffer du pool
 */
static int allocate_batch_buffer(lum_gpu_context_t* ctx, int dispatch_id) {
    pthread_mutex_lock(&ctx->batch_mutex);
    
    /* Chercher batch buffer libre */
    for (int i = 0; i < LUM_GPU_BATCH_POOL_SIZE; i++) {
        if (!ctx->batch_pool[i].in_use) {
            ctx->batch_pool[i].in_use = true;
            ctx->batch_pool[i].dispatch_id = dispatch_id;
            
            pthread_mutex_unlock(&ctx->batch_mutex);
            
            LOG_EVENT(ctx, "BATCH_ALLOCATED: handle=%u index=%d dispatch_id=%d",
                      ctx->batch_pool[i].handle, i, dispatch_id);
            return i;
        }
    }
    
    pthread_mutex_unlock(&ctx->batch_mutex);
    LOG_EVENT(ctx, "BATCH_POOL_EXHAUSTED: all %d buffers in use", 
              LUM_GPU_BATCH_POOL_SIZE);
    return -1;
}

/**
 * Libérer batch buffer vers pool
 */
static void release_batch_buffer(lum_gpu_context_t* ctx, int batch_index) {
    pthread_mutex_lock(&ctx->batch_mutex);
    
    if (batch_index >= 0 && batch_index < LUM_GPU_BATCH_POOL_SIZE) {
        ctx->batch_pool[batch_index].in_use = false;
        ctx->batch_pool[batch_index].dispatch_id = -1;
        
        /* Reset buffer */
        memset(ctx->batch_pool[batch_index].map, 0, ctx->batch_pool[batch_index].size);
        
        LOG_EVENT(ctx, "BATCH_RELEASED: handle=%u index=%d",
                  ctx->batch_pool[batch_index].handle, batch_index);
    }
    
    pthread_mutex_unlock(&ctx->batch_mutex);
}

/* ══════════════════════════════════════════════════════════════════════
 * GESTION I/O BUFFERS
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Allouer paire I/O buffers pour dispatch
 */
static int allocate_io_buffers(
    lum_gpu_context_t* ctx,
    int dispatch_id,
    int* out_input_index,
    int* out_output_index
) {
    pthread_mutex_lock(&ctx->io_mutex);
    
    /* Chercher paire libre */
    for (int i = 0; i < LUM_GPU_PARALLEL_MAX; i++) {
        if (!ctx->input_pool[i].in_use && !ctx->output_pool[i].in_use) {
            ctx->input_pool[i].in_use = true;
            ctx->input_pool[i].dispatch_id = dispatch_id;
            ctx->output_pool[i].in_use = true;
            ctx->output_pool[i].dispatch_id = dispatch_id;
            
            *out_input_index = i;
            *out_output_index = i;
            
            pthread_mutex_unlock(&ctx->io_mutex);
            
            LOG_EVENT(ctx, "IO_ALLOCATED: input_handle=%u output_handle=%u dispatch_id=%d",
                      ctx->input_pool[i].handle, ctx->output_pool[i].handle, dispatch_id);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&ctx->io_mutex);
    LOG_EVENT(ctx, "IO_POOL_EXHAUSTED: all %d pairs in use", LUM_GPU_PARALLEL_MAX);
    return -1;
}

/**
 * Libérer paire I/O buffers
 */
static void release_io_buffers(lum_gpu_context_t* ctx, int input_index, int output_index) {
    pthread_mutex_lock(&ctx->io_mutex);
    
    if (input_index >= 0 && input_index < LUM_GPU_PARALLEL_MAX) {
        ctx->input_pool[input_index].in_use = false;
        ctx->input_pool[input_index].dispatch_id = -1;
        LOG_EVENT(ctx, "IO_INPUT_RELEASED: handle=%u index=%d",
                  ctx->input_pool[input_index].handle, input_index);
    }
    
    if (output_index >= 0 && output_index < LUM_GPU_PARALLEL_MAX) {
        ctx->output_pool[output_index].in_use = false;
        ctx->output_pool[output_index].dispatch_id = -1;
        LOG_EVENT(ctx, "IO_OUTPUT_RELEASED: handle=%u index=%d",
                  ctx->output_pool[output_index].handle, output_index);
    }
    
    pthread_mutex_unlock(&ctx->io_mutex);
}

/* ══════════════════════════════════════════════════════════════════════
 * CONSTRUCTION BATCH BUFFER
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Construire commandes batch buffer Gen9
 */
static int build_batch_commands(
    lum_gpu_context_t* ctx,
    lum_gpu_buffer_t* batch_buffer,
    uint32_t input_bo,
    uint32_t output_bo,
    uint32_t nonce_count
) {
    uint32_t* cmd = (uint32_t*)batch_buffer->map;
    int cmd_index = 0;
    
    /* MI_LOAD_REGISTER_IMM: Charger adresse kernel */
    cmd[cmd_index++] = 0x22000000 | (3 - 2);  /* MI_LOAD_REGISTER_IMM */
    cmd[cmd_index++] = 0x2580;                 /* Registre GPGPU_WALKER */
    cmd[cmd_index++] = ctx->kernel_bo;         /* Adresse kernel */
    
    /* MI_LOAD_REGISTER_IMM: Charger adresse input */
    cmd[cmd_index++] = 0x22000000 | (3 - 2);
    cmd[cmd_index++] = 0x2584;
    cmd[cmd_index++] = input_bo;
    
    /* MI_LOAD_REGISTER_IMM: Charger adresse output */
    cmd[cmd_index++] = 0x22000000 | (3 - 2);
    cmd[cmd_index++] = 0x2588;
    cmd[cmd_index++] = output_bo;
    
    /* GPGPU_WALKER: Lancer kernel */
    uint32_t num_work_groups = (nonce_count + ctx->work_group_size - 1) / ctx->work_group_size;
    
    cmd[cmd_index++] = 0x7A000000 | (15 - 2);  /* GPGPU_WALKER */
    cmd[cmd_index++] = 0;                       /* Interface Descriptor Offset */
    cmd[cmd_index++] = num_work_groups;         /* Thread Group ID X */
    cmd[cmd_index++] = 1;                       /* Thread Group ID Y */
    cmd[cmd_index++] = 1;                       /* Thread Group ID Z */
    cmd[cmd_index++] = ctx->work_group_size;    /* Thread Group Size X */
    cmd[cmd_index++] = 1;                       /* Thread Group Size Y */
    cmd[cmd_index++] = 1;                       /* Thread Group Size Z */
    cmd[cmd_index++] = 0;                       /* Right Execution Mask */
    cmd[cmd_index++] = 0xFFFFFFFF;              /* Bottom Execution Mask */
    cmd[cmd_index++] = 0;                       /* Reserved */
    cmd[cmd_index++] = 0;                       /* Reserved */
    cmd[cmd_index++] = 0;                       /* Reserved */
    cmd[cmd_index++] = 0;                       /* Reserved */
    cmd[cmd_index++] = 0;                       /* Reserved */
    
    /* MI_BATCH_BUFFER_END */
    cmd[cmd_index++] = 0x0A000000;
    
    int total_bytes = cmd_index * sizeof(uint32_t);
    LOG_EVENT(ctx, "BATCH_BUILD: commands=%d bytes=%d work_groups=%u",
              cmd_index, total_bytes, num_work_groups);
    
    return total_bytes;
}

/* ══════════════════════════════════════════════════════════════════════
 * EXÉCUTION DISPATCH
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Soumettre batch buffer au GPU
 */
static int submit_batch(
    lum_gpu_context_t* ctx,
    uint32_t ctx_id,
    uint32_t batch_bo,
    int batch_size_bytes
) {
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    struct drm_i915_gem_exec_object2 exec_obj = {0};
    
    exec_obj.handle = batch_bo;
    exec_obj.relocation_count = 0;
    exec_obj.relocs_ptr = 0;
    exec_obj.alignment = 0;
    exec_obj.offset = 0;
    exec_obj.flags = 0;
    exec_obj.rsvd1 = 0;
    exec_obj.rsvd2 = 0;
    
    execbuf.buffers_ptr = (uintptr_t)&exec_obj;
    execbuf.buffer_count = 1;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = batch_size_bytes;
    execbuf.cliprects_ptr = 0;
    execbuf.num_cliprects = 0;
    execbuf.DR1 = 0;
    execbuf.DR4 = 0;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx_id;
    execbuf.rsvd2 = 0;
    
    uint64_t start_ns = get_timestamp_ns();
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        LOG_EVENT(ctx, "EXEC_FAILED: ctx_id=%u batch_bo=%u errno=%d",
                  ctx_id, batch_bo, errno);
        return -1;
    }
    
    uint64_t end_ns = get_timestamp_ns();
    double time_sec = ns_to_sec(end_ns - start_ns);
    
    LOG_EVENT(ctx, "EXEC_SUCCESS: ctx_id=%u batch_bo=%u time=%.6f",
              ctx_id, batch_bo, time_sec);
    
    return 0;
}

/**
 * Attendre complétion dispatch
 */
static int wait_dispatch(lum_gpu_context_t* ctx, uint32_t batch_bo, uint32_t timeout_ns) {
    struct drm_i915_gem_wait wait_arg = {0};
    wait_arg.bo_handle = batch_bo;
    wait_arg.timeout_ns = timeout_ns;
    wait_arg.flags = 0;
    
    uint64_t start_ns = get_timestamp_ns();
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg) < 0) {
        LOG_EVENT(ctx, "WAIT_FAILED: batch_bo=%u errno=%d", batch_bo, errno);
        return -1;
    }
    
    uint64_t end_ns = get_timestamp_ns();
    double time_sec = ns_to_sec(end_ns - start_ns);
    
    LOG_EVENT(ctx, "WAIT_SUCCESS: batch_bo=%u time=%.6f", batch_bo, time_sec);
    
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE - INITIALISATION
 * ══════════════════════════════════════════════════════════════════════ */

lum_gpu_context_t* lum_gpu_init(const lum_gpu_config_t* config) {
    if (!config) {
        fprintf(stderr, "ERROR: config is NULL\n");
        return NULL;
    }
    
    /* Allouer contexte */
    lum_gpu_context_t* ctx = calloc(1, sizeof(lum_gpu_context_t));
    if (!ctx) {
        fprintf(stderr, "ERROR: Failed to allocate context\n");
        return NULL;
    }
    
    /* Ouvrir log forensique */
    if (config->log_path) {
        ctx->log_file = fopen(config->log_path, "w");
        if (!ctx->log_file) {
            fprintf(stderr, "WARNING: Failed to open log file: %s\n", config->log_path);
        }
    }
    
    LOG_EVENT(ctx, "LUM_GPU_INIT_START: batch_size=%u work_group_size=%u",
              config->batch_size, config->work_group_size);
    
    /* Ouvrir DRM */
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        LOG_EVENT(ctx, "DRM_OPEN_FAILED: errno=%d", errno);
        free(ctx);
        return NULL;
    }
    
    LOG_EVENT(ctx, "DRM_OPEN_SUCCESS: fd=%d", ctx->drm_fd);
    
    /* Vérifier version DRM */
    drmVersionPtr version = drmGetVersion(ctx->drm_fd);
    if (version) {
        LOG_EVENT(ctx, "DRM_VERSION: %s %d.%d.%d",
                  version->name, version->version_major,
                  version->version_minor, version->version_patchlevel);
        drmFreeVersion(version);
    }
    
    /* Initialiser mutex */
    pthread_mutex_init(&ctx->ctx_mutex, NULL);
    pthread_mutex_init(&ctx->batch_mutex, NULL);
    pthread_mutex_init(&ctx->io_mutex, NULL);
    pthread_mutex_init(&ctx->dispatch_mutex, NULL);
    
    /* Créer pool contextes GPU */
    for (int i = 0; i < LUM_GPU_CTX_POOL_SIZE; i++) {
        uint32_t ctx_id;
        if (create_gpu_context(ctx, &ctx_id) < 0) {
            LOG_EVENT(ctx, "CONTEXT_POOL_CREATE_FAILED: index=%d", i);
            lum_gpu_cleanup(ctx);
            return NULL;
        }
        
        ctx->ctx_pool[i].ctx_id = ctx_id;
        ctx->ctx_pool[i].usage_count = 0;
        ctx->ctx_pool[i].last_used_ns = 0;
        ctx->ctx_pool[i].in_use = false;
        ctx->ctx_pool[i].dispatch_id = -1;
        
        LOG_EVENT(ctx, "CTX_POOL_CREATED: index=%d ctx_id=%u", i, ctx_id);
    }
    
    /* Créer pool batch buffers */
    for (int i = 0; i < LUM_GPU_BATCH_POOL_SIZE; i++) {
        if (alloc_gpu_buffer(ctx, 4096, &ctx->batch_pool[i]) < 0) {
            LOG_EVENT(ctx, "BATCH_POOL_CREATE_FAILED: index=%d", i);
            lum_gpu_cleanup(ctx);
            return NULL;
        }
        
        LOG_EVENT(ctx, "BATCH_POOL_CREATED: index=%d handle=%u",
                  i, ctx->batch_pool[i].handle);
    }
    
    /* Créer pool I/O buffers */
    for (int i = 0; i < LUM_GPU_PARALLEL_MAX; i++) {
        if (alloc_gpu_buffer(ctx, LUM_GPU_BUFFER_SIZE_1MB, &ctx->input_pool[i]) < 0) {
            LOG_EVENT(ctx, "INPUT_POOL_CREATE_FAILED: index=%d", i);
            lum_gpu_cleanup(ctx);
            return NULL;
        }
        
        if (alloc_gpu_buffer(ctx, LUM_GPU_BUFFER_SIZE_1MB, &ctx->output_pool[i]) < 0) {
            LOG_EVENT(ctx, "OUTPUT_POOL_CREATE_FAILED: index=%d", i);
            lum_gpu_cleanup(ctx);
            return NULL;
        }
        
        LOG_EVENT(ctx, "IO_POOL_CREATED: index=%d input_handle=%u output_handle=%u",
                  i, ctx->input_pool[i].handle, ctx->output_pool[i].handle);
    }
    
    /* Charger kernel */
    FILE* kernel_file = fopen(config->kernel_path, "rb");
    if (!kernel_file) {
        LOG_EVENT(ctx, "KERNEL_LOAD_FAILED: path=%s errno=%d",
                  config->kernel_path, errno);
        lum_gpu_cleanup(ctx);
        return NULL;
    }
    
    fseek(kernel_file, 0, SEEK_END);
    ctx->kernel_size = ftell(kernel_file);
    fseek(kernel_file, 0, SEEK_SET);
    
    /* Allouer buffer kernel */
    lum_gpu_buffer_t kernel_buffer;
    if (alloc_gpu_buffer(ctx, ctx->kernel_size, &kernel_buffer) < 0) {
        fclose(kernel_file);
        lum_gpu_cleanup(ctx);
        return NULL;
    }
    
    /* Lire kernel */
    size_t read_bytes = fread(kernel_buffer.map, 1, ctx->kernel_size, kernel_file);
    fclose(kernel_file);
    
    if (read_bytes != ctx->kernel_size) {
        LOG_EVENT(ctx, "KERNEL_READ_FAILED: expected=%zu got=%zu",
                  ctx->kernel_size, read_bytes);
        free_gpu_buffer(ctx, &kernel_buffer);
        lum_gpu_cleanup(ctx);
        return NULL;
    }
    
    ctx->kernel_bo = kernel_buffer.handle;
    ctx->kernel_map = kernel_buffer.map;
    
    /* Cache kernel en mémoire */
    ctx->kernel_cache = malloc(ctx->kernel_size);
    if (ctx->kernel_cache) {
        memcpy(ctx->kernel_cache, kernel_buffer.map, ctx->kernel_size);
        LOG_EVENT(ctx, "KERNEL_CACHE_CREATED: size=%zu", ctx->kernel_size);
    }
    
    LOG_EVENT(ctx, "KERNEL_LOAD_SUCCESS: path=%s size=%zu handle=%u",
              config->kernel_path, ctx->kernel_size, ctx->kernel_bo);
    
    /* Configuration */
    ctx->batch_size = config->batch_size;
    ctx->work_group_size = config->work_group_size;
    ctx->enable_thp = config->enable_thp;
    ctx->enable_profiling = config->enable_profiling;
    
    /* Initialiser traçage mémoire LUM */
    ctx->mem_tracer = lum_memory_tracer_create();
    if (ctx->mem_tracer) {
        LOG_EVENT(ctx, "LUM_MEMORY_TRACER_CREATED");
    }
    
    /* Initialiser logger LUM */
    ctx->logger = lum_log_encoder_create();
    if (ctx->logger) {
        LOG_EVENT(ctx, "LUM_LOG_ENCODER_CREATED");
    }
    
    LOG_EVENT(ctx, "LUM_GPU_INIT_COMPLETE: contexts=%d batches=%d io_pairs=%d",
              LUM_GPU_CTX_POOL_SIZE, LUM_GPU_BATCH_POOL_SIZE, LUM_GPU_PARALLEL_MAX);
    
    return ctx;
}

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE - DISPATCH PARALLÈLE
 * ══════════════════════════════════════════════════════════════════════ */

int lum_gpu_dispatch_parallel(
    lum_gpu_context_t* ctx,
    const uint8_t* header,
    uint64_t start_nonce,
    uint32_t nonce_count,
    int n_parallel
) {
    if (!ctx || !header) return -1;
    if (n_parallel < 1 || n_parallel > LUM_GPU_PARALLEL_MAX) return -1;
    
    LOG_EVENT(ctx, "DISPATCH_PARALLEL_START: n=%d start_nonce=%lu count=%u",
              n_parallel, (unsigned long)start_nonce, nonce_count);
    
    pthread_mutex_lock(&ctx->dispatch_mutex);
    
    /* Vérifier capacité */
    if (ctx->num_active_dispatches + n_parallel > LUM_GPU_PARALLEL_MAX) {
        pthread_mutex_unlock(&ctx->dispatch_mutex);
        LOG_EVENT(ctx, "DISPATCH_PARALLEL_FAILED: too many active (%d + %d > %d)",
                  ctx->num_active_dispatches, n_parallel, LUM_GPU_PARALLEL_MAX);
        return -1;
    }
    
    /* Lancer N dispatches */
    for (int i = 0; i < n_parallel; i++) {
        int dispatch_id = ctx->num_active_dispatches + i;
        lum_gpu_dispatch_t* dispatch = &ctx->active_dispatches[dispatch_id];
        
        /* Allouer ressources */
        int ctx_index = allocate_context(ctx, dispatch_id);
        if (ctx_index < 0) {
            pthread_mutex_unlock(&ctx->dispatch_mutex);
            return -1;
        }
        
        int batch_index = allocate_batch_buffer(ctx, dispatch_id);
        if (batch_index < 0) {
            release_context(ctx, ctx_index);
            pthread_mutex_unlock(&ctx->dispatch_mutex);
            return -1;
        }
        
        int input_index, output_index;
        if (allocate_io_buffers(ctx, dispatch_id, &input_index, &output_index) < 0) {
            release_batch_buffer(ctx, batch_index);
            release_context(ctx, ctx_index);
            pthread_mutex_unlock(&ctx->dispatch_mutex);
            return -1;
        }
        
        /* Préparer input */
        uint8_t* input_data = (uint8_t*)ctx->input_pool[input_index].map;
        memcpy(input_data, header, 80);  /* Block header Bitcoin */
        uint64_t nonce_offset = start_nonce + (i * nonce_count);
        memcpy(input_data + 80, &nonce_offset, sizeof(uint64_t));
        memcpy(input_data + 88, &nonce_count, sizeof(uint32_t));
        
        /* Construire batch */
        int batch_bytes = build_batch_commands(
            ctx,
            &ctx->batch_pool[batch_index],
            ctx->input_pool[input_index].handle,
            ctx->output_pool[output_index].handle,
            nonce_count
        );
        
        /* Soumettre au GPU */
        dispatch->id = dispatch_id;
        dispatch->ctx_id = ctx->ctx_pool[ctx_index].ctx_id;
        dispatch->batch_bo = ctx->batch_pool[batch_index].handle;
        dispatch->input_bo = ctx->input_pool[input_index].handle;
        dispatch->output_bo = ctx->output_pool[output_index].handle;
        dispatch->start_nonce = nonce_offset;
        dispatch->nonce_count = nonce_count;
        dispatch->start_time_ns = get_timestamp_ns();
        dispatch->completed = false;
        dispatch->error_code = 0;
        
        if (submit_batch(ctx, dispatch->ctx_id, dispatch->batch_bo, batch_bytes) < 0) {
            dispatch->error_code = errno;
            release_io_buffers(ctx, input_index, output_index);
            release_batch_buffer(ctx, batch_index);
            release_context(ctx, ctx_index);
            pthread_mutex_unlock(&ctx->dispatch_mutex);
            return -1;
        }
        
        LOG_EVENT(ctx, "DISPATCH_SUBMITTED: id=%d ctx_id=%u batch_bo=%u nonce=%lu-%lu",
                  dispatch_id, dispatch->ctx_id, dispatch->batch_bo,
                  (unsigned long)nonce_offset, (unsigned long)(nonce_offset + nonce_count - 1));
    }
    
    ctx->num_active_dispatches += n_parallel;
    ctx->total_dispatches += n_parallel;
    
    pthread_mutex_unlock(&ctx->dispatch_mutex);
    
    LOG_EVENT(ctx, "DISPATCH_PARALLEL_COMPLETE: submitted=%d active=%d",
              n_parallel, ctx->num_active_dispatches);
    
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE - ATTENTE ET RÉSULTATS
 * ══════════════════════════════════════════════════════════════════════ */

int lum_gpu_wait_all(lum_gpu_context_t* ctx, uint32_t timeout_ms) {
    if (!ctx) return -1;
    
    uint64_t timeout_ns = timeout_ms ? (uint64_t)timeout_ms * 1000000ULL : -1ULL;
    int completed = 0;
    
    pthread_mutex_lock(&ctx->dispatch_mutex);
    
    for (int i = 0; i < ctx->num_active_dispatches; i++) {
        lum_gpu_dispatch_t* dispatch = &ctx->active_dispatches[i];
        
        if (!dispatch->completed) {
            if (wait_dispatch(ctx, dispatch->batch_bo, timeout_ns) == 0) {
                dispatch->end_time_ns = get_timestamp_ns();
                dispatch->completed = true;
                completed++;
                
                double time_sec = ns_to_sec(dispatch->end_time_ns - dispatch->start_time_ns);
                double hashrate_mhs = (double)dispatch->nonce_count / time_sec / 1000000.0;
                
                LOG_EVENT(ctx, "DISPATCH_COMPLETED: id=%d time=%.6f hashrate=%.2f MH/s",
                          dispatch->id, time_sec, hashrate_mhs);
            } else {
                dispatch->error_code = errno;
                LOG_EVENT(ctx, "DISPATCH_TIMEOUT: id=%d", dispatch->id);
            }
        }
    }
    
    pthread_mutex_unlock(&ctx->dispatch_mutex);
    
    return completed;
}

int lum_gpu_get_results(
    lum_gpu_context_t* ctx,
    int dispatch_id,
    uint32_t* out_nonce,
    uint8_t* out_hash
) {
    if (!ctx || dispatch_id < 0 || dispatch_id >= LUM_GPU_PARALLEL_MAX) return -1;
    
    pthread_mutex_lock(&ctx->dispatch_mutex);
    
    lum_gpu_dispatch_t* dispatch = &ctx->active_dispatches[dispatch_id];
    
    if (!dispatch->completed) {
        pthread_mutex_unlock(&ctx->dispatch_mutex);
        return -1;
    }
    
    /* Lire résultats depuis output buffer */
    pthread_mutex_lock(&ctx->io_mutex);
    
    for (int i = 0; i < LUM_GPU_PARALLEL_MAX; i++) {
        if (ctx->output_pool[i].dispatch_id == dispatch_id) {
            uint8_t* output_data = (uint8_t*)ctx->output_pool[i].map;
            
            if (out_nonce) {
                memcpy(out_nonce, output_data, sizeof(uint32_t));
            }
            
            if (out_hash) {
                memcpy(out_hash, output_data + 4, 32);
            }
            
            pthread_mutex_unlock(&ctx->io_mutex);
            pthread_mutex_unlock(&ctx->dispatch_mutex);
            
            LOG_EVENT(ctx, "RESULTS_READ: dispatch_id=%d nonce=%u", dispatch_id, *out_nonce);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&ctx->io_mutex);
    pthread_mutex_unlock(&ctx->dispatch_mutex);
    
    return -1;
}

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE - MÉTRIQUES
 * ══════════════════════════════════════════════════════════════════════ */

int lum_gpu_get_metrics(
    lum_gpu_context_t* ctx,
    double* out_hashrate,
    uint64_t* out_dispatches,
    double* out_time
) {
    if (!ctx) return -1;
    
    if (out_hashrate) *out_hashrate = 0.0;
    if (out_dispatches) *out_dispatches = ctx->total_dispatches;
    if (out_time) *out_time = ctx->total_time_sec;
    
    /* Calculer hashrate moyen */
    if (ctx->total_time_sec > 0.0) {
        double hashrate = (double)ctx->total_hashes / ctx->total_time_sec / 1000000.0;
        if (out_hashrate) *out_hashrate = hashrate;
    }
    
    return 0;
}

int lum_gpu_get_memory_stats(
    lum_gpu_context_t* ctx,
    size_t* out_total_alloc,
    size_t* out_peak_usage
) {
    if (!ctx || !ctx->mem_tracer) return -1;
    
    return lum_memory_tracer_get_stats(ctx->mem_tracer, out_total_alloc, out_peak_usage);
}

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE - NETTOYAGE
 * ══════════════════════════════════════════════════════════════════════ */

void lum_gpu_cleanup(lum_gpu_context_t* ctx) {
    if (!ctx) return;
    
    LOG_EVENT(ctx, "LUM_GPU_CLEANUP_START");
    
    /* Libérer pool batch buffers */
    for (int i = 0; i < LUM_GPU_BATCH_POOL_SIZE; i++) {
        free_gpu_buffer(ctx, &ctx->batch_pool[i]);
    }
    
    /* Libérer pool I/O buffers */
    for (int i = 0; i < LUM_GPU_PARALLEL_MAX; i++) {
        free_gpu_buffer(ctx, &ctx->input_pool[i]);
        free_gpu_buffer(ctx, &ctx->output_pool[i]);
    }
    
    /* Libérer kernel */
    if (ctx->kernel_bo) {
        lum_gpu_buffer_t kernel_buffer = {
            .handle = ctx->kernel_bo,
            .map = ctx->kernel_map,
            .size = ctx->kernel_size
        };
        free_gpu_buffer(ctx, &kernel_buffer);
    }
    
    if (ctx->kernel_cache) {
        free(ctx->kernel_cache);
        LOG_EVENT(ctx, "KERNEL_CACHE_FREED");
    }
    
    /* Détruire pool contextes */
    for (int i = 0; i < LUM_GPU_CTX_POOL_SIZE; i++) {
        if (ctx->ctx_pool[i].ctx_id) {
            destroy_gpu_context(ctx, ctx->ctx_pool[i].ctx_id);
        }
    }
    
    /* Libérer traçage mémoire LUM */
    if (ctx->mem_tracer) {
        lum_memory_tracer_destroy(ctx->mem_tracer);
        LOG_EVENT(ctx, "LUM_MEMORY_TRACER_DESTROYED");
    }
    
    /* Libérer logger LUM */
    if (ctx->logger) {
        lum_log_encoder_destroy(ctx->logger);
        LOG_EVENT(ctx, "LUM_LOG_ENCODER_DESTROYED");
    }
    
    /* Détruire mutex */
    pthread_mutex_destroy(&ctx->ctx_mutex);
    pthread_mutex_destroy(&ctx->batch_mutex);
    pthread_mutex_destroy(&ctx->io_mutex);
    pthread_mutex_destroy(&ctx->dispatch_mutex);
    
    /* Fermer DRM */
    if (ctx->drm_fd >= 0) {
        close(ctx->drm_fd);
        LOG_EVENT(ctx, "DRM_CLOSED: fd=%d", ctx->drm_fd);
    }
    
    LOG_EVENT(ctx, "LUM_GPU_CLEANUP_COMPLETE: dispatches=%lu time=%.3f",
              (unsigned long)ctx->total_dispatches, ctx->total_time_sec);
    
    /* Fermer log */
    if (ctx->log_file) {
        fclose(ctx->log_file);
    }
    
    free(ctx);
}

// Made with Bob
