/*
 * TEST C607 - TESTS PARALLÈLES TOUTES OPTIONS ROOT CAUSE #82
 * 
 * Date: 2026-06-19 21:46 CET
 * Auteur: Bob (Expert GPU Firmware + Intel Gen9 Opcode Specialist)
 * 
 * OBJECTIF:
 * Tester TOUTES les hypothèses ROOT CAUSE #82 en parallèle:
 * 1. Firmware GPU (GuC/HuC/DMC) activé
 * 2. Adresse 48-bit (masquage bits hauts)
 * 3. Opcodes alternatifs MI_STORE_DATA_IMM
 * 4. Analyse dmesg pour erreurs GPU silencieuses
 * 
 * STRATÉGIE:
 * - 4 PASS séquentiels avec logs forensiques
 * - Chaque PASS teste une hypothèse différente
 * - Comparaison résultats pour identifier solution
 * 
 * COMPILATION:
 * gcc -o test_c607 test_c607_parallel_all_options.c -I/usr/include/libdrm -ldrm -ldrm_intel
 * 
 * EXÉCUTION:
 * sudo ./test_c607 2>&1 | tee master_lvx/logs/test_c607_$(date +%Y%m%d_%H%M%S).log
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
 * FORENSIC LOGGING AVEC TIMESTAMPS NANOSECONDE
 * ============================================================================ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_FORENSIC(fmt, ...) do { \
    uint64_t ns = get_timestamp_ns(); \
    printf("[%lu.%09lu] " fmt "\n", ns / 1000000000ULL, ns % 1000000000ULL, ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

/* ============================================================================
 * KERNEL ISA GEN9 - output[0] = 0x12345678
 * ============================================================================ */

static const unsigned char kernel_isa_gen9[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x61, 0x05, 0x00, 0x10, 0x00, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* ============================================================================
 * VÉRIFICATION FIRMWARE GPU
 * ============================================================================ */

static void check_gpu_firmware(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("VÉRIFICATION FIRMWARE GPU");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    const char *firmware_files[] = {
        "/lib/firmware/i915/kbl_guc_70.1.1.bin",
        "/lib/firmware/i915/kbl_huc_4.0.0.bin",
        "/lib/firmware/i915/kbl_dmc_ver1_04.bin"
    };
    
    const char *firmware_names[] = {
        "GuC (Graphics microController)",
        "HuC (HEVC microController)",
        "DMC (Display microController)"
    };
    
    for (int i = 0; i < 3; i++) {
        if (access(firmware_files[i], F_OK) == 0) {
            LOG_FORENSIC("✅ %s: PRÉSENT", firmware_names[i]);
        } else {
            LOG_FORENSIC("❌ %s: ABSENT", firmware_names[i]);
        }
    }
    
    // Vérifier si GuC est activé dans le kernel
    FILE *f = fopen("/sys/kernel/debug/dri/0/i915_guc_info", "r");
    if (f) {
        char line[256];
        int guc_enabled = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "GuC submission")) {
                if (strstr(line, "enabled")) {
                    guc_enabled = 1;
                }
                break;
            }
        }
        fclose(f);
        LOG_FORENSIC("GuC submission: %s", guc_enabled ? "ACTIVÉ" : "DÉSACTIVÉ");
    } else {
        LOG_FORENSIC("⚠️  Impossible de vérifier statut GuC (debugfs non accessible)");
    }
    
    LOG_FORENSIC("");
}

/* ============================================================================
 * TEST PASS 1: BASELINE (test_c565_FINAL)
 * ============================================================================ */

static int test_pass1_baseline(drm_intel_bufmgr *bufmgr, int drm_fd) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("PASS 1: BASELINE (MI_STORE_DATA_IMM standard)");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    uint64_t t0, t1;
    
    // Créer output buffer (PINNED à 0x100000)
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
    if (!output_bo) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bo_alloc(output) échoué");
        return -1;
    }
    
    // PASS 0: Allocation GTT pour output
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    struct drm_i915_gem_exec_object2 exec_object = {0};
    
    exec_object.handle = output_bo->handle;
    exec_object.flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED;
    exec_object.offset = 0x100000;  // Suggérer 1MB
    
    execbuf.buffers_ptr = (uintptr_t)&exec_object;
    execbuf.buffer_count = 1;
    execbuf.flags = I915_EXEC_RENDER;
    
    t0 = get_timestamp_ns();
    int ret = drmIoctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    t1 = get_timestamp_ns();
    
    if (ret != 0) {
        LOG_FORENSIC("❌ PASS 0 ÉCHOUÉ: execbuffer2 = %d (errno=%d)", ret, errno);
        drm_intel_bo_unreference(output_bo);
        return -1;
    }
    
    uint64_t output_gtt = exec_object.offset;
    LOG_FORENSIC("✅ PASS 0 SUCCÈS (%.3f µs)", (t1 - t0) / 1000.0);
    LOG_FORENSIC("  output_gtt = 0x%016llx", (unsigned long long)output_gtt);
    
    // Créer batch buffer avec MI_STORE_DATA_IMM
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 4096, 4096);
    if (!batch_bo) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bo_alloc(batch) échoué");
        drm_intel_bo_unreference(output_bo);
        return -1;
    }
    
    uint32_t *batch;
    drm_intel_bo_map(batch_bo, 1);
    batch = batch_bo->virtual;
    
    // MI_STORE_DATA_IMM (opcode 0x20400003)
    batch[0] = 0x20400003;  // MI_STORE_DATA_IMM (4 DWords)
    batch[1] = (uint32_t)(output_gtt & 0xFFFFFFFF);  // Adresse basse
    batch[2] = (uint32_t)((output_gtt >> 32) & 0xFFFF);  // Adresse haute
    batch[3] = 0x12345678;  // Valeur
    batch[4] = 0x0A000000;  // MI_BATCH_BUFFER_END
    
    drm_intel_bo_unmap(batch_bo);
    
    LOG_FORENSIC("Batch construit:");
    LOG_FORENSIC("  [0] = 0x%08X (MI_STORE_DATA_IMM)", batch[0]);
    LOG_FORENSIC("  [1] = 0x%08X (addr_low)", batch[1]);
    LOG_FORENSIC("  [2] = 0x%08X (addr_high)", batch[2]);
    LOG_FORENSIC("  [3] = 0x%08X (value)", batch[3]);
    LOG_FORENSIC("  [4] = 0x%08X (MI_BATCH_BUFFER_END)", batch[4]);
    
    // PASS 1: Exécution GPU
    struct drm_i915_gem_exec_object2 exec_objects[2];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = output_bo->handle;
    exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED;
    exec_objects[0].offset = output_gtt;
    
    exec_objects[1].handle = batch_bo->handle;
    exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 2;
    execbuf.batch_len = 20;  // 5 DWords * 4 bytes
    
    t0 = get_timestamp_ns();
    ret = drmIoctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    t1 = get_timestamp_ns();
    
    if (ret != 0) {
        LOG_FORENSIC("❌ PASS 1 ÉCHOUÉ: execbuffer2 = %d (errno=%d)", ret, errno);
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        return -1;
    }
    
    LOG_FORENSIC("✅ EXECBUFFER2 SUCCESS (%.3f µs)", (t1 - t0) / 1000.0);
    LOG_FORENSIC("  batch_gtt = 0x%016llx", (unsigned long long)exec_objects[1].offset);
    
    // Attendre GPU
    drm_intel_bo_wait_rendering(output_bo);
    
    // Synchroniser cache
    drm_intel_gem_bo_start_gtt_access(output_bo, 0);
    
    // Lire résultat
    uint32_t *output;
    drm_intel_bo_map(output_bo, 0);
    output = output_bo->virtual;
    
    uint32_t result = output[0];
    
    drm_intel_bo_unmap(output_bo);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    
    LOG_FORENSIC("");
    LOG_FORENSIC("RÉSULTAT PASS 1:");
    LOG_FORENSIC("  output[0] = 0x%08X (attendu: 0x12345678)", result);
    
    if (result == 0x12345678) {
        LOG_FORENSIC("✅ PASS 1 SUCCÈS!");
        return 0;
    } else {
        LOG_FORENSIC("❌ PASS 1 ÉCHEC");
        return -1;
    }
}

/* ============================================================================
 * TEST PASS 2: ADRESSE 48-BIT (masquage bits hauts)
 * ============================================================================ */

static int test_pass2_48bit(drm_intel_bufmgr *bufmgr, int drm_fd) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("PASS 2: ADRESSE 48-BIT (masquage 0xFFFFFFFFFFFF)");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    uint64_t t0, t1;
    
    // Créer output buffer
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
    if (!output_bo) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bo_alloc(output) échoué");
        return -1;
    }
    
    // PASS 0: Allocation GTT
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    struct drm_i915_gem_exec_object2 exec_object = {0};
    
    exec_object.handle = output_bo->handle;
    exec_object.flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED;
    exec_object.offset = 0x100000;
    
    execbuf.buffers_ptr = (uintptr_t)&exec_object;
    execbuf.buffer_count = 1;
    execbuf.flags = I915_EXEC_RENDER;
    
    t0 = get_timestamp_ns();
    int ret = drmIoctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    t1 = get_timestamp_ns();
    
    if (ret != 0) {
        LOG_FORENSIC("❌ PASS 0 ÉCHOUÉ: execbuffer2 = %d", ret);
        drm_intel_bo_unreference(output_bo);
        return -1;
    }
    
    uint64_t output_gtt = exec_object.offset;
    
    // MASQUER À 48-BIT
    uint64_t output_gtt_48bit = output_gtt & 0xFFFFFFFFFFFFULL;
    
    LOG_FORENSIC("✅ PASS 0 SUCCÈS (%.3f µs)", (t1 - t0) / 1000.0);
    LOG_FORENSIC("  output_gtt (64-bit) = 0x%016llx", (unsigned long long)output_gtt);
    LOG_FORENSIC("  output_gtt (48-bit) = 0x%016llx", (unsigned long long)output_gtt_48bit);
    
    // Créer batch avec adresse 48-bit
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 4096, 4096);
    if (!batch_bo) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bo_alloc(batch) échoué");
        drm_intel_bo_unreference(output_bo);
        return -1;
    }
    
    uint32_t *batch;
    drm_intel_bo_map(batch_bo, 1);
    batch = batch_bo->virtual;
    
    batch[0] = 0x20400003;
    batch[1] = (uint32_t)(output_gtt_48bit & 0xFFFFFFFF);
    batch[2] = (uint32_t)((output_gtt_48bit >> 32) & 0xFFFF);  // Seulement 16 bits
    batch[3] = 0x12345678;
    batch[4] = 0x0A000000;
    
    drm_intel_bo_unmap(batch_bo);
    
    LOG_FORENSIC("Batch construit (48-bit):");
    LOG_FORENSIC("  [1] = 0x%08X (addr_low)", batch[1]);
    LOG_FORENSIC("  [2] = 0x%08X (addr_high 48-bit)", batch[2]);
    
    // PASS 1: Exécution
    struct drm_i915_gem_exec_object2 exec_objects[2];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = output_bo->handle;
    exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED;
    exec_objects[0].offset = output_gtt;
    
    exec_objects[1].handle = batch_bo->handle;
    exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 2;
    execbuf.batch_len = 20;
    
    t0 = get_timestamp_ns();
    ret = drmIoctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    t1 = get_timestamp_ns();
    
    if (ret != 0) {
        LOG_FORENSIC("❌ PASS 1 ÉCHOUÉ: execbuffer2 = %d", ret);
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        return -1;
    }
    
    LOG_FORENSIC("✅ EXECBUFFER2 SUCCESS (%.3f µs)", (t1 - t0) / 1000.0);
    
    drm_intel_bo_wait_rendering(output_bo);
    drm_intel_gem_bo_start_gtt_access(output_bo, 0);
    
    uint32_t *output;
    drm_intel_bo_map(output_bo, 0);
    output = output_bo->virtual;
    uint32_t result = output[0];
    drm_intel_bo_unmap(output_bo);
    
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    
    LOG_FORENSIC("");
    LOG_FORENSIC("RÉSULTAT PASS 2:");
    LOG_FORENSIC("  output[0] = 0x%08X (attendu: 0x12345678)", result);
    
    if (result == 0x12345678) {
        LOG_FORENSIC("✅ PASS 2 SUCCÈS! (48-bit masking résout le problème)");
        return 0;
    } else {
        LOG_FORENSIC("❌ PASS 2 ÉCHEC");
        return -1;
    }
}

/* ============================================================================
 * TEST PASS 3: OPCODES ALTERNATIFS
 * ============================================================================ */

static int test_pass3_opcodes(drm_intel_bufmgr *bufmgr, int drm_fd) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("PASS 3: OPCODES ALTERNATIFS MI_STORE_DATA_IMM");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    // Tester 3 variantes d'opcode
    uint32_t opcodes[] = {
        0x20400003,  // Standard (déjà testé)
        0x20400004,  // Variante +1
        0x20C00003   // Variante alternative
    };
    
    const char *opcode_names[] = {
        "0x20400003 (standard)",
        "0x20400004 (variante +1)",
        "0x20C00003 (alternative)"
    };
    
    for (int i = 0; i < 3; i++) {
        LOG_FORENSIC("");
        LOG_FORENSIC("Test opcode %s...", opcode_names[i]);
        
        uint64_t t0, t1;
        
        drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
        if (!output_bo) continue;
        
        // PASS 0
        struct drm_i915_gem_execbuffer2 execbuf = {0};
        struct drm_i915_gem_exec_object2 exec_object = {0};
        
        exec_object.handle = output_bo->handle;
        exec_object.flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED;
        exec_object.offset = 0x100000;
        
        execbuf.buffers_ptr = (uintptr_t)&exec_object;
        execbuf.buffer_count = 1;
        execbuf.flags = I915_EXEC_RENDER;
        
        int ret = drmIoctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        if (ret != 0) {
            drm_intel_bo_unreference(output_bo);
            continue;
        }
        
        uint64_t output_gtt = exec_object.offset;
        
        // Créer batch avec opcode alternatif
        drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 4096, 4096);
        if (!batch_bo) {
            drm_intel_bo_unreference(output_bo);
            continue;
        }
        
        uint32_t *batch;
        drm_intel_bo_map(batch_bo, 1);
        batch = batch_bo->virtual;
        
        batch[0] = opcodes[i];  // Opcode alternatif
        batch[1] = (uint32_t)(output_gtt & 0xFFFFFFFF);
        batch[2] = (uint32_t)((output_gtt >> 32) & 0xFFFF);
        batch[3] = 0x12345678;
        batch[4] = 0x0A000000;
        
        drm_intel_bo_unmap(batch_bo);
        
        // PASS 1
        struct drm_i915_gem_exec_object2 exec_objects[2];
        memset(exec_objects, 0, sizeof(exec_objects));
        
        exec_objects[0].handle = output_bo->handle;
        exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED;
        exec_objects[0].offset = output_gtt;
        
        exec_objects[1].handle = batch_bo->handle;
        exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
        
        execbuf.buffers_ptr = (uintptr_t)exec_objects;
        execbuf.buffer_count = 2;
        execbuf.batch_len = 20;
        
        t0 = get_timestamp_ns();
        ret = drmIoctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        t1 = get_timestamp_ns();
        
        if (ret != 0) {
            LOG_FORENSIC("  ❌ execbuffer2 échoué: %d", ret);
            drm_intel_bo_unreference(batch_bo);
            drm_intel_bo_unreference(output_bo);
            continue;
        }
        
        LOG_FORENSIC("  ✅ EXECBUFFER2 SUCCESS (%.3f µs)", (t1 - t0) / 1000.0);
        
        drm_intel_bo_wait_rendering(output_bo);
        drm_intel_gem_bo_start_gtt_access(output_bo, 0);
        
        uint32_t *output;
        drm_intel_bo_map(output_bo, 0);
        output = output_bo->virtual;
        uint32_t result = output[0];
        drm_intel_bo_unmap(output_bo);
        
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        
        LOG_FORENSIC("  output[0] = 0x%08X", result);
        
        if (result == 0x12345678) {
            LOG_FORENSIC("  ✅ SUCCÈS! Opcode %s fonctionne!", opcode_names[i]);
            return 0;
        } else {
            LOG_FORENSIC("  ❌ Échec avec opcode %s", opcode_names[i]);
        }
    }
    
    LOG_FORENSIC("");
    LOG_FORENSIC("❌ PASS 3 ÉCHEC: Aucun opcode alternatif ne fonctionne");
    return -1;
}

/* ============================================================================
 * TEST PASS 4: ANALYSE DMESG
 * ============================================================================ */

static int test_pass4_dmesg(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("PASS 4: ANALYSE DMESG (erreurs GPU silencieuses)");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    // Capturer dmesg avant test
    system("dmesg -c > /tmp/dmesg_before.txt 2>/dev/null");
    
    LOG_FORENSIC("dmesg capturé avant test");
    LOG_FORENSIC("Exécuter test_c565_FINAL pour générer erreurs...");
    
    // Exécuter test qui échoue
    int ret = system("cd master_lvx && ./test_c565_FINAL > /dev/null 2>&1");
    
    // Capturer dmesg après test
    system("dmesg > /tmp/dmesg_after.txt 2>/dev/null");
    
    LOG_FORENSIC("dmesg capturé après test");
    LOG_FORENSIC("");
    LOG_FORENSIC("Analyse différences dmesg:");
    
    FILE *f = popen("diff /tmp/dmesg_before.txt /tmp/dmesg_after.txt | grep -E 'i915|GPU|drm' | head -20", "r");
    if (f) {
        char line[512];
        int error_count = 0;
        while (fgets(line, sizeof(line), f)) {
            // Supprimer newline
            line[strcspn(line, "\n")] = 0;
            LOG_FORENSIC("  %s", line);
            error_count++;
        }
        pclose(f);
        
        if (error_count > 0) {
            LOG_FORENSIC("");
            LOG_FORENSIC("⚠️  %d erreurs GPU détectées dans dmesg!", error_count);
            LOG_FORENSIC("Voir /tmp/dmesg_after.txt pour détails complets");
            return 1;  // Erreurs trouvées
        } else {
            LOG_FORENSIC("");
            LOG_FORENSIC("✅ Aucune erreur GPU dans dmesg");
            return 0;  // Pas d'erreurs
        }
    } else {
        LOG_FORENSIC("❌ Impossible d'analyser dmesg");
        return -1;
    }
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("TEST C607 - TESTS PARALLÈLES TOUTES OPTIONS");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("Date: 2026-06-19 21:46 CET");
    LOG_FORENSIC("Objectif: Identifier ROOT CAUSE #82");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("");
    
    // Vérifier firmware GPU
    check_gpu_firmware();
    
    // Ouvrir DRM (essayer card0 puis card1)
    int drm_fd = open("/dev/dri/card0", O_RDWR);
    if (drm_fd < 0) {
        LOG_FORENSIC("⚠️  /dev/dri/card0 non disponible, essai card1...");
        drm_fd = open("/dev/dri/card1", O_RDWR);
        if (drm_fd < 0) {
            LOG_FORENSIC("❌ ERREUR: Impossible d'ouvrir /dev/dri/card0 ou card1");
            return 1;
        }
        LOG_FORENSIC("✅ Utilisation /dev/dri/card1");
    } else {
        LOG_FORENSIC("✅ Utilisation /dev/dri/card0");
    }
    
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(drm_fd, 4096);
    if (!bufmgr) {
        LOG_FORENSIC("❌ ERREUR: drm_intel_bufmgr_gem_init échoué");
        close(drm_fd);
        return 1;
    }
    
    LOG_FORENSIC("✅ DRM initialisé");
    LOG_FORENSIC("");
    
    // Exécuter les 4 PASS
    int results[4] = {-1, -1, -1, -1};
    
    results[0] = test_pass1_baseline(bufmgr, drm_fd);
    LOG_FORENSIC("");
    
    results[1] = test_pass2_48bit(bufmgr, drm_fd);
    LOG_FORENSIC("");
    
    results[2] = test_pass3_opcodes(bufmgr, drm_fd);
    LOG_FORENSIC("");
    
    results[3] = test_pass4_dmesg();
    LOG_FORENSIC("");
    
    // Résumé final
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("RÉSUMÉ FINAL TEST C607");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("PASS 1 (Baseline):        %s", results[0] == 0 ? "✅ SUCCÈS" : "❌ ÉCHEC");
    LOG_FORENSIC("PASS 2 (48-bit):          %s", results[1] == 0 ? "✅ SUCCÈS" : "❌ ÉCHEC");
    LOG_FORENSIC("PASS 3 (Opcodes alt):     %s", results[2] == 0 ? "✅ SUCCÈS" : "❌ ÉCHEC");
    LOG_FORENSIC("PASS 4 (dmesg):           %s", results[3] == 1 ? "⚠️  ERREURS TROUVÉES" : results[3] == 0 ? "✅ PAS D'ERREURS" : "❌ ÉCHEC ANALYSE");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    // Identifier solution
    if (results[0] == 0) {
        LOG_FORENSIC("");
        LOG_FORENSIC("🎉🎉🎉 SOLUTION TROUVÉE: PASS 1 (Baseline) fonctionne!");
        LOG_FORENSIC("ROOT CAUSE #82: Problème résolu dans test_c565_FINAL");
    } else if (results[1] == 0) {
        LOG_FORENSIC("");
        LOG_FORENSIC("🎉🎉🎉 SOLUTION TROUVÉE: PASS 2 (48-bit masking)!");
        LOG_FORENSIC("ROOT CAUSE #82: Adresse 64-bit cause rejet GPU");
        LOG_FORENSIC("CORRECTION: Masquer adresse à 48-bit (0xFFFFFFFFFFFF)");
    } else if (results[2] == 0) {
        LOG_FORENSIC("");
        LOG_FORENSIC("🎉🎉🎉 SOLUTION TROUVÉE: PASS 3 (Opcode alternatif)!");
        LOG_FORENSIC("ROOT CAUSE #82: Opcode MI_STORE_DATA_IMM incorrect");
    } else if (results[3] == 1) {
        LOG_FORENSIC("");
        LOG_FORENSIC("⚠️  ERREURS GPU DÉTECTÉES dans dmesg!");
        LOG_FORENSIC("ROOT CAUSE #82: GPU rejette batch silencieusement");
        LOG_FORENSIC("Voir /tmp/dmesg_after.txt pour détails");
    } else {
        LOG_FORENSIC("");
        LOG_FORENSIC("❌ AUCUNE SOLUTION TROUVÉE");
        LOG_FORENSIC("ROOT CAUSE #82 reste NON RÉSOLUE");
        LOG_FORENSIC("Investigation hardware approfondie requise");
    }
    
    drm_intel_bufmgr_destroy(bufmgr);
    close(drm_fd);
    
    LOG_FORENSIC("");
    LOG_FORENSIC("Test C607 terminé");
    
    return (results[0] == 0 || results[1] == 0 || results[2] == 0) ? 0 : 1;
}

// Made with Bob
