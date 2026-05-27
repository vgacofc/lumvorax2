/*
 * TEST C457 - Batch buffer en DERNIER (comme test_c433 qui fonctionne)
 * Hypothèse: i915 DRM requiert que le batch buffer soit le dernier dans exec_objects[]
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
    FORENSIC_LOG("🚀 TEST C457 - Batch buffer en DERNIER position");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM");
        return 1;
    }
    
    struct drm_i915_gem_context_create ctx_create = {0};
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte: %u", ctx_id);
    
    /* Créer 2 buffers: dummy + batch */
    struct drm_i915_gem_create gem_create = {.size = 4096};
    
    /* Buffer dummy (sera en position 0) */
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t dummy_handle = gem_create.handle;
    FORENSIC_LOG("✅ Dummy buffer: handle=%u", dummy_handle);
    
    /* Batch buffer (sera en position 1 = DERNIER) */
    gem_create.size = 4096;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t batch_handle = gem_create.handle;
    
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = batch_handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *batch = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    batch[0] = GEN9_MI_BATCH_BUFFER_END;
    FORENSIC_LOG("✅ Batch buffer: handle=%u (1 DWORD)", batch_handle);
    
    /* EXECBUFFER2 avec batch EN DERNIER */
    struct drm_i915_gem_exec_object2 exec_objects[2];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = dummy_handle;  /* Dummy en premier */
    exec_objects[1].handle = batch_handle;  /* Batch EN DERNIER */
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uint64_t)exec_objects;
    execbuf.buffer_count = 2;  /* 2 buffers */
    execbuf.batch_len = 4;     /* 1 DWORD */
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx_id;
    
    FORENSIC_LOG("⚡ Test EXECBUFFER2 avec batch EN DERNIER...");
    FORENSIC_LOG("   exec_objects[0] = dummy (handle=%u)", exec_objects[0].handle);
    FORENSIC_LOG("   exec_objects[1] = batch (handle=%u)", exec_objects[1].handle);
    FORENSIC_LOG("   buffer_count = %u", execbuf.buffer_count);
    FORENSIC_LOG("   batch_len = %u", execbuf.batch_len);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 échoue: %s (errno=%d)", strerror(errno), errno);
        munmap(batch, 4096);
        close(fd);
        return 1;
    }
    
    FORENSIC_LOG("✅✅✅ EXECBUFFER2 RÉUSSIT avec batch EN DERNIER!");
    FORENSIC_LOG("🎉 Hypothèse C457 CONFIRMÉE: batch doit être en dernier!");
    
    munmap(batch, 4096);
    close(fd);
    return 0;
}

// Made with Bob
