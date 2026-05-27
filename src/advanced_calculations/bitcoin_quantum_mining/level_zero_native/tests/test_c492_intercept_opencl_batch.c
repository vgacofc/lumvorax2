/*
 * LUMVORAX C492 - Interception Batch Buffer OpenCL
 *
 * Objectif: Intercepter via LD_PRELOAD les appels ioctl EXECBUFFER2
 *           d'OpenCL pour capturer le batch buffer exact et le comparer
 *           avec notre batch i915 natif
 *
 * Méthode: Wrapper ioctl() qui dump le batch avant soumission
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <libdrm/drm.h>
#include <libdrm/i915_drm.h>

// Pointeur vers le vrai ioctl
static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;

// Compteur appels
static int execbuffer_count = 0;
static FILE *dump_file = NULL;

// Initialisation
static void __attribute__((constructor)) init_intercept(void) {
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    if (!real_ioctl) {
        fprintf(stderr, "ERREUR: dlsym(ioctl) failed\n");
        exit(1);
    }
    
    // Ouvrir fichier dump
    char filename[256];
    snprintf(filename, sizeof(filename), 
             "logs/opencl_batch_dump_%ld.log", time(NULL));
    dump_file = fopen(filename, "w");
    if (!dump_file) {
        fprintf(stderr, "ERREUR: fopen(%s) failed\n", filename);
        exit(1);
    }
    
    fprintf(dump_file, "=== LUMVORAX C492 - Interception Batch OpenCL ===\n\n");
    fprintf(stderr, "✓ Interception ioctl activée - Dump: %s\n", filename);
}

// Cleanup
static void __attribute__((destructor)) cleanup_intercept(void) {
    if (dump_file) {
        fprintf(dump_file, "\n=== FIN INTERCEPTION ===\n");
        fprintf(dump_file, "Total EXECBUFFER2: %d\n", execbuffer_count);
        fclose(dump_file);
    }
}

// Dump batch buffer
static void dump_batch_buffer(struct drm_i915_gem_execbuffer2 *execbuf) {
    fprintf(dump_file, "\n=== EXECBUFFER2 #%d ===\n", ++execbuffer_count);
    fprintf(dump_file, "Timestamp: %ld\n", time(NULL));
    fprintf(dump_file, "buffers_ptr: 0x%llx\n", execbuf->buffers_ptr);
    fprintf(dump_file, "buffer_count: %u\n", execbuf->buffer_count);
    fprintf(dump_file, "batch_start_offset: %u\n", execbuf->batch_start_offset);
    fprintf(dump_file, "batch_len: %u\n", execbuf->batch_len);
    fprintf(dump_file, "flags: 0x%llx\n", execbuf->flags);
    fprintf(dump_file, "rsvd1: 0x%llx\n", execbuf->rsvd1);
    fprintf(dump_file, "rsvd2: 0x%llx\n\n", execbuf->rsvd2);
    
    // Dump exec_objects
    struct drm_i915_gem_exec_object2 *objects = 
        (struct drm_i915_gem_exec_object2 *)execbuf->buffers_ptr;
    
    fprintf(dump_file, "=== EXEC OBJECTS (%u) ===\n", execbuf->buffer_count);
    for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
        fprintf(dump_file, "\nObject[%u]:\n", i);
        fprintf(dump_file, "  handle: %u\n", objects[i].handle);
        fprintf(dump_file, "  relocation_count: %u\n", objects[i].relocation_count);
        fprintf(dump_file, "  relocs_ptr: 0x%llx\n", objects[i].relocs_ptr);
        fprintf(dump_file, "  alignment: %llu\n", objects[i].alignment);
        fprintf(dump_file, "  offset: 0x%llx\n", objects[i].offset);
        fprintf(dump_file, "  flags: 0x%llx\n", objects[i].flags);
        
        // Dump relocations si présentes
        if (objects[i].relocation_count > 0) {
            struct drm_i915_gem_relocation_entry *relocs =
                (struct drm_i915_gem_relocation_entry *)objects[i].relocs_ptr;
            
            fprintf(dump_file, "\n  Relocations (%u):\n", objects[i].relocation_count);
            for (uint32_t j = 0; j < objects[i].relocation_count; j++) {
                fprintf(dump_file, "    [%u] offset=0x%llx target_handle=%u "
                       "delta=0x%x read_domains=0x%x write_domain=0x%x\n",
                       j, (unsigned long long)relocs[j].offset, relocs[j].target_handle,
                       relocs[j].delta, relocs[j].read_domains,
                       relocs[j].write_domain);
            }
        }
    }
    
    fprintf(dump_file, "\n=== BATCH BUFFER CONTENT ===\n");
    fprintf(dump_file, "Length: %u bytes (%u DWORDs)\n", 
            execbuf->batch_len, execbuf->batch_len / 4);
    
    // Note: On ne peut pas dumper le contenu du batch ici car il est
    // dans un buffer GPU. Il faudrait mapper le buffer via GEM_MMAP.
    // Pour l'instant, on enregistre juste les métadonnées.
    
    fprintf(dump_file, "\nPour dumper le contenu batch, utiliser:\n");
    fprintf(dump_file, "  intel_gpu_top --batch-buffer\n");
    fprintf(dump_file, "  ou mapper le buffer via drm_intel_bo_map()\n");
    
    fflush(dump_file);
}

// Wrapper ioctl
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void *);
    va_end(args);
    
    // Intercepter EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *execbuf = 
            (struct drm_i915_gem_execbuffer2 *)argp;
        
        if (dump_file) {
            dump_batch_buffer(execbuf);
        }
    }
    
    // Appeler le vrai ioctl
    return real_ioctl(fd, request, argp);
}

// Made with Bob
