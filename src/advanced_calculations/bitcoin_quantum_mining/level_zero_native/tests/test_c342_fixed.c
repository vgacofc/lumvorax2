/**
 * C342 TEST MINIMAL DIAGNOSTIC - Version corrigée avec format exact btc_gen9_native_runner.c
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

#define LOG(fmt, ...) fprintf(stderr, "[C342] " fmt "\n", ##__VA_ARGS__)

// Gen9 command opcodes (from btc_gen9_native_runner.c)
#define GEN9_PIPE_CONTROL 0x7a000000
#define GEN9_PIPELINE_SELECT 0x69040000
#define GEN9_STATE_BASE_ADDRESS 0x61010000
#define GEN9_MEDIA_VFE_STATE 0x70000000
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020000
#define GEN9_GPGPU_WALKER 0x70050000
#define GEN9_MI_BATCH_BUFFER_END 0x0a000000

#define BASE_ADDRESS_MODIFY 0x1
#define CACHE_CTRL 0x3

// C342 FIX #17: Adresses GPU fixes (soft-pin) - Méthode Beignet
#define GPU_KERNEL_BASE  0x0000000000100000ULL
#define GPU_INPUT_BASE   0x0000000000200000ULL
#define GPU_OUTPUT_BASE  0x0000000000300000ULL
#define GPU_BATCH_BASE   0x0000000000400000ULL
#define GPU_SSH_BASE     0x0000000000500000ULL
#define GPU_DSH_BASE     0x0000000000600000ULL
#define GPU_IOH_BASE     0x0000000000700000ULL

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t kernel_bo;
    void* kernel_map;
    size_t kernel_size;
    uint32_t input_bo;
    void* input_map;
    uint32_t output_bo;
    void* output_map;
    uint32_t batch_bo;
    void* batch_map;
    uint32_t ssh_bo;
    void* ssh_map;
    uint32_t dsh_bo;
    void* dsh_map;
    uint32_t ioh_bo;
    void* ioh_map;
} test_context_t;

static int alloc_gpu_buffer(test_context_t* ctx, size_t size, uint32_t* bo_out, void** map_out) {
    struct drm_i915_gem_create create = { .size = size };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        LOG("GEM_CREATE failed: %s", strerror(errno));
        return -1;
    }
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = create.handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        LOG("GEM_MMAP failed: %s", strerror(errno));
        struct drm_gem_close close_arg = { .handle = create.handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        return -1;
    }
    
    *bo_out = create.handle;
    *map_out = (void*)(uintptr_t)mmap_arg.addr_ptr;
    memset(*map_out, 0, size);
    
    LOG("Allocated buffer: handle=%u size=%zu addr=%p", create.handle, size, *map_out);
    return 0;
}

// C342 FIX #15: SUPPRIMÉ - N'utiliser QUE des adresses GPU via relocations
// Les adresses CPU ne fonctionnent PAS dans les structures GPU

static int load_kernel(test_context_t* ctx, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        LOG("Failed to open kernel: %s", path);
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    ctx->kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (alloc_gpu_buffer(ctx, ctx->kernel_size, &ctx->kernel_bo, &ctx->kernel_map) < 0) {
        fclose(f);
        return -1;
    }
    
    fread(ctx->kernel_map, 1, ctx->kernel_size, f);
    fclose(f);
    
    LOG("Loaded kernel: %s size=%zu", path, ctx->kernel_size);
    return 0;
}

static void create_surface_states(test_context_t* ctx) {
    uint32_t* ssh = (uint32_t*)ctx->ssh_map;
    memset(ssh, 0, 4096);  // Clear SSH
    
    // Surface State 0: Input buffer (RAW) - 16 DWORDs (Gen9 format)
    // Offset 0x00 (DW0-15)
    ssh[0] = 0x87fc0000;  // DW0: Surface Type=BUFFER, Format=RAW, MOCS=0x1f
    ssh[1] = 0;           // DW1: Surface Base Address LOW (will be relocated)
    ssh[2] = 0x00ffffff;  // DW2: Width (16MB-1)
    ssh[3] = 0;           // DW3: Height, Depth
    ssh[4] = 0;           // DW4: Surface Pitch
    ssh[5] = 0;           // DW5: Reserved
    ssh[6] = 0;           // DW6: Reserved
    ssh[7] = 0;           // DW7: Reserved
    ssh[8] = 0x87fc0000;  // DW8: Duplicate DW0 (Gen9 quirk)
    ssh[9] = 0;           // DW9: Surface Base Address HIGH (will be relocated)
    ssh[10] = 0x00ffffff; // DW10: Duplicate Width
    ssh[11] = 0;          // DW11-15: Reserved
    ssh[12] = 0;
    ssh[13] = 0;
    ssh[14] = 0;
    ssh[15] = 0;
    
    // Binding Table at offset 0x40 (DW16-17)
    ssh[16] = 0;   // BTI[0]: Surface State 0 @ offset 0x00
    ssh[17] = 0x48; // BTI[1]: Surface State 1 @ offset 0x48 (C342 FIX #6)
    
    // Surface State 1: Output buffer (RAW) - 16 DWORDs (Gen9 format)   ²   ²   
    // Offset 0x48 (DW18-33)
    ssh[18] = 0x87fc0000;  // DW0: Surface Type=BUFFER, Format=RAW, MOCS=0x1f
    ssh[19] = 0;           // DW1: Surface Base Address LOW (will be relocated)
    ssh[20] = 0x00ffffff;  // DW2: Width (16MB-1)
    ssh[21] = 0;           // DW3: Height, Depth
    ssh[22] = 0;           // DW4: Surface Pitch
    ssh[23] = 0;           // DW5: Reserved
    ssh[24] = 0;           // DW6: Reserved
    ssh[25] = 0;           // DW7: Reserved
    ssh[26] = 0;           // DW8: Reserved (no duplicate for output)
    ssh[27] = 0;           // DW9: Surface Base Address HIGH (will be relocated)
    ssh[28] = 0;           // DW10-15: Reserved
    ssh[29] = 0;
    ssh[30] = 0;
    ssh[31] = 0;
    ssh[32] = 0;
    ssh[33] = 0;
    
    LOG("Created surface states + binding table (Gen9 16-DWORD format)");
}

static void create_interface_descriptor(test_context_t* ctx) {
    // C342 FIX #17: ADRESSE GPU ABSOLUE (soft-pin) - Méthode Beignet
    // Interface Descriptor in DSH
    uint32_t* dsh = (uint32_t*)ctx->dsh_map;
    
    // DW0-1: Kernel start pointer (adresse GPU ABSOLUE fixe)
    uint64_t kernel_gpu_addr = GPU_KERNEL_BASE + 0x40;  // Base GPU + offset code Gen9
    dsh[0] = (uint32_t)(kernel_gpu_addr & 0xFFFFFFFF);  // LOW 32 bits
    dsh[1] = (uint32_t)(kernel_gpu_addr >> 32);         // HIGH 32 bits
    dsh[2] = 0;  // Sampler state pointer
    dsh[3] = 0x00000040;  // Binding table pointer (offset 64 bytes = 0x40 in SSH)
    dsh[4] = 0;  // CURBE read length
    dsh[5] = 0x00400000;  // Number of threads in group (256 = 0x100, shifted left 10 bits = 0x00400000)
    dsh[6] = 0;
    dsh[7] = 0;
    
    LOG("Created interface descriptor in DSH: kernel_gpu_addr=0x%lx binding_table=0x40 threads=256 (SOFT-PIN)", kernel_gpu_addr);
}

static void dump_gpu_structures(test_context_t* ctx, uint32_t* batch, int batch_len) {
    LOG("========================================");
    LOG("DUMP GPU STRUCTURES (AVANT SUBMIT)");
    LOG("========================================");
    
    // Dump STATE_BASE_ADDRESS (19 DWORDs starting at batch offset 8)
    LOG("\n=== STATE_BASE_ADDRESS (19 DWORDs) ===");
    for (int i = 0; i < 19; i++) {
        LOG("  DW%02d: 0x%08x", i, batch[8 + i]);
    }
    
    // Dump INTERFACE_DESCRIPTOR (8 DWORDs in DSH)
    LOG("\n=== INTERFACE_DESCRIPTOR (8 DWORDs in DSH) ===");
    uint32_t* dsh = (uint32_t*)ctx->dsh_map;
    for (int i = 0; i < 8; i++) {
        LOG("  DW%02d: 0x%08x", i, dsh[i]);
    }
    
    // Dump BINDING_TABLE (2 entries at SSH offset 0x40)
    LOG("\n=== BINDING_TABLE (2 entries at SSH+0x40) ===");
    uint32_t* ssh = (uint32_t*)ctx->ssh_map;
    LOG("  BTI[0]: 0x%08x (Surface State 0 offset)", ssh[16]);
    LOG("  BTI[1]: 0x%08x (Surface State 1 offset)", ssh[17]);
    
    // Dump SURFACE_STATE 0 (16 DWORDs at offset 0x00)
    LOG("\n=== SURFACE_STATE 0 (Input, 16 DWORDs @ 0x00) ===");
    for (int i = 0; i < 16; i++) {
        LOG("  DW%02d: 0x%08x", i, ssh[i]);
    }
    
    // Dump SURFACE_STATE 1 (16 DWORDs at offset 0x48 = DW18)
    LOG("\n=== SURFACE_STATE 1 (Output, 16 DWORDs @ 0x48) ===");
    for (int i = 0; i < 16; i++) {
        LOG("  DW%02d: 0x%08x", i, ssh[18 + i]);
    }
    
    // Dump Buffer Addresses
    LOG("\n=== BUFFER ADDRESSES ===");
    LOG("  kernel_bo:  handle=%u addr=%p size=%zu", ctx->kernel_bo, ctx->kernel_map, ctx->kernel_size);
    LOG("  input_bo:   handle=%u addr=%p", ctx->input_bo, ctx->input_map);
    LOG("  output_bo:  handle=%u addr=%p", ctx->output_bo, ctx->output_map);
    LOG("  ssh_bo:     handle=%u addr=%p", ctx->ssh_bo, ctx->ssh_map);
    LOG("  dsh_bo:     handle=%u addr=%p", ctx->dsh_bo, ctx->dsh_map);
    LOG("  ioh_bo:     handle=%u addr=%p", ctx->ioh_bo, ctx->ioh_map);
    LOG("  batch_bo:   handle=%u addr=%p", ctx->batch_bo, ctx->batch_map);
    
    LOG("========================================\n");
}

static int run_test(test_context_t* ctx, const char* test_name, int kernel_offset) {
    LOG("========================================");
    LOG("TEST: %s (kernel_offset=%d)", test_name, kernel_offset);
    LOG("========================================");
    
    // Clear output buffer
    memset(ctx->output_map, 0, 4096);
    
    // Build batch buffer (EXACT format from btc_gen9_native_runner.c)
    uint32_t* batch = (uint32_t*)ctx->batch_map;
    int offset = 0;
    
    // PIPE_CONTROL initial
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000 | (1 << 20);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // PIPELINE_SELECT
    batch[offset++] = GEN9_PIPELINE_SELECT | (1 - 2);
    batch[offset++] = 0x00000002;
    
    // STATE_BASE_ADDRESS (C304 fix - 19 DWORDs)
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | 17;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 16);
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;  // DW10 - Instruction Base Address (will be relocated)
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000;
    
    // MEDIA_VFE_STATE
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = (24 << 16) | (24 << 8);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;
    batch[offset++] = (uint32_t)(GPU_DSH_BASE & 0xFFFFFFFF);  // C342 FIX #17: Adresse GPU absolue DSH
    
    // GPGPU_WALKER (C341 fix - dimensions 256/1/1)
    batch[offset++] = GEN9_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = (1 << 30);
    batch[offset++] = 0;
    batch[offset++] = 256;  // C341 FIX
    batch[offset++] = 0;
    batch[offset++] = 1;    // C341 FIX
    batch[offset++] = 0;
    batch[offset++] = 1;    // C341 FIX
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    batch[offset++] = 1;    // Thread Group Count X
    batch[offset++] = 1;
    
    // PIPE_CONTROL final
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00101004;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    int batch_len = offset * 4;
    LOG("Batch buffer: %d commands, %d bytes", offset, batch_len);
    
    // DUMP GPU STRUCTURES AVANT SUBMIT
    dump_gpu_structures(ctx, batch, offset);
    
    // C342 FIX #14: Setup relocations - HARDWARE-AWARE
    // Relocations UNIQUEMENT pour Surface States (seul endroit où i915 les supporte)
    struct drm_i915_gem_relocation_entry relocs_batch[2];  // 2 relocations: SSH, DSH seulement
    struct drm_i915_gem_relocation_entry relocs_ssh[4];  // 4 relocations (DW1+DW9 pour input et output)
    memset(relocs_batch, 0, sizeof(relocs_batch));
    memset(relocs_ssh, 0, sizeof(relocs_ssh));
    
    // C342 FIX #13: Relocation 0: SSH in STATE_BASE_ADDRESS Surface State Base
    relocs_batch[0].target_handle = ctx->ssh_bo;
    relocs_batch[0].delta = 0;
    relocs_batch[0].offset = (6 + 2 + 4) * 4;  // PIPE_CONTROL(6) + PIPELINE_SELECT(2) + SBA DW04
    relocs_batch[0].presumed_offset = 0;
    relocs_batch[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_batch[0].write_domain = 0;
    
    // C342 FIX #13: Relocation 1: DSH in STATE_BASE_ADDRESS Dynamic State Base
    relocs_batch[1].target_handle = ctx->dsh_bo;
    relocs_batch[1].delta = 0;
    relocs_batch[1].offset = (6 + 2 + 6) * 4;  // PIPE_CONTROL(6) + PIPELINE_SELECT(2) + SBA DW06
    relocs_batch[1].presumed_offset = 0;
    relocs_batch[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_batch[1].write_domain = 0;
    
    // Relocation 1: Input buffer in Surface State 0 DW01 (offset 0x04)
    relocs_ssh[0].target_handle = ctx->input_bo;
    relocs_ssh[0].delta = 0;
    relocs_ssh[0].offset = 0x04;  // Surface State 0 @ 0x00, DW01 @ +0x04
    relocs_ssh[0].presumed_offset = 0;  // C342 FIX #7: Force i915 à appliquer relocation
    relocs_ssh[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_ssh[0].write_domain = 0;
    
    // Relocation 2: Input buffer in Surface State 0 DW09 (offset 0x24)
    relocs_ssh[1].target_handle = ctx->input_bo;
    relocs_ssh[1].delta = 0;
    relocs_ssh[1].offset = 0x24;  // Surface State 0 @ 0x00, DW09 @ +0x24
    relocs_ssh[1].presumed_offset = 0;  // C342 FIX #7: Force i915 à appliquer relocation
    relocs_ssh[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_ssh[1].write_domain = 0;
    
    // Relocation 3: Output buffer in Surface State 1 DW01 (offset 0x4C)
    // C342 FIX #6: Surface State 1 @ 0x48 (DW18), DW01 @ +0x04 = 0x4C
    relocs_ssh[2].target_handle = ctx->output_bo;
    relocs_ssh[2].delta = 0;
    relocs_ssh[2].offset = 0x4C;  // 0x48 + 0x04 (FIX C342 ROOT CAUSE)
    relocs_ssh[2].presumed_offset = 0;  // C342 FIX #7: Force i915 à appliquer relocation
    relocs_ssh[2].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_ssh[2].write_domain = I915_GEM_DOMAIN_RENDER;
    
    // Relocation 4: Output buffer in Surface State 1 DW09 (offset 0x6C)
    // C342 FIX #6: Surface State 1 @ 0x48 (DW18), DW09 @ +0x24 = 0x6C
    relocs_ssh[3].target_handle = ctx->output_bo;
    relocs_ssh[3].delta = 0;
    relocs_ssh[3].offset = 0x6C;  // 0x48 + 0x24 (FIX C342 ROOT CAUSE)
    relocs_ssh[3].presumed_offset = 0;  // C342 FIX #7: Force i915 à appliquer relocation
    relocs_ssh[3].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs_ssh[3].write_domain = I915_GEM_DOMAIN_RENDER;
    
    // C342 FIX #17: EXEC_OBJECT_PINNED (soft-pin) - Méthode Beignet
    // Fixer adresses GPU pour tous les buffers
    
    // Setup exec_objects avec PINNED flags
    struct drm_i915_gem_exec_object2 exec_objects[7] = {
        { .handle = ctx->kernel_bo, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = GPU_KERNEL_BASE },
        { .handle = ctx->input_bo, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = GPU_INPUT_BASE },
        { .handle = ctx->output_bo, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE, .offset = GPU_OUTPUT_BASE },
        { .handle = ctx->ssh_bo, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = GPU_SSH_BASE },
        { .handle = ctx->dsh_bo, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = GPU_DSH_BASE },
        { .handle = ctx->ioh_bo, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = GPU_IOH_BASE },
        { .handle = ctx->batch_bo, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = GPU_BATCH_BASE }
    };
    
    LOG("Using EXEC_OBJECT_PINNED (soft-pin) - kernel=0x%lx input=0x%lx output=0x%lx",
        GPU_KERNEL_BASE, GPU_INPUT_BASE, GPU_OUTPUT_BASE);
    
    // Execute
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 7,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx->ctx_id
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        LOG("EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        return -1;
    }
    
    LOG("EXECBUFFER2 success!");
    
    // Wait for GPU completion
    struct drm_i915_gem_wait wait_arg = {
        .bo_handle = ctx->output_bo,
        .timeout_ns = 1000000000,
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg) < 0) {
        LOG("GEM_WAIT failed: %s", strerror(errno));
        return -1;
    }
    
    // C342 FIX #8: Synchronize GPU→CPU for all buffers to see relocations
    LOG("Synchronizing GPU→CPU (GEM_SET_DOMAIN)...");
    
    struct drm_i915_gem_set_domain set_domain_ssh = {
        .handle = ctx->ssh_bo,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain_ssh) < 0) {
        LOG("GEM_SET_DOMAIN ssh failed: %s", strerror(errno));
    }
    
    struct drm_i915_gem_set_domain set_domain_dsh = {
        .handle = ctx->dsh_bo,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain_dsh) < 0) {
        LOG("GEM_SET_DOMAIN dsh failed: %s", strerror(errno));
    }
    
    struct drm_i915_gem_set_domain set_domain_output = {
        .handle = ctx->output_bo,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain_output) < 0) {
        LOG("GEM_SET_DOMAIN output failed: %s", strerror(errno));
    }
    
    // C342 FIX #9: Sync batch buffer to see STATE_BASE_ADDRESS relocations
    struct drm_i915_gem_set_domain set_domain_batch = {
        .handle = ctx->batch_bo,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain_batch) < 0) {
        LOG("GEM_SET_DOMAIN batch failed: %s", strerror(errno));
    }
    
    // Dump GPU structures AFTER sync to see relocations
    LOG("========================================");
    LOG("DUMP GPU STRUCTURES (APRÈS SYNC GPU→CPU)");
    LOG("========================================");
    
    // C342 FIX #11: Dump PIPE_CONTROL (6 DWORDs)
    LOG("\n=== PIPE_CONTROL (6 DWORDs) ===");
    for (int i = 0; i < 6; i++) {
        LOG("  DW%02d: 0x%08x", i, batch[i]);
    }
    
    // C342 FIX #11: Dump PIPELINE_SELECT (2 DWORDs)
    LOG("\n=== PIPELINE_SELECT (2 DWORDs) ===");
    for (int i = 0; i < 2; i++) {
        LOG("  DW%02d: 0x%08x", i, batch[6 + i]);
    }
    
    // C342 FIX #11: Dump STATE_BASE_ADDRESS from batch buffer AFTER relocations
    // STATE_BASE_ADDRESS commence à l'offset 8 (après PIPE_CONTROL + PIPELINE_SELECT)
    LOG("\n=== STATE_BASE_ADDRESS (19 DWORDs) APRÈS RELOCATIONS ===");
    for (int i = 0; i < 19; i++) {
        LOG("  DW%02d: 0x%08x", i, batch[8 + i]);
    }
    
    // Dump SURFACE_STATE 0 (Input, 16 DWORDs at offset 0x00)
    LOG("\n=== SURFACE_STATE 0 (Input, 16 DWORDs @ 0x00) APRÈS SYNC ===");
    uint32_t* ssh = (uint32_t*)ctx->ssh_map;
    for (int i = 0; i < 16; i++) {
        LOG("  DW%02d: 0x%08x", i, ssh[i]);
    }
    
    // Dump SURFACE_STATE 1 (Output, 16 DWORDs at offset 0x48 = DW18)
    LOG("\n=== SURFACE_STATE 1 (Output, 16 DWORDs @ 0x48) APRÈS SYNC ===");
    for (int i = 0; i < 16; i++) {
        LOG("  DW%02d: 0x%08x", i, ssh[18 + i]);
    }
    
    // Dump INTERFACE_DESCRIPTOR (8 DWORDs in DSH)
    LOG("\n=== INTERFACE_DESCRIPTOR (8 DWORDs in DSH) APRÈS SYNC ===");
    uint32_t* dsh = (uint32_t*)ctx->dsh_map;
    for (int i = 0; i < 8; i++) {
        LOG("  DW%02d: 0x%08x", i, dsh[i]);
    }
    
    LOG("========================================\n");
    
    // Read results
    uint32_t* output = (uint32_t*)ctx->output_map;
    LOG("Results:");
    for (int i = 0; i < 16; i++) {
        LOG("  output[%d] = 0x%08x (%u)", i, output[i], output[i]);
    }
    
    return 0;
}

int main() {
    test_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    ctx.drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.drm_fd < 0) {
        LOG("Failed to open DRM device");
        return 1;
    }
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx.drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        LOG("Failed to create context");
        close(ctx.drm_fd);
        return 1;
    }
    ctx.ctx_id = ctx_create.ctx_id;
    LOG("Created context: ctx_id=%u", ctx.ctx_id);
    
    if (load_kernel(&ctx, "kernels/test_diagnostic_c342.bin_kbl.bin") < 0) {
        close(ctx.drm_fd);
        return 1;
    }
    
    alloc_gpu_buffer(&ctx, 4096, &ctx.input_bo, &ctx.input_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.output_bo, &ctx.output_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.batch_bo, &ctx.batch_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.ssh_bo, &ctx.ssh_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.dsh_bo, &ctx.dsh_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.ioh_bo, &ctx.ioh_map);
    
    // Initialize input
    uint32_t* input = (uint32_t*)ctx.input_map;
    for (int i = 0; i < 256; i++) {
        input[i] = i * 100;
    }
    
    create_surface_states(&ctx);
    create_interface_descriptor(&ctx);
    
    // Run tests
    run_test(&ctx, "TEST #1: Hardcoded Write", 0);
    
    LOG("========================================");
    LOG("Test completed");
    LOG("========================================");
    
    close(ctx.drm_fd);
    return 0;
}

// Made with Bob
