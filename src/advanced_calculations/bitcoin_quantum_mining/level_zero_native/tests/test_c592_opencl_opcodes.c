/*
 * TEST C592 - FIX OPCODES AVEC VALEURS OPENCL RÉELLES
 * 
 * ROOT CAUSE #139: Opcodes Gen9 INCORRECTS dans C584-C591
 * 
 * DÉCOUVERTE MAJEURE C592:
 * - MEDIA_INTERFACE_DESCRIPTOR_LOAD: 0x61010011 (19 DWords) vs 0x70020000 (4 DWords)
 * - GPGPU_WALKER: 0x61020001 vs 0x71050000
 * 
 * Source: Batch OpenCL capturé C590 (opencl_batch_c590_capture_1.txt)
 * Base: test_c591_vfe_fix.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <xf86drm.h>
#include <i915_drm.h>

/* Gen9 Command Opcodes - CORRECTED FROM OPENCL CAPTURE C590 */
#define GEN9_PIPE_CONTROL           0x7a000004
#define GEN9_STATE_BASE_ADDRESS     0x61040001
#define GEN9_MEDIA_VFE_STATE        0x70000007  // 9 DWords
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x61010011  // FIX C592: 19 DWords (OpenCL)
#define GEN9_GPGPU_WALKER           0x61020001  // FIX C592: OpenCL opcode
#define GEN9_MI_BATCH_BUFFER_END    0x05000000

/* Heap offsets */
#define OFFSET_KERNEL_ISA    0x0000
#define OFFSET_SURFACE_STATE 0x1000
#define OFFSET_BINDING_TABLE 0x1100
#define OFFSET_IDRT          0x1200
#define OFFSET_OUTPUT        0x2000

/* Kernel ISA Gen9 (128 bytes) - output[0] = 0x12345678 */
static const uint32_t kernel_isa[] = {
    0x00000065, 0x01000010, 0x20000004, 0x00000000,
    0x00000061, 0x02050220, 0x00000000, 0x00000000,
    0x00000041, 0x03000010, 0x20000004, 0x00000000,
    0x00000001, 0x04000010, 0x20000004, 0x12345678,
    0x00000031, 0x05000010, 0x20000004, 0x00000000,
    0x00000001, 0x06000010, 0x20000004, 0x00000000,
    0x00000001, 0x07000010, 0x20000004, 0x00000000,
    0x00000001, 0x08000010, 0x20000004, 0x00000000,
    0x00000001, 0x09000010, 0x20000004, 0x00000000,
    0x00000001, 0x0a000010, 0x20000004, 0x00000000,
    0x00000001, 0x0b000010, 0x20000004, 0x00000000,
    0x00000001, 0x0c000010, 0x20000004, 0x00000000,
    0x00000001, 0x0d000010, 0x20000004, 0x00000000,
    0x00000001, 0x0e000010, 0x20000004, 0x00000000,
    0x00000001, 0x0f000010, 0x20000004, 0x00000000,
    0x00000001, 0x10000010, 0x20000004, 0x00000000,
    0x00000001, 0x11000010, 0x20000004, 0x00000000,
    0x00000001, 0x12000010, 0x20000004, 0x00000000,
    0x00000001, 0x13000010, 0x20000004, 0x00000000,
    0x00000001, 0x14000010, 0x20000004, 0x00000000,
    0x00000001, 0x15000010, 0x20000004, 0x00000000,
    0x00000001, 0x16000010, 0x20000004, 0x00000000,
    0x00000001, 0x17000010, 0x20000004, 0x00000000,
    0x00000001, 0x18000010, 0x20000004, 0x00000000,
    0x00000001, 0x19000010, 0x20000004, 0x00000000,
    0x00000001, 0x1a000010, 0x20000004, 0x00000000,
    0x00000001, 0x1b000010, 0x20000004, 0x00000000,
    0x00000001, 0x1c000010, 0x20000004, 0x00000000,
    0x00000001, 0x1d000010, 0x20000004, 0x00000000,
    0x00000001, 0x1e000010, 0x20000004, 0x00000000,
    0x00000001, 0x1f000010, 0x20000004, 0x00000000,
    0x00000001, 0x20000010, 0x20000004, 0x00000000
};

static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main(void) {
    printf("🚀 TEST C592 - FIX OPCODES OPENCL (ROOT CAUSE #139)\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    uint64_t start_time = get_time_ns();
    int fd, ret;
    uint32_t heap_handle, batch_handle, ctx_id;
    void *heap_ptr, *batch_ptr;
    
    /* Open DRM device */
    printf("[  5%%] Opening DRM device...\n");
    uint64_t t0 = get_time_ns();
    fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("Failed to open DRM device");
        return 1;
    }
    printf("✅ DRM device opened: fd=%d (%.3f µs)\n\n", fd, (get_time_ns() - t0) / 1000.0);
    
    /* Create GEM Context */
    printf("[ 10%%] Creating GEM Context...\n");
    t0 = get_time_ns();
    struct drm_i915_gem_context_create ctx_create = {0};
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    if (ret) {
        perror("Failed to create context");
        close(fd);
        return 1;
    }
    ctx_id = ctx_create.ctx_id;
    printf("✅ GEM Context created: ctx_id=%u (%.3f µs)\n\n", ctx_id, (get_time_ns() - t0) / 1000.0);
    
    /* Create heap buffer (64KB) */
    printf("[ 20%%] Creating heap buffer (64KB)...\n");
    t0 = get_time_ns();
    struct drm_i915_gem_create heap_create = {
        .size = 65536
    };
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &heap_create);
    if (ret) {
        perror("Failed to create heap buffer");
        close(fd);
        return 1;
    }
    heap_handle = heap_create.handle;
    printf("✅ Heap buffer created: handle=%u, size=%llu (%.3f µs)\n\n", 
           heap_handle, heap_create.size, (get_time_ns() - t0) / 1000.0);
    
    /* Create batch buffer (4KB) */
    printf("[ 25%%] Creating batch buffer (4KB)...\n");
    t0 = get_time_ns();
    struct drm_i915_gem_create batch_create = {
        .size = 4096
    };
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create);
    if (ret) {
        perror("Failed to create batch buffer");
        close(fd);
        return 1;
    }
    batch_handle = batch_create.handle;
    printf("✅ Batch buffer created: handle=%u, size=%llu (%.3f µs)\n\n", 
           batch_handle, batch_create.size, (get_time_ns() - t0) / 1000.0);
    
    /* Map heap buffer */
    printf("[ 30%%] Mapping heap buffer...\n");
    t0 = get_time_ns();
    struct drm_i915_gem_mmap heap_mmap = {
        .handle = heap_handle,
        .offset = 0,
        .size = 65536,
        .flags = 0
    };
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &heap_mmap);
    if (ret) {
        perror("Failed to map heap buffer");
        close(fd);
        return 1;
    }
    heap_ptr = (void *)(uintptr_t)heap_mmap.addr_ptr;
    printf("✅ Heap buffer mapped: %p (%.3f µs)\n\n", heap_ptr, (get_time_ns() - t0) / 1000.0);
    
    /* Map batch buffer */
    printf("[ 35%%] Mapping batch buffer...\n");
    t0 = get_time_ns();
    struct drm_i915_gem_mmap batch_mmap = {
        .handle = batch_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &batch_mmap);
    if (ret) {
        perror("Failed to map batch buffer");
        munmap(heap_ptr, 65536);
        close(fd);
        return 1;
    }
    batch_ptr = (void *)(uintptr_t)batch_mmap.addr_ptr;
    printf("✅ Batch buffer mapped: %p (%.3f µs)\n\n", batch_ptr, (get_time_ns() - t0) / 1000.0);
    
    /* Setup heap layout */
    printf("[ 40%%] Setting up heap layout...\n");
    t0 = get_time_ns();
    uint32_t *heap = (uint32_t *)heap_ptr;
    memset(heap, 0, 65536);
    
    /* Copy kernel ISA */
    memcpy(&heap[OFFSET_KERNEL_ISA / 4], kernel_isa, sizeof(kernel_isa));
    printf("  ✅ Kernel ISA copied to offset 0x%04x (%zu bytes)\n", OFFSET_KERNEL_ISA, sizeof(kernel_isa));
    
    /* Surface State (output buffer) */
    uint32_t *surface_state = &heap[OFFSET_SURFACE_STATE / 4];
    surface_state[0] = 0x00000000;
    surface_state[1] = OFFSET_OUTPUT;
    surface_state[2] = 0x00000100;
    surface_state[3] = 0x00000000;
    printf("  ✅ Surface State setup at offset 0x%04x (output @ 0x%04x)\n", 
           OFFSET_SURFACE_STATE, OFFSET_OUTPUT);
    
    /* Binding Table */
    uint32_t *binding_table = &heap[OFFSET_BINDING_TABLE / 4];
    binding_table[0] = OFFSET_SURFACE_STATE;
    printf("  ✅ Binding Table setup at offset 0x%04x (BTI[0] = 0x%04x)\n", 
           OFFSET_BINDING_TABLE, OFFSET_SURFACE_STATE);
    
    /* Interface Descriptor */
    uint32_t *idrt = &heap[OFFSET_IDRT / 4];
    idrt[0] = OFFSET_KERNEL_ISA;
    idrt[1] = 0x00000000;
    idrt[2] = 0x00000000;
    idrt[3] = OFFSET_BINDING_TABLE;
    idrt[4] = 0x00000000;
    idrt[5] = 0x00000000;
    idrt[6] = 0x00000000;
    idrt[7] = 0x00000000;
    printf("  ✅ IDRT setup at offset 0x%04x\n", OFFSET_IDRT);
    printf("      Kernel Start: 0x%04x\n", OFFSET_KERNEL_ISA);
    printf("      Binding Table: 0x%04x\n", OFFSET_BINDING_TABLE);
    
    /* Initialize output buffer */
    uint32_t *output = &heap[OFFSET_OUTPUT / 4];
    output[0] = 0xDEADBEEF;
    printf("  ✅ Output buffer initialized at offset 0x%04x (sentinel: 0x%08x)\n", 
           OFFSET_OUTPUT, output[0]);
    
    printf("✅ Heap layout complete (%.3f µs)\n\n", (get_time_ns() - t0) / 1000.0);
    
    /* Build batch commands */
    printf("[ 50%%] Building batch commands...\n");
    t0 = get_time_ns();
    uint32_t *batch = (uint32_t *)batch_ptr;
    memset(batch, 0, 4096);
    int offset = 0;
    
    printf("  Building batch commands:\n");
    
    /* 1. PIPE_CONTROL (flush initial) */
    printf("    [%2d] PIPE_CONTROL (flush initial)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL;
    batch[offset++] = 0x00100000;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 2. STATE_BASE_ADDRESS - 19 DWords (relocation target) */
    printf("    [%2d] STATE_BASE_ADDRESS (relocation target)\n", offset);
    batch[offset++] = GEN9_STATE_BASE_ADDRESS;
    batch[offset++] = 0;  // General State Base (will be relocated)
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 3. MEDIA_VFE_STATE - 9 DWords - FIX C591: Configuration OpenCL */
    printf("    [%2d] MEDIA_VFE_STATE (FIX C591 OpenCL config)\n", offset);
    batch[offset++] = GEN9_MEDIA_VFE_STATE;
    batch[offset++] = 0;  // Scratch Space Base
    batch[offset++] = 0;  // Scratch Space High
    batch[offset++] = 0x00a70100;  // FIX C591: DWord 3 = Max Threads=256, URB Entries=167
    batch[offset++] = 0;  // Max Threads (unused in DWord 4)
    batch[offset++] = 0x07820000;  // FIX C591: DWord 5 = URB Entry Size
    batch[offset++] = 0;  // CURBE Allocation
    batch[offset++] = 0x00000100;  // Max threads = 256
    batch[offset++] = 0;  // Reserved
    
    /* 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD - FIX C592: 19 DWords (OpenCL) */
    printf("    [%2d] MEDIA_INTERFACE_DESCRIPTOR_LOAD (FIX C592: 19 DWords OpenCL)\n", offset);
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD;
    batch[offset++] = 0;
    batch[offset++] = 32;  // Descriptor length
    batch[offset++] = OFFSET_IDRT;  // IDRT offset (relatif heap base)
    /* Padding to 19 DWords total */
    for (int i = 0; i < 15; i++) {
        batch[offset++] = 0;
    }
    
    /* 5. GPGPU_WALKER - FIX C592: OpenCL opcode 0x61020001 */
    printf("    [%2d] GPGPU_WALKER (FIX C592: OpenCL opcode)\n", offset);
    batch[offset++] = GEN9_GPGPU_WALKER;
    batch[offset++] = 0;  // Interface descriptor offset
    batch[offset++] = 0;  // Indirect data length
    batch[offset++] = 0;  // Indirect data start
    batch[offset++] = 1;  // Thread width
    batch[offset++] = 1;  // Thread height
    batch[offset++] = 1;  // Thread depth
    batch[offset++] = 1;  // Thread group width
    batch[offset++] = 1;  // Thread group height
    batch[offset++] = 1;  // Thread group depth
    batch[offset++] = 0xFFFFFFFF;  // Right execution mask
    batch[offset++] = 0xFFFFFFFF;  // Bottom execution mask
    batch[offset++] = 0;  // SIMD size
    batch[offset++] = 0;  // Local X/Y/Z
    batch[offset++] = 0;  // Reserved
    
    /* 6. PIPE_CONTROL (flush final) */
    printf("    [%2d] PIPE_CONTROL (flush final)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL;
    batch[offset++] = 0x00100000;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 7. MI_BATCH_BUFFER_END */
    printf("    [%2d] MI_BATCH_BUFFER_END\n", offset);
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    int batch_length = offset * 4;
    printf("✅ Batch built: %d DWords (%d bytes, %.3f µs)\n\n", offset, batch_length, (get_time_ns() - t0) / 1000.0);
    
    /* Create relocation entries */
    printf("[ 60%%] Creating relocation entries...\n");
    t0 = get_time_ns();
    struct drm_i915_gem_relocation_entry relocs[2];
    
    /* Relocation 0: STATE_BASE_ADDRESS DWord 1 → heap base */
    relocs[0].target_handle = heap_handle;
    relocs[0].delta = 0;
    relocs[0].offset = 7 * 4;  // DWord 7 (after opcode + 6 DWords)
    relocs[0].presumed_offset = 0;
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[0].write_domain = I915_GEM_DOMAIN_RENDER;
    
    /* Relocation 1: MEDIA_INTERFACE_DESCRIPTOR_LOAD DWord 3 → IDRT */
    relocs[1].target_handle = heap_handle;
    relocs[1].delta = OFFSET_IDRT;
    relocs[1].offset = 37 * 4;  // DWord 37 (MIDL parameter 3)
    relocs[1].presumed_offset = 0;
    relocs[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[1].write_domain = 0;
    
    printf("✅ Relocations created (%.3f µs)\n", (get_time_ns() - t0) / 1000.0);
    printf("  [0] STATE_BASE_ADDRESS:\n");
    printf("      Target: heap, Delta: 0x%x, Offset: 0x%llx (DWord %d)\n",
           relocs[0].delta, relocs[0].offset, (int)(relocs[0].offset / 4));
    printf("  [1] MEDIA_INTERFACE_DESCRIPTOR_LOAD:\n");
    printf("      Target: heap, Delta: 0x%x, Offset: 0x%llx (DWord %d)\n\n",
           relocs[1].delta, relocs[1].offset, (int)(relocs[1].offset / 4));
    
    /* Prepare EXECBUFFER2 */
    printf("[ 70%%] Preparing EXECBUFFER2...\n");
    printf("  Configuration:\n");
    printf("    Context ID: %u\n", ctx_id);
    printf("    Ring: RCS (Render)\n");
    printf("    Buffer count: 2 (heap + batch)\n");
    printf("    Batch length: %d bytes\n", batch_length);
    printf("    Relocations: 2 (STATE_BASE_ADDRESS + MIDL → heap)\n\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[2];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = heap_handle;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;
    exec_objects[0].alignment = 0;
    exec_objects[0].offset = 0;
    exec_objects[0].flags = 0;
    
    exec_objects[1].handle = batch_handle;
    exec_objects[1].relocation_count = 2;
    exec_objects[1].relocs_ptr = (uintptr_t)relocs;
    exec_objects[1].alignment = 0;
    exec_objects[1].offset = 0;
    exec_objects[1].flags = 0;
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 2;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = batch_length;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx_id;
    
    /* Submit to GPU */
    printf("[ 80%%] Submitting to GPU...\n");
    t0 = get_time_ns();
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t exec_time = get_time_ns() - t0;
    
    if (ret) {
        printf("❌ EXECBUFFER2 FAILED! errno=%d (%s)\n", errno, strerror(errno));
        munmap(batch_ptr, 4096);
        munmap(heap_ptr, 65536);
        close(fd);
        return 1;
    }
    
    printf("✅ EXECBUFFER2 SUCCESS! (%.3f µs)\n", exec_time / 1000.0);
    printf("  Heap GTT:  0x%016llx\n", exec_objects[0].offset);
    printf("  Batch GTT: 0x%016llx\n\n", exec_objects[1].offset);
    
    /* Wait for GPU completion */
    printf("[ 90%%] Waiting for GPU completion...\n");
    t0 = get_time_ns();
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_handle,
        .timeout_ns = -1,
        .flags = 0
    };
    ret = ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    uint64_t wait_time = get_time_ns() - t0;
    
    if (ret) {
        printf("❌ GPU wait failed! errno=%d\n", errno);
    } else {
        printf("✅ GPU execution completed (%.3f ms)\n\n", wait_time / 1000000.0);
    }
    
    /* Read output buffer */
    printf("[ 95%%] Reading output buffer...\n");
    t0 = get_time_ns();
    uint32_t result = output[0];
    printf("  Output[0] = 0x%08x (%.3f µs)\n\n", result, (get_time_ns() - t0) / 1000.0);
    
    /* Validate result */
    printf("[100%%] Validating result...\n\n");
    uint64_t total_time = get_time_ns() - start_time;
    
    if (result == 0x12345678) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  🎉 SUCCÈS! PREMIER WRITE GPU NATIF i915 VALIDÉ!         ║\n");
        printf("║  Expected: 0x12345678                                     ║\n");
        printf("║  Got:      0x%08x                                     ║\n", result);
        printf("║  ⏱️  Temps total: %.3f ms                                 ║\n", total_time / 1000000.0);
        printf("║  🚀 ROOT CAUSE #139 RÉSOLU: Opcodes OpenCL corrects!     ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  RÉSULTAT INATTENDU                                   ║\n");
        printf("║  Expected: 0x12345678                                     ║\n");
        printf("║  Got:      0x%08x                                     ║\n", result);
        printf("║  ⏱️  Temps total: %.3f ms                                 ║\n", total_time / 1000000.0);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    /* Cleanup */
    printf("\nCleanup...\n");
    munmap(batch_ptr, 4096);
    munmap(heap_ptr, 65536);
    
    struct drm_i915_gem_context_destroy ctx_destroy = {
        .ctx_id = ctx_id
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    printf("✅ Context destroyed\n");
    
    struct drm_gem_close close_heap = { .handle = heap_handle };
    struct drm_gem_close close_batch = { .handle = batch_handle };
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_heap);
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_batch);
    
    close(fd);
    
    printf("\n✅ TEST C592 TERMINÉ\n");
    return (result == 0x12345678) ? 0 : 1;
}

// Made with Bob
