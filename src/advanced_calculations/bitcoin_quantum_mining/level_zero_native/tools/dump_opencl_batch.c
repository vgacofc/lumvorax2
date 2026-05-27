/*
 * DUMP OPENCL BATCH - Intercepteur ioctl pour capturer batch buffers
 * ==================================================================
 * 
 * Compile: gcc -shared -fPIC -o dump_opencl_batch.so dump_opencl_batch.c -ldl
 * Usage: LD_PRELOAD=./dump_opencl_batch.so ./test_c603_opencl_batch_capture
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

// Définitions i915 DRM
#define DRM_IOCTL_BASE 'd'
#define DRM_IOCTL_I915_GEM_EXECBUFFER2 DRM_IOWR(DRM_COMMAND_BASE + 0x29, struct drm_i915_gem_execbuffer2)
#define DRM_COMMAND_BASE 0x40
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

static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;
static int execbuffer_count = 0;
static FILE *dump_file = NULL;

void __attribute__((constructor)) init_interceptor() {
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    dump_file = fopen("logs/opencl_batch_dump_c603.txt", "w");
    if (dump_file) {
        fprintf(dump_file, "=== OPENCL BATCH BUFFER DUMP C603 ===\n\n");
        fflush(dump_file);
    }
}

void __attribute__((destructor)) cleanup_interceptor() {
    if (dump_file) {
        fclose(dump_file);
    }
}

void dump_batch_buffer(void *batch_ptr, uint32_t batch_len) {
    if (!dump_file || !batch_ptr) return;
    
    uint32_t *batch = (uint32_t*)batch_ptr;
    uint32_t dwords = batch_len / 4;
    
    fprintf(dump_file, "\n[BATCH BUFFER] %u bytes (%u DWords)\n", batch_len, dwords);
    fprintf(dump_file, "================================================\n");
    
    for (uint32_t i = 0; i < dwords; i += 4) {
        fprintf(dump_file, "[%4u] ", i);
        for (uint32_t j = 0; j < 4 && (i + j) < dwords; j++) {
            fprintf(dump_file, "0x%08x ", batch[i + j]);
        }
        fprintf(dump_file, "\n");
    }
    
    fprintf(dump_file, "================================================\n\n");
    fflush(dump_file);
}

void dump_exec_objects(struct drm_i915_gem_exec_object2 *objects, uint32_t count) {
    if (!dump_file || !objects) return;
    
    fprintf(dump_file, "[EXEC OBJECTS] %u buffers\n", count);
    fprintf(dump_file, "================================================\n");
    
    for (uint32_t i = 0; i < count; i++) {
        fprintf(dump_file, "[%2u] handle=%u offset=0x%016lx flags=0x%016lx\n",
                i, objects[i].handle, objects[i].offset, objects[i].flags);
    }
    
    fprintf(dump_file, "================================================\n\n");
    fflush(dump_file);
}

int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void*);
    va_end(args);
    
    // Vérifier si c'est EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *exec = (struct drm_i915_gem_execbuffer2*)argp;
        
        if (dump_file && exec) {
            execbuffer_count++;
            
            fprintf(dump_file, "\n╔════════════════════════════════════════════════════════════╗\n");
            fprintf(dump_file, "║  EXECBUFFER2 #%d                                            ║\n", execbuffer_count);
            fprintf(dump_file, "╚════════════════════════════════════════════════════════════╝\n\n");
            
            fprintf(dump_file, "[EXECBUFFER2 STRUCTURE]\n");
            fprintf(dump_file, "  buffer_count:        %u\n", exec->buffer_count);
            fprintf(dump_file, "  batch_start_offset:  %u\n", exec->batch_start_offset);
            fprintf(dump_file, "  batch_len:           %u bytes\n", exec->batch_len);
            fprintf(dump_file, "  flags:               0x%016lx\n", exec->flags);
            fprintf(dump_file, "\n");
            
            // Dump exec objects
            if (exec->buffers_ptr) {
                struct drm_i915_gem_exec_object2 *objects = 
                    (struct drm_i915_gem_exec_object2*)exec->buffers_ptr;
                dump_exec_objects(objects, exec->buffer_count);
                
                // Trouver le batch buffer (premier buffer généralement)
                if (exec->buffer_count > 0) {
                    // Le batch est dans le premier buffer
                    // On doit mapper le buffer pour lire son contenu
                    // Pour l'instant, on note juste les informations
                    fprintf(dump_file, "[BATCH BUFFER INFO]\n");
                    fprintf(dump_file, "  Handle: %u\n", objects[0].handle);
                    fprintf(dump_file, "  Offset: 0x%016lx\n", objects[0].offset);
                    fprintf(dump_file, "  Length: %u bytes\n", exec->batch_len);
                    fprintf(dump_file, "\n");
                }
            }
            
            fflush(dump_file);
        }
    }
    
    // Appeler le vrai ioctl
    return real_ioctl(fd, request, argp);
}

// Made with Bob
