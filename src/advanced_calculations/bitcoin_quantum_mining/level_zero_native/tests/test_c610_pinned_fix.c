// Test C610 - Fix ROOT CAUSE #87 avec EXEC_OBJECT_PINNED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <drm/i915_drm.h>
#include <time.h>

#define EXEC_OBJECT_PINNED (1<<4)
#define EXEC_OBJECT_WRITE (1<<2)
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS (1<<3)

int main(int argc, char **argv) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C610 - FIX ROOT CAUSE #87                           ║\n");
    printf("║  EXEC_OBJECT_PINNED sur output buffer                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            printf("❌ Erreur: Impossible d'ouvrir DRM\n");
            return 1;
        }
    }
    printf("✅ DRM ouvert: fd=%d\n", fd);
    
    // Créer context
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("❌ Erreur: Context creation failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Context créé: ctx_id=%u\n", ctx_create.ctx_id);
    
    // Allouer output buffer
    struct drm_i915_gem_create create = {
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        printf("❌ Erreur: GEM_CREATE failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Output buffer créé: handle=%u\n", create.handle);
    
    // Écrire sentinel
    uint32_t sentinel = 0xDEADBEEF;
    struct drm_i915_gem_pwrite pwrite = {
        .handle = create.handle,
        .offset = 0,
        .size = sizeof(sentinel),
        .data_ptr = (uint64_t)&sentinel
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        printf("❌ Erreur: PWRITE failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Sentinel écrit: 0x%08X\n", sentinel);
    
    // Créer batch buffer minimal (écriture 0x12345678)
    struct drm_i915_gem_create batch_create = {
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) < 0) {
        printf("❌ Erreur: Batch GEM_CREATE failed\n");
        close(fd);
        return 1;
    }
    
    uint32_t batch[] = {
        0x7A000004,  // MI_STORE_DATA_IMM
        0x00100000,  // Address low (sera relocalisé)
        0x00000000,  // Address high
        0x12345678,  // Data
        0x00000000,  // Data high
        0x05000000   // MI_BATCH_BUFFER_END
    };
    
    struct drm_i915_gem_pwrite batch_pwrite = {
        .handle = batch_create.handle,
        .offset = 0,
        .size = sizeof(batch),
        .data_ptr = (uint64_t)batch
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &batch_pwrite) < 0) {
        printf("❌ Erreur: Batch PWRITE failed\n");
        close(fd);
        return 1;
    }
    printf("✅ Batch buffer créé et écrit\n");
    
    // Configuration EXEC avec PINNED
    struct drm_i915_gem_exec_object2 exec_objects[2] = {
        {
            .handle = create.handle,
            .offset = 0x0000000000100000,  // Adresse fixe
            .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE | EXEC_OBJECT_SUPPORTS_48B_ADDRESS
        },
        {
            .handle = batch_create.handle,
            .offset = 0x0000000000040000,
            .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS
        }
    };
    
    struct drm_i915_gem_relocation_entry reloc = {
        .target_handle = create.handle,
        .delta = 0,
        .offset = 4,  // Offset dans batch (address low)
        .presumed_offset = 0x100000,
        .read_domains = 0,
        .write_domain = 0x00000001  // I915_GEM_DOMAIN_RENDER
    };
    
    exec_objects[1].relocation_count = 1;
    exec_objects[1].relocs_ptr = (uint64_t)&reloc;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 2,
        .batch_start_offset = 0,
        .batch_len = sizeof(batch),
        .flags = 0,
        .rsvd1 = ctx_create.ctx_id
    };
    
    printf("\n[PRE-EXEC] Output offset: 0x%016llx (PINNED)\n", exec_objects[0].offset);
    printf("[PRE-EXEC] Batch offset:  0x%016llx (PINNED)\n", exec_objects[1].offset);
    
    // EXECBUFFER2
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        printf("❌ Erreur: EXECBUFFER2 failed (errno=%d)\n", errno);
        close(fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("\n[POST-EXEC] Output offset: 0x%016llx\n", exec_objects[0].offset);
    printf("[POST-EXEC] Batch offset:  0x%016llx\n", exec_objects[1].offset);
    printf("✅ EXECBUFFER2 réussi (%.6f sec)\n", elapsed);
    
    // Vérifier si offset a changé
    if (exec_objects[0].offset != 0x100000) {
        printf("⚠️  WARNING: Output offset a changé! (attendu: 0x100000)\n");
    } else {
        printf("✅ Output offset STABLE @ 0x100000 (PINNED fonctionne!)\n");
    }
    
    // Lire résultat
    uint32_t result = 0;
    struct drm_i915_gem_pread pread = {
        .handle = create.handle,
        .offset = 0,
        .size = sizeof(result),
        .data_ptr = (uint64_t)&result
    };
    
    // Wait GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = create.handle,
        .timeout_ns = 1000000000  // 1 seconde
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PREAD, &pread) < 0) {
        printf("❌ Erreur: PREAD failed\n");
        close(fd);
        return 1;
    }
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS TEST C610                                       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("Sentinel avant:  0x%08X\n", sentinel);
    printf("Résultat après:  0x%08X\n", result);
    
    if (result == 0x12345678) {
        printf("\n✅ SUCCESS: GPU a écrit la valeur correcte!\n");
        printf("✅ ROOT CAUSE #87 CORRIGÉ avec EXEC_OBJECT_PINNED!\n");
        close(fd);
        return 0;
    } else if (result != sentinel) {
        printf("\n⚠️  PARTIAL: GPU a écrit mais valeur incorrecte\n");
        close(fd);
        return 2;
    } else {
        printf("\n❌ FAIL: GPU n'a pas écrit (sentinel intact)\n");
        close(fd);
        return 1;
    }
}
