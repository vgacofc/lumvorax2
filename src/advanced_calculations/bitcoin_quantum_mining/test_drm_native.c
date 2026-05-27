/* test_drm_native.c - Test accès GPU réel via DRM minimal */

#include "lum_drm_native.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  TEST ACCÈS GPU RÉEL VIA DRM MINIMAL\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    /* Init DRM */
    lum_drm_context_t ctx;
    if (lum_drm_init(&ctx) < 0) {
        fprintf(stderr, "❌ DRM init failed\n");
        return 1;
    }
    
    printf("✅ DRM initialized\n");
    printf("   Driver: %s\n", lum_drm_get_version(&ctx));
    printf("   i915: %s\n\n", lum_drm_check_i915(&ctx) ? "YES" : "NO");
    
    /* Test GEM buffer */
    printf("=== TEST GEM BUFFER ===\n");
    uint32_t handle;
    if (lum_drm_gem_create(&ctx, 4096, &handle) < 0) {
        fprintf(stderr, "❌ GEM create failed\n");
        lum_drm_cleanup(&ctx);
        return 1;
    }
    
    printf("✅ GEM buffer created\n\n");
    
    /* Test mmap */
    printf("=== TEST MMAP ===\n");
    void *addr;
    if (lum_drm_gem_mmap(&ctx, handle, 4096, &addr) < 0) {
        fprintf(stderr, "❌ GEM mmap failed\n");
        lum_drm_gem_close(&ctx, handle);
        lum_drm_cleanup(&ctx);
        return 1;
    }
    
    printf("✅ GEM buffer mapped\n\n");
    
    /* Cleanup */
    lum_drm_gem_close(&ctx, handle);
    lum_drm_cleanup(&ctx);
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✅ TOUS LES TESTS PASSÉS\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}

// Made with Bob
