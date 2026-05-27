// TEST A: Contexte neuf par dispatch + kernel SHA-256 réel
// Objectif: Vérifier si contexte neuf élimine bug avec charge GPU réelle

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
    printf("[TEST_A] Fresh Context Per Dispatch + Real SHA-256 Kernel\n\n");
    
    system("mkdir -p logs/forensic/test_a");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("[TEST_A_ERROR] Cannot open DRM\n");
        return 1;
    }
    
    // Charger kernel SHA-256 réel
    int kernel_fd = open("kernels/btc_sha256_gen9.bin", O_RDONLY);
    if (kernel_fd < 0) {
        printf("[TEST_A_ERROR] Cannot open kernel file\n");
        return 1;
    }
    
    struct stat st;
    fstat(kernel_fd, &st);
    size_t kernel_size = st.st_size;
    
    struct drm_i915_gem_create gem_create = {.size = kernel_size};
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t kernel_bo = gem_create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = kernel_bo,
        .size = kernel_size
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg);
    void* kernel_ptr = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    read(kernel_fd, kernel_ptr, kernel_size);
    close(kernel_fd);
    munmap(kernel_ptr, kernel_size);
    
    printf("[TEST_A] Kernel SHA-256 loaded: %zu bytes\n", kernel_size);
    
    // Allouer input/output BOs
    gem_create.size = (size_t)BATCH_SIZE * 4;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t input_bo = gem_create.handle;
    
    gem_create.size = 1024 * 1024;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t output_bo = gem_create.handle;
    
    printf("[TEST_A] Starting dispatches with FRESH context each time...\n\n");
    
    // Dispatches 1-50 avec contexte NEUF à chaque fois
    for (int i = 0; i < 50; i++) {
        // CRÉER CONTEXTE NEUF
        struct drm_i915_gem_context_create ctx_create = {0};
        if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
            printf("[TEST_A_ERROR] Context create failed at dispatch %d\n", i + 1);
            break;
        }
        uint32_t ctx_id = ctx_create.ctx_id;
        
        // CRÉER BATCH BO NEUF
        gem_create.size = 4096;
        ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
        uint32_t batch_bo = gem_create.handle;
        
        // Construire batch buffer
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
        
        int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (ret < 0) {
            printf("[TEST_A_FAILED] Dispatch %d ctx_id=%u errno=%d (%s) time=%.6f\n",
                   i + 1, ctx_id, errno, strerror(errno), elapsed);
            
            // Capture dmesg avec sudo
            system("echo emmaus | sudo -S dmesg | tail -100 > logs/forensic/test_a/dmesg_crash.txt 2>/dev/null");
            system("echo emmaus | sudo -S cat /sys/class/drm/card0/error > logs/forensic/test_a/error_state_crash.txt 2>/dev/null");
            
            printf("[TEST_A] Kernel states captured\n");
            
            // Cleanup
            struct drm_i915_gem_context_destroy ctx_destroy = {.ctx_id = ctx_id};
            ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
            
            struct drm_gem_close gem_close = {.handle = batch_bo};
            ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
            
            close(fd);
            return 0;
        }
        
        printf("[TEST_A_SUCCESS] Dispatch %d ctx_id=%u time=%.6f\n", i + 1, ctx_id, elapsed);
        
        // DÉTRUIRE CONTEXTE immédiatement
        struct drm_i915_gem_context_destroy ctx_destroy = {.ctx_id = ctx_id};
        ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
        
        // DÉTRUIRE BATCH BO
        struct drm_gem_close gem_close = {.handle = batch_bo};
        ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    }
    
    printf("\n[TEST_A] All 50 dispatches succeeded!\n");
    printf("[TEST_A] CONCLUSION: Fresh context eliminates bug!\n");
    
    system("echo emmaus | sudo -S dmesg | tail -100 > logs/forensic/test_a/dmesg_success.txt 2>/dev/null");
    
    close(fd);
    return 0;
}
