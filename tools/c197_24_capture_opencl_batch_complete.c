/*
 * C197.24 - Capture Complète Batch Buffer OpenCL
 * 
 * OBJECTIF: Capturer le batch buffer EXACT qu'OpenCL génère
 * MÉTHODE: Hook mmap() + EXECBUFFER2 pour dumper batch avant soumission
 * RÉSULTAT: Batch buffer binaire + désassemblage complet
 * 
 * STRATÉGIE:
 * 1. Intercepter mmap() pour identifier batch buffer
 * 2. Intercepter EXECBUFFER2 pour capturer avant soumission
 * 3. Dumper batch buffer complet (hex + désassemblage)
 * 4. Reproduire exactement dans notre code DRM
 * 
 * COMPILATION:
 *   gcc -shared -fPIC -o libcapture_batch.so c197_24_capture_opencl_batch_complete.c -ldl
 * 
 * USAGE:
 *   LD_PRELOAD=./libcapture_batch.so ./test_opencl_write_magic
 *   cat /tmp/opencl_batch_dump.bin | xxd
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <drm/drm.h>
#include <drm/i915_drm.h>

// Original functions
static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;
static void* (*real_mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = NULL;

// Tracking
#define MAX_MMAPS 1000
static struct {
    void *addr;
    size_t length;
    int fd;
    off_t offset;
    int is_batch;
} g_mmaps[MAX_MMAPS];
static int g_mmap_count = 0;

static int g_initialized = 0;
static FILE *g_log = NULL;
static FILE *g_batch_dump = NULL;

// ============================================================================
// INITIALIZATION
// ============================================================================

static void init_capture(void) {
    if (g_initialized) return;
    
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    real_mmap = dlsym(RTLD_NEXT, "mmap");
    
    g_log = fopen("/tmp/opencl_batch_capture.log", "w");
    g_batch_dump = fopen("/tmp/opencl_batch_dump.bin", "wb");
    
    if (g_log) {
        fprintf(g_log, "=== OpenCL Batch Buffer Capture ===\n");
        fprintf(g_log, "PID: %d\n\n", getpid());
        fflush(g_log);
    }
    
    g_initialized = 1;
    fprintf(stderr, "[Capture] Initialized (PID %d)\n", getpid());
}

// ============================================================================
// BATCH BUFFER ANALYSIS
// ============================================================================

static void analyze_batch_buffer(const uint32_t *batch, size_t length_dwords) {
    if (!g_log) return;
    
    fprintf(g_log, "\n=== BATCH BUFFER ANALYSIS ===\n");
    fprintf(g_log, "Length: %zu DWORDs (%zu bytes)\n\n", length_dwords, length_dwords * 4);
    
    for (size_t i = 0; i < length_dwords; i++) {
        uint32_t dword = batch[i];
        uint32_t opcode = (dword >> 23) & 0x1FF;  // Bits 23-31
        
        fprintf(g_log, "[%04zu] 0x%08X  ", i, dword);
        
        // Decode common Gen9 commands
        if ((dword & 0xFFFF0000) == 0x7A000000) {
            fprintf(g_log, "PIPE_CONTROL\n");
        } else if ((dword & 0xFFFF0000) == 0x61010000) {
            fprintf(g_log, "STATE_BASE_ADDRESS\n");
        } else if ((dword & 0xFFFF0000) == 0x70000000) {
            fprintf(g_log, "MEDIA_VFE_STATE\n");
        } else if ((dword & 0xFFFF0000) == 0x70020000) {
            fprintf(g_log, "MEDIA_INTERFACE_DESCRIPTOR_LOAD\n");
        } else if ((dword & 0xFFFF0000) == 0x75020000) {
            fprintf(g_log, "GPGPU_WALKER\n");
        } else if (dword == 0x05000000) {
            fprintf(g_log, "MI_BATCH_BUFFER_END\n");
        } else if (dword == 0x00000000) {
            fprintf(g_log, "MI_NOOP\n");
        } else {
            fprintf(g_log, "opcode=0x%03X\n", opcode);
        }
    }
    
    fprintf(g_log, "\n");
    fflush(g_log);
}

// ============================================================================
// MMAP INTERCEPTION
// ============================================================================

void* mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (!g_initialized) init_capture();
    
    void *result = real_mmap(addr, length, prot, flags, fd, offset);
    
    if (result != MAP_FAILED && g_mmap_count < MAX_MMAPS) {
        g_mmaps[g_mmap_count].addr = result;
        g_mmaps[g_mmap_count].length = length;
        g_mmaps[g_mmap_count].fd = fd;
        g_mmaps[g_mmap_count].offset = offset;
        g_mmaps[g_mmap_count].is_batch = 0;  // Will be determined later
        g_mmap_count++;
        
        if (g_log) {
            fprintf(g_log, "[mmap] addr=%p, length=%zu, fd=%d, offset=%ld\n",
                    result, length, fd, offset);
            fflush(g_log);
        }
    }
    
    return result;
}

// ============================================================================
// IOCTL INTERCEPTION (EXECBUFFER2)
// ============================================================================

int ioctl(int fd, unsigned long request, ...) {
    if (!g_initialized) init_capture();
    
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void*);
    va_end(args);
    
    // Intercept EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *execbuf = (struct drm_i915_gem_execbuffer2 *)argp;
        
        if (g_log) {
            fprintf(g_log, "\n=== EXECBUFFER2 INTERCEPTED ===\n");
            fprintf(g_log, "buffer_count: %u\n", execbuf->buffer_count);
            fprintf(g_log, "batch_start_offset: %u\n", execbuf->batch_start_offset);
            fprintf(g_log, "batch_len: %u\n", execbuf->batch_len);
            fprintf(g_log, "flags: 0x%llx\n", execbuf->flags);
            fprintf(g_log, "rsvd1 (ctx_id): %llu\n", execbuf->rsvd1);
            fflush(g_log);
        }
        
        // Get batch buffer from exec_objects
        struct drm_i915_gem_exec_object2 *exec_objects = 
            (struct drm_i915_gem_exec_object2 *)execbuf->buffers_ptr;
        
        if (exec_objects && execbuf->buffer_count > 0) {
            // Last buffer is usually the batch buffer
            struct drm_i915_gem_exec_object2 *batch_obj = 
                &exec_objects[execbuf->buffer_count - 1];
            
            if (g_log) {
                fprintf(g_log, "\nBatch buffer object:\n");
                fprintf(g_log, "  handle: %u\n", batch_obj->handle);
                fprintf(g_log, "  offset: 0x%llx\n", batch_obj->offset);
                fprintf(g_log, "  flags: 0x%llx\n", batch_obj->flags);
                fflush(g_log);
            }
            
            // Find corresponding mmap
            for (int i = 0; i < g_mmap_count; i++) {
                // Heuristic: batch buffer is usually small (< 64KB) and writable
                if (g_mmaps[i].length <= 65536 && g_mmaps[i].addr) {
                    uint32_t *batch = (uint32_t *)g_mmaps[i].addr;
                    size_t batch_dwords = execbuf->batch_len / 4;
                    
                    if (g_log) {
                        fprintf(g_log, "\nCandidate batch buffer at %p:\n", batch);
                    }
                    
                    // Check if it looks like a batch buffer (starts with valid command)
                    if (batch_dwords > 0 && (batch[0] & 0xE0000000) != 0) {
                        if (g_log) {
                            fprintf(g_log, "✅ Valid batch buffer found!\n");
                        }
                        
                        // Dump to binary file
                        if (g_batch_dump) {
                            fwrite(batch, 1, execbuf->batch_len, g_batch_dump);
                            fflush(g_batch_dump);
                            fprintf(g_log, "✅ Dumped %u bytes to /tmp/opencl_batch_dump.bin\n", 
                                    execbuf->batch_len);
                        }
                        
                        // Analyze
                        analyze_batch_buffer(batch, batch_dwords);
                        
                        g_mmaps[i].is_batch = 1;
                        break;
                    }
                }
            }
        }
    }
    
    // Call real ioctl
    int result = real_ioctl(fd, request, argp);
    
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2 && g_log) {
        fprintf(g_log, "EXECBUFFER2 result: %d\n", result);
        if (result < 0) {
            fprintf(g_log, "ERROR: %s\n", strerror(errno));
        }
        fflush(g_log);
    }
    
    return result;
}

// ============================================================================
// CLEANUP
// ============================================================================

__attribute__((destructor))
static void cleanup_capture(void) {
    if (g_log) {
        fprintf(g_log, "\n=== CAPTURE COMPLETE ===\n");
        fprintf(g_log, "Total mmaps tracked: %d\n", g_mmap_count);
        
        int batch_count = 0;
        for (int i = 0; i < g_mmap_count; i++) {
            if (g_mmaps[i].is_batch) batch_count++;
        }
        fprintf(g_log, "Batch buffers identified: %d\n", batch_count);
        
        fclose(g_log);
    }
    
    if (g_batch_dump) {
        fclose(g_batch_dump);
    }
    
    fprintf(stderr, "[Capture] Complete - check /tmp/opencl_batch_capture.log\n");
    fprintf(stderr, "[Capture] Batch buffer dumped to /tmp/opencl_batch_dump.bin\n");
}

// Made with Bob
