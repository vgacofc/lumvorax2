/*
 * CYCLE C501: INTERCEPTION BATCH OPENCL COMPLET
 * 
 * Objectif: Capturer le batch buffer exact généré par OpenCL
 * pour comparer avec notre batch i915 natif et identifier
 * les différences critiques causant le GPU HANG
 * 
 * Stratégie:
 * 1. Hook drm_intel_gem_bo_context_exec via LD_PRELOAD
 * 2. Dumper batch buffer complet avant soumission
 * 3. Sauvegarder dans fichier binaire pour analyse
 * 4. Comparer avec batch C500 byte-à-byte
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

// Prototype fonction originale libdrm_intel
typedef int (*orig_exec_func_t)(void *bo, int used, void *cliprects,
                                 int num_cliprects, int DR4, 
                                 unsigned int rings, void *ctx);

// Compteur global pour nommer fichiers
static int batch_counter = 0;

// Hook de drm_intel_gem_bo_context_exec
int drm_intel_gem_bo_context_exec(void *bo, int used, void *cliprects,
                                   int num_cliprects, int DR4,
                                   unsigned int rings, void *ctx)
{
    printf("\n=== INTERCEPTION BATCH OPENCL C501 ===\n");
    printf("Batch counter: %d\n", batch_counter);
    printf("Batch size: %d bytes (%d DWORDs)\n", used, used/4);
    printf("Rings: 0x%x\n", rings);
    
    // Récupérer fonction originale
    orig_exec_func_t orig_exec = (orig_exec_func_t)dlsym(RTLD_NEXT, 
                                    "drm_intel_gem_bo_context_exec");
    if (!orig_exec) {
        fprintf(stderr, "ERREUR: Impossible de trouver fonction originale\n");
        return -1;
    }
    
    // Mapper buffer pour lire contenu
    typedef int (*map_func_t)(void *bo, int write_enable);
    map_func_t map_func = (map_func_t)dlsym(RTLD_NEXT, "drm_intel_bo_map");
    
    if (map_func && map_func(bo, 0) == 0) {
        // Récupérer pointeur vers données
        void **bo_ptr = (void**)bo;
        void *virtual = bo_ptr[8]; // Offset connu dans structure drm_intel_bo
        
        if (virtual) {
            // Créer nom fichier unique avec timestamp
            char filename[256];
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            snprintf(filename, sizeof(filename),
                    "logs/c501_batch_opencl_%d_%ld.bin",
                    batch_counter, ts.tv_sec);
            
            // Sauvegarder batch complet
            FILE *f = fopen(filename, "wb");
            if (f) {
                fwrite(virtual, 1, used, f);
                fclose(f);
                printf("✓ Batch sauvegardé: %s\n", filename);
                
                // Dump hexadécimal des 256 premiers bytes
                printf("\n=== DUMP HEXADÉCIMAL (256 premiers bytes) ===\n");
                uint32_t *batch = (uint32_t*)virtual;
                for (int i = 0; i < 64 && i < used/4; i++) {
                    if (i % 4 == 0) printf("0x%04x: ", i*4);
                    printf("0x%08x ", batch[i]);
                    if ((i+1) % 4 == 0) printf("\n");
                }
                printf("\n");
                
                // Analyser commandes principales
                printf("=== ANALYSE COMMANDES ===\n");
                for (int i = 0; i < used/4; i++) {
                    uint32_t cmd = batch[i] & 0xFFFF0000;
                    
                    if (cmd == 0x79000000) {
                        printf("0x%04x: PIPE_CONTROL\n", i*4);
                    } else if (cmd == 0x69040000) {
                        printf("0x%04x: PIPELINE_SELECT (0x%08x)\n", i*4, batch[i]);
                    } else if (cmd == 0x61010000) {
                        printf("0x%04x: STATE_BASE_ADDRESS\n", i*4);
                    } else if (cmd == 0x70000000) {
                        printf("0x%04x: MEDIA_VFE_STATE\n", i*4);
                    } else if (cmd == 0x70020000) {
                        printf("0x%04x: MEDIA_INTERFACE_DESCRIPTOR_LOAD\n", i*4);
                    } else if (cmd == 0x71050000) {
                        printf("0x%04x: GPGPU_WALKER\n", i*4);
                        printf("  DW1: 0x%08x\n", batch[i+1]);
                        printf("  DW2: 0x%08x\n", batch[i+2]);
                        printf("  DW3: 0x%08x\n", batch[i+3]);
                        printf("  DW4: 0x%08x (SIMD=%d, Threads=%d)\n", 
                               batch[i+4],
                               (batch[i+4] >> 30) & 0x3,
                               (batch[i+4] & 0xFF) + 1);
                        printf("  DW5: 0x%08x\n", batch[i+5]);
                    } else if (cmd == 0x70040000) {
                        printf("0x%04x: MEDIA_STATE_FLUSH\n", i*4);
                    } else if (cmd == 0x05000000) {
                        printf("0x%04x: MI_BATCH_BUFFER_END\n", i*4);
                    }
                }
            } else {
                fprintf(stderr, "ERREUR: Impossible de créer %s\n", filename);
            }
        }
        
        // Unmap buffer
        typedef int (*unmap_func_t)(void *bo);
        unmap_func_t unmap_func = (unmap_func_t)dlsym(RTLD_NEXT, 
                                                       "drm_intel_bo_unmap");
        if (unmap_func) unmap_func(bo);
    }
    
    batch_counter++;
    
    // Appeler fonction originale
    return orig_exec(bo, used, cliprects, num_cliprects, DR4, rings, ctx);
}

// Hook alternatif pour drm_intel_bo_mrb_exec
int drm_intel_bo_mrb_exec(void *bo, int used, void *cliprects,
                          int num_cliprects, int DR4,
                          unsigned int rings)
{
    return drm_intel_gem_bo_context_exec(bo, used, cliprects,
                                         num_cliprects, DR4, rings, NULL);
}

// Made with Bob
