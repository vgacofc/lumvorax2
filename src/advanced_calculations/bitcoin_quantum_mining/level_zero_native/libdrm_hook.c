/*
 * LD_PRELOAD Hook for DRM ioctl Interception
 * Captures batch buffers from EXECBUFFER2 calls
 * 
 * Compilation: gcc -shared -fPIC -o libdrm_hook.so libdrm_hook.c -ldl
 * Usage: LD_PRELOAD=./libdrm_hook.so ./opencl_reference
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

// i915 DRM definitions
#define DRM_IOCTL_BASE 'd'
#define DRM_I915_GEM_EXECBUFFER2 0x29
#define DRM_IOWR(nr,type) _IOWR(DRM_IOCTL_BASE,nr,type)

struct drm_i915_gem_exec_object2 {
    uint32_t handle;
    uint32_t relocation_count;
    uint64_t relocs_ptr;
    uint64_t alignment;
    uint64_t offset;
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
};

struct drm_i915_gem_execbuffer2 {
    uint64_t buffers_ptr;
    uint32_t buffer_count;
    uint32_t batch_start_offset;
    uint32_t batch_len;
    uint32_t DR1;
    uint32_t DR4;
    uint32_t num_cliprects;
    uint64_t cliprects_ptr;
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
};

#define DRM_IOCTL_I915_GEM_EXECBUFFER2 DRM_IOWR(DRM_I915_GEM_EXECBUFFER2, struct drm_i915_gem_execbuffer2)

// GEM MMAP offset
#define DRM_I915_GEM_MMAP_OFFSET 0x24
#define DRM_IOCTL_I915_GEM_MMAP_OFFSET DRM_IOWR(DRM_I915_GEM_MMAP_OFFSET, struct drm_i915_gem_mmap_offset)

struct drm_i915_gem_mmap_offset {
    uint32_t handle;
    uint32_t pad;
    uint64_t offset;
    uint64_t flags;
    uint64_t extensions;
};

// Global state
static int (*real_ioctl)(int fd, unsigned long request, void *argp) = NULL;
static int execbuffer_count = 0;
static FILE *log_file = NULL;

// Initialize hook
static void __attribute__((constructor)) init_hook(void) {
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    if (!real_ioctl) {
        fprintf(stderr, "libdrm_hook: Failed to find real ioctl\n");
        exit(1);
    }
    
    log_file = fopen("/tmp/drm_hook.log", "w");
    if (!log_file) {
        fprintf(stderr, "libdrm_hook: Failed to open log file\n");
        exit(1);
    }
    
    fprintf(log_file, "=== DRM Hook Initialized ===\n");
    fflush(log_file);
}

// Cleanup hook
static void __attribute__((destructor)) cleanup_hook(void) {
    if (log_file) {
        fprintf(log_file, "=== DRM Hook Cleanup (captured %d EXECBUFFER2) ===\n", execbuffer_count);
        fclose(log_file);
    }
}

// Dump buffer to file
static void dump_buffer(int fd, uint32_t handle, uint32_t size, const char *name) {
    // Try to mmap the buffer
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = handle,
        .flags = 0,
    };
    
    if (real_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) != 0) {
        fprintf(log_file, "  Failed to get mmap offset for handle %u\n", handle);
        return;
    }
    
    // Map the buffer
    void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, mmap_arg.offset);
    if (ptr == MAP_FAILED) {
        fprintf(log_file, "  Failed to mmap buffer handle %u\n", handle);
        return;
    }
    
    // Dump to file
    char filename[256];
    snprintf(filename, sizeof(filename), "/tmp/%s_handle_%u.bin", name, handle);
    FILE *f = fopen(filename, "wb");
    if (f) {
        fwrite(ptr, 1, size, f);
        fclose(f);
        fprintf(log_file, "  Dumped %u bytes to %s\n", size, filename);
        
        // Also dump hex preview
        fprintf(log_file, "  First 256 bytes (hex):\n");
        for (uint32_t i = 0; i < 256 && i < size; i += 16) {
            fprintf(log_file, "    %04x: ", i);
            for (uint32_t j = 0; j < 16 && (i + j) < size; j++) {
                fprintf(log_file, "%02x ", ((uint8_t*)ptr)[i + j]);
            }
            fprintf(log_file, "\n");
        }
    }
    
    munmap(ptr, size);
}

// Intercept ioctl
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void*);
    va_end(args);
    
    // Log all DRM ioctl
    if ((request & 0xFF00) == (DRM_IOCTL_BASE << 8)) {
        fprintf(log_file, "DRM ioctl: fd=%d request=0x%lx\n", fd, request);
        fflush(log_file);
    }
    
    // Call real ioctl first
    int ret = real_ioctl(fd, request, argp);
    
    // Check if this is EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2 && ret == 0) {
        struct drm_i915_gem_execbuffer2 *exec = (struct drm_i915_gem_execbuffer2 *)argp;
        execbuffer_count++;
        
        fprintf(log_file, "\n=== EXECBUFFER2 #%d ===\n", execbuffer_count);
        fprintf(log_file, "buffer_count: %u\n", exec->buffer_count);
        fprintf(log_file, "batch_start_offset: 0x%x\n", exec->batch_start_offset);
        fprintf(log_file, "batch_len: %u bytes\n", exec->batch_len);
        fprintf(log_file, "flags: 0x%lx\n", exec->flags);
        
        // Get buffer objects
        struct drm_i915_gem_exec_object2 *objects = 
            (struct drm_i915_gem_exec_object2 *)exec->buffers_ptr;
        
        fprintf(log_file, "\nBuffer Objects:\n");
        for (uint32_t i = 0; i < exec->buffer_count; i++) {
            fprintf(log_file, "  [%u] handle=%u offset=0x%lx flags=0x%lx\n",
                   i, objects[i].handle, objects[i].offset, objects[i].flags);
            
            // Last buffer is typically the batch buffer
            if (i == exec->buffer_count - 1) {
                fprintf(log_file, "  ^ This is the BATCH BUFFER\n");
                dump_buffer(fd, objects[i].handle, exec->batch_len, "batch");
            }
        }
        
        fflush(log_file);
    }
    
    return ret;
}

// Made with Bob
