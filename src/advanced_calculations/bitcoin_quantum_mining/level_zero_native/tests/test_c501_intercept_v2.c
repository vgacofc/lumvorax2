/*
 * CYCLE C501 V2: INTERCEPTION BATCH OPENCL - VERSION COMPLÈTE
 * 
 * Intercepte TOUTES les fonctions possibles d'exécution batch:
 * - drm_intel_gem_bo_context_exec
 * - drm_intel_bo_mrb_exec  
 * - drm_intel_gem_bo_exec
 * - __drm_intel_gem_bo_context_exec (symbole interne)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

static int batch_counter = 0;
static int initialized = 0;

// Structure drm_intel_bo (approximative)
typedef struct {
    uint32_t handle;
    uint64_t offset64;
    uint32_t size;
    uint32_t align;
    uint64_t offset;
    void *virtual;
    // ... autres champs
} drm_intel_bo_t;

static void dump_batch(void *bo_ptr, int used, const char *func_name)
{
    if (!initialized) {
        mkdir("logs", 0755);
        initialized = 1;
    }
    
    printf("\n=== INTERCEPTION C501 V2: %s ===\n", func_name);
    printf("Batch #%d, Size: %d bytes (%d DWORDs)\n", 
           batch_counter, used, used/4);
    
    // Essayer de mapper le buffer
    typedef int (*map_func_t)(void *bo, int write_enable);
    map_func_t map_func = (map_func_t)dlsym(RTLD_NEXT, "drm_intel_bo_map");
    
    void *virtual = NULL;
    int mapped = 0;
    
    if (map_func) {
        if (map_func(bo_ptr, 0) == 0) {
            mapped = 1;
            // Essayer différents offsets pour trouver virtual
            drm_intel_bo_t *bo = (drm_intel_bo_t*)bo_ptr;
            virtual = bo->virtual;
            
            if (!virtual) {
                // Essayer offset alternatif
                void **bo_array = (void**)bo_ptr;
                for (int i = 0; i < 20; i++) {
                    if (bo_array[i] != NULL && 
                        (uintptr_t)bo_array[i] > 0x1000 &&
                        (uintptr_t)bo_array[i] < 0x7fffffffffff) {
                        virtual = bo_array[i];
                        printf("Virtual trouvé à offset %d: %p\n", i, virtual);
                        break;
                    }
                }
            }
        }
    }
    
    if (virtual) {
        // Créer nom fichier
        char filename[256];
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        snprintf(filename, sizeof(filename),
                "logs/c501_batch_opencl_%d_%ld.bin",
                batch_counter, ts.tv_sec);
        
        // Sauvegarder batch
        FILE *f = fopen(filename, "wb");
        if (f) {
            fwrite(virtual, 1, used, f);
            fclose(f);
            printf("✓ Batch sauvegardé: %s\n", filename);
            
            // Dump hexadécimal
            printf("\n=== DUMP HEX (premiers 512 bytes) ===\n");
            uint32_t *batch = (uint32_t*)virtual;
            int dump_size = (used < 512) ? used/4 : 128;
            
            for (int i = 0; i < dump_size; i++) {
                if (i % 4 == 0) printf("0x%04x: ", i*4);
                printf("0x%08x ", batch[i]);
                if ((i+1) % 4 == 0) printf("\n");
            }
            
            // Analyser commandes
            printf("\n=== ANALYSE COMMANDES ===\n");
            for (int i = 0; i < used/4; i++) {
                uint32_t cmd = batch[i];
                uint32_t opcode = cmd & 0xFFFF0000;
                
                if (opcode == 0x79000000) {
                    printf("0x%04x: PIPE_CONTROL (len=%d)\n", 
                           i*4, (cmd & 0xFF) + 2);
                } else if (opcode == 0x69040000) {
                    printf("0x%04x: PIPELINE_SELECT (mode=%d)\n", 
                           i*4, cmd & 0x3);
                } else if (opcode == 0x61010000) {
                    int len = (cmd & 0xFF) + 2;
                    printf("0x%04x: STATE_BASE_ADDRESS (len=%d)\n", i*4, len);
                    for (int j = 1; j < len && (i+j) < used/4; j++) {
                        printf("  DW%d: 0x%08x\n", j, batch[i+j]);
                    }
                } else if (opcode == 0x70000000) {
                    int len = (cmd & 0xFF) + 2;
                    printf("0x%04x: MEDIA_VFE_STATE (len=%d)\n", i*4, len);
                    for (int j = 1; j < len && (i+j) < used/4; j++) {
                        printf("  DW%d: 0x%08x\n", j, batch[i+j]);
                    }
                } else if (opcode == 0x70020000) {
                    printf("0x%04x: MEDIA_INTERFACE_DESCRIPTOR_LOAD\n", i*4);
                    printf("  DW1: 0x%08x\n", batch[i+1]);
                    printf("  DW2: 0x%08x (length=%d)\n", 
                           batch[i+2], batch[i+2] & 0xFFFF);
                    printf("  DW3: 0x%08x (offset=0x%x)\n", 
                           batch[i+3], batch[i+3] & 0xFFFF);
                } else if (opcode == 0x71050000) {
                    printf("0x%04x: GPGPU_WALKER\n", i*4);
                    printf("  DW0: 0x%08x\n", batch[i]);
                    printf("  DW1: 0x%08x (Interface Descriptor Offset)\n", batch[i+1]);
                    printf("  DW2: 0x%08x\n", batch[i+2]);
                    printf("  DW3: 0x%08x\n", batch[i+3]);
                    printf("  DW4: 0x%08x (SIMD=%d, ThreadWidth=%d)\n",
                           batch[i+4],
                           (batch[i+4] >> 30) & 0x3,
                           (batch[i+4] & 0xFF) + 1);
                    printf("  DW5: 0x%08x (ThreadGroupX=%d)\n",
                           batch[i+5], batch[i+5] & 0xFFFF);
                    printf("  DW6: 0x%08x (ThreadGroupY=%d)\n",
                           batch[i+6], batch[i+6] & 0xFFFF);
                    printf("  DW7: 0x%08x (ThreadGroupZ=%d)\n",
                           batch[i+7], batch[i+7] & 0xFFFF);
                    printf("  DW8: 0x%08x (Right Execution Mask)\n", batch[i+8]);
                    printf("  DW9: 0x%08x (Bottom Execution Mask)\n", batch[i+9]);
                } else if (opcode == 0x70040000) {
                    printf("0x%04x: MEDIA_STATE_FLUSH\n", i*4);
                } else if (cmd == 0x05000000) {
                    printf("0x%04x: MI_BATCH_BUFFER_END\n", i*4);
                    break;
                }
            }
        } else {
            fprintf(stderr, "ERREUR: Impossible de créer %s\n", filename);
        }
        
        // Unmap
        if (mapped) {
            typedef int (*unmap_func_t)(void *bo);
            unmap_func_t unmap_func = (unmap_func_t)dlsym(RTLD_NEXT, 
                                                           "drm_intel_bo_unmap");
            if (unmap_func) unmap_func(bo_ptr);
        }
    } else {
        printf("⚠ Impossible de mapper buffer\n");
    }
    
    batch_counter++;
}

// Hook principal
int drm_intel_gem_bo_context_exec(void *bo, int used, void *cliprects,
                                   int num_cliprects, int DR4,
                                   unsigned int rings, void *ctx)
{
    dump_batch(bo, used, "drm_intel_gem_bo_context_exec");
    
    typedef int (*orig_func_t)(void*, int, void*, int, int, unsigned int, void*);
    orig_func_t orig = (orig_func_t)dlsym(RTLD_NEXT, 
                                          "drm_intel_gem_bo_context_exec");
    if (orig) return orig(bo, used, cliprects, num_cliprects, DR4, rings, ctx);
    return -1;
}

// Hook alternatif 1
int drm_intel_bo_mrb_exec(void *bo, int used, void *cliprects,
                          int num_cliprects, int DR4, unsigned int rings)
{
    dump_batch(bo, used, "drm_intel_bo_mrb_exec");
    
    typedef int (*orig_func_t)(void*, int, void*, int, int, unsigned int);
    orig_func_t orig = (orig_func_t)dlsym(RTLD_NEXT, "drm_intel_bo_mrb_exec");
    if (orig) return orig(bo, used, cliprects, num_cliprects, DR4, rings);
    return -1;
}

// Hook alternatif 2
int drm_intel_gem_bo_exec(void *bo, int used, void *cliprects,
                          int num_cliprects, int DR4)
{
    dump_batch(bo, used, "drm_intel_gem_bo_exec");
    
    typedef int (*orig_func_t)(void*, int, void*, int, int);
    orig_func_t orig = (orig_func_t)dlsym(RTLD_NEXT, "drm_intel_gem_bo_exec");
    if (orig) return orig(bo, used, cliprects, num_cliprects, DR4);
    return -1;
}

// Hook alternatif 3 (symbole interne)
int __drm_intel_gem_bo_context_exec(void *bo, int used, void *cliprects,
                                     int num_cliprects, int DR4,
                                     unsigned int rings, void *ctx)
{
    dump_batch(bo, used, "__drm_intel_gem_bo_context_exec");
    
    typedef int (*orig_func_t)(void*, int, void*, int, int, unsigned int, void*);
    orig_func_t orig = (orig_func_t)dlsym(RTLD_NEXT,
                                          "__drm_intel_gem_bo_context_exec");
    if (orig) return orig(bo, used, cliprects, num_cliprects, DR4, rings, ctx);
    return -1;
}

// Made with Bob
