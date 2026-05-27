// TEST F: Capture /sys/class/drm/card0/error au crash
// Objectif: Obtenir état kernel i915 exact au moment du crash errno=5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <drm/i915_drm.h>

#define CONTEXT_POOL_SIZE 9
#define BATCH_POOL_SIZE 90

typedef struct {
    int fd;
    uint32_t ctx_ids[CONTEXT_POOL_SIZE];
    uint32_t batch_bos[BATCH_POOL_SIZE];
    uint32_t kernel_bo;
    uint32_t input_bo;
    uint32_t output_bo;
} test_f_context_t;

static void capture_error_state(const char* filename) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
        "cat /sys/class/drm/card0/error > %s 2>/dev/null || echo 'error state not available' > %s",
        filename, filename);
    system(cmd);
}

static void capture_dmesg(const char* filename) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "dmesg | tail -200 > %s", filename);
    system(cmd);
}

static void capture_journal(const char* filename) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
        "journalctl -k --since '1 minute ago' > %s 2>/dev/null || echo 'journal not available' > %s",
        filename, filename);
    system(cmd);
}

static void capture_debugfs(const char* filename) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "cat /sys/kernel/debug/dri/0/i915_context_info > %s 2>/dev/null || echo 'debugfs not available' > %s",
        filename, filename);
    system(cmd);
}

int main() {
    printf("[TEST_F] Capture Error State Test\n");
    printf("[TEST_F] Objectif: Capturer état kernel i915 au crash\n\n");
    
    // Créer répertoire logs
    system("mkdir -p logs/forensic/test_f");
    
    test_f_context_t ctx = {0};
    
    // Ouvrir DRM
    ctx.fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.fd < 0) {
        printf("[TEST_F_ERROR] Cannot open DRM: %s\n", strerror(errno));
        return 1;
    }
    printf("[TEST_F] DRM opened: fd=%d\n", ctx.fd);
    
    // Créer 9 contextes
    for (int i = 0; i < CONTEXT_POOL_SIZE; i++) {
        struct drm_i915_gem_context_create ctx_create = {0};
        if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
            printf("[TEST_F_ERROR] Context %d create failed: %s\n", i, strerror(errno));
            return 1;
        }
        ctx.ctx_ids[i] = ctx_create.ctx_id;
        printf("[TEST_F] Context %d created: ctx_id=%u\n", i, ctx.ctx_ids[i]);
    }
    
    // Allouer kernel BO (dummy 4KB)
    struct drm_i915_gem_create gem_create = {.size = 4096};
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        printf("[TEST_F_ERROR] Kernel BO create failed\n");
        return 1;
    }
    ctx.kernel_bo = gem_create.handle;
    
    // Allouer input/output BOs
    gem_create.size = 1024 * 1024;
    ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    ctx.input_bo = gem_create.handle;
    
    gem_create.size = 1024 * 1024;
    ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    ctx.output_bo = gem_create.handle;
    
    // Allouer batch BOs
    for (int i = 0; i < BATCH_POOL_SIZE; i++) {
        gem_create.size = 4096;
        if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
            printf("[TEST_F_ERROR] Batch BO %d create failed\n", i);
            return 1;
        }
        ctx.batch_bos[i] = gem_create.handle;
    }
    
    printf("[TEST_F] All resources allocated\n");
    printf("[TEST_F] Starting dispatches...\n\n");
    
    // Capturer état AVANT dispatches
    capture_error_state("logs/forensic/test_f/error_state_before.txt");
    capture_dmesg("logs/forensic/test_f/dmesg_before.txt");
    
    // Dispatches 1-30
    for (int i = 0; i < 30; i++) {
        int ctx_idx = i % CONTEXT_POOL_SIZE;
        int batch_idx = i % BATCH_POOL_SIZE;
        
        // Construire batch buffer minimal
        struct drm_i915_gem_execbuffer2 execbuf = {0};
        struct drm_i915_gem_exec_object2 exec_objs[4] = {0};
        
        exec_objs[0].handle = ctx.kernel_bo;
        exec_objs[1].handle = ctx.input_bo;
        exec_objs[2].handle = ctx.output_bo;
        exec_objs[3].handle = ctx.batch_bos[batch_idx];
        
        execbuf.buffers_ptr = (uintptr_t)exec_objs;
        execbuf.buffer_count = 4;
        execbuf.batch_len = 32;
        execbuf.rsvd1 = ctx.ctx_ids[ctx_idx];
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int ret = ioctl(ctx.fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (ret < 0) {
            printf("[TEST_F_FAILED] Dispatch %d ctx_id=%u errno=%d (%s) time=%.6f\n",
                   i + 1, ctx.ctx_ids[ctx_idx], errno, strerror(errno), elapsed);
            
            // CAPTURE IMMÉDIATE ÉTAT KERNEL
            printf("[TEST_F_CAPTURE] Capturing kernel state...\n");
            capture_error_state("logs/forensic/test_f/error_state_crash.txt");
            capture_dmesg("logs/forensic/test_f/dmesg_crash.txt");
            capture_journal("logs/forensic/test_f/journal_crash.txt");
            capture_debugfs("logs/forensic/test_f/debugfs_crash.txt");
            
            // Attendre 1s pour laisser kernel logger
            sleep(1);
            
            // Capture finale
            capture_dmesg("logs/forensic/test_f/dmesg_final.txt");
            
            printf("[TEST_F_CAPTURE] All kernel states captured\n");
            printf("[TEST_F] Test completed with crash at dispatch %d\n", i + 1);
            
            // Cleanup
            close(ctx.fd);
            return 0;
        }
        
        printf("[TEST_F_SUCCESS] Dispatch %d ctx_id=%u ctx_idx=%d batch_idx=%d time=%.6f\n",
               i + 1, ctx.ctx_ids[ctx_idx], ctx_idx, batch_idx, elapsed);
    }
    
    printf("[TEST_F] All 30 dispatches succeeded (unexpected!)\n");
    capture_error_state("logs/forensic/test_f/error_state_success.txt");
    capture_dmesg("logs/forensic/test_f/dmesg_success.txt");
    
    close(ctx.fd);
    return 0;
}
