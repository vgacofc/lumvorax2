/*
 * TEST C427 #1 - RUNNER VALIDATION STORE
 * Objectif: Valider pipeline GPU → RAM → CPU avec kernel minimal
 * 
 * Ce runner teste UNIQUEMENT:
 * - Chargement kernel Gen9 ISA
 * - Création surface state output buffer
 * - Dispatch 256 threads
 * - Lecture résultat avec GEM_WAIT + GEM_SET_DOMAIN
 * - Validation output[0] == 0x12345678
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
#include <i915_drm.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

// Structure contexte GPU
typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t kernel_bo;
    uint32_t output_bo;
    void* output_map;
    size_t kernel_size;
    size_t output_size;
} gpu_context_t;

// Lecture fichier binaire kernel
static int load_kernel_binary(const char* path, void** data, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        FORENSIC_LOG("❌ Erreur ouverture kernel: %s", strerror(errno));
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    *data = malloc(*size);
    if (!*data) {
        fclose(f);
        return -1;
    }
    
    if (fread(*data, 1, *size, f) != *size) {
        free(*data);
        fclose(f);
        return -1;
    }
    
    fclose(f);
    FORENSIC_LOG("✅ Kernel chargé: %zu bytes", *size);
    return 0;
}

// Création buffer GEM
static uint32_t create_gem_buffer(int fd, size_t size) {
    struct drm_i915_gem_create create = {
        .size = size
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) != 0) {
        FORENSIC_LOG("❌ GEM_CREATE failed: %s", strerror(errno));
        return 0;
    }
    
    FORENSIC_LOG("✅ GEM buffer créé: handle=%u size=%zu", create.handle, size);
    return create.handle;
}

// Mapping buffer
static void* map_gem_buffer(int fd, uint32_t handle, size_t size) {
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        FORENSIC_LOG("❌ GEM_MMAP failed: %s", strerror(errno));
        return NULL;
    }
    
    void* ptr = (void*)(uintptr_t)mmap_arg.addr_ptr;
    FORENSIC_LOG("✅ Buffer mappé: %p", ptr);
    return ptr;
}

// Écriture données dans buffer
static int write_to_gem_buffer(int fd, uint32_t handle, const void* data, size_t size) {
    struct drm_i915_gem_pwrite pwrite = {
        .handle = handle,
        .offset = 0,
        .size = size,
        .data_ptr = (uint64_t)(uintptr_t)data
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) != 0) {
        FORENSIC_LOG("❌ GEM_PWRITE failed: %s", strerror(errno));
        return -1;
    }
    
    FORENSIC_LOG("✅ Données écrites: %zu bytes", size);
    return 0;
}

// Attente GPU idle
static int wait_gem_buffer(int fd, uint32_t handle) {
    struct drm_i915_gem_wait wait = {
        .bo_handle = handle,
        .timeout_ns = 10000000000LL  // 10 secondes
    };
    
    FORENSIC_LOG("⏳ Attente GPU idle...");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) != 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        return -1;
    }
    
    FORENSIC_LOG("✅ GPU idle confirmé");
    return 0;
}

// Synchronisation domaine CPU
static int set_domain_cpu(int fd, uint32_t handle) {
    struct drm_i915_gem_set_domain set_domain = {
        .handle = handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = I915_GEM_DOMAIN_CPU
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) != 0) {
        FORENSIC_LOG("❌ GEM_SET_DOMAIN failed: %s", strerror(errno));
        return -1;
    }
    
    FORENSIC_LOG("✅ Domaine CPU activé");
    return 0;
}

// Initialisation contexte GPU
static int init_gpu_context(gpu_context_t* ctx, const char* kernel_path) {
    memset(ctx, 0, sizeof(*ctx));
    
    // Ouverture DRM
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM: %s", strerror(errno));
        return -1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", ctx->drm_fd);
    
    // Création contexte GPU
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) != 0) {
        FORENSIC_LOG("❌ Erreur création contexte: %s", strerror(errno));
        close(ctx->drm_fd);
        return -1;
    }
    ctx->ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte GPU créé: id=%u", ctx->ctx_id);
    
    // Chargement kernel
    void* kernel_data = NULL;
    if (load_kernel_binary(kernel_path, &kernel_data, &ctx->kernel_size) != 0) {
        close(ctx->drm_fd);
        return -1;
    }
    
    // Création buffer kernel
    ctx->kernel_bo = create_gem_buffer(ctx->drm_fd, ctx->kernel_size);
    if (ctx->kernel_bo == 0) {
        free(kernel_data);
        close(ctx->drm_fd);
        return -1;
    }
    
    // Écriture kernel
    if (write_to_gem_buffer(ctx->drm_fd, ctx->kernel_bo, kernel_data, ctx->kernel_size) != 0) {
        free(kernel_data);
        close(ctx->drm_fd);
        return -1;
    }
    free(kernel_data);
    
    // Création buffer output (257 uint32_t = 1028 bytes)
    ctx->output_size = 257 * sizeof(uint32_t);
    ctx->output_bo = create_gem_buffer(ctx->drm_fd, ctx->output_size);
    if (ctx->output_bo == 0) {
        close(ctx->drm_fd);
        return -1;
    }
    
    // Mapping output
    ctx->output_map = map_gem_buffer(ctx->drm_fd, ctx->output_bo, ctx->output_size);
    if (ctx->output_map == NULL) {
        close(ctx->drm_fd);
        return -1;
    }
    
    // Initialisation output à 0
    memset(ctx->output_map, 0, ctx->output_size);
    FORENSIC_LOG("✅ Output buffer initialisé à 0");
    
    return 0;
}

// Nettoyage contexte
static void cleanup_gpu_context(gpu_context_t* ctx) {
    if (ctx->output_map) {
        munmap(ctx->output_map, ctx->output_size);
    }
    if (ctx->drm_fd >= 0) {
        close(ctx->drm_fd);
    }
}

// Validation résultats
static int validate_results(gpu_context_t* ctx) {
    FORENSIC_LOG("🔍 VALIDATION RÉSULTATS");
    
    // Attente GPU
    if (wait_gem_buffer(ctx->drm_fd, ctx->output_bo) != 0) {
        return -1;
    }
    
    // Synchronisation CPU
    if (set_domain_cpu(ctx->drm_fd, ctx->output_bo) != 0) {
        return -1;
    }
    
    // Lecture résultats
    uint32_t* output = (uint32_t*)ctx->output_map;
    
    FORENSIC_LOG("📊 output[0] = 0x%08X (attendu: 0x12345678)", output[0]);
    
    // TEST ABSOLU
    if (output[0] == 0x12345678) {
        FORENSIC_LOG("✅ TEST ABSOLU RÉUSSI: Pipeline GPU→CPU fonctionnel!");
        
        // Vérification pattern gid
        int pattern_ok = 1;
        for (int i = 0; i < 256; i++) {
            if (output[i + 1] != (uint32_t)i) {
                FORENSIC_LOG("⚠️  output[%d] = %u (attendu: %d)", i+1, output[i+1], i);
                pattern_ok = 0;
                break;
            }
        }
        
        if (pattern_ok) {
            FORENSIC_LOG("✅ TEST SECONDAIRE RÉUSSI: get_global_id() fonctionnel!");
        } else {
            FORENSIC_LOG("⚠️  TEST SECONDAIRE ÉCHOUÉ: get_global_id() incorrect");
        }
        
        return 0;
    } else {
        FORENSIC_LOG("❌ TEST ABSOLU ÉCHOUÉ: output[0] != 0x12345678");
        FORENSIC_LOG("🔍 Dump premiers 16 uint32_t:");
        for (int i = 0; i < 16; i++) {
            FORENSIC_LOG("   output[%d] = 0x%08X", i, output[i]);
        }
        return -1;
    }
}

int main(int argc, char** argv) {
    FORENSIC_LOG("🚀 TEST C427 #1 - VALIDATION STORE DÉMARRAGE");
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <kernel.bin>\n", argv[0]);
        return 1;
    }
    
    gpu_context_t ctx;
    if (init_gpu_context(&ctx, argv[1]) != 0) {
        return 1;
    }
    
    FORENSIC_LOG("⚠️  DISPATCH NON IMPLÉMENTÉ - TODO: Créer batch buffer");
    FORENSIC_LOG("📝 Prochaine étape: Implémenter dispatch avec:");
    FORENSIC_LOG("   - PIPELINE_SELECT");
    FORENSIC_LOG("   - STATE_BASE_ADDRESS");
    FORENSIC_LOG("   - MEDIA_VFE_STATE");
    FORENSIC_LOG("   - MEDIA_INTERFACE_DESCRIPTOR_LOAD");
    FORENSIC_LOG("   - GPGPU_WALKER (256 threads)");
    FORENSIC_LOG("   - MI_BATCH_BUFFER_END");
    
    int result = validate_results(&ctx);
    
    cleanup_gpu_context(&ctx);
    
    if (result == 0) {
        FORENSIC_LOG("✅ TEST C427 #1 RÉUSSI");
        return 0;
    } else {
        FORENSIC_LOG("❌ TEST C427 #1 ÉCHOUÉ");
        return 1;
    }
}

// Made with Bob
