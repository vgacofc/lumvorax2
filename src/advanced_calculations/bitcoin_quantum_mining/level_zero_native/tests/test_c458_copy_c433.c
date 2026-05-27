/*
 * TEST C458 - Copie EXACTE de la structure test_c433 mais batch minimal
 * Objectif: Identifier quelle partie de c433 permet à EXECBUFFER2 de réussir
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
    FORENSIC_LOG("🚀 TEST C458 - Copie structure C433 avec batch minimal");
    
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
    
    /* Mapper le dernier buffer (batch) */
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = handles[5],
        .flags = I915_MMAP_OFFSET_WB
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *batch = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
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
    
    FORENSIC_LOG("⚡ Test EXECBUFFER2 avec 1 relocation...");
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 échoue: %s (errno=%d)", strerror(errno), errno);
        munmap(batch, 4096);
        close(fd);
        return 1;
    }
    
    FORENSIC_LOG("✅✅✅ EXECBUFFER2 RÉUSSIT avec structure C433!");
    FORENSIC_LOG("🎉 La structure avec 6 buffers fonctionne!");
    
    munmap(batch, 4096);
    close(fd);
    return 0;
}

// Made with Bob
