// TEST A-BIS: gem_wait() synchrone + capture bit-level driver
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <drm/i915_drm.h>

#define BATCH_SIZE 286331153

int main() {
    printf("[TEST_A_BIS] gem_wait() Synchronous + Bit-Level Driver Trace\n\n");
    
    system("mkdir -p logs/forensic/test_a_bis");
    
    // Capture état initial
    system("echo emmaus | sudo -S dmesg -c > /dev/null 2>&1");
    system("echo emmaus | sudo -S cat /sys/class/drm/card0/error > logs/forensic/test_a_bis/error_state_initial.txt 2>/dev/null");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("[ERROR] Cannot open DRM: %s\n", strerror(errno));
        return 1;
    }
    
    // Charger kernel SHA-256
    int kernel_fd = open("kernels/btc_sha256_gen9.bin", O_RDONLY);
    if (kernel_fd < 0) {
        printf("[ERROR] Cannot open kernel: %s\n", strerror(errno));
        return 1;
    }
    
    struct stat st;
    fstat(kernel_fd, &st);
    size_t kernel_size = st.st_size;
    
    struct drm_i915_gem_create gem_create = {.size = kernel_size};
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t kernel_bo = gem_create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {.handle = kernel_bo, .size = kernel_size};
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg);
    void* kernel_ptr = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    read(kernel_fd, kernel_ptr, kernel_size);
    close(kernel_fd);
    munmap(kernel_ptr, kernel_size);
    
    printf("[TEST_A_BIS] Kernel loaded: %zu bytes\n", kernel_size);
    
    // Allouer BOs
    gem_create.size = (size_t)BATCH_SIZE * 4;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t input_bo = gem_create.handle;
    
    gem_create.size = 1024 * 1024;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t output_bo = gem_create.handle;
    
    // Créer contexte UNIQUE
    struct drm_i915_gem_context_create ctx_create = {0};
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    uint32_t ctx_id = ctx_create.ctx_id;
    
    printf("[TEST_A_BIS] Context: ctx_id=%u\n\n", ctx_id);
    
    FILE *log = fopen("logs/forensic/test_a_bis/execution_detailed.log", "w");
    
    // Dispatches 1-50 avec gem_wait() synchrone
    for (int i = 0; i < 50; i++) {
        fprintf(log, "\n=== DISPATCH %d ===\n", i + 1);
        
        // Créer batch BO
        gem_create.size = 4096;
        ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
        uint32_t batch_bo = gem_create.handle;
        
        fprintf(log, "batch_bo=%u\n", batch_bo);
        
        // Construire execbuffer
        struct drm_i915_gem_execbuffer2 execbuf = {0};
        struct drm_i915_gem_exec_object2 exec_objs[4] = {0};
        
        exec_objs[0].handle = kernel_bo;
        exec_objs[1].handle = input_bo;
        exec_objs[2].handle = output_bo;
        exec_objs[3].handle = batch_bo;
        
        execbuf.buffers_ptr = (uintptr_t)exec_objs;
        execbuf.buffer_count = 4;
        execbuf.batch_len = 288;
        execbuf.rsvd1 = ctx_id;
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // DISPATCH
        int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        fprintf(log, "execbuffer2: ret=%d errno=%d\n", ret, errno);
        
        if (ret < 0) {
            printf("[DISPATCH_FAILED] Dispatch %d errno=%d (%s)\n", i + 1, errno, strerror(errno));
            fprintf(log, "FAILED: %s\n", strerror(errno));
            
            // Capture état crash
            char cmd[256];
            sprintf(cmd, "echo emmaus | sudo -S dmesg | tail -100 > logs/forensic/test_a_bis/dmesg_dispatch_%d_failed.txt 2>/dev/null", i + 1);
            system(cmd);
            sprintf(cmd, "echo emmaus | sudo -S cat /sys/class/drm/card0/error > logs/forensic/test_a_bis/error_state_dispatch_%d_failed.txt 2>/dev/null", i + 1);
            system(cmd);
            
            fclose(log);
            close(fd);
            return 0;
        }
        
        // GEM_WAIT SYNCHRONE (10s timeout)
        struct drm_i915_gem_wait wait = {
            .bo_handle = batch_bo,
            .timeout_ns = 10000000000LL
        };
        
        int ret_wait = ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        fprintf(log, "gem_wait: ret=%d errno=%d time=%.6f\n", ret_wait, errno, elapsed);
        
        if (ret_wait < 0) {
            printf("[WAIT_FAILED] Dispatch %d gem_wait errno=%d (%s) time=%.6f\n",
                   i + 1, errno, strerror(errno), elapsed);
            fprintf(log, "WAIT_FAILED: %s\n", strerror(errno));
            
            // Capture état wait failed
            char cmd[256];
            sprintf(cmd, "echo emmaus | sudo -S dmesg | tail -100 > logs/forensic/test_a_bis/dmesg_dispatch_%d_wait_failed.txt 2>/dev/null", i + 1);
            system(cmd);
            sprintf(cmd, "echo emmaus | sudo -S cat /sys/class/drm/card0/error > logs/forensic/test_a_bis/error_state_dispatch_%d_wait_failed.txt 2>/dev/null", i + 1);
            system(cmd);
            
            struct drm_gem_close gem_close = {.handle = batch_bo};
            ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
            
            fclose(log);
            close(fd);
            return 0;
        }
        
        printf("[SUCCESS] Dispatch %d execbuf=0 gem_wait=0 time=%.6f\n", i + 1, elapsed);
        fprintf(log, "SUCCESS time=%.6f\n", elapsed);
        
        // Capturer error state tous les 5 dispatches
        if ((i + 1) % 5 == 0) {
            char cmd[256];
            sprintf(cmd, "echo emmaus | sudo -S cat /sys/class/drm/card0/error > logs/forensic/test_a_bis/error_state_dispatch_%d.txt 2>/dev/null", i + 1);
            system(cmd);
        }
        
        // Cleanup batch BO
        struct drm_gem_close gem_close = {.handle = batch_bo};
        ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    }
    
    printf("\n[TEST_A_BIS] All 50 dispatches completed!\n");
    fprintf(log, "\n=== TEST COMPLETED ===\n");
    fclose(log);
    
    system("echo emmaus | sudo -S dmesg | tail -200 > logs/forensic/test_a_bis/dmesg_final.txt 2>/dev/null");
    system("echo emmaus | sudo -S cat /sys/class/drm/card0/error > logs/forensic/test_a_bis/error_state_final.txt 2>/dev/null");
    
    close(fd);
    return 0;
}
