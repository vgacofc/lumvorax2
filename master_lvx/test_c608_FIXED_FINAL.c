/*
 * TEST C608 - VERSION CORRIGÉE FINALE (ROOT CAUSES #83 + #84 + #86)
 *
 * Date: 2026-06-19 22:14 CET
 * Auteur: Bob (Expert GPU + 19 domaines)
 *
 * CORRECTIONS APPLIQUÉES:
 * - ROOT CAUSE #83: Fallback card0 → card1
 * - ROOT CAUSE #84: Suppression EXEC_OBJECT_PINNED
 * - ROOT CAUSE #86: Memory domains corrects (RENDER/RENDER)
 * - Utilisation relocation entries pour patcher batch
 * - Allocation GTT dynamique par driver i915
 *
 * OBJECTIF: Valider output[0] = 0x12345678 via GPU natif i915 DRM
 *
 * COMPILATION:
 * gcc -o test_c608 test_c608_FIXED_FINAL.c -I/usr/include/libdrm -ldrm -ldrm_intel
 *
 * EXÉCUTION:
 * ./test_c608 2>&1 | tee master_lvx/logs/test_c608_$(date +%Y%m%d_%H%M%S).log
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <xf86drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

/* ============================================================================
 * FORENSIC LOGGING
 * ============================================================================ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_FORENSIC(fmt, ...) do { \
    uint64_t ns = get_timestamp_ns(); \
    printf("[%llu.%09llu] " fmt "\n", \
           (unsigned long long)(ns / 1000000000ULL), \
           (unsigned long long)(ns % 1000000000ULL), \
           ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("TEST C608 - VERSION CORRIGÉE FINALE");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("Corrections: ROOT CAUSES #83 + #84 + #86");
    LOG_FORENSIC("  #83: Fallback card0 → card1");
    LOG_FORENSIC("  #84: Suppression EXEC_OBJECT_PINNED");
    LOG_FORENSIC("  #86: Memory domains RENDER/RENDER");
    LOG_FORENSIC("");
    
    uint64_t t0, t1;
    
    // Ouvrir DRM avec fallback card0 → card1
    int drm_fd = open("/dev/dri/card0", O_RDWR);
    if (drm_fd < 0) {
        LOG_FORENSIC("⚠️  /dev/dri/card0 non disponible, essai card1...");
        drm_fd = open("/dev/dri/card1", O_RDWR);
        if (drm_fd < 0) {
            LOG_FORENSIC("❌ ERREUR: Impossible d'ouvrir DRM (errno=%d)", errno);
            return 1;
        }
        LOG_FORENSIC("✅ Utilisation /dev/dri/card1");
    } else {
        LOG_FORENSIC("✅ Utilisation /dev/dri/card0");
    }
    
    // Initialiser bufmgr
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(drm_fd, 4096);
    if (!bufmgr) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bufmgr_gem_init échoué");
        close(drm_fd);
        return 1;
    }
    
    LOG_FORENSIC("✅ DRM initialisé");
    LOG_FORENSIC("");
    
    // Créer output buffer (SANS PINNED)
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
    if (!output_bo) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bo_alloc(output) échoué");
        drm_intel_bufmgr_destroy(bufmgr);
        close(drm_fd);
        return 1;
    }
    
    // Initialiser output avec pattern
    uint32_t *output;
    drm_intel_bo_map(output_bo, 1);
    output = output_bo->virtual;
    memset(output, 0xFF, 4096);  // Pattern 0xFFFFFFFF
    drm_intel_bo_unmap(output_bo);
    
    LOG_FORENSIC("✅ Output buffer créé (4096 bytes)");
    
    // Créer batch buffer
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 4096, 4096);
    if (!batch_bo) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bo_alloc(batch) échoué");
        drm_intel_bo_unreference(output_bo);
        drm_intel_bufmgr_destroy(bufmgr);
        close(drm_fd);
        return 1;
    }
    
    // Construire batch avec MI_STORE_DATA_IMM
    uint32_t *batch;
    drm_intel_bo_map(batch_bo, 1);
    batch = batch_bo->virtual;
    
    // MI_STORE_DATA_IMM (4 DWords)
    batch[0] = 0x20400003;  // Opcode
    batch[1] = 0x00000000;  // Adresse basse (sera patché par relocation)
    batch[2] = 0x00000000;  // Adresse haute (sera patché par relocation)
    batch[3] = 0x12345678;  // Valeur à écrire
    batch[4] = 0x0A000000;  // MI_BATCH_BUFFER_END
    
    drm_intel_bo_unmap(batch_bo);
    
    LOG_FORENSIC("✅ Batch buffer créé:");
    LOG_FORENSIC("  [0] = 0x20400003 (MI_STORE_DATA_IMM)");
    LOG_FORENSIC("  [1] = 0x00000000 (addr_low - sera patché)");
    LOG_FORENSIC("  [2] = 0x00000000 (addr_high - sera patché)");
    LOG_FORENSIC("  [3] = 0x12345678 (value)");
    LOG_FORENSIC("  [4] = 0x0A000000 (MI_BATCH_BUFFER_END)");
    LOG_FORENSIC("");
    
    // Créer relocation entry pour patcher batch[1] (adresse complète 64-bit)
    struct drm_i915_gem_relocation_entry reloc;
    memset(&reloc, 0, sizeof(reloc));
    
    // Relocation pour adresse complète (batch[1] et batch[2])
    // ROOT CAUSE #86: MI_STORE_DATA_IMM nécessite RENDER domains (GPU écrit)
    reloc.target_handle = output_bo->handle;
    reloc.delta = 0;
    reloc.offset = 4;  // Position batch[1] (driver patche 64-bit automatiquement)
    reloc.presumed_offset = 0;
    reloc.read_domains = I915_GEM_DOMAIN_RENDER;  // GPU lit l'adresse
    reloc.write_domain = I915_GEM_DOMAIN_RENDER;  // GPU écrit dans output
    
    LOG_FORENSIC("✅ Relocation entry créée (1 entry pour adresse 64-bit)");
    LOG_FORENSIC("  target=output, offset=4");
    LOG_FORENSIC("  read_domains=RENDER, write_domain=RENDER (ROOT CAUSE #86)");
    LOG_FORENSIC("");
    
    // Préparer exec_objects
    struct drm_i915_gem_exec_object2 exec_objects[2];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    // Output buffer (SANS PINNED)
    exec_objects[0].handle = output_bo->handle;
    exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE;
    // offset sera assigné par driver
    
    // Batch buffer avec relocation
    exec_objects[1].handle = batch_bo->handle;
    exec_objects[1].relocation_count = 1;
    exec_objects[1].relocs_ptr = (uintptr_t)&reloc;
    exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    // Préparer execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 2;
    execbuf.batch_len = 20;  // 5 DWords * 4 bytes
    execbuf.flags = I915_EXEC_RENDER;
    
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("EXÉCUTION GPU");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    t0 = get_timestamp_ns();
    int ret = drmIoctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    t1 = get_timestamp_ns();
    
    if (ret != 0) {
        LOG_FORENSIC("❌ EXECBUFFER2 ÉCHOUÉ: ret=%d, errno=%d (%s)", 
                     ret, errno, strerror(errno));
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        drm_intel_bufmgr_destroy(bufmgr);
        close(drm_fd);
        return 1;
    }
    
    LOG_FORENSIC("✅ EXECBUFFER2 SUCCESS! (%.3f µs)", (t1 - t0) / 1000.0);
    LOG_FORENSIC("  output_gtt = 0x%016llx (assigné par driver)", 
                 (unsigned long long)exec_objects[0].offset);
    LOG_FORENSIC("  batch_gtt  = 0x%016llx (assigné par driver)", 
                 (unsigned long long)exec_objects[1].offset);
    LOG_FORENSIC("");
    
    // Attendre GPU
    LOG_FORENSIC("Attente GPU...");
    t0 = get_timestamp_ns();
    drm_intel_bo_wait_rendering(output_bo);
    t1 = get_timestamp_ns();
    LOG_FORENSIC("✅ GPU terminé (%.3f ms)", (t1 - t0) / 1000000.0);
    
    // Synchroniser cache
    drm_intel_gem_bo_start_gtt_access(output_bo, 0);
    LOG_FORENSIC("✅ Cache synchronisé");
    LOG_FORENSIC("");
    
    // Lire résultat
    drm_intel_bo_map(output_bo, 0);
    output = output_bo->virtual;
    
    uint32_t result = output[0];
    
    drm_intel_bo_unmap(output_bo);
    
    // Afficher résultat
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("RÉSULTAT FINAL");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("output[0] = 0x%08X (attendu: 0x12345678)", result);
    LOG_FORENSIC("");
    
    int success = (result == 0x12345678);
    
    if (success) {
        LOG_FORENSIC("🎉🎉🎉 SUCCÈS HISTORIQUE ABSOLU! 🎉🎉🎉");
        LOG_FORENSIC("✅ GPU a écrit 0x12345678!");
        LOG_FORENSIC("✅ ROOT CAUSES #83, #84 et #86 RÉSOLUES!");
        LOG_FORENSIC("✅ PREMIER TEST LUMVORAX QUI PASSE!");
        LOG_FORENSIC("");
        LOG_FORENSIC("CORRECTIONS VALIDÉES:");
        LOG_FORENSIC("  ✓ ROOT CAUSE #83: Fallback card0 → card1");
        LOG_FORENSIC("  ✓ ROOT CAUSE #84: Suppression EXEC_OBJECT_PINNED");
        LOG_FORENSIC("  ✓ ROOT CAUSE #86: Memory domains RENDER/RENDER");
        LOG_FORENSIC("  ✓ Utilisation relocation entries");
        LOG_FORENSIC("  ✓ Allocation GTT dynamique");
    } else {
        LOG_FORENSIC("❌ ÉCHEC: output[0] = 0x%08X != 0x12345678", result);
        LOG_FORENSIC("⚠️  GPU a exécuté mais résultat incorrect");
        LOG_FORENSIC("");
        LOG_FORENSIC("ANALYSE:");
        if (result == 0xFFFFFFFF) {
            LOG_FORENSIC("  → Mémoire non modifiée (pattern initial)");
            LOG_FORENSIC("  → GPU n'a pas écrit ou cache non synchronisé");
        } else if (result == 0x00000000) {
            LOG_FORENSIC("  → Mémoire écrite à 0 (possible clear)");
            LOG_FORENSIC("  → Batch peut ne pas avoir exécuté MI_STORE_DATA_IMM");
        } else {
            LOG_FORENSIC("  → Valeur inattendue: 0x%08X", result);
            LOG_FORENSIC("  → Possible corruption mémoire ou mauvaise adresse");
        }
    }
    
    // Cleanup
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(drm_fd);
    
    LOG_FORENSIC("");
    LOG_FORENSIC("Test C608 terminé");
    
    return success ? 0 : 1;
}

// Made with Bob
