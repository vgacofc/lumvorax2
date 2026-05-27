/*
 * C197.22 - INTERCEPT OPENCL IOCTL
 * 
 * Objectif: Capturer les vrais ioctl() i915 d'OpenCL pour comprendre
 * ce qui manque dans notre pipeline DRM natif
 * 
 * Usage: LD_PRELOAD=./libintercept.so ./opencl_program
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <drm/drm.h>
#include <drm/i915_drm.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

// Original ioctl function pointer
static int (*original_ioctl)(int fd, unsigned long request, ...) = NULL;

// Log file
static FILE* log_file = NULL;
static int intercept_enabled = 0;

// Counter
static uint64_t ioctl_count = 0;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static void init_intercept(void) {
    if (log_file) return;
    
    log_file = fopen("/tmp/opencl_ioctl_intercept.log", "w");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file\n");
        return;
    }
    
    fprintf(log_file, "=== OpenCL ioctl() Intercept Log ===\n");
    fprintf(log_file, "Timestamp: %ld\n\n", time(NULL));
    fflush(log_file);
    
    intercept_enabled = 1;
}

static const char* ioctl_name(unsigned long request) {
    switch (request) {
        case DRM_IOCTL_VERSION: return "DRM_IOCTL_VERSION";
        case DRM_IOCTL_GET_UNIQUE: return "DRM_IOCTL_GET_UNIQUE";
        case DRM_IOCTL_GET_MAGIC: return "DRM_IOCTL_GET_MAGIC";
        case DRM_IOCTL_IRQ_BUSID: return "DRM_IOCTL_IRQ_BUSID";
        case DRM_IOCTL_GET_MAP: return "DRM_IOCTL_GET_MAP";
        case DRM_IOCTL_GET_CLIENT: return "DRM_IOCTL_GET_CLIENT";
        case DRM_IOCTL_GET_STATS: return "DRM_IOCTL_GET_STATS";
        case DRM_IOCTL_SET_VERSION: return "DRM_IOCTL_SET_VERSION";
        case DRM_IOCTL_MODESET_CTL: return "DRM_IOCTL_MODESET_CTL";
        case DRM_IOCTL_GEM_CLOSE: return "DRM_IOCTL_GEM_CLOSE";
        case DRM_IOCTL_GEM_FLINK: return "DRM_IOCTL_GEM_FLINK";
        case DRM_IOCTL_GEM_OPEN: return "DRM_IOCTL_GEM_OPEN";
        case DRM_IOCTL_GET_CAP: return "DRM_IOCTL_GET_CAP";
        case DRM_IOCTL_SET_CLIENT_CAP: return "DRM_IOCTL_SET_CLIENT_CAP";
        
        // i915 specific
        case DRM_IOCTL_I915_INIT: return "DRM_IOCTL_I915_INIT";
        case DRM_IOCTL_I915_FLUSH: return "DRM_IOCTL_I915_FLUSH";
        case DRM_IOCTL_I915_FLIP: return "DRM_IOCTL_I915_FLIP";
        case DRM_IOCTL_I915_BATCHBUFFER: return "DRM_IOCTL_I915_BATCHBUFFER";
        case DRM_IOCTL_I915_IRQ_EMIT: return "DRM_IOCTL_I915_IRQ_EMIT";
        case DRM_IOCTL_I915_IRQ_WAIT: return "DRM_IOCTL_I915_IRQ_WAIT";
        case DRM_IOCTL_I915_GETPARAM: return "DRM_IOCTL_I915_GETPARAM";
        case DRM_IOCTL_I915_SETPARAM: return "DRM_IOCTL_I915_SETPARAM";
        case DRM_IOCTL_I915_ALLOC: return "DRM_IOCTL_I915_ALLOC";
        case DRM_IOCTL_I915_FREE: return "DRM_IOCTL_I915_FREE";
        case DRM_IOCTL_I915_INIT_HEAP: return "DRM_IOCTL_I915_INIT_HEAP";
        case DRM_IOCTL_I915_CMDBUFFER: return "DRM_IOCTL_I915_CMDBUFFER";
        case DRM_IOCTL_I915_DESTROY_HEAP: return "DRM_IOCTL_I915_DESTROY_HEAP";
        case DRM_IOCTL_I915_SET_VBLANK_PIPE: return "DRM_IOCTL_I915_SET_VBLANK_PIPE";
        case DRM_IOCTL_I915_GET_VBLANK_PIPE: return "DRM_IOCTL_I915_GET_VBLANK_PIPE";
        case DRM_IOCTL_I915_VBLANK_SWAP: return "DRM_IOCTL_I915_VBLANK_SWAP";
        case DRM_IOCTL_I915_HWS_ADDR: return "DRM_IOCTL_I915_HWS_ADDR";
        case DRM_IOCTL_I915_GEM_INIT: return "DRM_IOCTL_I915_GEM_INIT";
        case DRM_IOCTL_I915_GEM_EXECBUFFER: return "DRM_IOCTL_I915_GEM_EXECBUFFER";
        case DRM_IOCTL_I915_GEM_EXECBUFFER2: return "DRM_IOCTL_I915_GEM_EXECBUFFER2";
        case DRM_IOCTL_I915_GEM_PIN: return "DRM_IOCTL_I915_GEM_PIN";
        case DRM_IOCTL_I915_GEM_UNPIN: return "DRM_IOCTL_I915_GEM_UNPIN";
        case DRM_IOCTL_I915_GEM_BUSY: return "DRM_IOCTL_I915_GEM_BUSY";
        case DRM_IOCTL_I915_GEM_SET_CACHING: return "DRM_IOCTL_I915_GEM_SET_CACHING";
        case DRM_IOCTL_I915_GEM_GET_CACHING: return "DRM_IOCTL_I915_GEM_GET_CACHING";
        case DRM_IOCTL_I915_GEM_THROTTLE: return "DRM_IOCTL_I915_GEM_THROTTLE";
        case DRM_IOCTL_I915_GEM_ENTERVT: return "DRM_IOCTL_I915_GEM_ENTERVT";
        case DRM_IOCTL_I915_GEM_LEAVEVT: return "DRM_IOCTL_I915_GEM_LEAVEVT";
        case DRM_IOCTL_I915_GEM_CREATE: return "DRM_IOCTL_I915_GEM_CREATE";
        case DRM_IOCTL_I915_GEM_PREAD: return "DRM_IOCTL_I915_GEM_PREAD";
        case DRM_IOCTL_I915_GEM_PWRITE: return "DRM_IOCTL_I915_GEM_PWRITE";
        case DRM_IOCTL_I915_GEM_MMAP: return "DRM_IOCTL_I915_GEM_MMAP";
        case DRM_IOCTL_I915_GEM_MMAP_GTT: return "DRM_IOCTL_I915_GEM_MMAP_GTT";
        case DRM_IOCTL_I915_GEM_SET_DOMAIN: return "DRM_IOCTL_I915_GEM_SET_DOMAIN";
        case DRM_IOCTL_I915_GEM_SW_FINISH: return "DRM_IOCTL_I915_GEM_SW_FINISH";
        case DRM_IOCTL_I915_GEM_SET_TILING: return "DRM_IOCTL_I915_GEM_SET_TILING";
        case DRM_IOCTL_I915_GEM_GET_TILING: return "DRM_IOCTL_I915_GEM_GET_TILING";
        case DRM_IOCTL_I915_GEM_GET_APERTURE: return "DRM_IOCTL_I915_GEM_GET_APERTURE";
        case DRM_IOCTL_I915_GET_PIPE_FROM_CRTC_ID: return "DRM_IOCTL_I915_GET_PIPE_FROM_CRTC_ID";
        case DRM_IOCTL_I915_GEM_MADVISE: return "DRM_IOCTL_I915_GEM_MADVISE";
        case DRM_IOCTL_I915_OVERLAY_PUT_IMAGE: return "DRM_IOCTL_I915_OVERLAY_PUT_IMAGE";
        case DRM_IOCTL_I915_OVERLAY_ATTRS: return "DRM_IOCTL_I915_OVERLAY_ATTRS";
        case DRM_IOCTL_I915_SET_SPRITE_COLORKEY: return "DRM_IOCTL_I915_SET_SPRITE_COLORKEY";
        case DRM_IOCTL_I915_GET_SPRITE_COLORKEY: return "DRM_IOCTL_I915_GET_SPRITE_COLORKEY";
        case DRM_IOCTL_I915_GEM_WAIT: return "DRM_IOCTL_I915_GEM_WAIT";
        case DRM_IOCTL_I915_GEM_CONTEXT_CREATE: return "DRM_IOCTL_I915_GEM_CONTEXT_CREATE";
        case DRM_IOCTL_I915_GEM_CONTEXT_DESTROY: return "DRM_IOCTL_I915_GEM_CONTEXT_DESTROY";
        case DRM_IOCTL_I915_REG_READ: return "DRM_IOCTL_I915_REG_READ";
        case DRM_IOCTL_I915_GET_RESET_STATS: return "DRM_IOCTL_I915_GET_RESET_STATS";
        case DRM_IOCTL_I915_GEM_USERPTR: return "DRM_IOCTL_I915_GEM_USERPTR";
        case DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM: return "DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM";
        case DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM: return "DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM";
        case DRM_IOCTL_I915_PERF_OPEN: return "DRM_IOCTL_I915_PERF_OPEN";
        case DRM_IOCTL_I915_PERF_ADD_CONFIG: return "DRM_IOCTL_I915_PERF_ADD_CONFIG";
        case DRM_IOCTL_I915_PERF_REMOVE_CONFIG: return "DRM_IOCTL_I915_PERF_REMOVE_CONFIG";
        case DRM_IOCTL_I915_QUERY: return "DRM_IOCTL_I915_QUERY";
        case DRM_IOCTL_I915_GEM_VM_CREATE: return "DRM_IOCTL_I915_GEM_VM_CREATE";
        case DRM_IOCTL_I915_GEM_VM_DESTROY: return "DRM_IOCTL_I915_GEM_VM_DESTROY";
        
        default: return "UNKNOWN";
    }
}

// ============================================================================
// IOCTL INTERCEPTOR
// ============================================================================

int ioctl(int fd, unsigned long request, ...) {
    // Initialize on first call
    if (!original_ioctl) {
        original_ioctl = dlsym(RTLD_NEXT, "ioctl");
        if (!original_ioctl) {
            fprintf(stderr, "Failed to find original ioctl\n");
            abort();
        }
        init_intercept();
    }
    
    // Get argument
    va_list args;
    va_start(args, request);
    void* arg = va_arg(args, void*);
    va_end(args);
    
    // Check if this is a DRM ioctl
    int is_drm = (request >= DRM_IOCTL_BASE && request < DRM_IOCTL_BASE + 0x100);
    
    if (is_drm && intercept_enabled) {
        ioctl_count++;
        
        fprintf(log_file, "[%06lu] ioctl(fd=%d, request=0x%08lx, arg=%p)\n",
                ioctl_count, fd, request, arg);
        fprintf(log_file, "        Name: %s\n", ioctl_name(request));
        
        // Special handling for EXECBUFFER2
        if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2 && arg) {
            struct drm_i915_gem_execbuffer2* execbuf = (struct drm_i915_gem_execbuffer2*)arg;
            fprintf(log_file, "        EXECBUFFER2:\n");
            fprintf(log_file, "          buffers_ptr: 0x%016lx\n", execbuf->buffers_ptr);
            fprintf(log_file, "          buffer_count: %u\n", execbuf->buffer_count);
            fprintf(log_file, "          batch_start_offset: %u\n", execbuf->batch_start_offset);
            fprintf(log_file, "          batch_len: %u\n", execbuf->batch_len);
            fprintf(log_file, "          flags: 0x%016lx\n", execbuf->flags);
            fprintf(log_file, "          rsvd1 (context_id): %lu\n", execbuf->rsvd1);
            
            // Dump exec_objects
            if (execbuf->buffers_ptr && execbuf->buffer_count > 0) {
                struct drm_i915_gem_exec_object2* objects = 
                    (struct drm_i915_gem_exec_object2*)execbuf->buffers_ptr;
                
                fprintf(log_file, "          exec_objects:\n");
                for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
                    fprintf(log_file, "            [%u] handle=%u, offset=0x%016lx, flags=0x%016lx\n",
                            i, objects[i].handle, objects[i].offset, objects[i].flags);
                }
            }
        }
        
        fflush(log_file);
    }
    
    // Call original ioctl
    int result = original_ioctl(fd, request, arg);
    
    if (is_drm && intercept_enabled) {
        fprintf(log_file, "        Result: %d\n\n", result);
        fflush(log_file);
    }
    
    return result;
}

// ============================================================================
// CONSTRUCTOR/DESTRUCTOR
// ============================================================================

__attribute__((constructor))
static void intercept_init(void) {
    fprintf(stderr, "=== OpenCL ioctl() Interceptor Loaded ===\n");
    fprintf(stderr, "Log file: /tmp/opencl_ioctl_intercept.log\n");
}

__attribute__((destructor))
static void intercept_fini(void) {
    if (log_file) {
        fprintf(log_file, "\n=== Total ioctl() calls: %lu ===\n", ioctl_count);
        fclose(log_file);
    }
    fprintf(stderr, "=== OpenCL ioctl() Interceptor Unloaded ===\n");
    fprintf(stderr, "Total ioctl() calls: %lu\n", ioctl_count);
}

// Made with Bob
