// TEST C545 - DUMP BATCH OPENCL EXACT
// Objectif: Capturer le batch buffer OpenCL qui fonctionne
// Méthode: Hook ioctl() avec LD_PRELOAD pour dumper les données

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libdrm/i915_drm.h>

// Pointeur vers le vrai ioctl
static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;

// Compteur d'appels
static int execbuffer_count = 0;

// Hook ioctl
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, char *);
    va_end(args);
    
    // Charger le vrai ioctl si nécessaire
    if (!real_ioctl) {
        real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    }
    
    // Intercepter DRM_IOCTL_I915_GEM_EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *execbuf = (struct drm_i915_gem_execbuffer2 *)argp;
        
        printf("\n=== EXECBUFFER2 #%d INTERCEPTÉ ===\n", ++execbuffer_count);
        printf("buffer_count: %u\n", execbuf->buffer_count);
        printf("batch_start_offset: 0x%x\n", execbuf->batch_start_offset);
        printf("batch_len: %u bytes\n", execbuf->batch_len);
        printf("flags: 0x%llx\n", (unsigned long long)execbuf->flags);
        
        // Accéder aux exec_objects
        struct drm_i915_gem_exec_object2 *objects = 
            (struct drm_i915_gem_exec_object2 *)execbuf->buffers_ptr;
        
        printf("\n=== BUFFERS ===\n");
        for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
            printf("Buffer[%u]: handle=%u, offset=0x%llx, size=%llu, flags=0x%llx\n",
                   i, objects[i].handle, objects[i].offset, 
                   objects[i].alignment, objects[i].flags);
            
            // Dumper TOUS les buffers (pas seulement le batch)
            printf("\n=== DUMP BUFFER[%u] (handle=%u) ===\n", i, objects[i].handle);
            
            // Utiliser le fd existant passé à ioctl
            struct drm_i915_gem_mmap mmap_arg = {
                .handle = objects[i].handle,
                .offset = 0,
                .size = 8192,  // Taille max pour dump
                .flags = 0
            };
            
            if (real_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) == 0) {
                uint8_t *buffer = (uint8_t *)mmap_arg.addr_ptr;
                
                printf("Buffer mappé @ %p\n", buffer);
                
                // Afficher premiers 256 bytes
                printf("\nPremiers 256 bytes:\n");
                for (int j = 0; j < 64; j++) {
                    if (j % 8 == 0) printf("%04x: ", j * 4);
                    printf("%08x ", ((uint32_t *)buffer)[j]);
                    if ((j + 1) % 8 == 0) printf("\n");
                }
                
                // Sauvegarder dans un fichier
                char filename[256];
                snprintf(filename, sizeof(filename),
                         "logs/opencl_buffer_%d_%u.bin", execbuffer_count, i);
                FILE *f = fopen(filename, "wb");
                if (f) {
                    size_t write_size = 8192;
                    // Pour le batch, utiliser la taille exacte
                    if (i == execbuf->buffer_count - 1) {
                        write_size = execbuf->batch_len;
                    }
                    fwrite(buffer, 1, write_size, f);
                    fclose(f);
                    printf("\n✓ Buffer sauvegardé: %s (%zu bytes)\n",
                           filename, write_size);
                }
                
                munmap((void *)mmap_arg.addr_ptr, mmap_arg.size);
            } else {
                printf("❌ Échec mmap buffer[%u]\n", i);
            }
        }
        
        printf("\n");
    }
    
    // Appeler le vrai ioctl
    return real_ioctl(fd, request, argp);
}

// Made with Bob
