/*
 * Test rapide format Intel: VM + CREATE_EXT + SETPARAM
 * Objectif: Valider que VM élimine limite 3 utilisations par contexte
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <drm/i915_drm.h>
#include <xf86drm.h>

/* Les structures VM sont déjà dans i915_drm.h (drm_i915_gem_vm_control) */

int create_context_with_vm(int fd, uint32_t *vm_id_out, uint32_t *ctx_id_out) {
    int ret;
    
    /* 1. Créer VM */
    struct drm_i915_gem_vm_control vm_create = {0};
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create);
    if (ret < 0) {
        printf("❌ VM_CREATE failed: %s\n", strerror(errno));
        return -1;
    }
    *vm_id_out = vm_create.vm_id;
    printf("✅ VM_CREATE success: vm_id=%u\n", *vm_id_out);
    
    /* 2. Créer contexte EXT */
    struct drm_i915_gem_context_create_ext ctx_create = {0};
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create);
    if (ret < 0) {
        printf("❌ CONTEXT_CREATE_EXT failed: %s\n", strerror(errno));
        struct drm_i915_gem_vm_control vm_destroy = { .vm_id = *vm_id_out };
        ioctl(fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
        return -1;
    }
    *ctx_id_out = ctx_create.ctx_id;
    printf("✅ CONTEXT_CREATE_EXT success: ctx_id=%u\n", *ctx_id_out);
    
    /* 3. Associer VM */
    struct drm_i915_gem_context_param param = {
        .ctx_id = *ctx_id_out,
        .param = I915_CONTEXT_PARAM_VM,
        .value = *vm_id_out
    };
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &param);
    if (ret < 0) {
        printf("❌ SETPARAM VM failed: %s\n", strerror(errno));
        return -1;
    }
    printf("✅ SETPARAM VM success\n");
    
    /* 4. Activer recovery */
    param.param = I915_CONTEXT_PARAM_RECOVERABLE;
    param.value = 1;
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &param);
    if (ret < 0) {
        printf("⚠️  SETPARAM RECOVERABLE failed: %s (non-fatal)\n", strerror(errno));
    } else {
        printf("✅ SETPARAM RECOVERABLE success\n");
    }
    
    return 0;
}

int create_context_simple(int fd, uint32_t *ctx_id_out) {
    struct drm_i915_gem_context_create create = {0};
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
    if (ret < 0) {
        printf("❌ CONTEXT_CREATE failed: %s\n", strerror(errno));
        return -1;
    }
    *ctx_id_out = create.ctx_id;
    printf("✅ CONTEXT_CREATE success: ctx_id=%u\n", *ctx_id_out);
    return 0;
}

void destroy_context_with_vm(int fd, uint32_t vm_id, uint32_t ctx_id) {
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    struct drm_i915_gem_vm_control vm_destroy = { .vm_id = vm_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
}

void destroy_context_simple(int fd, uint32_t ctx_id) {
    struct drm_i915_gem_context_destroy destroy = { .ctx_id = ctx_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &destroy);
}

int main() {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("TEST FORMAT INTEL: VM + CREATE_EXT vs CREATE SIMPLE\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    /* Ouvrir DRM */
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("❌ Cannot open /dev/dri/renderD128\n");
        return 1;
    }
    printf("✅ DRM opened: fd=%d\n\n", fd);
    
    /* TEST 1: Format simple (limite 3 utilisations) */
    printf("TEST 1: Format SIMPLE (CREATE sans VM)\n");
    printf("───────────────────────────────────────────────────────────\n");
    printf("Prédiction: Limite 3 créations/destructions\n\n");
    
    int simple_success = 0;
    for (int i = 0; i < 10; i++) {
        uint32_t ctx_id;
        printf("[%d] Création contexte simple... ", i+1);
        if (create_context_simple(fd, &ctx_id) < 0) {
            printf("❌ ÉCHEC à l'itération %d\n", i+1);
            break;
        }
        destroy_context_simple(fd, ctx_id);
        printf("✅ Destruction OK\n");
        simple_success++;
    }
    printf("\n📊 Résultat: %d/10 créations réussies\n\n", simple_success);
    
    /* TEST 2: Format Intel avec VM (∞ utilisations) */
    printf("TEST 2: Format INTEL (CREATE_EXT avec VM)\n");
    printf("───────────────────────────────────────────────────────────\n");
    printf("Prédiction: ∞ créations/destructions possibles\n\n");
    
    int vm_success = 0;
    for (int i = 0; i < 10; i++) {
        uint32_t vm_id, ctx_id;
        printf("[%d] Création contexte avec VM... ", i+1);
        if (create_context_with_vm(fd, &vm_id, &ctx_id) < 0) {
            printf("❌ ÉCHEC à l'itération %d\n", i+1);
            break;
        }
        destroy_context_with_vm(fd, vm_id, ctx_id);
        printf("✅ Destruction OK\n");
        vm_success++;
    }
    printf("\n📊 Résultat: %d/10 créations réussies\n\n", vm_success);
    
    /* RÉSULTATS */
    printf("═══════════════════════════════════════════════════════════\n");
    printf("RÉSULTATS FINAUX\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Format SIMPLE:  %d/10 (%d%%)\n", simple_success, simple_success*10);
    printf("Format INTEL:   %d/10 (%d%%)\n", vm_success, vm_success*10);
    
    if (vm_success > simple_success) {
        printf("\n✅ HYPOTHÈSE VALIDÉE: Format Intel avec VM élimine limitation\n");
        printf("   Gain: +%d créations possibles (+%d%%)\n", 
               vm_success - simple_success, 
               (vm_success - simple_success) * 10);
    } else if (vm_success == simple_success && vm_success == 10) {
        printf("\n✅ Les deux formats fonctionnent (pas de limitation détectée)\n");
    } else {
        printf("\n❌ HYPOTHÈSE RÉFUTÉE: Format Intel n'améliore pas\n");
    }
    
    close(fd);
    return 0;
}

// Made with Bob
