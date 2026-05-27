/*
 * TEST C459 - FIX GEM_MMAP au lieu de GEM_MMAP_OFFSET
 * ROOT CAUSE: test_c433 utilise GEM_MMAP (ancien), test_c458 utilise GEM_MMAP_OFFSET (nouveau)
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
#include <xf86drm.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

#define GEN9_MI_BATCH_BUFFER_END 0x0a000000
#define GEN9_PIPELINE_SELECT     ((0x3 << 29) | (0x1 << 27) | (0x1 << 24) | (0x4 << 16))
#define PIPELINE_SELECT_MEDIA    (1 << 0)

int main() {
    FORENSIC_LOG("🚀 TEST C459 - FIX GEM_MMAP au lieu de GEM_MMAP_OFFSET");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM");
        return 1;
    }
    
    struct drm_i915_gem_context_create ctx_create = {0};
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte: %u", ctx_id);
    
    /* Créer 6 buffers comme C433 */
    struct drm_i915_gem_create gem_create;
    uint32_t handles[6];
    
    for (int i = 0; i < 6; i++) {
        gem_create.size = 4096;
        ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
        handles[i] = gem_create.handle;
    }
    FORENSIC_LOG("✅ 6 buffers créés: handles=%u,%u,%u,%u,%u,%u", 
        handles[0], handles[1], handles[2], handles[3], handles[4], handles[5]);
    
    /* C459 FIX: Utiliser GEM_MMAP au lieu de GEM_MMAP_OFFSET */
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = handles[5],
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        FORENSIC_LOG("❌ GEM_MMAP échoue: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t*)(uintptr_t)mmap_arg.addr_ptr;
    FORENSIC_LOG("✅ Batch mappé avec GEM_MMAP: %p", batch);
    
    /* Batch buffer avec PIPELINE_SELECT comme C433 */
    batch[0] = 0;  /* NOP */
    batch[1] = 0;  /* NOP */
    batch[2] = 0;  /* NOP */
    batch[3] = 0;  /* NOP */
    batch[4] = GEN9_PIPELINE_SELECT | (1 << 8);
    batch[5] = PIPELINE_SELECT_MEDIA;
    batch[6] = GEN9_MI_BATCH_BUFFER_END;
    FORENSIC_LOG("✅ Batch buffer: 4 NOPs + PIPELINE_SELECT + END");
    
    /* Ajouter UNE relocation dummy */
    struct drm_i915_gem_relocation_entry reloc = {
        .target_handle = handles[0],
        .offset = 0,
        .read_domains = I915_GEM_DOMAIN_RENDER,
        .write_domain = 0
    };
    
    /* EXECBUFFER2 avec structure EXACTE de C433 */
    struct drm_i915_gem_exec_object2 exec_objects[6];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = handles[0];
    exec_objects[1].handle = handles[1];
    exec_objects[2].handle = handles[2];
    exec_objects[3].handle = handles[3];
    exec_objects[4].handle = handles[4];
    exec_objects[5].handle = handles[5];  /* Batch en dernier */
    exec_objects[5].relocation_count = 1;  /* 1 relocation */
    exec_objects[5].relocs_ptr = (uint64_t)&reloc;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 6,
        .batch_len = 28,  /* 7 DWORDs * 4 bytes */
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Test EXECBUFFER2 avec GEM_MMAP...");
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 échoue: %s (errno=%d)", strerror(errno), errno);
        munmap(batch, 4096);
        close(fd);
        return 1;
    }
    
    FORENSIC_LOG("✅✅✅ EXECBUFFER2 RÉUSSIT avec GEM_MMAP!");
    FORENSIC_LOG("🎉 ROOT CAUSE CONFIRMÉ: GEM_MMAP vs GEM_MMAP_OFFSET!");
    
    munmap(batch, 4096);
    close(fd);
    return 0;
}

// Made with Bob