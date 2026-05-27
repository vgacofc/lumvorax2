// test_c569_validate_gtt.c - VALIDATION ROOT CAUSE #82
// Vérification explicite allocation GTT après PASS 1
// Cycle C569 - 2026-05-25 18:35 CET
// LUMVORAX GPU Native Mining - Infrastructure i915 DRM

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <errno.h>

// DRM/i915 headers
#include <drm/drm.h>
#include <drm/i915_drm.h>

int main() {
    printf("=== TEST C569 - VALIDATION GTT ALLOCATION ===\n");
    printf("ROOT CAUSE #82: GTT allocation PASS 1 échoue silencieusement\n");
    printf("OBJECTIF: Vérifier si output_gtt == 0x0 après PASS 1\n\n");
    
    // === OUVERTURE DRM ===
    printf("=== PHASE 1: OUVERTURE DRM (0%%) ===\n");
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open /dev/dri/card1");
        return 1;
    }
    printf("[5%%] ✓ DRM ouvert (fd=%d)\n", fd);
    
    // === CRÉATION CONTEXT ===
    printf("\n=== PHASE 2: CRÉATION CONTEXT (10%%) ===\n");
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("context_create");
        close(fd);
        return 1;
    }
    uint32_t context_id = ctx_create.ctx_id;
    printf("[15%%] ✓ Context créé (ID=%u)\n", context_id);
    
    // === ALLOCATION BUFFERS ===
    printf("\n=== PHASE 3: ALLOCATION BUFFERS (20%%) ===\n");
    
    // Output buffer (4KB)
    struct drm_i915_gem_create create_output = {
        .size = 4096,
        .handle = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        perror("create output");
        close(fd);
        return 1;
    }
    uint32_t output_handle = create_output.handle;
    printf("[25%%] ✓ Output buffer créé (handle=%u, 4KB)\n", output_handle);
    
    // Batch buffer (4KB)
    struct drm_i915_gem_create create_batch = {
        .size = 4096,
        .handle = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("create batch");
        close(fd);
        return 1;
    }
    uint32_t batch_handle = create_batch.handle;
    printf("[30%%] ✓ Batch buffer créé (handle=%u, 4KB)\n", batch_handle);
    
    // === MAPPING BATCH ===
    printf("\n=== PHASE 4: MAPPING BATCH (35%%) ===\n");
    
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = batch_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        perror("mmap batch");
        close(fd);
        return 1;
    }
    uint32_t *batch = (uint32_t *)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);
    printf("[40%%] ✓ Batch mappé @ %p\n", batch);
    
    // === CONSTRUCTION BATCH MINIMAL ===
    printf("\n=== PHASE 5: CONSTRUCTION BATCH MINIMAL (45%%) ===\n");
    
    int idx = 0;
    batch[idx++] = 0x0A000000;  // MI_BATCH_BUFFER_END
    batch[idx++] = 0x00000000;  // NOOP (padding alignement 8)
    
    uint32_t batch_len = 8;  // 2 DWords = 8 bytes
    
    printf("[50%%] ✓ Batch minimal construit: %u bytes\n", batch_len);
    printf("       Contenu: MI_BATCH_BUFFER_END + NOOP\n");
    printf("       ⚠️  AUCUNE référence à output buffer!\n");
    
    // === PASS 1: ALLOCATION GTT (TEST CRITIQUE) ===
    printf("\n=== PHASE 6: PASS 1 - ALLOCATION GTT (55%%) ===\n");
    printf("⚠️  TEST CRITIQUE: output buffer NON référencé dans batch\n");
    printf("⚠️  Hypothèse: i915 n'allouera PAS de GTT pour output\n\n");
    
    struct drm_i915_gem_exec_object2 objects_pass1[2] = {
        {
            .handle = output_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE,
            .offset = 0  // NON-PINNED → i915 devrait allouer GTT
        },
        {
            .handle = batch_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0  // NON-PINNED → i915 devrait allouer GTT
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf_pass1 = {
        .buffers_ptr = (uint64_t)objects_pass1,
        .buffer_count = 2,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = context_id,
        .rsvd2 = 0
    };
    
    printf("[DEBUG] AVANT PASS 1:\n");
    printf("[DEBUG]   objects[0].offset = 0x%016lx (output)\n", objects_pass1[0].offset);
    printf("[DEBUG]   objects[1].offset = 0x%016lx (batch)\n", objects_pass1[1].offset);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1) < 0) {
        perror("execbuffer2 pass1");
        printf("❌ PASS 1 ÉCHOUÉ - errno=%d\n", errno);
        close(fd);
        return 1;
    }
    
    uint64_t output_gtt = objects_pass1[0].offset;
    uint64_t batch_gtt = objects_pass1[1].offset;
    
    printf("\n[60%%] ✓ PASS 1 réussi - Analyse GTT:\n");
    printf("       output_gtt = 0x%016lx\n", output_gtt);
    printf("       batch_gtt  = 0x%016lx\n", batch_gtt);
    
    // === VALIDATION ROOT CAUSE #82 ===
    printf("\n=== PHASE 7: VALIDATION ROOT CAUSE #82 (70%%) ===\n");
    
    bool output_gtt_valid = (output_gtt != 0x0);
    bool batch_gtt_valid = (batch_gtt != 0x0);
    
    printf("\n📊 RÉSULTATS VALIDATION:\n");
    printf("   output_gtt valide: %s\n", output_gtt_valid ? "✅ OUI" : "❌ NON");
    printf("   batch_gtt valide:  %s\n", batch_gtt_valid ? "✅ OUI" : "❌ NON");
    
    if (!output_gtt_valid) {
        printf("\n🎯 ROOT CAUSE #82 CONFIRMÉ!\n");
        printf("═══════════════════════════════════════════════════════\n");
        printf("DÉCOUVERTE: i915 n'alloue PAS de GTT pour buffers\n");
        printf("            NON référencés dans le batch PASS 1!\n");
        printf("═══════════════════════════════════════════════════════\n\n");
        
        printf("📋 ANALYSE DÉTAILLÉE:\n");
        printf("   1. Batch PASS 1 contient: MI_BATCH_BUFFER_END + NOOP\n");
        printf("   2. Aucune instruction ne référence output buffer\n");
        printf("   3. i915 parse batch et détecte: 0 références à output\n");
        printf("   4. i915 décide: pas besoin d'allouer GTT pour output\n");
        printf("   5. Résultat: output_gtt reste 0x0\n\n");
        
        printf("⚠️  CONSÉQUENCE:\n");
        printf("   - PASS 2 utilise output_gtt = 0x0\n");
        printf("   - MI_STORE_DATA_IMM écrit vers adresse 0x0\n");
        printf("   - GPU Memory Protection Fault\n");
        printf("   - GPU HANG (ecode 9:1:e757fefe)\n\n");
        
        printf("✅ SOLUTION IDENTIFIÉE:\n");
        printf("   Option 1: Batch PASS 1 doit référencer output\n");
        printf("   Option 2: Utiliser relocation entries\n");
        printf("   Option 3: Reproduire pattern OpenCL complet\n\n");
        
    } else {
        printf("\n⚠️  RÉSULTAT INATTENDU!\n");
        printf("   output_gtt = 0x%016lx (NON-NUL)\n", output_gtt);
        printf("   i915 a alloué GTT malgré absence de référence\n");
        printf("   ROOT CAUSE #82 INFIRMÉ - Autre cause à identifier\n\n");
    }
    
    if (!batch_gtt_valid) {
        printf("❌ ERREUR CRITIQUE: batch_gtt = 0x0\n");
        printf("   Batch buffer DOIT avoir GTT valide!\n");
        printf("   Ceci indique un problème i915 plus profond\n\n");
    }
    
    // === STATISTIQUES ===
    printf("\n=== PHASE 8: STATISTIQUES (80%%) ===\n");
    printf("📊 Métriques GTT allocation:\n");
    printf("   Buffers soumis:     2\n");
    printf("   GTT allouées:       %d\n", (output_gtt_valid ? 1 : 0) + (batch_gtt_valid ? 1 : 0));
    printf("   Taux succès:        %d%%\n", ((output_gtt_valid ? 1 : 0) + (batch_gtt_valid ? 1 : 0)) * 50);
    printf("   Références batch:   0 (output), 1 (batch via execution)\n\n");
    
    // === CLEANUP ===
    printf("=== PHASE 9: CLEANUP (90%%) ===\n");
    
    munmap(batch, 4096);
    printf("[95%%] ✓ Batch unmappé\n");
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = context_id };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy) < 0) {
        perror("context_destroy");
    } else {
        printf("[98%%] ✓ Context détruit\n");
    }
    
    close(fd);
    printf("[100%%] ✓ DRM fermé\n");
    
    // === RÉSULTAT FINAL ===
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("           RÉSULTAT FINAL TEST C569\n");
    printf("═══════════════════════════════════════════════════════\n");
    
    if (!output_gtt_valid) {
        printf("✅ ROOT CAUSE #82 CONFIRMÉ\n");
        printf("✅ Cause identifiée: GTT allocation lazy\n");
        printf("✅ Solution: Référencer output dans PASS 1\n");
        printf("🎯 Prochaine étape: C570 - Batch avec relocation\n");
        return 0;
    } else {
        printf("⚠️  ROOT CAUSE #82 INFIRMÉ\n");
        printf("⚠️  GTT alloué malgré absence référence\n");
        printf("⚠️  Analyse supplémentaire requise\n");
        printf("🔍 Prochaine étape: Analyser pourquoi C568 échoue\n");
        return 1;
    }
}

// Made with Bob - Cycle C569
// ROOT CAUSE #82: GTT allocation lazy - VALIDATION
// OBJECTIF: Confirmer output_gtt = 0x0 après PASS 1 minimal