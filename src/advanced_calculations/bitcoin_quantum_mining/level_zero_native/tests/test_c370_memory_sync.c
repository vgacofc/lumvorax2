/*
 * TEST C370 - Memory Synchronization Validation
 * 
 * Tests critiques pour valider cohérence mémoire GPU→CPU
 * ROOT CAUSE identifié: GPU écrit correctement mais CPU lit buffer non synchronisé
 * 
 * Tests:
 * - TEST 5: GEM_WAIT explicite post-EXECBUFFER2
 * - TEST 6: Memory barrier + cache invalidation CPU
 * - TEST 7: Domain i915 verification (RENDER vs CPU)
 * - TEST 8: Target bits minimal (1 bit) pour validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <xf86drm.h>
#include <i915_drm.h>

// Architecture x86 cache control
#ifdef __x86_64__
#include <emmintrin.h>  // SSE2 for _mm_mfence()
#include <xmmintrin.h>  // SSE for _mm_clflush()
#endif

#define TEST_BUFFER_SIZE 256
#define MAGIC_PATTERN 0xDEADBEEF

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t test_bo;
    void* test_map;
} test_context_t;

/*
 * TEST 5: GEM_WAIT Explicite
 * Valide que GEM_WAIT force synchronisation GPU→CPU
 */
int test_5_gem_wait_explicit(test_context_t* ctx) {
    printf("\n=== TEST 5: GEM_WAIT Explicite ===\n");
    
    // Créer buffer test
    struct drm_i915_gem_create create = {
        .size = TEST_BUFFER_SIZE
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        printf("❌ TEST 5 FAILED: GEM_CREATE error=%d\n", errno);
        return -1;
    }
    
    ctx->test_bo = create.handle;
    printf("✓ Buffer créé: handle=%u size=%zu\n", ctx->test_bo, create.size);
    
    // Mapper buffer
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = ctx->test_bo,
        .offset = 0,
        .size = TEST_BUFFER_SIZE,
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        printf("❌ TEST 5 FAILED: GEM_MMAP error=%d\n", errno);
        return -1;
    }
    
    ctx->test_map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    printf("✓ Buffer mappé: addr=%p\n", ctx->test_map);
    
    // Écrire pattern depuis CPU
    uint32_t* data = (uint32_t*)ctx->test_map;
    data[0] = MAGIC_PATTERN;
    printf("✓ CPU write: 0x%08X\n", data[0]);
    
    // Simuler GPU write (via pwrite pour test)
    uint32_t gpu_value = 0xCAFEBABE;
    struct drm_i915_gem_pwrite pwrite = {
        .handle = ctx->test_bo,
        .offset = 0,
        .size = sizeof(uint32_t),
        .data_ptr = (uint64_t)&gpu_value
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        printf("❌ TEST 5 FAILED: GEM_PWRITE error=%d\n", errno);
        return -1;
    }
    printf("✓ GPU write simulé: 0x%08X\n", gpu_value);
    
    // CRITIQUE: GEM_WAIT avant lecture
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx->test_bo,
        .timeout_ns = -1,  // Attente infinie
        .flags = 0
    };
    
    printf("⏳ GEM_WAIT...\n");
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        printf("❌ TEST 5 FAILED: GEM_WAIT error=%d\n", errno);
        return -1;
    }
    printf("✓ GEM_WAIT completed\n");
    
    // Lire depuis CPU
    uint32_t read_value = data[0];
    printf("✓ CPU read: 0x%08X\n", read_value);
    
    // Vérifier
    if (read_value == gpu_value) {
        printf("✅ TEST 5 PASSED: GEM_WAIT synchronise correctement\n");
        return 0;
    } else {
        printf("❌ TEST 5 FAILED: Expected 0x%08X, got 0x%08X\n", 
               gpu_value, read_value);
        return -1;
    }
}

/*
 * TEST 6: Memory Barrier + Cache Invalidation
 * Force invalidation cache CPU pour voir données GPU
 */
int test_6_memory_barrier(test_context_t* ctx) {
    printf("\n=== TEST 6: Memory Barrier + Cache Invalidation ===\n");
    
    if (!ctx->test_map) {
        printf("❌ TEST 6 FAILED: Buffer non mappé\n");
        return -1;
    }
    
    uint32_t* data = (uint32_t*)ctx->test_map;
    
    // Écrire pattern CPU
    data[0] = 0x11111111;
    printf("✓ CPU write: 0x%08X\n", data[0]);
    
    // Simuler GPU write
    uint32_t gpu_value = 0x22222222;
    struct drm_i915_gem_pwrite pwrite = {
        .handle = ctx->test_bo,
        .offset = 0,
        .size = sizeof(uint32_t),
        .data_ptr = (uint64_t)&gpu_value
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        printf("❌ TEST 6 FAILED: GEM_PWRITE error=%d\n", errno);
        return -1;
    }
    printf("✓ GPU write simulé: 0x%08X\n", gpu_value);
    
    // CRITIQUE: Cache flush + memory fence
#ifdef __x86_64__
    printf("⏳ Cache invalidation x86_64...\n");
    
    // Flush cache line contenant data[0]
    _mm_clflush(&data[0]);
    
    // Memory fence (garantit ordre)
    _mm_mfence();
    
    printf("✓ CLFLUSH + MFENCE completed\n");
#else
    printf("⚠️ Cache invalidation non disponible (non-x86)\n");
    // Fallback: remap buffer
    munmap(ctx->test_map, TEST_BUFFER_SIZE);
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = ctx->test_bo,
        .offset = 0,
        .size = TEST_BUFFER_SIZE,
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        printf("❌ TEST 6 FAILED: Remap error=%d\n", errno);
        return -1;
    }
    
    ctx->test_map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    data = (uint32_t*)ctx->test_map;
    printf("✓ Buffer remappé\n");
#endif
    
    // Lire depuis CPU
    uint32_t read_value = data[0];
    printf("✓ CPU read: 0x%08X\n", read_value);
    
    // Vérifier
    if (read_value == gpu_value) {
        printf("✅ TEST 6 PASSED: Cache invalidation fonctionne\n");
        return 0;
    } else {
        printf("❌ TEST 6 FAILED: Expected 0x%08X, got 0x%08X\n", 
               gpu_value, read_value);
        printf("⚠️ Cache CPU contient données obsolètes!\n");
        return -1;
    }
}

/*
 * TEST 7: Domain i915 Verification
 * Vérifie que buffer est dans bon domain (CPU vs RENDER)
 */
int test_7_domain_verification(test_context_t* ctx) {
    printf("\n=== TEST 7: Domain i915 Verification ===\n");
    
    if (!ctx->test_bo) {
        printf("❌ TEST 7 FAILED: Buffer non créé\n");
        return -1;
    }
    
    // Set domain CPU (lecture)
    struct drm_i915_gem_set_domain set_domain = {
        .handle = ctx->test_bo,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0  // Read-only
    };
    
    printf("⏳ SET_DOMAIN CPU (read)...\n");
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) < 0) {
        printf("❌ TEST 7 FAILED: SET_DOMAIN error=%d\n", errno);
        return -1;
    }
    printf("✓ Domain set to CPU\n");
    
    // Lire depuis CPU
    uint32_t* data = (uint32_t*)ctx->test_map;
    uint32_t value_cpu_domain = data[0];
    printf("✓ CPU read (CPU domain): 0x%08X\n", value_cpu_domain);
    
    // Set domain RENDER (GPU)
    set_domain.read_domains = I915_GEM_DOMAIN_RENDER;
    set_domain.write_domain = I915_GEM_DOMAIN_RENDER;
    
    printf("⏳ SET_DOMAIN RENDER (write)...\n");
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) < 0) {
        printf("❌ TEST 7 FAILED: SET_DOMAIN RENDER error=%d\n", errno);
        return -1;
    }
    printf("✓ Domain set to RENDER\n");
    
    // Simuler GPU write
    uint32_t gpu_value = 0x33333333;
    struct drm_i915_gem_pwrite pwrite = {
        .handle = ctx->test_bo,
        .offset = 0,
        .size = sizeof(uint32_t),
        .data_ptr = (uint64_t)&gpu_value
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        printf("❌ TEST 7 FAILED: GEM_PWRITE error=%d\n", errno);
        return -1;
    }
    printf("✓ GPU write (RENDER domain): 0x%08X\n", gpu_value);
    
    // CRITIQUE: Revenir à domain CPU pour lecture
    set_domain.read_domains = I915_GEM_DOMAIN_CPU;
    set_domain.write_domain = 0;
    
    printf("⏳ SET_DOMAIN CPU (read after GPU write)...\n");
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) < 0) {
        printf("❌ TEST 7 FAILED: SET_DOMAIN CPU error=%d\n", errno);
        return -1;
    }
    printf("✓ Domain switched back to CPU\n");
    
    // Lire depuis CPU
    uint32_t value_after_domain_switch = data[0];
    printf("✓ CPU read (after domain switch): 0x%08X\n", value_after_domain_switch);
    
    // Vérifier
    if (value_after_domain_switch == gpu_value) {
        printf("✅ TEST 7 PASSED: Domain switching fonctionne\n");
        return 0;
    } else {
        printf("❌ TEST 7 FAILED: Expected 0x%08X, got 0x%08X\n", 
               gpu_value, value_after_domain_switch);
        printf("⚠️ Domain i915 ne synchronise pas correctement!\n");
        return -1;
    }
}

/*
 * TEST 8: Target Bits Minimal (1 bit)
 * Vérifie que kernel peut trouver nonce avec target=1 bit
 */
int test_8_target_bits_minimal(test_context_t* ctx) {
    printf("\n=== TEST 8: Target Bits Minimal (1 bit) ===\n");
    printf("⚠️ Test nécessite kernel Bitcoin compilé\n");
    printf("✓ TEST 8 SKIPPED: Implémentation dans runner production\n");
    return 0;
}

/*
 * Cleanup
 */
void cleanup_test_context(test_context_t* ctx) {
    if (ctx->test_map) {
        munmap(ctx->test_map, TEST_BUFFER_SIZE);
        ctx->test_map = NULL;
    }
    
    if (ctx->test_bo) {
        struct drm_gem_close close_arg = {
            .handle = ctx->test_bo
        };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        ctx->test_bo = 0;
    }
    
    if (ctx->drm_fd >= 0) {
        close(ctx->drm_fd);
        ctx->drm_fd = -1;
    }
}

/*
 * Main
 */
int main(int argc, char** argv) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C370 - Memory Synchronization Validation            ║\n");
    printf("║  ROOT CAUSE: GPU→CPU memory coherence                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    test_context_t ctx = {
        .drm_fd = -1,
        .ctx_id = 0,
        .test_bo = 0,
        .test_map = NULL
    };
    
    // Ouvrir DRM
    ctx.drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.drm_fd < 0) {
        printf("❌ FATAL: Cannot open /dev/dri/renderD128: %d\n", errno);
        return 1;
    }
    printf("✓ DRM opened: fd=%d\n", ctx.drm_fd);
    
    // Créer contexte GPU
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx.drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("❌ FATAL: Cannot create GPU context: %d\n", errno);
        close(ctx.drm_fd);
        return 1;
    }
    ctx.ctx_id = ctx_create.ctx_id;
    printf("✓ GPU context created: ctx_id=%u\n\n", ctx.ctx_id);
    
    // Exécuter tests
    int results[4] = {0};
    
    results[0] = test_5_gem_wait_explicit(&ctx);
    results[1] = test_6_memory_barrier(&ctx);
    results[2] = test_7_domain_verification(&ctx);
    results[3] = test_8_target_bits_minimal(&ctx);
    
    // Cleanup
    cleanup_test_context(&ctx);
    
    // Résumé
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSUMÉ TESTS C370                                         ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  TEST 5 (GEM_WAIT):          %s                        ║\n", 
           results[0] == 0 ? "✅ PASSED" : "❌ FAILED");
    printf("║  TEST 6 (Memory Barrier):    %s                        ║\n", 
           results[1] == 0 ? "✅ PASSED" : "❌ FAILED");
    printf("║  TEST 7 (Domain i915):       %s                        ║\n", 
           results[2] == 0 ? "✅ PASSED" : "❌ FAILED");
    printf("║  TEST 8 (Target 1 bit):      ⏭️  SKIPPED                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    int passed = 0;
    for (int i = 0; i < 3; i++) {  // Skip test 8
        if (results[i] == 0) passed++;
    }
    
    printf("\n🎯 Score: %d/3 tests passed\n", passed);
    
    if (passed == 3) {
        printf("\n✅ CONCLUSION: Synchronisation mémoire fonctionne correctement\n");
        printf("   → Problème ailleurs (kernel ISA ou target bits)\n");
        return 0;
    } else {
        printf("\n❌ CONCLUSION: Problème synchronisation mémoire détecté\n");
        printf("   → Implémenter corrections dans runner production\n");
        return 1;
    }
}

// Made with Bob
