/*
 * TEST C440 - Intercepteur i915 pour capturer SSH OpenCL
 * 
 * STRATÉGIE: Utiliser LD_PRELOAD pour intercepter les appels ioctl i915
 * et dumper le SSH au moment où OpenCL le soumet au GPU
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

// Définitions i915
#define DRM_IOCTL_BASE 'd'
#define DRM_IOCTL_I915_GEM_EXECBUFFER2 _IOW(DRM_IOCTL_BASE, 0x49, struct drm_i915_gem_execbuffer2)

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

struct drm_i915_gem_relocation_entry {
    uint32_t target_handle;
    uint32_t delta;
    uint64_t offset;
    uint32_t read_domains;
    uint32_t write_domain;
    uint32_t presumed_offset;
};

// Pointeur vers le vrai ioctl
static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;
static int execbuffer_count = 0;
static FILE* dump_file = NULL;

// Fonction pour dumper mémoire
void dump_memory(const char* label, const void* data, size_t size) {
    if (!dump_file) return;
    
    const uint32_t* dwords = (const uint32_t*)data;
    size_t count = size / 4;
    
    fprintf(dump_file, "\n[%s] %zu bytes (%zu DWORDs):\n", label, size, count);
    for (size_t i = 0; i < count; i++) {
        fprintf(dump_file, "  DW%02zu: 0x%08x\n", i, dwords[i]);
    }
    fflush(dump_file);
}

// Intercepteur ioctl
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void* argp = va_arg(args, void*);
    va_end(args);
    
    // Initialiser le vrai ioctl
    if (!real_ioctl) {
        real_ioctl = dlsym(RTLD_NEXT, "ioctl");
        if (!real_ioctl) {
            fprintf(stderr, "❌ Impossible de trouver le vrai ioctl\n");
            exit(1);
        }
    }
    
    // Ouvrir fichier de dump au premier appel
    if (!dump_file) {
        dump_file = fopen("logs/c440_i915_intercept.log", "w");
        if (dump_file) {
            fprintf(dump_file, "=== INTERCEPTION i915 OPENCL ===\n");
            fflush(dump_file);
        }
    }
    
    // Intercepter EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        execbuffer_count++;
        
        if (dump_file) {
            fprintf(dump_file, "\n=== EXECBUFFER2 #%d ===\n", execbuffer_count);
            
            struct drm_i915_gem_execbuffer2* exec = (struct drm_i915_gem_execbuffer2*)argp;
            fprintf(dump_file, "buffer_count: %u\n", exec->buffer_count);
            fprintf(dump_file, "batch_len: %u bytes\n", exec->batch_len);
            fprintf(dump_file, "flags: 0x%lx\n", exec->flags);
            
            // Analyser les buffers
            struct drm_i915_gem_exec_object2* objects = 
                (struct drm_i915_gem_exec_object2*)exec->buffers_ptr;
            
            for (uint32_t i = 0; i < exec->buffer_count; i++) {
                fprintf(dump_file, "\nBuffer[%u]:\n", i);
                fprintf(dump_file, "  handle: %u\n", objects[i].handle);
                fprintf(dump_file, "  offset: 0x%lx\n", objects[i].offset);
                fprintf(dump_file, "  flags: 0x%lx\n", objects[i].flags);
                fprintf(dump_file, "  relocation_count: %u\n", objects[i].relocation_count);
                
                // Analyser les relocations
                if (objects[i].relocation_count > 0) {
                    struct drm_i915_gem_relocation_entry* relocs = 
                        (struct drm_i915_gem_relocation_entry*)objects[i].relocs_ptr;
                    
                    for (uint32_t j = 0; j < objects[i].relocation_count; j++) {
                        fprintf(dump_file, "  Reloc[%u]:\n", j);
                        fprintf(dump_file, "    target_handle: %u\n", relocs[j].target_handle);
                        fprintf(dump_file, "    offset: 0x%lx\n", relocs[j].offset);
                        fprintf(dump_file, "    delta: 0x%x\n", relocs[j].delta);
                        fprintf(dump_file, "    read_domains: 0x%x\n", relocs[j].read_domains);
                        fprintf(dump_file, "    write_domain: 0x%x\n", relocs[j].write_domain);
                    }
                }
            }
            
            fflush(dump_file);
        }
    }
    
    // Appeler le vrai ioctl
    return real_ioctl(fd, request, argp);
}

// Constructeur pour initialiser l'interception
__attribute__((constructor))
static void init_intercept() {
    printf("🔍 Intercepteur i915 activé\n");
}

// Destructeur pour fermer le fichier
__attribute__((destructor))
static void cleanup_intercept() {
    if (dump_file) {
        fprintf(dump_file, "\n=== FIN INTERCEPTION ===\n");
        fprintf(dump_file, "Total EXECBUFFER2: %d\n", execbuffer_count);
        fclose(dump_file);
        printf("✅ Log sauvegardé: logs/c440_i915_intercept.log\n");
    }
}

// Made with Bob
