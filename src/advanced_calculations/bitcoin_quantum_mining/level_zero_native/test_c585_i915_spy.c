// Test C585: i915 SPY - Capture batch buffer OpenCL en temps réel
// Utilise LD_PRELOAD pour intercepter ioctl() et capturer EXECBUFFER2
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <i915_drm.h>
#include <xf86drm.h>
#include <time.h>

// Pointeur vers le vrai ioctl
static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;

// Compteur d'appels
static int execbuffer_count = 0;
static FILE* log_file = NULL;

// Initialisation
__attribute__((constructor))
static void init_spy(void) {
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    if (!real_ioctl) {
        fprintf(stderr, "❌ Failed to find real ioctl\n");
        exit(1);
    }
    
    char filename[256];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(filename, sizeof(filename), "logs/i915_spy_%Y%m%d_%H%M%S.log", tm_info);
    
    log_file = fopen(filename, "w");
    if (!log_file) {
        fprintf(stderr, "❌ Failed to open log file: %s\n", filename);
        exit(1);
    }
    
    fprintf(log_file, "=== i915 SPY - BATCH CAPTURE LOG ===\n\n");
    fprintf(stderr, "✅ i915 SPY initialized - logging to %s\n", filename);
}

// Cleanup
__attribute__((destructor))
static void cleanup_spy(void) {
    if (log_file) {
        fprintf(log_file, "\n=== TOTAL EXECBUFFER2 CALLS: %d ===\n", execbuffer_count);
        fclose(log_file);
        fprintf(stderr, "✅ i915 SPY: Captured %d EXECBUFFER2 calls\n", execbuffer_count);
    }
}

// Dump batch buffer content
static void dump_batch_buffer(const char* label, const void* data, size_t size) {
    fprintf(log_file, "\n--- %s (size=%zu bytes) ---\n", label, size);
    
    const uint32_t* dwords = (const uint32_t*)data;
    size_t num_dwords = size / 4;
    
    for (size_t i = 0; i < num_dwords; i++) {
        uint32_t dword = dwords[i];
        
        // Decode command
        uint32_t opcode = (dword >> 29) & 0x7;
        uint32_t subopcode = (dword >> 27) & 0x3;
        uint32_t cmd_type = (dword >> 23) & 0xF;
        
        fprintf(log_file, "[%04zx] 0x%08X  ", i * 4, dword);
        
        // Identify common commands
        if (opcode == 0x7) {
            // 3D/Media commands
            if ((dword & 0xFFFF0000) == 0x79000000) {
                fprintf(log_file, "← MEDIA_VFE_STATE");
            } else if ((dword & 0xFFFF0000) == 0x79020000) {
                fprintf(log_file, "← MEDIA_CURBE_LOAD");
            } else if ((dword & 0xFFFF0000) == 0x79040000) {
                fprintf(log_file, "← MEDIA_INTERFACE_DESCRIPTOR_LOAD");
            } else if ((dword & 0xFFFF0000) == 0x79050000) {
                fprintf(log_file, "← MEDIA_STATE_FLUSH");
            } else if ((dword & 0xFFFF0000) == 0x79070000) {
                fprintf(log_file, "← GPGPU_WALKER");
            } else if ((dword & 0xFFFF0000) == 0x61010000) {
                fprintf(log_file, "← STATE_BASE_ADDRESS");
            } else if ((dword & 0xFFFF0000) == 0x78000000) {
                fprintf(log_file, "← PIPELINE_SELECT");
            } else if ((dword & 0xFFFF0000) == 0x78010000) {
                fprintf(log_file, "← MEDIA_PIPELINE_SELECT");
            } else {
                fprintf(log_file, "← 3D/MEDIA cmd");
            }
        } else if (opcode == 0x0) {
            // MI commands
            if ((dword & 0xFFE00000) == 0x00000000) {
                fprintf(log_file, "← MI_NOOP");
            } else if ((dword & 0xFFE00000) == 0x0A000000) {
                fprintf(log_file, "← MI_BATCH_BUFFER_END");
            } else if ((dword & 0xFFE00000) == 0x24000000) {
                fprintf(log_file, "← MI_STORE_DATA_IMM");
            } else if ((dword & 0xFFE00000) == 0x7A000000) {
                fprintf(log_file, "← PIPE_CONTROL");
            } else {
                fprintf(log_file, "← MI cmd");
            }
        }
        
        fprintf(log_file, "\n");
        
        // Stop at BATCH_BUFFER_END
        if ((dword & 0xFFE00000) == 0x0A000000) {
            fprintf(log_file, "--- END OF BATCH ---\n");
            break;
        }
    }
}

// Intercepted ioctl
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void* argp = va_arg(args, void*);
    va_end(args);
    
    // Check if this is DRM_IOCTL_I915_GEM_EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2 || 
        request == DRM_IOCTL_I915_GEM_EXECBUFFER2_WR) {
        
        execbuffer_count++;
        
        struct drm_i915_gem_execbuffer2* execbuf = (struct drm_i915_gem_execbuffer2*)argp;
        
        fprintf(log_file, "\n========================================\n");
        fprintf(log_file, "EXECBUFFER2 CALL #%d\n", execbuffer_count);
        fprintf(log_file, "========================================\n");
        fprintf(log_file, "buffers_ptr: 0x%llx\n", execbuf->buffers_ptr);
        fprintf(log_file, "buffer_count: %u\n", execbuf->buffer_count);
        fprintf(log_file, "batch_start_offset: 0x%x\n", execbuf->batch_start_offset);
        fprintf(log_file, "batch_len: %u bytes\n", execbuf->batch_len);
        fprintf(log_file, "flags: 0x%llx\n", execbuf->flags);
        fprintf(log_file, "rsvd1 (context): %llu\n", execbuf->rsvd1);
        
        // Dump exec_objects
        struct drm_i915_gem_exec_object2* objects = 
            (struct drm_i915_gem_exec_object2*)execbuf->buffers_ptr;
        
        fprintf(log_file, "\n--- EXEC OBJECTS (%u) ---\n", execbuf->buffer_count);
        for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
            fprintf(log_file, "Object[%u]:\n", i);
            fprintf(log_file, "  handle: %u\n", objects[i].handle);
            fprintf(log_file, "  offset: 0x%llx\n", objects[i].offset);
            // Note: size field may not exist in all kernel versions
            fprintf(log_file, "  alignment: %llu\n", objects[i].alignment);
            fprintf(log_file, "  flags: 0x%llx\n", objects[i].flags);
            
            if (objects[i].relocation_count > 0) {
                fprintf(log_file, "  relocations: %u\n", objects[i].relocation_count);
                
                struct drm_i915_gem_relocation_entry* relocs = 
                    (struct drm_i915_gem_relocation_entry*)objects[i].relocs_ptr;
                
                for (uint64_t j = 0; j < objects[i].relocation_count; j++) {
                    fprintf(log_file, "    Reloc[%lu]: offset=0x%llx, target_handle=%u, delta=0x%x\n",
                        (unsigned long)j, (unsigned long long)relocs[j].offset,
                        relocs[j].target_handle, relocs[j].delta);
                }
            }
        }
        
        // Try to dump batch buffer content
        // Note: We can't directly access GPU memory, but we can try to read from handle
        fprintf(log_file, "\n--- ATTEMPTING BATCH BUFFER DUMP ---\n");
        fprintf(log_file, "Note: Batch buffer is in GPU memory, cannot directly access\n");
        fprintf(log_file, "Batch start offset: 0x%x\n", execbuf->batch_start_offset);
        fprintf(log_file, "Batch length: %u bytes\n", execbuf->batch_len);
    }
    
    // Call real ioctl
    int result = real_ioctl(fd, request, argp);
    
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2 || 
        request == DRM_IOCTL_I915_GEM_EXECBUFFER2_WR) {
        fprintf(log_file, "\nResult: %d (errno=%d)\n", result, result < 0 ? errno : 0);
        fflush(log_file);
    }
    
    return result;
}
