// OPENCL BATCH DUMPER - Capture complète du batch buffer OpenCL
// Intercepte EXECBUFFER2 et dump le contenu COMPLET du batch

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <i915_drm.h>
#include <xf86drm.h>

static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;
static int dump_count = 0;

// Fonction pour dumper un buffer GEM
static void dump_gem_buffer(int fd, uint32_t handle, const char *name, size_t size) {
    // Ouvrir le buffer via mmap
    struct drm_i915_gem_mmap mmap_arg;
    memset(&mmap_arg, 0, sizeof(mmap_arg));
    mmap_arg.handle = handle;
    mmap_arg.offset = 0;
    mmap_arg.size = size;
    
    if (real_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) == 0) {
        void *ptr = (void*)(uintptr_t)mmap_arg.addr_ptr;
        
        char filename[256];
        snprintf(filename, sizeof(filename), 
                 "/tmp/opencl_batch_dump_%s_%d.bin", name, dump_count);
        
        FILE *f = fopen(filename, "wb");
        if (f) {
            fwrite(ptr, 1, size, f);
            fclose(f);
            printf("  📦 Dumped %s: %zu bytes -> %s\n", name, size, filename);
            
            // Dump hexadécimal des premiers 512 bytes
            snprintf(filename, sizeof(filename), 
                     "/tmp/opencl_batch_dump_%s_%d.hex", name, dump_count);
            f = fopen(filename, "w");
            if (f) {
                uint32_t *data = (uint32_t*)ptr;
                size_t dwords = (size < 512 ? size : 512) / 4;
                for (size_t i = 0; i < dwords; i++) {
                    if (i % 8 == 0) fprintf(f, "\n%04zx: ", i * 4);
                    fprintf(f, "%08x ", data[i]);
                }
                fprintf(f, "\n");
                fclose(f);
                printf("  📝 Hex dump: %s\n", filename);
            }
        }
        
        munmap(ptr, size);
    }
}

int ioctl(int fd, unsigned long request, ...) {
    if (!real_ioctl) {
        real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    }
    
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void*);
    va_end(args);
    
    // Intercepter EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *execbuf = (struct drm_i915_gem_execbuffer2*)argp;
        struct drm_i915_gem_exec_object2 *exec_objects = 
            (struct drm_i915_gem_exec_object2*)(uintptr_t)execbuf->buffers_ptr;
        
        printf("\n╔════════════════════════════════════════════════════════════════╗\n");
        printf("║  OPENCL EXECBUFFER2 INTERCEPTÉ - DUMP COMPLET #%d            ║\n", dump_count);
        printf("╚════════════════════════════════════════════════════════════════╝\n\n");
        
        printf("📊 EXECBUFFER2 METADATA:\n");
        printf("  buffer_count:  %u\n", execbuf->buffer_count);
        printf("  batch_len:     %u bytes\n", execbuf->batch_len);
        printf("  batch_start:   %u\n", execbuf->batch_start_offset);
        printf("  flags:         0x%llx\n", (unsigned long long)execbuf->flags);
        printf("  context_id:    %llu\n\n", (unsigned long long)execbuf->rsvd1);
        
        printf("📦 EXEC_OBJECTS ARRAY:\n");
        for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
            printf("  [%2u] handle=%-6u offset=0x%016llx flags=0x%llx relocs=%u\n",
                   i,
                   exec_objects[i].handle,
                   (unsigned long long)exec_objects[i].offset,
                   (unsigned long long)exec_objects[i].flags,
                   exec_objects[i].relocation_count);
        }
        printf("\n");
        
        // Identifier le batch buffer (dernier avec batch_len)
        uint32_t batch_idx = execbuf->buffer_count - 1;
        for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
            if (exec_objects[i].flags & EXEC_OBJECT_WRITE) {
                // Probablement pas le batch
                continue;
            }
        }
        
        printf("🔍 DUMP BUFFERS:\n");
        
        // Dumper le batch buffer
        printf("  Batch buffer (index %u):\n", batch_idx);
        dump_gem_buffer(fd, exec_objects[batch_idx].handle, "batch", execbuf->batch_len);
        
        // Dumper tous les autres buffers (limité à 4KB chacun)
        for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
            if (i == batch_idx) continue;
            
            char name[64];
            snprintf(name, sizeof(name), "buffer_%02u", i);
            dump_gem_buffer(fd, exec_objects[i].handle, name, 4096);
        }
        
        dump_count++;
        printf("\n");
    }
    
    // Appeler le vrai ioctl
    int ret = real_ioctl(fd, request, argp);
    
    return ret;
}

// Made with Bob
