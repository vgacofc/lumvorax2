/*
 * Test réutilisation contexte: Reproduire limitation 3 utilisations
 * Hypothèse: Problème apparaît quand on RÉUTILISE même contexte, pas quand on recrée
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <drm/i915_drm.h>
#include <xf86drm.h>

/* Batch buffer minimal: MI_BATCH_BUFFER_END */
static const uint32_t batch_data[] = {
    0x0A000000,  /* MI_BATCH_BUFFER_END */
};

int test_context_reuse_simple(int fd) {
    printf("\nTEST 1: RÉUTILISATION contexte SIMPLE (sans VM)\n");
    printf("───────────────────────────────────────────────────────────\n");
    
    /* Créer contexte */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("❌ CONTEXT_CREATE failed: %s\n", strerror(errno));
        return -1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    printf("✅ Contexte créé: ctx_id=%u\n", ctx_id);
    
    /* Créer batch buffer */
    struct drm_i915_gem_create gem_create = {
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        printf("❌ GEM_CREATE failed: %s\n", strerror(errno));
        return -1;
    }
    uint32_t batch_bo = gem_create.handle;
    
    /* Mapper et écrire batch */
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = batch_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        printf("❌ MMAP_OFFSET failed: %s\n", strerror(errno));
        return -1;
    }
    
    void* batch_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    if (batch_map == MAP_FAILED) {
        printf("❌ mmap failed: %s\n", strerror(errno));
        return -1;
    }
    memcpy(batch_map, batch_data, sizeof(batch_data));
    munmap(batch_map, 4096);
    
    /* Réutiliser MÊME contexte 30 fois */
    int success_count = 0;
    for (int i = 0; i < 30; i++) {
        struct drm_i915_gem_exec_object2 exec_obj = {
            .handle = batch_bo,
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
            .batch_len = sizeof(batch_data),
            .flags = I915_EXEC_RENDER,
            .rsvd1 = ctx_id,  /* RÉUTILISE MÊME CONTEXTE */
            .rsvd2 = 0
        };
        
        int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        if (ret < 0) {
            printf("[%d] ❌ EXECBUFFER2 failed: %s (errno=%d)\n", i+1, strerror(errno), errno);
            break;
        }
        
        /* Wait completion */
        struct drm_i915_gem_wait wait_arg = {
            .bo_handle = batch_bo,
            .timeout_ns = 1000000000,  /* 1 seconde */
            .flags = 0
        };
        ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg);
        
        printf("[%d] ✅ Dispatch OK\n", i+1);
        success_count++;
    }
    
    /* Cleanup */
    struct drm_gem_close gem_close = { .handle = batch_bo };
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    printf("\n📊 Résultat: %d/30 dispatches réussis\n", success_count);
    return success_count;
}

int test_context_reuse_with_vm(int fd) {
    printf("\nTEST 2: RÉUTILISATION contexte avec VM\n");
    printf("───────────────────────────────────────────────────────────\n");
    
    /* Créer VM */
    struct drm_i915_gem_vm_control vm_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create) < 0) {
        printf("❌ VM_CREATE failed: %s\n", strerror(errno));
        return -1;
    }
    uint32_t vm_id = vm_create.vm_id;
    printf("✅ VM créée: vm_id=%u\n", vm_id);
    
    /* Créer contexte EXT */
    struct drm_i915_gem_context_create_ext ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create) < 0) {
        printf("❌ CONTEXT_CREATE_EXT failed: %s\n", strerror(errno));
        return -1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    printf("✅ Contexte créé: ctx_id=%u\n", ctx_id);
    
    /* Associer VM */
    struct drm_i915_gem_context_param param = {
        .ctx_id = ctx_id,
        .param = I915_CONTEXT_PARAM_VM,
        .value = vm_id
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &param) < 0) {
        printf("❌ SETPARAM VM failed: %s\n", strerror(errno));
        return -1;
    }
    printf("✅ VM associée au contexte\n");
    
    /* Créer batch buffer */
    struct drm_i915_gem_create gem_create = {
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        printf("❌ GEM_CREATE failed: %s\n", strerror(errno));
        return -1;
    }
    uint32_t batch_bo = gem_create.handle;
    
    /* Mapper et écrire batch */
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = batch_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        printf("❌ MMAP_OFFSET failed: %s\n", strerror(errno));
        return -1;
    }
    
    void* batch_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    if (batch_map == MAP_FAILED) {
        printf("❌ mmap failed: %s\n", strerror(errno));
        return -1;
    }
    memcpy(batch_map, batch_data, sizeof(batch_data));
    munmap(batch_map, 4096);
    
    /* Réutiliser MÊME contexte 30 fois */
    int success_count = 0;
    for (int i = 0; i < 30; i++) {
        struct drm_i915_gem_exec_object2 exec_obj = {
            .handle = batch_bo,
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
            .batch_len = sizeof(batch_data),
            .flags = I915_EXEC_RENDER,
            .rsvd1 = ctx_id,  /* RÉUTILISE MÊME CONTEXTE */
            .rsvd2 = 0
        };
        
        int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        if (ret < 0) {
            printf("[%d] ❌ EXECBUFFER2 failed: %s (errno=%d)\n", i+1, strerror(errno), errno);
            break;
        }
        
        /* Wait completion */
        struct drm_i915_gem_wait wait_arg = {
            .bo_handle = batch_bo,
            .timeout_ns = 1000000000,
            .flags = 0
        };
        ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg);
        
        printf("[%d] ✅ Dispatch OK\n", i+1);
        success_count++;
    }
    
    /* Cleanup */
    struct drm_gem_close gem_close = { .handle = batch_bo };
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    struct drm_i915_gem_vm_control vm_destroy = { .vm_id = vm_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
    
    printf("\n📊 Résultat: %d/30 dispatches réussis\n", success_count);
    return success_count;
}

int main() {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("TEST RÉUTILISATION CONTEXTE: Simple vs VM\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("❌ Cannot open /dev/dri/renderD128\n");
        return 1;
    }
    printf("✅ DRM opened: fd=%d\n", fd);
    
    int simple_count = test_context_reuse_simple(fd);
    int vm_count = test_context_reuse_with_vm(fd);
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("RÉSULTATS FINAUX\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Format SIMPLE:  %d/30 dispatches\n", simple_count);
    printf("Format VM:      %d/30 dispatches\n", vm_count);
    
    if (vm_count > simple_count) {
        printf("\n✅ HYPOTHÈSE VALIDÉE: VM permet plus de réutilisations\n");
        printf("   Gain: +%d dispatches\n", vm_count - simple_count);
    } else if (simple_count == vm_count && simple_count == 30) {
        printf("\n✅ Les deux formats permettent 30 réutilisations\n");
    } else {
        printf("\n⚠️  Résultat inattendu\n");
    }
    
    close(fd);
    return 0;
}

// Made with Bob
