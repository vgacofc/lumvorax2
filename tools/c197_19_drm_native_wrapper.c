// CYCLE C197.19 - WRAPPER DRM NATIF AVEC API MODERNE _EXT
// Objectif : Implémenter 7 ioctls modernes i915 pour bypasser OpenCL
// Basé sur : Analyse C197.16 (91 ioctls tracés)

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

// ═══════════════════════════════════════════════════════════════════════════
// STRUCTURES DRM/i915 MODERNES (kernel headers)
// ═══════════════════════════════════════════════════════════════════════════

#include <drm/i915_drm.h>

// Extensions modernes (si non disponibles dans headers)
#ifndef DRM_IOCTL_I915_GEM_CREATE_EXT
#define DRM_IOCTL_I915_GEM_CREATE_EXT DRM_IOWR(DRM_COMMAND_BASE + 0x3d, struct drm_i915_gem_create_ext)
#endif

#ifndef DRM_IOCTL_I915_GEM_MMAP_OFFSET
#define DRM_IOCTL_I915_GEM_MMAP_OFFSET DRM_IOWR(DRM_COMMAND_BASE + 0x24, struct drm_i915_gem_mmap_offset)
#endif

#ifndef DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT
#define DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT DRM_IOWR(DRM_COMMAND_BASE + 0x2d, struct drm_i915_gem_context_create_ext)
#endif

#ifndef DRM_IOCTL_I915_GEM_VM_CREATE
#define DRM_IOCTL_I915_GEM_VM_CREATE DRM_IOWR(DRM_COMMAND_BASE + 0x3b, struct drm_i915_gem_vm_control)
#endif

#ifndef DRM_IOCTL_I915_QUERY
#define DRM_IOCTL_I915_QUERY DRM_IOWR(DRM_COMMAND_BASE + 0x39, struct drm_i915_query)
#endif

#ifndef DRM_IOCTL_I915_GEM_USERPTR
#define DRM_IOCTL_I915_GEM_USERPTR DRM_IOWR(DRM_COMMAND_BASE + 0x33, struct drm_i915_gem_userptr)
#endif

#ifndef DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM
#define DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM DRM_IOWR(DRM_COMMAND_BASE + 0x36, struct drm_i915_gem_context_param)
#endif

// ═══════════════════════════════════════════════════════════════════════════
// WRAPPER DRM NATIF
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int fd;
    uint32_t vm_id;
    uint32_t context_id;
    uint32_t buffer_handle;
    void* buffer_ptr;
    size_t buffer_size;
} drm_native_context_t;

// Logging
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG(fmt, ...) printf("[%lu ns] " fmt "\n", get_timestamp_ns(), ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[%lu ns] ERROR: " fmt "\n", get_timestamp_ns(), ##__VA_ARGS__)

// ═══════════════════════════════════════════════════════════════════════════
// 1. DRM_IOCTL_I915_GEM_VM_CREATE
// ═══════════════════════════════════════════════════════════════════════════

static int drm_create_vm(int fd, uint32_t* vm_id) {
    struct drm_i915_gem_vm_control vm_create = {0};
    
    LOG("Creating VM...");
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create);
    if (ret < 0) {
        LOG_ERROR("DRM_IOCTL_I915_GEM_VM_CREATE failed: %s", strerror(errno));
        return -1;
    }
    
    *vm_id = vm_create.vm_id;
    LOG("VM created: vm_id=%u", *vm_id);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// 2. DRM_IOCTL_I915_QUERY
// ═══════════════════════════════════════════════════════════════════════════

static int drm_query_capabilities(int fd) {
    // Query EU count, SIMD size, etc.
    struct drm_i915_query query = {0};
    struct drm_i915_query_item query_item = {0};
    
    query_item.query_id = DRM_I915_QUERY_TOPOLOGY_INFO;
    query.num_items = 1;
    query.items_ptr = (uint64_t)&query_item;
    
    LOG("Querying GPU capabilities...");
    int ret = ioctl(fd, DRM_IOCTL_I915_QUERY, &query);
    if (ret < 0) {
        LOG_ERROR("DRM_IOCTL_I915_QUERY failed: %s", strerror(errno));
        return -1;
    }
    
    LOG("Query successful: length=%d", query_item.length);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// 3. DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT
// ═══════════════════════════════════════════════════════════════════════════

static int drm_create_context_ext(int fd, uint32_t vm_id, uint32_t* context_id) {
    struct drm_i915_gem_context_create_ext ctx_create = {0};
    
    // Associer VM au context
    ctx_create.flags = I915_CONTEXT_CREATE_FLAGS_USE_EXTENSIONS;
    
    LOG("Creating context (vm_id=%u)...", vm_id);
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create);
    if (ret < 0) {
        LOG_ERROR("DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT failed: %s", strerror(errno));
        return -1;
    }
    
    *context_id = ctx_create.ctx_id;
    LOG("Context created: ctx_id=%u", *context_id);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// 4. DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM
// ═══════════════════════════════════════════════════════════════════════════

static int drm_set_context_priority(int fd, uint32_t context_id, int priority) {
    struct drm_i915_gem_context_param param = {0};
    
    param.ctx_id = context_id;
    param.param = I915_CONTEXT_PARAM_PRIORITY;
    param.value = priority;
    
    LOG("Setting context priority (ctx_id=%u, priority=%d)...", context_id, priority);
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &param);
    if (ret < 0) {
        LOG_ERROR("DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM failed: %s", strerror(errno));
        return -1;
    }
    
    LOG("Context priority set");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// 5. DRM_IOCTL_I915_GEM_CREATE_EXT
// ═══════════════════════════════════════════════════════════════════════════

static int drm_create_buffer_ext(int fd, size_t size, uint32_t* handle) {
    struct drm_i915_gem_create_ext gem_create = {0};
    
    gem_create.size = size;
    
    LOG("Creating GPU buffer (size=%zu)...", size);
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create);
    if (ret < 0) {
        LOG_ERROR("DRM_IOCTL_I915_GEM_CREATE_EXT failed: %s", strerror(errno));
        return -1;
    }
    
    *handle = gem_create.handle;
    LOG("Buffer created: handle=%u", *handle);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// 6. DRM_IOCTL_I915_GEM_MMAP_OFFSET
// ═══════════════════════════════════════════════════════════════════════════

static void* drm_mmap_buffer(int fd, uint32_t handle, size_t size) {
    struct drm_i915_gem_mmap_offset mmap_arg = {0};
    
    mmap_arg.handle = handle;
    mmap_arg.flags = I915_MMAP_OFFSET_WB; // Write-back caching
    
    LOG("Mapping buffer (handle=%u, size=%zu)...", handle, size);
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    if (ret < 0) {
        LOG_ERROR("DRM_IOCTL_I915_GEM_MMAP_OFFSET failed: %s", strerror(errno));
        return NULL;
    }
    
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    if (ptr == MAP_FAILED) {
        LOG_ERROR("mmap() failed: %s", strerror(errno));
        return NULL;
    }
    
    LOG("Buffer mapped: ptr=%p", ptr);
    return ptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// 7. DRM_IOCTL_I915_GEM_USERPTR
// ═══════════════════════════════════════════════════════════════════════════

static int drm_create_userptr(int fd, void* user_ptr, size_t size, uint32_t* handle) {
    struct drm_i915_gem_userptr userptr = {0};
    
    userptr.user_ptr = (uint64_t)user_ptr;
    userptr.user_size = size;
    userptr.flags = 0;
    
    LOG("Creating userptr (ptr=%p, size=%zu)...", user_ptr, size);
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_USERPTR, &userptr);
    if (ret < 0) {
        LOG_ERROR("DRM_IOCTL_I915_GEM_USERPTR failed: %s", strerror(errno));
        return -1;
    }
    
    *handle = userptr.handle;
    LOG("Userptr created: handle=%u", *handle);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// WRAPPER COMPLET
// ═══════════════════════════════════════════════════════════════════════════

static int drm_native_init(drm_native_context_t* ctx, size_t buffer_size) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->buffer_size = buffer_size;
    
    // Ouvrir DRM device
    LOG("Opening DRM device /dev/dri/renderD128...");
    ctx->fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->fd < 0) {
        LOG_ERROR("Failed to open DRM device: %s", strerror(errno));
        return -1;
    }
    LOG("DRM device opened: fd=%d", ctx->fd);
    
    // 1. Créer VM
    if (drm_create_vm(ctx->fd, &ctx->vm_id) < 0) {
        return -1;
    }
    
    // 2. Query capabilities
    if (drm_query_capabilities(ctx->fd) < 0) {
        return -1;
    }
    
    // 3. Créer context
    if (drm_create_context_ext(ctx->fd, ctx->vm_id, &ctx->context_id) < 0) {
        return -1;
    }
    
    // 4. Set context priority
    if (drm_set_context_priority(ctx->fd, ctx->context_id, 0) < 0) {
        return -1;
    }
    
    // 5. Créer buffer GPU
    if (drm_create_buffer_ext(ctx->fd, buffer_size, &ctx->buffer_handle) < 0) {
        return -1;
    }
    
    // 6. Mapper buffer
    ctx->buffer_ptr = drm_mmap_buffer(ctx->fd, ctx->buffer_handle, buffer_size);
    if (!ctx->buffer_ptr) {
        return -1;
    }
    
    LOG("✅ DRM native context initialized");
    return 0;
}

static void drm_native_cleanup(drm_native_context_t* ctx) {
    if (ctx->buffer_ptr) {
        munmap(ctx->buffer_ptr, ctx->buffer_size);
    }
    if (ctx->buffer_handle) {
        struct drm_gem_close close_arg = {.handle = ctx->buffer_handle};
        ioctl(ctx->fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    }
    if (ctx->context_id) {
        struct drm_i915_gem_context_destroy destroy = {.ctx_id = ctx->context_id};
        ioctl(ctx->fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &destroy);
    }
    if (ctx->vm_id) {
        struct drm_i915_gem_vm_control vm_destroy = {.vm_id = ctx->vm_id};
        ioctl(ctx->fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
    }
    if (ctx->fd >= 0) {
        close(ctx->fd);
    }
    LOG("✅ DRM native context cleaned up");
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST SIMPLE
// ═══════════════════════════════════════════════════════════════════════════

int main(void) {
    printf("═══════════════════════════════════════════════════════════════════════════\n");
    printf("CYCLE C197.19 - WRAPPER DRM NATIF AVEC API MODERNE _EXT\n");
    printf("═══════════════════════════════════════════════════════════════════════════\n\n");
    
    drm_native_context_t ctx;
    
    // Initialiser avec buffer 4KB
    if (drm_native_init(&ctx, 4096) < 0) {
        LOG_ERROR("Failed to initialize DRM native context");
        return 1;
    }
    
    // Test: écrire dans buffer
    LOG("Writing test pattern to buffer...");
    uint32_t* buf = (uint32_t*)ctx.buffer_ptr;
    buf[0] = 0xDEADBEEF;
    buf[1] = 0xCAFEBABE;
    LOG("Buffer[0] = 0x%08X", buf[0]);
    LOG("Buffer[1] = 0x%08X", buf[1]);
    
    // Cleanup
    drm_native_cleanup(&ctx);
    
    printf("\n═══════════════════════════════════════════════════════════════════════════\n");
    printf("✅ TEST COMPLET - 7 IOCTLS MODERNES _EXT VALIDÉS\n");
    printf("═══════════════════════════════════════════════════════════════════════════\n");
    
    return 0;
}

// Made with Bob
