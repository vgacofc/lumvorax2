/*
 * TEST J — PREEMPTION / CONTEXT SAVE/RESTORE
 * 
 * HYPOTHÈSE CRITIQUE :
 * Le problème n'est PAS "GPU corrompu" mais "contexte incomplet qui échoue au save/restore"
 * 
 * PREUVE RECHERCHÉE :
 * - Cas A (destroy/recreate) : Fonctionne indéfiniment
 * - Cas B (même contexte) : Crash dispatch 2
 * 
 * Si A fonctionne et B crash → PREUVE que le problème est dans save/restore contexte
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include <drm/i915_drm.h>
#include <xf86drm.h>

#define BATCH_SIZE 4096

// Créer batch minimal compute
uint32_t* create_minimal_batch(int fd, uint32_t *bo_handle, size_t *size) {
    *size = BATCH_SIZE;
    
    struct drm_i915_gem_create create = {
        .size = *size
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        printf("Erreur create BO: %s\n", strerror(errno));
        return NULL;
    }
    
    *bo_handle = create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = *bo_handle,
        .offset = 0,
        .size = *size,
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        printf("Erreur mmap: %s\n", strerror(errno));
        return NULL;
    }
    
    uint32_t *batch = (uint32_t*)mmap_arg.addr_ptr;
    
    // Batch ultra-minimal : juste MI_BATCH_BUFFER_END
    batch[0] = 0x0A000000; // MI_BATCH_BUFFER_END
    batch[1] = 0x00000000; // MI_NOOP (padding)
    
    return batch;
}

// Test Cas A : Destroy/Recreate contexte entre chaque dispatch
int test_case_a_destroy_recreate(int fd, int iterations) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  CAS A : DESTROY/RECREATE CONTEXTE ENTRE DISPATCHES           ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    for (int i = 0; i < iterations; i++) {
        // Créer contexte
        struct drm_i915_gem_context_create ctx_create = {0};
        if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
            printf("Erreur create context iteration %d: %s\n", i+1, strerror(errno));
            return -1;
        }
        uint32_t ctx_id = ctx_create.ctx_id;
        
        // Créer batch
        uint32_t bo_handle;
        size_t batch_size;
        uint32_t *batch = create_minimal_batch(fd, &bo_handle, &batch_size);
        if (!batch) {
            return -1;
        }
        
        // Soumettre batch
        struct drm_i915_gem_exec_object2 exec_obj = {
            .handle = bo_handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .alignment = 0,
            .offset = 0,
            .flags = 0,
            .rsvd1 = 0,
            .rsvd2 = 0
        };
        
        struct drm_i915_gem_execbuffer2 execbuf = {
            .buffers_ptr = (uintptr_t)&exec_obj,
            .buffer_count = 1,
            .batch_start_offset = 0,
            .batch_len = 8,
            .cliprects_ptr = 0,
            .num_cliprects = 0,
            .DR1 = 0,
            .DR4 = 0,
            .flags = I915_EXEC_RENDER,
            .rsvd1 = ctx_id,
            .rsvd2 = 0
        };
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (ret < 0) {
            printf("❌ Iteration %d FAILED: errno=%d (%s)\n", i+1, errno, strerror(errno));
            
            // Cleanup
            munmap(batch, batch_size);
            struct drm_gem_close close_arg = { .handle = bo_handle };
            ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
            struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
            ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
            
            return i+1;
        }
        
        // Wait completion
        struct drm_i915_gem_wait wait = {
            .bo_handle = bo_handle,
            .timeout_ns = 10000000000LL // 10s
        };
        ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
        
        printf("✅ Iteration %d SUCCESS (%.6fs)\n", i+1, elapsed);
        
        // Cleanup
        munmap(batch, batch_size);
        struct drm_gem_close close_arg = { .handle = bo_handle };
        ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        
        // Détruire contexte
        struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
        if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy) < 0) {
            printf("Erreur destroy context: %s\n", strerror(errno));
            return -1;
        }
    }
    
    printf("\n✅ CAS A : %d iterations SUCCÈS (contexte détruit/recréé)\n", iterations);
    return 0;
}

// Test Cas B : Même contexte pour tous les dispatches
int test_case_b_same_context(int fd, int iterations) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  CAS B : MÊME CONTEXTE POUR TOUS LES DISPATCHES               ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    // Créer contexte UNE FOIS
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("Erreur create context: %s\n", strerror(errno));
        return -1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    printf("Contexte créé : ID=%u\n\n", ctx_id);
    
    for (int i = 0; i < iterations; i++) {
        // Créer batch
        uint32_t bo_handle;
        size_t batch_size;
        uint32_t *batch = create_minimal_batch(fd, &bo_handle, &batch_size);
        if (!batch) {
            return -1;
        }
        
        // Soumettre batch avec MÊME CONTEXTE
        struct drm_i915_gem_exec_object2 exec_obj = {
            .handle = bo_handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .alignment = 0,
            .offset = 0,
            .flags = 0,
            .rsvd1 = 0,
            .rsvd2 = 0
        };
        
        struct drm_i915_gem_execbuffer2 execbuf = {
            .buffers_ptr = (uintptr_t)&exec_obj,
            .buffer_count = 1,
            .batch_start_offset = 0,
            .batch_len = 8,
            .cliprects_ptr = 0,
            .num_cliprects = 0,
            .DR1 = 0,
            .DR4 = 0,
            .flags = I915_EXEC_RENDER,
            .rsvd1 = ctx_id, // MÊME CONTEXTE
            .rsvd2 = 0
        };
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (ret < 0) {
            printf("❌ Iteration %d FAILED: errno=%d (%s)\n", i+1, errno, strerror(errno));
            printf("\n🔥 CRASH DÉTECTÉ AU DISPATCH %d AVEC MÊME CONTEXTE\n", i+1);
            
            // Cleanup
            munmap(batch, batch_size);
            struct drm_gem_close close_arg = { .handle = bo_handle };
            ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
            struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
            ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
            
            return i+1;
        }
        
        // Wait completion
        struct drm_i915_gem_wait wait = {
            .bo_handle = bo_handle,
            .timeout_ns = 10000000000LL // 10s
        };
        ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
        
        printf("✅ Iteration %d SUCCESS (%.6fs)\n", i+1, elapsed);
        
        // Cleanup batch
        munmap(batch, batch_size);
        struct drm_gem_close close_arg = { .handle = bo_handle };
        ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    }
    
    // Détruire contexte
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    printf("\n✅ CAS B : %d iterations SUCCÈS (même contexte)\n", iterations);
    return 0;
}

int main() {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST J — PREEMPTION / CONTEXT SAVE/RESTORE                   ║\n");
    printf("║  Hypothèse : Le problème est dans save/restore contexte       ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("Erreur ouverture /dev/dri/renderD128: %s\n", strerror(errno));
        return 1;
    }
    
    printf("\n📊 Configuration Test :\n");
    printf("  - Batch : MI_BATCH_BUFFER_END minimal\n");
    printf("  - Iterations : 10 dispatches\n");
    printf("  - Objectif : Prouver que save/restore contexte est le problème\n");
    printf("\n");
    
    // Test Cas A : Destroy/Recreate
    printf("═══════════════════════════════════════════════════════════════\n");
    int result_a = test_case_a_destroy_recreate(fd, 10);
    
    // Attendre un peu
    sleep(2);
    
    // Test Cas B : Même contexte
    printf("\n═══════════════════════════════════════════════════════════════\n");
    int result_b = test_case_b_same_context(fd, 10);
    
    close(fd);
    
    // Analyse résultats
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS ANALYSE                                             ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("CAS A (destroy/recreate) : ");
    if (result_a == 0) {
        printf("✅ 10/10 dispatches SUCCESS\n");
    } else {
        printf("❌ Crash au dispatch %d\n", result_a);
    }
    
    printf("CAS B (même contexte)    : ");
    if (result_b == 0) {
        printf("✅ 10/10 dispatches SUCCESS\n");
    } else {
        printf("❌ Crash au dispatch %d\n", result_b);
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  CONCLUSION                                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    if (result_a == 0 && result_b > 0) {
        printf("🔥 PREUVE OBTENUE !\n\n");
        printf("Le problème est CONFIRMÉ dans le mécanisme save/restore contexte :\n");
        printf("  - Contexte neuf à chaque dispatch : ✅ Fonctionne\n");
        printf("  - Même contexte réutilisé : ❌ Crash dispatch %d\n", result_b);
        printf("\n");
        printf("Cause racine identifiée :\n");
        printf("  → Le contexte GPU que nous créons est INCOMPLET\n");
        printf("  → Il manque des états obligatoires pour save/restore\n");
        printf("  → Le premier dispatch fonctionne (état initial propre)\n");
        printf("  → Le save/restore échoue (état incomplet)\n");
        printf("  → Preemption timeout → GPU HANG\n");
        printf("\n");
        printf("Prochaine étape :\n");
        printf("  → Comparer contexte OpenCL vs notre contexte\n");
        printf("  → Identifier états manquants (scratch, preemption, etc.)\n");
        printf("  → Ajouter configuration contexte complète\n");
    } else if (result_a == 0 && result_b == 0) {
        printf("❓ Résultat inattendu : Les deux cas fonctionnent\n");
        printf("Le batch minimal ne déclenche peut-être pas le problème.\n");
        printf("Il faut tester avec un vrai kernel compute (SHA-256).\n");
    } else {
        printf("❌ Les deux cas crashent\n");
        printf("Le problème est peut-être plus profond (batch invalide).\n");
    }
    
    return 0;
}

// Made with Bob
