/*
 * TEST C456 - Test SANS relocations pour isoler le problème EXECBUFFER2
 * Objectif: Vérifier si "Invalid argument" vient des relocations ou d'autre chose
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

int main() {
    FORENSIC_LOG("🚀 TEST C456 - EXECBUFFER2 SANS relocations");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM");
        return 1;
    }
    
    struct drm_i915_gem_context_create ctx_create = {0};
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte: %u", ctx_id);
    
    /* Batch buffer minimal: juste MI_BATCH_BUFFER_END */
    struct drm_i915_gem_create gem_create = {.size = 4096};
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t batch_handle = gem_create.handle;
    
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = batch_handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *batch = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    /* Ajouter quelques NOPs avant MI_BATCH_BUFFER_END */
    batch[0] = 0;  /* NOP */
    batch[1] = 0;  /* NOP */
    batch[2] = 0;  /* NOP */
    batch[3] = 0;  /* NOP */
    batch[4] = GEN9_MI_BATCH_BUFFER_END;
    FORENSIC_LOG("✅ Batch buffer: 5 DWORDs (4 NOPs + MI_BATCH_BUFFER_END)");
    
    /* EXECBUFFER2 SANS relocations */
    struct drm_i915_gem_exec_object2 exec_obj;
    memset(&exec_obj, 0, sizeof(exec_obj));
    exec_obj.handle = batch_handle;
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uint64_t)&exec_obj;
    execbuf.buffer_count = 1;
    execbuf.batch_len = 20;  /* 5 DWORDs * 4 bytes */
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx_id;
    
    FORENSIC_LOG("⚡ Test EXECBUFFER2 SANS relocations...");
    FORENSIC_LOG("   buffers_ptr = 0x%lx", execbuf.buffers_ptr);
    FORENSIC_LOG("   buffer_count = %u", execbuf.buffer_count);
    FORENSIC_LOG("   batch_len = %u", execbuf.batch_len);
    FORENSIC_LOG("   flags = 0x%llx", execbuf.flags);
    FORENSIC_LOG("   rsvd1 (ctx_id) = %u", execbuf.rsvd1);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 échoue: %s (errno=%d)", strerror(errno), errno);
        munmap(batch, 4096);
        close(fd);
        return 1;
    }
    
    FORENSIC_LOG("✅✅✅ EXECBUFFER2 RÉUSSIT sans relocations!");
    
    munmap(batch, 4096);
    close(fd);
    return 0;
}

// Made with Bob
