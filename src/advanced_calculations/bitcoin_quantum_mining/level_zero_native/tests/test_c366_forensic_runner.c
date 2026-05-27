/*
 * TEST C366 - FORENSIC RUNNER ULTRA-GRANULAIRE
 * =============================================
 * Runner de test pour diagnostics T1, T2, T11
 * Instrumentation forensique MAXIMALE:
 *   - Checksums CRC32 (kernel, batch, SSH, output)
 *   - Timestamps nanoseconde (chaque étape)
 *   - Hex dumps complets (tous les buffers)
 *   - Validation bit-level (chaque dword)
 *
 * OBJECTIF: Isoler définitivement la couche Gen9 défaillante
 *   - ISA kernel execution
 *   - Thread payload / GID
 *   - CURBE / Cross-thread data
 *   - Scoreboard / Thread dispatch
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

/* DRM headers */
#include <drm/i915_drm.h>
#include <xf86drm.h>

/* Gen9 structures */
#include "../src/btc_gen9_structures.h"

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 1: UTILITAIRES FORENSIQUES
 * ═══════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static uint32_t crc32_compute(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    
    return ~crc;
}

static void hex_dump(FILE* log, const char* label, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    fprintf(log, "%s (size=%zu bytes, CRC32=0x%08x):\n", label, size, crc32_compute(data, size));
    
    for (size_t i = 0; i < size; i += 16) {
        fprintf(log, "  %04zx: ", i);
        
        /* Hex */
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            fprintf(log, "%02x ", bytes[i + j]);
        }
        
        /* Padding */
        for (size_t j = size - i; j < 16; j++) {
            fprintf(log, "   ");
        }
        
        /* ASCII */
        fprintf(log, " |");
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            uint8_t c = bytes[i + j];
            fprintf(log, "%c", (c >= 32 && c < 127) ? c : '.');
        }
        fprintf(log, "|\n");
    }
    
    fflush(log);
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 2: CONTEXTE TEST
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    
    /* Buffers GPU */
    uint32_t kernel_bo;
    void* kernel_map;
    size_t kernel_size;
    
    uint32_t batch_bo;
    void* batch_map;
    size_t batch_size;
    
    uint32_t ssh_bo;
    void* ssh_map;
    size_t ssh_size;
    
    uint32_t dsh_bo;
    void* dsh_map;
    size_t dsh_size;
    
    uint32_t ioh_bo;
    void* ioh_map;
    size_t ioh_size;
    
    uint32_t output_bo;
    void* output_map;
    size_t output_size;
    
    /* Forensique */
    FILE* log_file;
    uint64_t event_counter;
} test_context_t;

#define LOG_EVENT(ctx, fmt, ...) do { \
    uint64_t ts = get_timestamp_ns(); \
    fprintf((ctx)->log_file, "[%llu.%09llu] " fmt "\n", \
            ts / 1000000000ULL, ts % 1000000000ULL, ##__VA_ARGS__); \
    fflush((ctx)->log_file); \
    (ctx)->event_counter++; \
} while(0)

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 3: INITIALISATION
 * ═══════════════════════════════════════════════════════════════════ */

static int test_init_drm(test_context_t* ctx) {
    LOG_EVENT(ctx, "=== INIT DRM START ===");
    
    /* Ouvrir /dev/dri/renderD128 */
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        LOG_EVENT(ctx, "ERROR: Failed to open /dev/dri/renderD128: %s", strerror(errno));
        return -1;
    }
    LOG_EVENT(ctx, "DRM_FD: %d", ctx->drm_fd);
    
    /* Créer contexte i915 */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        LOG_EVENT(ctx, "ERROR: Failed to create context: %s", strerror(errno));
        close(ctx->drm_fd);
        return -1;
    }
    ctx->ctx_id = ctx_create.ctx_id;
    LOG_EVENT(ctx, "CONTEXT_ID: %u", ctx->ctx_id);
    
    LOG_EVENT(ctx, "=== INIT DRM SUCCESS ===");
    return 0;
}

static int test_create_buffer(test_context_t* ctx, uint32_t* bo, void** map, size_t size, const char* name) {
    LOG_EVENT(ctx, "CREATE_BUFFER: %s size=%zu", name, size);
    
    /* Créer buffer object */
    struct drm_i915_gem_create create = {
        .size = size
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        LOG_EVENT(ctx, "ERROR: Failed to create %s: %s", name, strerror(errno));
        return -1;
    }
    
    *bo = create.handle;
    LOG_EVENT(ctx, "  BO_HANDLE: %u", *bo);
    
    /* Mapper en mémoire CPU */
    struct drm_i915_gem_mmap_offset mmap_offset = {
        .handle = *bo,
        .flags = I915_MMAP_OFFSET_WB  /* Write-back cache */
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_offset) < 0) {
        LOG_EVENT(ctx, "ERROR: Failed to get mmap offset for %s: %s", name, strerror(errno));
        return -1;
    }
    
    *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->drm_fd, mmap_offset.offset);
    if (*map == MAP_FAILED) {
        LOG_EVENT(ctx, "ERROR: Failed to mmap %s: %s", name, strerror(errno));
        return -1;
    }
    
    LOG_EVENT(ctx, "  MAPPED: %p", *map);
    LOG_EVENT(ctx, "CREATE_BUFFER_SUCCESS: %s", name);
    
    return 0;
}

static int test_load_kernel(test_context_t* ctx, const char* kernel_path) {
    LOG_EVENT(ctx, "=== LOAD KERNEL START: %s ===", kernel_path);
    
    /* Lire fichier kernel */
    FILE* fp = fopen(kernel_path, "rb");
    if (!fp) {
        LOG_EVENT(ctx, "ERROR: Failed to open kernel: %s", strerror(errno));
        return -1;
    }
    
    fseek(fp, 0, SEEK_END);
    ctx->kernel_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    LOG_EVENT(ctx, "KERNEL_SIZE: %zu bytes", ctx->kernel_size);
    
    /* Créer buffer kernel */
    if (test_create_buffer(ctx, &ctx->kernel_bo, &ctx->kernel_map, ctx->kernel_size, "KERNEL") < 0) {
        fclose(fp);
        return -1;
    }
    
    /* Charger kernel en mémoire */
    size_t read_size = fread(ctx->kernel_map, 1, ctx->kernel_size, fp);
    fclose(fp);
    
    if (read_size != ctx->kernel_size) {
        LOG_EVENT(ctx, "ERROR: Failed to read kernel (got %zu, expected %zu)", read_size, ctx->kernel_size);
        return -1;
    }
    
    /* Checksum kernel */
    uint32_t kernel_crc = crc32_compute(ctx->kernel_map, ctx->kernel_size);
    LOG_EVENT(ctx, "KERNEL_CRC32: 0x%08x", kernel_crc);
    
    /* Hex dump premiers 256 bytes */
    hex_dump(ctx->log_file, "KERNEL_HEXDUMP_HEAD", ctx->kernel_map, 
             ctx->kernel_size < 256 ? ctx->kernel_size : 256);
    
    LOG_EVENT(ctx, "=== LOAD KERNEL SUCCESS ===");
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 4: CONFIGURATION SURFACE STATES
 * ═══════════════════════════════════════════════════════════════════ */

static int test_setup_ssh(test_context_t* ctx) {
    LOG_EVENT(ctx, "=== SETUP SSH START ===");
    
    /* Créer SSH (2 surface states = 128 bytes) */
    ctx->ssh_size = 128;
    if (test_create_buffer(ctx, &ctx->ssh_bo, &ctx->ssh_map, ctx->ssh_size, "SSH") < 0) {
        return -1;
    }
    
    struct gen9_surface_state* surfaces = (struct gen9_surface_state*)ctx->ssh_map;
    
    /* Surface 0: Output buffer (BTI 0) */
    surfaces[0].dw0 = 0x87fc0000;  /* SURFTYPE_BUFFER | RAW */
    surfaces[0].dw1 = 0x00000000;  /* Sera patché par i915 */
    surfaces[0].dw2 = ctx->output_size - 1;  /* Taille - 1 */
    surfaces[0].dw3 = 0x00000000;
    
    LOG_EVENT(ctx, "SSH_SURFACE[0]: dw0=0x%08x dw1=0x%08x dw2=0x%08x dw3=0x%08x",
             surfaces[0].dw0, surfaces[0].dw1, surfaces[0].dw2, surfaces[0].dw3);
    
    /* Checksum SSH */
    uint32_t ssh_crc = crc32_compute(ctx->ssh_map, ctx->ssh_size);
    LOG_EVENT(ctx, "SSH_CRC32: 0x%08x", ssh_crc);
    
    hex_dump(ctx->log_file, "SSH_HEXDUMP", ctx->ssh_map, ctx->ssh_size);
    
    LOG_EVENT(ctx, "=== SETUP SSH SUCCESS ===");
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 5: BATCH BUFFER GENERATION
 * ═══════════════════════════════════════════════════════════════════ */

static int test_build_batch(test_context_t* ctx, uint32_t global_size) {
    LOG_EVENT(ctx, "=== BUILD BATCH START: global_size=%u ===", global_size);
    
    ctx->batch_size = 4096;  /* 4KB batch buffer */
    if (test_create_buffer(ctx, &ctx->batch_bo, &ctx->batch_map, ctx->batch_size, "BATCH") < 0) {
        return -1;
    }
    
    uint32_t* batch = (uint32_t*)ctx->batch_map;
    int idx = 0;
    
    /* PIPELINE_SELECT */
    batch[idx++] = 0x69040001;  /* GPGPU mode, DW_LENGTH=1 */
    
    /* STATE_BASE_ADDRESS - C367 FIX: Utiliser 0x00000000 au lieu de 0x00000001 */
    batch[idx++] = 0x61010000 | (16 - 2);
    batch[idx++] = 0x00000000;  /* General State Base */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  /* Surface State Base = SSH (sera patché par relocation) */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  /* Dynamic State Base = DSH (sera patché par relocation) */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  /* Indirect Object Base = IOH (sera patché par relocation) */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  /* Instruction Base = Kernel (sera patché par relocation) */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  /* General State Buffer Size */
    batch[idx++] = 0xfffff000;  /* Dynamic State Buffer Size */
    batch[idx++] = 0xfffff000;  /* Indirect Object Buffer Size */
    batch[idx++] = 0xfffff000;  /* Instruction Buffer Size */
    
    /* MEDIA_VFE_STATE */
    batch[idx++] = 0x70000000 | (9 - 2);
    batch[idx++] = 0x00000000;  /* Scratch Space */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  /* Max Threads = 0 (auto) */
    batch[idx++] = 0x00000000;  /* URB Entries */
    batch[idx++] = 0x00000000;  /* CURBE Allocation */
    batch[idx++] = 0x00000000;  /* Scoreboard */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    /* MEDIA_INTERFACE_DESCRIPTOR_LOAD */
    batch[idx++] = 0x70020000 | (4 - 2);
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000020;  /* Length = 32 bytes (1 descriptor) */
    batch[idx++] = 0x00000000;  /* Offset dans DSH */
    
    /* GPGPU_WALKER */
    uint32_t thread_width = (global_size + 15) / 16;  /* SIMD16 */
    batch[idx++] = 0x71050000 | (15 - 2);
    batch[idx++] = 0x00000000;  /* Interface Descriptor Offset */
    batch[idx++] = 0x00000000;  /* Indirect Data Length */
    batch[idx++] = 0x00000000;  /* Indirect Data Start */
    batch[idx++] = 0xffffffff;  /* Thread Group ID X/Y/Z */
    batch[idx++] = 0x00000000;  /* Thread Group ID Starting X */
    batch[idx++] = 0x00000000;  /* Thread Group ID X Dimension */
    batch[idx++] = 0x00000000;  /* Thread Group ID Starting Y */
    batch[idx++] = 0x00000001;  /* Thread Group ID Y Dimension */
    batch[idx++] = 0x00000000;  /* Thread Group ID Starting Z */
    batch[idx++] = 0x00000001;  /* Thread Group ID Z Dimension */
    batch[idx++] = thread_width;  /* Right Execution Mask */
    batch[idx++] = 0xffffffff;  /* Bottom Execution Mask */
    
    /* PIPE_CONTROL (flush) */
    batch[idx++] = 0x7a000000 | (6 - 2);
    batch[idx++] = 0x00100000;  /* CS_STALL */
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    /* MI_BATCH_BUFFER_END */
    batch[idx++] = 0x0a000000;
    
    size_t batch_len = idx * 4;
    LOG_EVENT(ctx, "BATCH_LENGTH: %zu bytes (%d dwords)", batch_len, idx);
    
    /* Checksum batch */
    uint32_t batch_crc = crc32_compute(ctx->batch_map, batch_len);
    LOG_EVENT(ctx, "BATCH_CRC32: 0x%08x", batch_crc);
    
    hex_dump(ctx->log_file, "BATCH_HEXDUMP", ctx->batch_map, batch_len);
    
    LOG_EVENT(ctx, "=== BUILD BATCH SUCCESS ===");
    return batch_len;
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 6: INTERFACE DESCRIPTOR (DSH)
 * ═══════════════════════════════════════════════════════════════════ */

static int test_setup_dsh(test_context_t* ctx) {
    LOG_EVENT(ctx, "=== SETUP DSH START ===");
    
    ctx->dsh_size = 4096;
    if (test_create_buffer(ctx, &ctx->dsh_bo, &ctx->dsh_map, ctx->dsh_size, "DSH") < 0) {
        return -1;
    }
    
    struct gen9_interface_descriptor* desc = (struct gen9_interface_descriptor*)ctx->dsh_map;
    
    desc->dw0 = 0x00000000;  /* Kernel Start Pointer (relatif) */
    desc->dw1 = 0x00000000;
    desc->dw2 = 0x00000000;  /* SPF=0, FP mode */
    desc->dw3 = 0x00000040;  /* Binding Table Entries = 1 */
    desc->dw4 = 0x00000000;  /* CURBE Read Length = 0 */
    desc->dw5 = 0x00e00000;  /* Barrier Enable, SIMD16 */
    desc->dw6 = 0x00000000;
    desc->dw7 = 0x00000000;
    
    LOG_EVENT(ctx, "INTERFACE_DESCRIPTOR:");
    LOG_EVENT(ctx, "  dw0=0x%08x dw1=0x%08x dw2=0x%08x dw3=0x%08x",
             desc->dw0, desc->dw1, desc->dw2, desc->dw3);
    LOG_EVENT(ctx, "  dw4=0x%08x dw5=0x%08x dw6=0x%08x dw7=0x%08x",
             desc->dw4, desc->dw5, desc->dw6, desc->dw7);
    
    uint32_t dsh_crc = crc32_compute(ctx->dsh_map, 32);
    LOG_EVENT(ctx, "DSH_CRC32: 0x%08x", dsh_crc);
    
    hex_dump(ctx->log_file, "DSH_HEXDUMP", ctx->dsh_map, 32);
    
    LOG_EVENT(ctx, "=== SETUP DSH SUCCESS ===");
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 7: BINDING TABLE (IOH)
 * ═══════════════════════════════════════════════════════════════════ */

static int test_setup_ioh(test_context_t* ctx) {
    LOG_EVENT(ctx, "=== SETUP IOH START ===");
    
    ctx->ioh_size = 4096;
    if (test_create_buffer(ctx, &ctx->ioh_bo, &ctx->ioh_map, ctx->ioh_size, "IOH") < 0) {
        return -1;
    }
    
    uint32_t* binding_table = (uint32_t*)ctx->ioh_map;
    
    /* BTI 0 → Surface State 0 (offset 0 dans SSH) */
    binding_table[0] = 0x00000000;
    
    LOG_EVENT(ctx, "BINDING_TABLE: BTI[0]=0x%08x", binding_table[0]);
    
    uint32_t ioh_crc = crc32_compute(ctx->ioh_map, 4);
    LOG_EVENT(ctx, "IOH_CRC32: 0x%08x", ioh_crc);
    
    hex_dump(ctx->log_file, "IOH_HEXDUMP", ctx->ioh_map, 4);
    
    LOG_EVENT(ctx, "=== SETUP IOH SUCCESS ===");
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 8: EXÉCUTION GPU
 * ═══════════════════════════════════════════════════════════════════ */

static int test_execute_gpu(test_context_t* ctx, int batch_len) {
    LOG_EVENT(ctx, "=== EXECUTE GPU START ===");
    
    uint64_t start_ts = get_timestamp_ns();
    
    /* C367 FIX ROOT CAUSE: Ajouter relocations batch pour STATE_BASE_ADDRESS */
    static uint64_t cached_output_gtt = 0x02627000;  /* Offset présumé comme runner production */
    
    /* C367: Relocations pour STATE_BASE_ADDRESS dans batch buffer
     * PIPELINE_SELECT = 2 dwords (offset 0-1)
     * STATE_BASE_ADDRESS commence à offset 2 (8 bytes)
     * DW4 (SSH) = offset 2 + 4 = 6 dwords = 24 bytes
     * DW6 (DSH) = offset 2 + 6 = 8 dwords = 32 bytes
     * DW8 (IOH) = offset 2 + 8 = 10 dwords = 40 bytes
     * DW10 (Kernel) = offset 2 + 10 = 12 dwords = 48 bytes
     */
    struct drm_i915_gem_relocation_entry relocs_batch[4];
    memset(relocs_batch, 0, sizeof(relocs_batch));
    
    /* Relocation 0: SSH dans STATE_BASE_ADDRESS DW4 */
    relocs_batch[0].target_handle = ctx->ssh_bo;
    relocs_batch[0].delta = 0;
    relocs_batch[0].offset = 2 * 4 + 4 * 4;  /* STATE_BASE_ADDRESS offset + DW4 */
    relocs_batch[0].presumed_offset = 0;
    relocs_batch[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_batch[0].write_domain = 0;
    
    /* Relocation 1: DSH dans STATE_BASE_ADDRESS DW6 */
    relocs_batch[1].target_handle = ctx->dsh_bo;
    relocs_batch[1].delta = 0;
    relocs_batch[1].offset = 2 * 4 + 6 * 4;
    relocs_batch[1].presumed_offset = 0;
    relocs_batch[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_batch[1].write_domain = 0;
    
    /* Relocation 2: IOH dans STATE_BASE_ADDRESS DW8 */
    relocs_batch[2].target_handle = ctx->ioh_bo;
    relocs_batch[2].delta = 0;
    relocs_batch[2].offset = 2 * 4 + 8 * 4;
    relocs_batch[2].presumed_offset = 0;
    relocs_batch[2].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_batch[2].write_domain = 0;
    
    /* Relocation 3: Kernel dans STATE_BASE_ADDRESS DW10 */
    relocs_batch[3].target_handle = ctx->kernel_bo;
    relocs_batch[3].delta = 0;
    relocs_batch[3].offset = 2 * 4 + 10 * 4;
    relocs_batch[3].presumed_offset = 0;
    relocs_batch[3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs_batch[3].write_domain = 0;
    
    LOG_EVENT(ctx, "C367_RELOCATIONS_BATCH: 4 relocations configured");
    
    /* Préparer exec_objects SANS PINNED flags */
    struct drm_i915_gem_exec_object2 exec_objects[7] = {
        { .handle = ctx->kernel_bo, .flags = 0, .offset = 0 },
        { .handle = ctx->output_bo, .flags = EXEC_OBJECT_WRITE, .offset = cached_output_gtt },
        { .handle = ctx->ssh_bo, .flags = 0, .offset = 0 },
        { .handle = ctx->dsh_bo, .flags = 0, .offset = 0 },
        { .handle = ctx->ioh_bo, .flags = 0, .offset = 0 },
        { .handle = ctx->batch_bo, .flags = 0, .offset = 0,
          .relocation_count = 4, .relocs_ptr = (uintptr_t)relocs_batch },
        { .handle = 0 }  /* Padding */
    };
    
    /* C367 FIX: Écrire adresse GTT dans SSH AVANT EXECBUFFER2 (comme runner production) */
    if (ctx->ssh_map) {
        struct gen9_surface_state* surfaces = (struct gen9_surface_state*)ctx->ssh_map;
        
        /* Configurer dw1 avec adresse GTT présumée */
        surfaces[0].dw1 = (uint32_t)((cached_output_gtt >> 12) << 12);  /* Page-aligned */
        
        /* Flush cache pour que GPU voit les valeurs */
        __builtin___clear_cache((char*)ctx->ssh_map, (char*)ctx->ssh_map + 128);
        __asm__ volatile("mfence" ::: "memory");
        
        LOG_EVENT(ctx, "C367_SSH_PRE_EXEC: dw1=0x%08x (GTT=0x%016lx)", surfaces[0].dw1, cached_output_gtt);
    }
    
    LOG_EVENT(ctx, "EXEC_OBJECTS:");
    for (int i = 0; i < 6; i++) {
        LOG_EVENT(ctx, "  [%d] handle=%u flags=0x%llx", i, exec_objects[i].handle, exec_objects[i].flags);
    }
    
    /* Execbuffer2 */
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 6,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx->ctx_id
    };
    
    LOG_EVENT(ctx, "EXECBUFFER2: batch_len=%u ctx_id=%u", batch_len, ctx->ctx_id);
    
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret < 0) {
        LOG_EVENT(ctx, "ERROR: EXECBUFFER2 failed: %s", strerror(errno));
        return -1;
    }
    
    LOG_EVENT(ctx, "EXECBUFFER2_SUCCESS");
    
    /* Attendre fin exécution */
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx->batch_bo,
        .timeout_ns = 10000000000ULL,  /* 10s timeout */
        .flags = 0
    };
    
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    if (ret < 0) {
        LOG_EVENT(ctx, "ERROR: WAIT failed: %s", strerror(errno));
        return -1;
    }
    
    uint64_t end_ts = get_timestamp_ns();
    double exec_time_ms = (end_ts - start_ts) / 1000000.0;
    
    LOG_EVENT(ctx, "GPU_EXECUTION_TIME: %.3f ms", exec_time_ms);
    LOG_EVENT(ctx, "=== EXECUTE GPU SUCCESS ===");
    
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 9: VALIDATION RÉSULTATS
 * ═══════════════════════════════════════════════════════════════════ */

static int test_validate_output(test_context_t* ctx, const char* test_name, 
                                uint32_t expected_pattern, bool check_gid) {
    LOG_EVENT(ctx, "=== VALIDATE OUTPUT START: %s ===", test_name);
    
    /* Synchroniser CPU */
    struct drm_i915_gem_set_domain set_domain = {
        .handle = ctx->output_bo,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    
    uint32_t* output = (uint32_t*)ctx->output_map;
    
    /* Checksum output */
    uint32_t output_crc = crc32_compute(ctx->output_map, ctx->output_size);
    LOG_EVENT(ctx, "OUTPUT_CRC32: 0x%08x", output_crc);
    
    /* Hex dump complet */
    hex_dump(ctx->log_file, "OUTPUT_HEXDUMP_FULL", ctx->output_map, ctx->output_size);
    
    /* Analyser résultats */
    int success_count = 0;
    int zero_count = 0;
    int wrong_count = 0;
    
    for (int i = 0; i < 64; i++) {
        uint32_t expected = check_gid ? (expected_pattern + i) : expected_pattern;
        uint32_t actual = output[i];
        
        if (actual == expected) {
            success_count++;
            LOG_EVENT(ctx, "  [%02d] ✅ CORRECT: 0x%08x", i, actual);
        } else if (actual == 0x00000000) {
            zero_count++;
            LOG_EVENT(ctx, "  [%02d] ❌ ZERO: expected=0x%08x actual=0x%08x", i, expected, actual);
        } else {
            wrong_count++;
            LOG_EVENT(ctx, "  [%02d] ❌ WRONG: expected=0x%08x actual=0x%08x", i, expected, actual);
        }
    }
    
    LOG_EVENT(ctx, "VALIDATION_SUMMARY:");
    LOG_EVENT(ctx, "  SUCCESS: %d/64 (%.1f%%)", success_count, success_count * 100.0 / 64);
    LOG_EVENT(ctx, "  ZERO: %d/64 (%.1f%%)", zero_count, zero_count * 100.0 / 64);
    LOG_EVENT(ctx, "  WRONG: %d/64 (%.1f%%)", wrong_count, wrong_count * 100.0 / 64);
    
    /* Diagnostic */
    if (success_count == 64) {
        LOG_EVENT(ctx, "DIAGNOSTIC: ✅ COMPUTE PIPELINE VALIDÉ");
    } else if (zero_count == 64) {
        LOG_EVENT(ctx, "DIAGNOSTIC: ❌ KERNEL NON EXÉCUTÉ (ISA problem)");
    } else if (wrong_count > 0 && zero_count == 0) {
        LOG_EVENT(ctx, "DIAGNOSTIC: ❌ PAYLOAD CASSÉ (GID incorrect)");
    } else {
        LOG_EVENT(ctx, "DIAGNOSTIC: ❌ THREAD DISPATCH CASSÉ (pattern incohérent)");
    }
    
    LOG_EVENT(ctx, "=== VALIDATE OUTPUT END ===");
    
    return (success_count == 64) ? 0 : -1;
}

/* ═══════════════════════════════════════════════════════════════════
 * SECTION 10: MAIN - TEST T11
 * ═══════════════════════════════════════════════════════════════════ */

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <kernel.bin> <log_file>\n", argv[0]);
        return 1;
    }
    
    const char* kernel_path = argv[1];
    const char* log_path = argv[2];
    
    test_context_t ctx = {0};
    
    /* Ouvrir log */
    ctx.log_file = fopen(log_path, "w");
    if (!ctx.log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", strerror(errno));
        return 1;
    }
    
    LOG_EVENT(&ctx, "╔═══════════════════════════════════════════════════════════════╗");
    LOG_EVENT(&ctx, "║  TEST C366 - FORENSIC RUNNER ULTRA-GRANULAIRE                ║");
    LOG_EVENT(&ctx, "║  Instrumentation: CRC32 + Timestamps + Hex Dumps             ║");
    LOG_EVENT(&ctx, "╚═══════════════════════════════════════════════════════════════╝");
    
    /* Initialiser DRM */
    if (test_init_drm(&ctx) < 0) {
        fclose(ctx.log_file);
        return 1;
    }
    
    /* Créer output buffer */
    ctx.output_size = 256;  /* 64 uint32 */
    if (test_create_buffer(&ctx, &ctx.output_bo, &ctx.output_map, ctx.output_size, "OUTPUT") < 0) {
        fclose(ctx.log_file);
        return 1;
    }
    
    /* Initialiser output avec pattern 0xDEADBEEF */
    memset(ctx.output_map, 0xEF, ctx.output_size);
    LOG_EVENT(&ctx, "OUTPUT_INITIALIZED: pattern=0xDEADBEEF");
    
    /* Charger kernel */
    if (test_load_kernel(&ctx, kernel_path) < 0) {
        fclose(ctx.log_file);
        return 1;
    }
    
    /* Setup heaps */
    if (test_setup_ssh(&ctx) < 0 ||
        test_setup_dsh(&ctx) < 0 ||
        test_setup_ioh(&ctx) < 0) {
        fclose(ctx.log_file);
        return 1;
    }
    
    /* Build batch */
    int batch_len = test_build_batch(&ctx, 64);
    if (batch_len < 0) {
        fclose(ctx.log_file);
        return 1;
    }
    
    /* Exécuter GPU */
    if (test_execute_gpu(&ctx, batch_len) < 0) {
        fclose(ctx.log_file);
        return 1;
    }
    
    /* Valider résultats */
    int result = test_validate_output(&ctx, "T11_COMPUTE_MINIMAL", 0xCAFE0000, true);
    
    LOG_EVENT(&ctx, "╔═══════════════════════════════════════════════════════════════╗");
    LOG_EVENT(&ctx, "║  TEST C366 COMPLETE - Total events: %llu                     ║", ctx.event_counter);
    LOG_EVENT(&ctx, "╚═══════════════════════════════════════════════════════════════╝");
    
    fclose(ctx.log_file);
    
    return result;
}

// Made with Bob
