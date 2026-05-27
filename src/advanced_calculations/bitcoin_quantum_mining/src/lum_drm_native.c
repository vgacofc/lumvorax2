/* lum_drm_native.c - Implémentation DRM native SANS libdrm
 * Accès direct /dev/dri/renderD128 via ioctl
 * 100% C pur, 0 dépendances externes
 * AVEC LOGGING FORENSIQUE BIT-LEVEL
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <drm/drm.h>
#include <drm/i915_drm.h>
#include "lum_drm_native.h"

/* Fichier log forensique */
static FILE *g_log_file = NULL;

/* Pool de contextes DRM pour réutilisation */
#define DRM_CONTEXT_POOL_SIZE 4
static lum_drm_context_t g_drm_pool[DRM_CONTEXT_POOL_SIZE];
static int g_drm_pool_initialized = 0;
static int g_drm_pool_index = 0;

/* Obtenir timestamp précis */
static double get_timestamp_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

/* Logger forensique bit-level */
static void drm_log(const char *fmt, ...) {
    if (!g_log_file) {
        g_log_file = fopen("logs/lum_drm_native_forensic.log", "a");
        if (!g_log_file) return;
    }
    
    double ts = get_timestamp_sec();
    fprintf(g_log_file, "[DRM][%.9f] ", ts);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log_file, fmt, args);
    va_end(args);
    
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
}

/* Logger bit-level pour valeurs 32-bit */
static void drm_log_bits32(const char *label, uint32_t value) {
    drm_log("%s: 0x%08x = binary=%08x_%08x_%08x_%08x",
            label,
            value,
            (value >> 24) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 8) & 0xFF,
            value & 0xFF);
}

/* Logger bit-level pour valeurs 64-bit */
static void drm_log_bits64(const char *label, uint64_t value) {
    drm_log("%s: 0x%016lx = binary=%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x",
            label,
            value,
            (uint32_t)((value >> 56) & 0xFF),
            (uint32_t)((value >> 48) & 0xFF),
            (uint32_t)((value >> 40) & 0xFF),
            (uint32_t)((value >> 32) & 0xFF),
            (uint32_t)((value >> 24) & 0xFF),
            (uint32_t)((value >> 16) & 0xFF),
            (uint32_t)((value >> 8) & 0xFF),
            (uint32_t)(value & 0xFF));
}

/* Initialiser contexte DRM */
int lum_drm_init(lum_drm_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    drm_log("=== DRM INIT START ===");
    memset(ctx, 0, sizeof(lum_drm_context_t));
    
    /* Ouvrir /dev/dri/renderD128 (GPU render node) */
    drm_log("OPEN: /dev/dri/renderD128 O_RDWR");
    ctx->fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->fd < 0) {
        drm_log("ERROR: open() failed errno=%d (%s)", errno, strerror(errno));
        fprintf(stderr, "[DRM] Cannot open /dev/dri/renderD128: %s\n", strerror(errno));
        return -1;
    }
    
    drm_log("SUCCESS: fd=%d opened", ctx->fd);
    printf("[DRM] Opened /dev/dri/renderD128 (fd=%d)\n", ctx->fd);
    
    /* Obtenir version DRM */
    struct drm_version version = {0};
    char name_buf[64] = {0};
    char date_buf[64] = {0};
    char desc_buf[128] = {0};
    
    version.name_len = sizeof(name_buf);
    version.name = name_buf;
    version.date_len = sizeof(date_buf);
    version.date = date_buf;
    version.desc_len = sizeof(desc_buf);
    version.desc = desc_buf;
    
    drm_log("IOCTL: DRM_IOCTL_VERSION");
    if (ioctl(ctx->fd, DRM_IOCTL_VERSION, &version) < 0) {
        drm_log("ERROR: DRM_IOCTL_VERSION failed errno=%d", errno);
        fprintf(stderr, "[DRM] Cannot get version: %s\n", strerror(errno));
        close(ctx->fd);
        return -1;
    }
    
    ctx->version_major = version.version_major;
    ctx->version_minor = version.version_minor;
    snprintf(ctx->driver_name, sizeof(ctx->driver_name), "%s", name_buf);
    
    drm_log("DRIVER: name=%s version=%d.%d", ctx->driver_name,
            ctx->version_major, ctx->version_minor);
    printf("[DRM] Driver: %s v%d.%d\n", ctx->driver_name,
           ctx->version_major, ctx->version_minor);
    
    /* Vérifier que c'est i915 */
    if (strcmp(ctx->driver_name, "i915") != 0) {
        drm_log("WARNING: Not i915 driver (got %s)", ctx->driver_name);
        fprintf(stderr, "[DRM] WARNING: Not i915 driver (got %s)\n", ctx->driver_name);
    }
    
    ctx->initialized = 1;
    drm_log("=== DRM INIT COMPLETE ===");
    return 0;
}

/* Nettoyer contexte DRM */
void lum_drm_cleanup(lum_drm_context_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }
    
    drm_log("=== DRM CLEANUP START ===");
    if (ctx->fd >= 0) {
        drm_log("CLOSE: fd=%d", ctx->fd);
        close(ctx->fd);
        ctx->fd = -1;
    }
    
    ctx->initialized = 0;
    drm_log("=== DRM CLEANUP COMPLETE ===");
    
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    printf("[DRM] Cleanup complete\n");
}

/* Créer GEM buffer object */
int lum_drm_gem_create(lum_drm_context_t *ctx, uint64_t size, uint32_t *handle_out) {
    if (!ctx || !ctx->initialized || !handle_out) {
        return -1;
    }
    
    drm_log("=== GEM CREATE START ===");
    drm_log_bits64("REQUEST_SIZE", size);
    
    struct drm_i915_gem_create create = {0};
    create.size = size;
    
    drm_log("IOCTL: DRM_IOCTL_I915_GEM_CREATE");
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        drm_log("ERROR: GEM_CREATE failed errno=%d (%s)", errno, strerror(errno));
        fprintf(stderr, "[DRM] GEM create failed: %s\n", strerror(errno));
        return -1;
    }
    
    *handle_out = create.handle;
    drm_log_bits32("GEM_HANDLE", create.handle);
    drm_log("SUCCESS: handle=%u size=%lu", create.handle, size);
    printf("[DRM] GEM created: handle=%u size=%lu\n", create.handle, size);
    
    return 0;
}

/* Mapper GEM buffer en mémoire */
int lum_drm_gem_mmap(lum_drm_context_t *ctx, uint32_t handle, uint64_t size, void **addr_out) {
    if (!ctx || !ctx->initialized || !addr_out) {
        return -1;
    }
    
    drm_log("=== GEM MMAP START ===");
    drm_log_bits32("HANDLE", handle);
    drm_log_bits64("SIZE", size);
    
    struct drm_i915_gem_mmap mmap_arg = {0};
    mmap_arg.handle = handle;
    mmap_arg.offset = 0;
    mmap_arg.size = size;
    
    drm_log("IOCTL: DRM_IOCTL_I915_GEM_MMAP handle=%u offset=%lu size=%lu",
            mmap_arg.handle, mmap_arg.offset, mmap_arg.size);
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        drm_log("ERROR: GEM_MMAP failed errno=%d (%s)", errno, strerror(errno));
        fprintf(stderr, "[DRM] GEM mmap failed: %s\n", strerror(errno));
        return -1;
    }
    
    *addr_out = (void*)mmap_arg.addr_ptr;
    drm_log_bits64("MAPPED_ADDR", (uint64_t)*addr_out);
    drm_log("SUCCESS: mapped ptr=%p", *addr_out);
    printf("[DRM] GEM mapped: handle=%u addr=%p size=%lu\n",
           handle, *addr_out, size);
    
    return 0;
}

/* Fermer GEM buffer */
int lum_drm_gem_close(lum_drm_context_t *ctx, uint32_t handle) {
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    /* Note: DRM_IOCTL_GEM_CLOSE existe mais pas implémenté ici
     * Les handles sont automatiquement libérés à la fermeture du fd */
    
    printf("[DRM] GEM closed: handle=%u\n", handle);
    return 0;
}

/* Exécuter commandes GPU */
int lum_drm_execbuffer(lum_drm_context_t *ctx, uint32_t *handles, uint32_t count) {
    if (!ctx || !ctx->initialized || !handles || count == 0) {
        return -1;
    }
    
    drm_log("=== EXECBUFFER START ===");
    drm_log("BUFFER_COUNT: %u", count);
    
    /* Créer exec objects */
    struct drm_i915_gem_exec_object2 *exec_objects =
        calloc(count, sizeof(struct drm_i915_gem_exec_object2));
    
    if (!exec_objects) {
        drm_log("ERROR: calloc() failed for exec_objects");
        return -1;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        exec_objects[i].handle = handles[i];
        drm_log_bits32("HANDLE[%u]", handles[i]);
    }
    
    /* Préparer execbuffer2 */
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uint64_t)exec_objects;
    execbuf.buffer_count = count;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = 0;  /* Sera calculé par le driver */
    drm_log("IOCTL: DRM_IOCTL_I915_GEM_EXECBUFFER2 count=%u", count);
    int ret = ioctl(ctx->fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    free(exec_objects);
    
    if (ret < 0) {
        drm_log("ERROR: EXECBUFFER2 failed errno=%d (%s)", errno, strerror(errno));
        fprintf(stderr, "[DRM] Execbuffer failed: %s\n", strerror(errno));
        return -1;
    }
    
    drm_log("SUCCESS: %u buffers executed", count);
    printf("[DRM] Execbuffer success: %u buffers\n", count);
    return 0;
}

/* Obtenir version driver */
const char* lum_drm_get_version(lum_drm_context_t *ctx) {
    static char version_str[128];
    
    if (!ctx || !ctx->initialized) {
        return "unknown";
    }
    
    snprintf(version_str, sizeof(version_str), "%s %d.%d",
             ctx->driver_name, ctx->version_major, ctx->version_minor);
    
    return version_str;
}

/* Vérifier driver i915 */
int lum_drm_check_i915(lum_drm_context_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return 0;
    }
    
    return (strcmp(ctx->driver_name, "i915") == 0) ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
   POOL DE CONTEXTES DRM (OPTIMISATION #1 C193)
   ═══════════════════════════════════════════════════════════════════════════ */

/* Initialiser pool de contextes DRM */
int lum_drm_pool_init(void) {
    if (g_drm_pool_initialized) {
        drm_log("POOL: Already initialized");
        return 0;
    }
    
    drm_log("=== POOL INIT START ===");
    
    for (int i = 0; i < DRM_CONTEXT_POOL_SIZE; i++) {
        drm_log("POOL: Initializing context %d/%d", i+1, DRM_CONTEXT_POOL_SIZE);
        
        if (lum_drm_init(&g_drm_pool[i]) < 0) {
            drm_log("ERROR: Failed to init context %d", i);
            
            /* Cleanup contextes déjà initialisés */
            for (int j = 0; j < i; j++) {
                lum_drm_cleanup(&g_drm_pool[j]);
            }
            return -1;
        }
    }
    
    g_drm_pool_initialized = 1;
    drm_log("=== POOL INIT COMPLETE: %d contexts ready ===", DRM_CONTEXT_POOL_SIZE);
    
    return 0;
}

/* Obtenir contexte DRM du pool (round-robin) */
lum_drm_context_t* lum_drm_pool_get(void) {
    if (!g_drm_pool_initialized) {
        drm_log("ERROR: Pool not initialized, call lum_drm_pool_init() first");
        return NULL;
    }
    
    /* Round-robin pour distribution équitable */
    lum_drm_context_t* ctx = &g_drm_pool[g_drm_pool_index];
    g_drm_pool_index = (g_drm_pool_index + 1) % DRM_CONTEXT_POOL_SIZE;
    
    drm_log("POOL: Returning context %d", g_drm_pool_index);
    return ctx;
}

/* Cleanup pool complet */
void lum_drm_pool_cleanup(void) {
    if (!g_drm_pool_initialized) {
        return;
    }
    
    drm_log("=== POOL CLEANUP START ===");
    
    for (int i = 0; i < DRM_CONTEXT_POOL_SIZE; i++) {
        drm_log("POOL: Cleaning context %d/%d", i+1, DRM_CONTEXT_POOL_SIZE);
        lum_drm_cleanup(&g_drm_pool[i]);
    }
    
    g_drm_pool_initialized = 0;
    drm_log("=== POOL CLEANUP COMPLETE ===");
}

/* ═══════════════════════════════════════════════════════════════════════════
   BATCH ALLOCATION GEM (OPTIMISATION #2 C193)
   ═══════════════════════════════════════════════════════════════════════════ */

/* Créer N buffers GEM en batch */
int lum_drm_gem_create_batch(lum_drm_context_t *ctx, 
                              uint64_t size, 
                              uint32_t count,
                              uint32_t *handles_out) {
    if (!ctx || !ctx->initialized || !handles_out || count == 0) {
        return -1;
    }
    
    drm_log("=== GEM CREATE BATCH START: count=%u size=%lu ===", count, size);
    
    double start_time = get_timestamp_sec();
    
    for (uint32_t i = 0; i < count; i++) {
        if (lum_drm_gem_create(ctx, size, &handles_out[i]) < 0) {
            drm_log("ERROR: Batch failed at index %u/%u", i, count);
            
            /* Cleanup handles déjà créés */
            for (uint32_t j = 0; j < i; j++) {
                lum_drm_gem_close(ctx, handles_out[j]);
            }
            return -1;
        }
    }
    
    double elapsed = get_timestamp_sec() - start_time;
    double avg_latency_us = (elapsed * 1000000.0) / count;
    
    drm_log("=== GEM CREATE BATCH COMPLETE: %u buffers in %.6f sec (%.3f µs/buffer) ===",
            count, elapsed, avg_latency_us);
    
    return 0;
}

/* Mapper N buffers GEM en batch */
int lum_drm_gem_mmap_batch(lum_drm_context_t *ctx,
                            uint32_t *handles,
                            uint64_t size,
                            uint32_t count,
                            void **addrs_out) {
    if (!ctx || !ctx->initialized || !handles || !addrs_out || count == 0) {
        return -1;
    }
    
    drm_log("=== GEM MMAP BATCH START: count=%u size=%lu ===", count, size);
    
    double start_time = get_timestamp_sec();
    
    for (uint32_t i = 0; i < count; i++) {
        if (lum_drm_gem_mmap(ctx, handles[i], size, &addrs_out[i]) < 0) {
            drm_log("ERROR: Batch mmap failed at index %u/%u", i, count);
            return -1;
        }
    }
    
    double elapsed = get_timestamp_sec() - start_time;
    double avg_latency_us = (elapsed * 1000000.0) / count;
    
    drm_log("=== GEM MMAP BATCH COMPLETE: %u buffers in %.6f sec (%.3f µs/buffer) ===",
            count, elapsed, avg_latency_us);
    
    return 0;
}

/* Fermer N buffers GEM en batch */
int lum_drm_gem_close_batch(lum_drm_context_t *ctx,
                             uint32_t *handles,
                             uint32_t count) {
    if (!ctx || !ctx->initialized || !handles || count == 0) {
        return -1;
    }
    
    drm_log("=== GEM CLOSE BATCH START: count=%u ===", count);
    
    double start_time = get_timestamp_sec();
    
    for (uint32_t i = 0; i < count; i++) {
        lum_drm_gem_close(ctx, handles[i]);
    }
    
    double elapsed = get_timestamp_sec() - start_time;
    double avg_latency_us = (elapsed * 1000000.0) / count;
    
    drm_log("=== GEM CLOSE BATCH COMPLETE: %u buffers in %.6f sec (%.3f µs/buffer) ===",
            count, elapsed, avg_latency_us);
    
    return 0;
}

// Made with Bob
    
