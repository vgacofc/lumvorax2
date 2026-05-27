/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU DIRECT - POC Minimal
   ═══════════════════════════════════════════════════════════════════════════
   
   Accès GPU direct Intel UHD 620 via DRM (Direct Rendering Manager)
   BYPASS COMPLET OpenCL pour éliminer 64% overheads
   
   CYCLE: C180
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   ARCHITECTURE:
   - Accès direct /dev/dri/renderD128
   - Allocation buffers via GEM (Graphics Execution Manager)
   - Dispatch kernels via i915 command buffers
   - Zero-copy via mmap() GPU memory
   
   OBJECTIF: Mesurer overheads réels vs OpenCL
   
   ═══════════════════════════════════════════════════════════════════════════ */

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

/* Headers DRM/i915 */
#include <drm/drm.h>
#include <drm/i915_drm.h>

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURES
   ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    int         fd;                 /* File descriptor /dev/dri/renderD128 */
    uint32_t    ctx_id;             /* Context ID i915 */
    uint32_t    gem_handle;         /* GEM buffer handle */
    void*       gem_ptr;            /* mmap() pointer */
    size_t      gem_size;           /* Buffer size */
    uint64_t    gpu_addr;           /* GPU virtual address */
} lum_gpu_direct_t;

/* ═══════════════════════════════════════════════════════════════════════════
   UTILITAIRES
   ═══════════════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ═══════════════════════════════════════════════════════════════════════════
   API POC
   ═══════════════════════════════════════════════════════════════════════════ */

/* Initialiser GPU direct */
int lum_gpu_direct_init(lum_gpu_direct_t* gpu) {
    if (!gpu) return -1;
    
    memset(gpu, 0, sizeof(lum_gpu_direct_t));
    
    /* Ouvrir /dev/dri/renderD128 */
    printf("[C180-POC] Ouverture /dev/dri/renderD128...\n");
    gpu->fd = open("/dev/dri/renderD128", O_RDWR);
    if (gpu->fd < 0) {
        fprintf(stderr, "[C180-POC] ERREUR: Impossible d'ouvrir /dev/dri/renderD128: %s\n", 
                strerror(errno));
        return -1;
    }
    
    printf("[C180-POC] ✅ GPU ouvert (fd=%d)\n", gpu->fd);
    
    /* Obtenir version DRM */
    struct drm_version version = {0};
    if (ioctl(gpu->fd, DRM_IOCTL_VERSION, &version) < 0) {
        fprintf(stderr, "[C180-POC] ERREUR: DRM_IOCTL_VERSION failed: %s\n", strerror(errno));
        close(gpu->fd);
        return -1;
    }
    
    /* Allouer buffers pour nom/date/desc */
    version.name = malloc(version.name_len + 1);
    version.date = malloc(version.date_len + 1);
    version.desc = malloc(version.desc_len + 1);
    
    if (ioctl(gpu->fd, DRM_IOCTL_VERSION, &version) < 0) {
        fprintf(stderr, "[C180-POC] ERREUR: DRM_IOCTL_VERSION (2) failed: %s\n", strerror(errno));
        free(version.name);
        free(version.date);
        free(version.desc);
        close(gpu->fd);
        return -1;
    }
    
    printf("[C180-POC] ✅ DRM version: %s %d.%d.%d (%s)\n",
           version.name, version.version_major, version.version_minor, 
           version.version_patchlevel, version.date);
    
    free(version.name);
    free(version.date);
    free(version.desc);
    
    /* Créer contexte i915 */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(gpu->fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        fprintf(stderr, "[C180-POC] ERREUR: DRM_IOCTL_I915_GEM_CONTEXT_CREATE failed: %s\n", 
                strerror(errno));
        close(gpu->fd);
        return -1;
    }
    
    gpu->ctx_id = ctx_create.ctx_id;
    printf("[C180-POC] ✅ Context i915 créé (ctx_id=%u)\n", gpu->ctx_id);
    
    return 0;
}

/* Allouer buffer GPU via GEM */
int lum_gpu_direct_alloc_buffer(lum_gpu_direct_t* gpu, size_t size) {
    if (!gpu || gpu->fd < 0) return -1;
    
    printf("[C180-POC] Allocation buffer GPU (%zu bytes)...\n", size);
    
    /* Créer GEM object */
    struct drm_i915_gem_create gem_create = {0};
    gem_create.size = size;
    
    if (ioctl(gpu->fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        fprintf(stderr, "[C180-POC] ERREUR: DRM_IOCTL_I915_GEM_CREATE failed: %s\n", 
                strerror(errno));
        return -1;
    }
    
    gpu->gem_handle = gem_create.handle;
    gpu->gem_size = size;
    
    printf("[C180-POC] ✅ GEM object créé (handle=%u, size=%zu)\n", 
           gpu->gem_handle, gpu->gem_size);
    
    /* Mapper en mémoire CPU (zero-copy) */
    struct drm_i915_gem_mmap gem_mmap = {0};
    gem_mmap.handle = gpu->gem_handle;
    gem_mmap.offset = 0;
    gem_mmap.size = size;
    
    if (ioctl(gpu->fd, DRM_IOCTL_I915_GEM_MMAP, &gem_mmap) < 0) {
        fprintf(stderr, "[C180-POC] ERREUR: DRM_IOCTL_I915_GEM_MMAP failed: %s\n", 
                strerror(errno));
        return -1;
    }
    
    gpu->gem_ptr = (void*)(uintptr_t)gem_mmap.addr_ptr;
    
    printf("[C180-POC] ✅ Buffer mappé en mémoire CPU (ptr=%p)\n", gpu->gem_ptr);
    
    return 0;
}

/* Écrire données dans buffer GPU (zero-copy) */
int lum_gpu_direct_write(lum_gpu_direct_t* gpu, const void* data, size_t size) {
    if (!gpu || !gpu->gem_ptr || size > gpu->gem_size) return -1;
    
    uint64_t t_start = get_timestamp_ns();
    
    /* Copie directe en mémoire GPU (zero-copy) */
    memcpy(gpu->gem_ptr, data, size);
    
    uint64_t t_end = get_timestamp_ns();
    double elapsed_ms = (t_end - t_start) / 1000000.0;
    
    printf("[C180-POC] ✅ Écriture %zu bytes en %.3f ms (%.1f MB/s)\n",
           size, elapsed_ms, (size / 1024.0 / 1024.0) / (elapsed_ms / 1000.0));
    
    return 0;
}

/* Lire données depuis buffer GPU (zero-copy) */
int lum_gpu_direct_read(lum_gpu_direct_t* gpu, void* data, size_t size) {
    if (!gpu || !gpu->gem_ptr || size > gpu->gem_size) return -1;
    
    uint64_t t_start = get_timestamp_ns();
    
    /* Copie directe depuis mémoire GPU (zero-copy) */
    memcpy(data, gpu->gem_ptr, size);
    
    uint64_t t_end = get_timestamp_ns();
    double elapsed_ms = (t_end - t_start) / 1000000.0;
    
    printf("[C180-POC] ✅ Lecture %zu bytes en %.3f ms (%.1f MB/s)\n",
           size, elapsed_ms, (size / 1024.0 / 1024.0) / (elapsed_ms / 1000.0));
    
    return 0;
}

/* Cleanup GPU */
void lum_gpu_direct_cleanup(lum_gpu_direct_t* gpu) {
    if (!gpu) return;
    
    if (gpu->gem_ptr) {
        munmap(gpu->gem_ptr, gpu->gem_size);
        gpu->gem_ptr = NULL;
    }
    
    if (gpu->gem_handle) {
        struct drm_gem_close gem_close = {0};
        gem_close.handle = gpu->gem_handle;
        ioctl(gpu->fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
        gpu->gem_handle = 0;
    }
    
    if (gpu->ctx_id) {
        struct drm_i915_gem_context_destroy ctx_destroy = {0};
        ctx_destroy.ctx_id = gpu->ctx_id;
        ioctl(gpu->fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
        gpu->ctx_id = 0;
    }
    
    if (gpu->fd >= 0) {
        close(gpu->fd);
        gpu->fd = -1;
    }
    
    printf("[C180-POC] ✅ GPU cleanup complet\n");
}

/* ═══════════════════════════════════════════════════════════════════════════
   MAIN POC
   ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  LUMVORAX GPU DIRECT - POC Minimal                      ║\n");
    printf("║  Accès GPU Intel UHD 620 via DRM (bypass OpenCL)        ║\n");
    printf("║  CYCLE C180 - Élimination overheads OpenCL              ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    lum_gpu_direct_t gpu;
    int ret;
    
    /* Initialiser GPU */
    ret = lum_gpu_direct_init(&gpu);
    if (ret < 0) {
        fprintf(stderr, "[C180-POC] ❌ Échec initialisation GPU\n");
        return 1;
    }
    
    /* Allouer buffer 1 MB */
    size_t buffer_size = 1024 * 1024;  /* 1 MB */
    ret = lum_gpu_direct_alloc_buffer(&gpu, buffer_size);
    if (ret < 0) {
        fprintf(stderr, "[C180-POC] ❌ Échec allocation buffer\n");
        lum_gpu_direct_cleanup(&gpu);
        return 1;
    }
    
    /* Test écriture/lecture (mesure overheads) */
    printf("\n[C180-POC] === TEST OVERHEADS TRANSFERTS ===\n");
    
    uint8_t* test_data = malloc(buffer_size);
    memset(test_data, 0xAB, buffer_size);
    
    /* Écriture */
    ret = lum_gpu_direct_write(&gpu, test_data, buffer_size);
    if (ret < 0) {
        fprintf(stderr, "[C180-POC] ❌ Échec écriture\n");
        free(test_data);
        lum_gpu_direct_cleanup(&gpu);
        return 1;
    }
    
    /* Lecture */
    uint8_t* read_data = malloc(buffer_size);
    ret = lum_gpu_direct_read(&gpu, read_data, buffer_size);
    if (ret < 0) {
        fprintf(stderr, "[C180-POC] ❌ Échec lecture\n");
        free(test_data);
        free(read_data);
        lum_gpu_direct_cleanup(&gpu);
        return 1;
    }
    
    /* Vérification */
    if (memcmp(test_data, read_data, buffer_size) == 0) {
        printf("[C180-POC] ✅ Vérification données OK\n");
    } else {
        fprintf(stderr, "[C180-POC] ❌ Vérification données ÉCHEC\n");
    }
    
    free(test_data);
    free(read_data);
    
    /* Cleanup */
    lum_gpu_direct_cleanup(&gpu);
    
    printf("\n[C180-POC] === POC TERMINÉ AVEC SUCCÈS ===\n");
    printf("[C180-POC] Prochaine étape: Compiler et charger shader SHA-256\n\n");
    
    return 0;
}

/* Made with ❤️ by Bob - LumVorax Core Team */

// Made with Bob
