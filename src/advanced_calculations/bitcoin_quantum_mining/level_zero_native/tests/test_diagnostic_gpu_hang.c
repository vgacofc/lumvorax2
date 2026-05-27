/**
 * TEST DIAGNOSTIC GPU HANG - Phase 15J
 *
 * Objectif: Isoler la cause du GPU hang après 3 dispatches
 *
 * Tests:
 * 1. Kernel simple (test_add) vs SHA256
 * 2. Batch buffer avec PIPE_CONTROL entre dispatches
 * 3. Capture intel_gpu_top temps réel
 * 4. Vérification dmesg pour GPU hang
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <i915_drm.h>

#define TEST_DISPATCHES 10
#define BATCH_SIZE 262144
#define WORK_GROUP_SIZE 256

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t batch_bo, kernel_bo, input_bo, output_bo;
    void* batch_map;
    void* kernel_map;
    void* input_map;
    void* output_map;
    size_t batch_size;
    size_t kernel_size;
    size_t input_size;
    size_t output_size;
    FILE* log_file;
    uint64_t total_dispatches;
    bool enable_pipe_control;
} diagnostic_context_t;

// Timestamp nanoseconde
static inline uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Log forensique
static void log_event(diagnostic_context_t* ctx, const char* event, const char* details) {
    uint64_t ts = get_timestamp_ns();
    fprintf(ctx->log_file, "[%llu.%09llu] %s: %s\n",
            (unsigned long long)(ts / 1000000000ULL),
            (unsigned long long)(ts % 1000000000ULL), event, details);
    fflush(ctx->log_file);
}

// Création GEM buffer
static int create_gem_buffer(int drm_fd, size_t size, uint32_t* handle_out, void** map_out) {
    struct drm_i915_gem_create create = {
        .size = size
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) != 0) {
        return -errno;
    }
    
    *handle_out = create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = create.handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        return -errno;
    }
    
    *map_out = (void*)(uintptr_t)mmap_arg.addr_ptr;
    return 0;
}

// Construction batch buffer Gen9 avec option PIPE_CONTROL
static void build_batch_buffer(diagnostic_context_t* ctx, uint32_t kernel_offset) {
    uint32_t* batch = (uint32_t*)ctx->batch_map;
    int idx = 0;
    
    // STATE_BASE_ADDRESS
    batch[idx++] = 0x61010010;  // Opcode
    batch[idx++] = 0x00000000;  // General State Base Address
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  // Surface State Base Address
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  // Dynamic State Base Address
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  // Indirect Object Base Address
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  // Instruction Base Address
    batch[idx++] = 0x00000000;
    batch[idx++] = 0xFFFFF001;  // General State Buffer Size
    batch[idx++] = 0xFFFFF001;  // Dynamic State Buffer Size
    batch[idx++] = 0xFFFFF001;  // Indirect Object Buffer Size
    batch[idx++] = 0xFFFFF001;  // Instruction Buffer Size
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MEDIA_VFE_STATE
    batch[idx++] = 0x70000007;  // Opcode
    batch[idx++] = 0x00000000;  // Scratch Space Base Pointer
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  // Max Threads
    batch[idx++] = 0x00000000;  // Number URB Entries
    batch[idx++] = 0x00000000;  // URB Entry Allocation Size
    batch[idx++] = 0x00000000;  // CURBE Allocation Size
    batch[idx++] = 0x00000000;  // Scoreboard Mask
    batch[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = 0x70020002;  // Opcode
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000020;  // Interface Descriptor Total Length
    batch[idx++] = kernel_offset;  // Interface Descriptor Data Start Address
    
    // GPGPU_WALKER
    uint32_t num_groups_x = (BATCH_SIZE + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
    batch[idx++] = 0x71050009;  // Opcode
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset
    batch[idx++] = 0x00000000;  // Indirect Data Length
    batch[idx++] = 0x00000000;  // Indirect Data Start Address
    batch[idx++] = 0x00000000;  // Thread Group ID Starting X
    batch[idx++] = 0x00000000;  // Thread Group ID X Dimension
    batch[idx++] = 0x00000000;  // Thread Group ID Starting Y
    batch[idx++] = 0x00000000;  // Thread Group ID Y Dimension
    batch[idx++] = 0x00000000;  // Thread Group ID Starting Z
    batch[idx++] = 0x00000000;  // Thread Group ID Z Dimension
    batch[idx++] = num_groups_x;  // Thread Group ID X Dimension
    batch[idx++] = 0x00000001;  // Thread Group ID Y Dimension
    batch[idx++] = 0x00000001;  // Thread Group ID Z Dimension
    
    // PIPE_CONTROL (si activé)
    if (ctx->enable_pipe_control) {
        batch[idx++] = 0x7A000004;  // PIPE_CONTROL opcode
        batch[idx++] = 0x00100000;  // CS Stall + Flush
        batch[idx++] = 0x00000000;  // Address Low
        batch[idx++] = 0x00000000;  // Address High
        batch[idx++] = 0x00000000;  // Immediate Data Low
        batch[idx++] = 0x00000000;  // Immediate Data High
    }
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    
    char details[256];
    snprintf(details, sizeof(details), "commands=%d pipe_control=%s", 
             idx, ctx->enable_pipe_control ? "enabled" : "disabled");
    log_event(ctx, "BATCH_BUFFER_BUILT", details);
}

// Initialisation
static int diagnostic_init(diagnostic_context_t** ctx_out, const char* kernel_path, 
                          bool enable_pipe_control, const char* log_path) {
    uint64_t start_ts = get_timestamp_ns();
    
    diagnostic_context_t* ctx = calloc(1, sizeof(diagnostic_context_t));
    if (!ctx) return -ENOMEM;
    
    ctx->enable_pipe_control = enable_pipe_control;
    ctx->log_file = fopen(log_path, "w");
    if (!ctx->log_file) {
        free(ctx);
        return -errno;
    }
    
    char details[256];
    snprintf(details, sizeof(details), "kernel=%s pipe_control=%s", 
             kernel_path, enable_pipe_control ? "enabled" : "disabled");
    log_event(ctx, "INIT_START", details);
    
    // Ouvrir DRM
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        log_event(ctx, "DRM_OPEN_FAILED", strerror(errno));
        fclose(ctx->log_file);
        free(ctx);
        return -errno;
    }
    
    snprintf(details, sizeof(details), "fd=%d", ctx->drm_fd);
    log_event(ctx, "DRM_OPEN_SUCCESS", details);
    
    // Créer contexte
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) != 0) {
        log_event(ctx, "CONTEXT_CREATE_FAILED", strerror(errno));
        close(ctx->drm_fd);
        fclose(ctx->log_file);
        free(ctx);
        return -errno;
    }
    ctx->ctx_id = ctx_create.ctx_id;
    
    snprintf(details, sizeof(details), "ctx_id=%u", ctx->ctx_id);
    log_event(ctx, "CONTEXT_CREATED", details);
    
    // Créer buffers
    ctx->batch_size = 4096;
    ctx->kernel_size = 65536;
    ctx->input_size = BATCH_SIZE * sizeof(uint32_t);
    ctx->output_size = 8 * 1024 * 1024;  // 8 MB
    
    if (create_gem_buffer(ctx->drm_fd, ctx->batch_size, &ctx->batch_bo, &ctx->batch_map) != 0 ||
        create_gem_buffer(ctx->drm_fd, ctx->kernel_size, &ctx->kernel_bo, &ctx->kernel_map) != 0 ||
        create_gem_buffer(ctx->drm_fd, ctx->input_size, &ctx->input_bo, &ctx->input_map) != 0 ||
        create_gem_buffer(ctx->drm_fd, ctx->output_size, &ctx->output_bo, &ctx->output_map) != 0) {
        log_event(ctx, "BUFFER_CREATE_FAILED", strerror(errno));
        close(ctx->drm_fd);
        fclose(ctx->log_file);
        free(ctx);
        return -errno;
    }
    
    log_event(ctx, "BUFFERS_CREATED", "batch+kernel+input+output");
    
    // Charger kernel
    FILE* kf = fopen(kernel_path, "rb");
    if (!kf) {
        log_event(ctx, "KERNEL_LOAD_FAILED", strerror(errno));
        close(ctx->drm_fd);
        fclose(ctx->log_file);
        free(ctx);
        return -errno;
    }
    
    fseek(kf, 0, SEEK_END);
    size_t kernel_file_size = ftell(kf);
    fseek(kf, 0, SEEK_SET);
    size_t bytes_read = fread(ctx->kernel_map, 1, kernel_file_size, kf);
    if (bytes_read != kernel_file_size) {
        log_event(ctx, "KERNEL_READ_INCOMPLETE", "");
        fclose(kf);
        close(ctx->drm_fd);
        fclose(ctx->log_file);
        free(ctx);
        return -EIO;
    }
    fclose(kf);
    
    snprintf(details, sizeof(details), "size=%zu bytes", kernel_file_size);
    log_event(ctx, "KERNEL_LOADED", details);
    
    // Construire batch buffer
    build_batch_buffer(ctx, 0);
    
    uint64_t end_ts = get_timestamp_ns();
    double init_time = (end_ts - start_ts) / 1e9;
    snprintf(details, sizeof(details), "time=%.6f sec", init_time);
    log_event(ctx, "INIT_COMPLETE", details);
    
    *ctx_out = ctx;
    return 0;
}

// Exécution dispatch
static int diagnostic_execute(diagnostic_context_t* ctx) {
    uint64_t start_ts = get_timestamp_ns();
    
    char details[256];
    snprintf(details, sizeof(details), "dispatch=%lu ctx_id=%u batch_bo=%u", 
             ctx->total_dispatches + 1, ctx->ctx_id, ctx->batch_bo);
    log_event(ctx, "EXEC_START", details);
    
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    struct drm_i915_gem_exec_object2 exec_objects[4] = {0};
    
    exec_objects[0].handle = ctx->kernel_bo;
    exec_objects[1].handle = ctx->input_bo;
    exec_objects[2].handle = ctx->output_bo;
    exec_objects[3].handle = ctx->batch_bo;
    
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 4;
    execbuf.batch_len = ctx->batch_size;
    execbuf.rsvd1 = ctx->ctx_id;
    execbuf.flags = I915_EXEC_RENDER;
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        uint64_t end_ts = get_timestamp_ns();
        double exec_time = (end_ts - start_ts) / 1e9;
        snprintf(details, sizeof(details), "errno=%d (%s) time=%.6f sec", 
                 errno, strerror(errno), exec_time);
        log_event(ctx, "EXEC_FAILED", details);
        return -errno;
    }
    
    // Attendre fin GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx->batch_bo,
        .timeout_ns = 10000000000ULL  // 10 secondes
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) != 0) {
        uint64_t end_ts = get_timestamp_ns();
        double exec_time = (end_ts - start_ts) / 1e9;
        snprintf(details, sizeof(details), "errno=%d (%s) time=%.6f sec", 
                 errno, strerror(errno), exec_time);
        log_event(ctx, "WAIT_FAILED", details);
        return -errno;
    }
    
    uint64_t end_ts = get_timestamp_ns();
    double exec_time = (end_ts - start_ts) / 1e9;
    double hashrate = (BATCH_SIZE / exec_time) / 1e6;
    
    snprintf(details, sizeof(details), "time=%.6f sec hashrate=%.2f MH/s", 
             exec_time, hashrate);
    log_event(ctx, "EXEC_SUCCESS", details);
    
    ctx->total_dispatches++;
    return 0;
}

// Cleanup
static void diagnostic_cleanup(diagnostic_context_t* ctx) {
    if (!ctx) return;
    
    char details[256];
    snprintf(details, sizeof(details), "dispatches=%lu", ctx->total_dispatches);
    log_event(ctx, "CLEANUP_START", details);
    
    if (ctx->drm_fd >= 0) {
        close(ctx->drm_fd);
    }
    
    if (ctx->log_file) {
        log_event(ctx, "CLEANUP_COMPLETE", "");
        fclose(ctx->log_file);
    }
    
    free(ctx);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <kernel_path> <pipe_control:0|1> [num_dispatches]\n", argv[0]);
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  %s kernels/test_add_gen9.bin 0 10\n", argv[0]);
        fprintf(stderr, "  %s kernels/btc_sha256_mining_gen9.bin 1 10\n", argv[0]);
        return 1;
    }
    
    const char* kernel_path = argv[1];
    bool enable_pipe_control = (atoi(argv[2]) != 0);
    int num_dispatches = (argc >= 4) ? atoi(argv[3]) : TEST_DISPATCHES;
    
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "logs/forensic/diagnostic_%s_pipe%d.log",
             strrchr(kernel_path, '/') ? strrchr(kernel_path, '/') + 1 : kernel_path,
             enable_pipe_control ? 1 : 0);
    
    printf("=== TEST DIAGNOSTIC GPU HANG - Phase 15J ===\n");
    printf("Kernel: %s\n", kernel_path);
    printf("PIPE_CONTROL: %s\n", enable_pipe_control ? "ENABLED" : "DISABLED");
    printf("Dispatches: %d\n", num_dispatches);
    printf("Log: %s\n\n", log_path);
    
    diagnostic_context_t* ctx = NULL;
    int ret = diagnostic_init(&ctx, kernel_path, enable_pipe_control, log_path);
    if (ret != 0) {
        fprintf(stderr, "ERREUR: Init failed (ret=%d)\n", ret);
        return 1;
    }
    
    printf("Init OK - Starting dispatches...\n");
    
    int success_count = 0;
    int fail_count = 0;
    
    for (int i = 0; i < num_dispatches; i++) {
        printf("Dispatch %d/%d... ", i + 1, num_dispatches);
        fflush(stdout);
        
        ret = diagnostic_execute(ctx);
        if (ret == 0) {
            printf("OK\n");
            success_count++;
        } else {
            printf("FAILED (errno=%d)\n", -ret);
            fail_count++;
            
            if (fail_count >= 3) {
                printf("\n⚠️  3 échecs consécutifs - Arrêt test\n");
                break;
            }
        }
    }
    
    printf("\n=== RÉSULTATS ===\n");
    printf("Succès: %d/%d (%.1f%%)\n", success_count, num_dispatches, 
           100.0 * success_count / num_dispatches);
    printf("Échecs: %d/%d (%.1f%%)\n", fail_count, num_dispatches,
           100.0 * fail_count / num_dispatches);
    
    if (success_count == num_dispatches) {
        printf("✅ TEST RÉUSSI - Aucun GPU hang détecté\n");
    } else if (success_count >= 3 && fail_count > 0) {
        printf("⚠️  GPU HANG DÉTECTÉ après %d dispatches\n", success_count);
    } else {
        printf("❌ ÉCHEC IMMÉDIAT - Problème critique\n");
    }
    
    diagnostic_cleanup(ctx);
    
    printf("\nLog forensique: %s\n", log_path);
    
    return (success_count == num_dispatches) ? 0 : 1;
}

// Made with Bob
