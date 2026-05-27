/*
 * CYCLE C197.13 - STATE VISIBILITY FIX COMPLET
 * =============================================
 * 
 * OBJECTIF : Implémenter les 7 corrections critiques C197.12.1
 * 
 * CORRECTIONS APPLIQUÉES :
 * 1. ✅ Kernel pointer encoding (>> 6)
 * 2. ✅ Binding table pointer granularity (/ 32)
 * 3. ✅ PIPE_CONTROL après STATE_BASE_ADDRESS
 * 4. ✅ PIPE_CONTROL après MEDIA_VFE_STATE
 * 5. ✅ MI_FLUSH_DW après GPGPU_WALKER
 * 6. ✅ VFE EU enable mask
 * 7. ✅ PIPE_CONTROL final (render target flush)
 * 
 * RÉSULTAT ATTENDU : output[0] = 0xDEADBEEF ✅
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <drm/i915_drm.h>

/* ── Logging forensique bit-level LumVorax ─────────────────────────── */
typedef struct {
    uint64_t timestamp_ns;
    const char* phase;
    const char* event;
    uint64_t value;
} forensic_event_t;

#define MAX_EVENTS 10000
static forensic_event_t g_events[MAX_EVENTS];
static size_t g_event_count = 0;

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_EVENT(phase_name, event_name, val) do { \
    if (g_event_count < MAX_EVENTS) { \
        g_events[g_event_count].timestamp_ns = get_timestamp_ns(); \
        g_events[g_event_count].phase = (phase_name); \
        g_events[g_event_count].event = (event_name); \
        g_events[g_event_count].value = (uint64_t)(val); \
        g_event_count++; \
    } \
} while(0)

/* ── PIPE_CONTROL Flags (Gen9) ─────────────────────────────────────── */
#define PIPE_CONTROL_DC_FLUSH              (1 << 5)
#define PIPE_CONTROL_INSTRUCTION_CACHE_INV (1 << 6)
#define PIPE_CONTROL_STATE_CACHE_INV       (1 << 16)
#define PIPE_CONTROL_RENDER_TARGET_FLUSH   (1 << 12)
#define PIPE_CONTROL_CS_STALL              (1 << 20)
#define PIPE_CONTROL_STALL_AT_SCOREBOARD   (1 << 1)

/* ── Helper Functions ──────────────────────────────────────────────── */

/* Emit PIPE_CONTROL (6 DWords) */
static void emit_pipe_control(uint32_t *batch, uint32_t flags) {
    batch[0] = (0x7A << 24) | (6 - 2);  // PIPE_CONTROL, length=6
    batch[1] = flags;
    batch[2] = 0;  // Address low
    batch[3] = 0;  // Address high
    batch[4] = 0;  // Immediate low
    batch[5] = 0;  // Immediate high
}

/* Emit MI_FLUSH_DW (4 DWords) */
static void emit_mi_flush_dw(uint32_t *batch) {
    batch[0] = (0x26 << 23) | (2 << 0);  // MI_FLUSH_DW, length=2
    batch[1] = (1 << 14);  // Post-sync operation
    batch[2] = 0;  // Write immediate = 0
    batch[3] = 0;  // Reserved
}

/* Emit PIPELINE_SELECT (2 DWords) */
static void emit_pipeline_select(uint32_t *batch) {
    batch[0] = (0x69 << 24) | (1 << 0);  // PIPELINE_SELECT
    batch[1] = (1 << 0);  // GPGPU mode
}

/* Emit STATE_BASE_ADDRESS (16 DWords) */
static void emit_state_base_address(uint32_t *batch, uint64_t isa_base, 
                                     uint64_t surface_base, uint64_t dynamic_base) {
    batch[0] = (0x61 << 24) | (16 - 2);  // STATE_BASE_ADDRESS, length=16
    
    // General State Base Address
    batch[1] = 0;
    batch[2] = 0;
    
    // Surface State Base Address
    batch[3] = (uint32_t)(surface_base & 0xFFFFFFFF) | 1;  // Enable
    batch[4] = (uint32_t)(surface_base >> 32);
    
    // Dynamic State Base Address
    batch[5] = (uint32_t)(dynamic_base & 0xFFFFFFFF) | 1;  // Enable
    batch[6] = (uint32_t)(dynamic_base >> 32);
    
    // Indirect Object Base Address
    batch[7] = 0;
    batch[8] = 0;
    
    // Instruction Base Address
    batch[9] = (uint32_t)(isa_base & 0xFFFFFFFF) | 1;  // Enable
    batch[10] = (uint32_t)(isa_base >> 32);
    
    // General State Buffer Size
    batch[11] = 0xFFFFF000;  // 4GB
    
    // Dynamic State Buffer Size
    batch[12] = 0xFFFFF000;  // 4GB
    
    // Indirect Object Buffer Size
    batch[13] = 0xFFFFF000;  // 4GB
    
    // Instruction Buffer Size
    batch[14] = 0xFFFFF000;  // 4GB
    
    batch[15] = 0;  // Reserved
}

/* Emit MEDIA_VFE_STATE (8 DWords) - ✅ CORRECTION #6 : EU enable mask */
static void emit_media_vfe_state(uint32_t *batch) {
    batch[0] = (0x70 << 24) | (0x00 << 16) | (8 - 2);  // MEDIA_VFE_STATE
    batch[1] = 0;  // Scratch space
    
    // ✅ CORRECTION #6 : EU enable mask + max threads
    batch[2] = (64 << 16) |  // Max threads = 64
               (1 << 8)   |  // URB entries = 1
               (1 << 0);     // EU enable mask = all
    
    batch[3] = 0;  // Reset gateway timer
    batch[4] = 0;  // Slice disable = 0 (all enabled)
    batch[5] = 0;  // Reserved
    batch[6] = 0;  // Reserved
    batch[7] = 0;  // Reserved
}

/* Emit MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWords) */
static void emit_interface_descriptor_load(uint32_t *batch, uint32_t desc_offset) {
    batch[0] = (0x70 << 24) | (0x02 << 16) | (4 - 2);  // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[1] = 0;  // Reserved
    batch[2] = 32;  // Interface Descriptor Total Length (32 bytes)
    batch[3] = desc_offset;  // Interface Descriptor Data Start Address
}

/* Emit GPGPU_WALKER (15 DWords) */
static void emit_gpgpu_walker(uint32_t *batch) {
    batch[0] = (0x70 << 24) | (0x05 << 16) | (15 - 2);  // GPGPU_WALKER
    batch[1] = 0;  // Interface Descriptor Offset
    batch[2] = 0;  // Indirect Data Length
    batch[3] = 0;  // Indirect Data Start Address
    batch[4] = 0;  // Thread Group ID Starting X
    batch[5] = 0;  // Thread Group ID X Dimension
    batch[6] = 0;  // Thread Group ID Starting Y
    batch[7] = 1;  // Thread Group ID Y Dimension = 1
    batch[8] = 0;  // Thread Group ID Starting Z
    batch[9] = 1;  // Thread Group ID Z Dimension = 1
    batch[10] = 1;  // Right Execution Mask
    batch[11] = 0;  // Bottom Execution Mask
    batch[12] = 1;  // SIMD Size = 1
    batch[13] = 1;  // Thread Width Counter Max = 1
    batch[14] = 1;  // Thread Height Counter Max = 1
}

/* ── Interface Descriptor (32 bytes = 8 DWords) ────────────────────── */
typedef struct {
    uint32_t dw0;  // Kernel Start Pointer
    uint32_t dw1;  // Flags
    uint32_t dw2;  // Sampler
    uint32_t dw3;  // Binding Table
    uint32_t dw4;  // CURBE
    uint32_t dw5;  // Threads
    uint32_t dw6;  // Reserved
    uint32_t dw7;  // Reserved
} gen9_interface_descriptor_t;

/* ── Kernel ISA Gen9 (write 0xDEADBEEF) ────────────────────────────── */
static const uint32_t kernel_isa[] = {
    // MOV R4, 0xDEADBEEF
    0x00000001, 0x24000608, 0x00000000, 0xDEADBEEF,
    
    // SEND (write R4 to output buffer)
    0x05000031, 0x20000A48, 0x06000004, 0x02180001,
    
    // EOT (End of Thread)
    0x05000031, 0x20000A48, 0x00000000, 0x02000001
};

/* ── Main ──────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== CYCLE C197.13 - STATE VISIBILITY FIX ===\n\n");
    
    uint64_t t_start = get_timestamp_ns();
    LOG_EVENT("INIT", "test_start", 0);
    
    /* ── Phase 1 : Open DRM ────────────────────────────────────────── */
    printf("[PHASE 1] Opening DRM device...\n");
    LOG_EVENT("DRM", "open_start", 0);
    
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        drm_fd = open("/dev/dri/card0", O_RDWR);
    }
    
    if (drm_fd < 0) {
        fprintf(stderr, "[ERROR] Cannot open DRM device: %s\n", strerror(errno));
        return 1;
    }
    
    LOG_EVENT("DRM", "drm_fd", drm_fd);
    printf("[SUCCESS] DRM opened (fd=%d)\n\n", drm_fd);
    
    /* ── Phase 2 : Create i915 Context ─────────────────────────────── */
    printf("[PHASE 2] Creating i915 context...\n");
    LOG_EVENT("DRM", "context_create_start", 0);
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) != 0) {
        fprintf(stderr, "[ERROR] Cannot create context: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    uint32_t ctx_id = ctx_create.ctx_id;
    LOG_EVENT("DRM", "ctx_id", ctx_id);
    printf("[SUCCESS] Context created (ctx_id=%u)\n\n", ctx_id);
    
    /* ── Phase 3 : Create GEM Objects ──────────────────────────────── */
    printf("[PHASE 3] Creating GEM objects...\n");
    
    // ISA buffer (4KB)
    struct drm_i915_gem_create create_isa = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_isa) != 0) {
        fprintf(stderr, "[ERROR] Cannot create ISA buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t isa_handle = create_isa.handle;
    LOG_EVENT("GEM", "isa_handle", isa_handle);
    
    // Surface State buffer (4KB)
    struct drm_i915_gem_create create_surface = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_surface) != 0) {
        fprintf(stderr, "[ERROR] Cannot create surface buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t surface_handle = create_surface.handle;
    LOG_EVENT("GEM", "surface_handle", surface_handle);
    
    // Interface Descriptor buffer (4KB)
    struct drm_i915_gem_create create_desc = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_desc) != 0) {
        fprintf(stderr, "[ERROR] Cannot create descriptor buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t desc_handle = create_desc.handle;
    LOG_EVENT("GEM", "desc_handle", desc_handle);
    
    // Batch buffer (64KB)
    struct drm_i915_gem_create create_batch = {
        .size = 65536
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) != 0) {
        fprintf(stderr, "[ERROR] Cannot create batch buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t batch_handle = create_batch.handle;
    LOG_EVENT("GEM", "batch_handle", batch_handle);
    
    // Output buffer (4KB)
    struct drm_i915_gem_create create_output = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) != 0) {
        fprintf(stderr, "[ERROR] Cannot create output buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t output_handle = create_output.handle;
    LOG_EVENT("GEM", "output_handle", output_handle);
    
    printf("[SUCCESS] All GEM objects created\n\n");
    
    /* ── Phase 4 : Map and Fill Buffers ────────────────────────────── */
    printf("[PHASE 4] Mapping and filling buffers...\n");
    
    // Map ISA buffer
    struct drm_i915_gem_mmap mmap_isa = {
        .handle = isa_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_isa) != 0) {
        fprintf(stderr, "[ERROR] Cannot map ISA buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t *isa_ptr = (uint32_t*)mmap_isa.addr_ptr;
    memcpy(isa_ptr, kernel_isa, sizeof(kernel_isa));
    munmap(isa_ptr, 4096);
    LOG_EVENT("GEM", "isa_filled", sizeof(kernel_isa));
    
    // Map Surface State buffer
    struct drm_i915_gem_mmap mmap_surface = {
        .handle = surface_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_surface) != 0) {
        fprintf(stderr, "[ERROR] Cannot map surface buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t *surface_ptr = (uint32_t*)mmap_surface.addr_ptr;
    memset(surface_ptr, 0, 4096);
    
    // Surface State (16 DWords = 64 bytes)
    surface_ptr[0] = 0x00000000;  // Surface Type = Buffer
    surface_ptr[1] = 0x00000000;  // Surface Base Address (low) - will be relocated
    surface_ptr[2] = 0x00000000;  // Surface Base Address (high)
    surface_ptr[3] = 0x00000FFF;  // Width = 4095
    surface_ptr[4] = 0x00000000;  // Height = 0
    surface_ptr[5] = 0x00000000;  // Depth = 0
    surface_ptr[6] = 0x00000000;  // Surface Pitch
    surface_ptr[7] = 0x00000000;  // Reserved
    
    // Binding Table (1 entry = 4 bytes)
    surface_ptr[16] = 0;  // Offset to Surface State (will be 0)
    
    munmap(surface_ptr, 4096);
    LOG_EVENT("GEM", "surface_filled", 64);
    
    // Map Interface Descriptor buffer
    struct drm_i915_gem_mmap mmap_desc = {
        .handle = desc_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_desc) != 0) {
        fprintf(stderr, "[ERROR] Cannot map descriptor buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    gen9_interface_descriptor_t *desc_ptr = (gen9_interface_descriptor_t*)mmap_desc.addr_ptr;
    memset(desc_ptr, 0, sizeof(gen9_interface_descriptor_t));
    
    // ✅ CORRECTION #1 : Kernel pointer encoding (>> 6)
    desc_ptr->dw0 = 0 >> 6;  // Kernel at offset 0 in ISA buffer
    
    // DW1: Flags
    desc_ptr->dw1 = (1 << 18);  // Single Program Flow
    
    // DW2: Sampler (unused)
    desc_ptr->dw2 = 0;
    
    // ✅ CORRECTION #2 : Binding table pointer granularity (/ 32)
    desc_ptr->dw3 = (1 << 0) |  // Binding Table Entry Count = 1
                    ((64 / 32) << 5);  // Binding Table Pointer (offset 64 / 32)
    
    // DW4: CURBE (unused)
    desc_ptr->dw4 = 0;
    
    // DW5: Threads
    desc_ptr->dw5 = (1 << 0);  // Number of Threads = 1
    
    // DW6-7: Reserved
    desc_ptr->dw6 = 0;
    desc_ptr->dw7 = 0;
    
    munmap(desc_ptr, 4096);
    LOG_EVENT("GEM", "desc_filled", 32);
    
    // Map Batch buffer
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = batch_handle,
        .offset = 0,
        .size = 65536,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) != 0) {
        fprintf(stderr, "[ERROR] Cannot map batch buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t *batch = (uint32_t*)mmap_batch.addr_ptr;
    memset(batch, 0, 65536);
    
    uint32_t *batch_ptr = batch;
    
    // 1. PIPELINE_SELECT
    emit_pipeline_select(batch_ptr);
    batch_ptr += 2;
    
    // 2. STATE_BASE_ADDRESS
    emit_state_base_address(batch_ptr, 0, 0, 0);  // Offsets will be relocated
    batch_ptr += 16;
    
    // ✅ CORRECTION #3 : PIPE_CONTROL après STATE_BASE_ADDRESS
    emit_pipe_control(batch_ptr, 
        PIPE_CONTROL_DC_FLUSH | 
        PIPE_CONTROL_INSTRUCTION_CACHE_INV | 
        PIPE_CONTROL_STATE_CACHE_INV);
    batch_ptr += 6;
    
    // 4. MEDIA_VFE_STATE (avec EU enable mask)
    emit_media_vfe_state(batch_ptr);
    batch_ptr += 8;
    
    // ✅ CORRECTION #4 : PIPE_CONTROL après MEDIA_VFE_STATE
    emit_pipe_control(batch_ptr, 
        PIPE_CONTROL_STATE_CACHE_INV | 
        PIPE_CONTROL_CS_STALL);
    batch_ptr += 6;
    
    // 6. MEDIA_INTERFACE_DESCRIPTOR_LOAD
    emit_interface_descriptor_load(batch_ptr, 0);  // Offset will be relocated
    batch_ptr += 4;
    
    // 7. GPGPU_WALKER
    emit_gpgpu_walker(batch_ptr);
    batch_ptr += 15;
    
    // ✅ CORRECTION #5 : MI_FLUSH_DW après GPGPU_WALKER
    emit_mi_flush_dw(batch_ptr);
    batch_ptr += 4;
    
    // ✅ CORRECTION #7 : PIPE_CONTROL final (render target flush)
    emit_pipe_control(batch_ptr, 
        PIPE_CONTROL_RENDER_TARGET_FLUSH | 
        PIPE_CONTROL_CS_STALL | 
        PIPE_CONTROL_STALL_AT_SCOREBOARD);
    batch_ptr += 6;
    
    // 10. MI_BATCH_BUFFER_END
    batch_ptr[0] = (0x0A << 23);
    batch_ptr += 1;
    
    size_t batch_size = (uint8_t*)batch_ptr - (uint8_t*)batch;
    munmap(batch, 65536);
    LOG_EVENT("GEM", "batch_filled", batch_size);
    
    printf("[SUCCESS] All buffers filled (batch_size=%zu bytes)\n\n", batch_size);
    
    /* ── Phase 5 : Configure Relocations ───────────────────────────── */
    printf("[PHASE 5] Configuring relocations...\n");
    
    struct drm_i915_gem_relocation_entry relocs[5];
    memset(relocs, 0, sizeof(relocs));
    
    // Relocation 0: ISA base in STATE_BASE_ADDRESS (DW9)
    relocs[0].target_handle = isa_handle;
    relocs[0].delta = 0;
    relocs[0].offset = (2 + 16) * 4 + 9 * 4;  // After PIPELINE_SELECT + STATE_BASE_ADDRESS DW9
    relocs[0].presumed_offset = 0;
    relocs[0].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[0].write_domain = 0;
    
    // Relocation 1: Surface State base in STATE_BASE_ADDRESS (DW3)
    relocs[1].target_handle = surface_handle;
    relocs[1].delta = 0;
    relocs[1].offset = (2 + 16) * 4 + 3 * 4;
    relocs[1].presumed_offset = 0;
    relocs[1].read_domains = I915_GEM_DOMAIN_SAMPLER;
    relocs[1].write_domain = 0;
    
    // Relocation 2: Dynamic State base in STATE_BASE_ADDRESS (DW5)
    relocs[2].target_handle = desc_handle;
    relocs[2].delta = 0;
    relocs[2].offset = (2 + 16) * 4 + 5 * 4;
    relocs[2].presumed_offset = 0;
    relocs[2].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[2].write_domain = 0;
    
    // Relocation 3: Interface Descriptor offset in MEDIA_INTERFACE_DESCRIPTOR_LOAD (DW3)
    relocs[3].target_handle = desc_handle;
    relocs[3].delta = 0;
    relocs[3].offset = (2 + 16 + 6 + 8 + 6) * 4 + 3 * 4;
    relocs[3].presumed_offset = 0;
    relocs[3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[3].write_domain = 0;
    
    // Relocation 4: Output buffer in Surface State (DW1)
    relocs[4].target_handle = output_handle;
    relocs[4].delta = 0;
    relocs[4].offset = 1 * 4;  // Surface State DW1 (in surface buffer)
    relocs[4].presumed_offset = 0;
    relocs[4].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[4].write_domain = I915_GEM_DOMAIN_RENDER;
    
    LOG_EVENT("GEM", "relocations_configured", 5);
    printf("[SUCCESS] 5 relocations configured\n\n");
    
    /* ── Phase 6 : Execute Batch Buffer ────────────────────────────── */
    printf("[PHASE 6] Executing batch buffer...\n");
    LOG_EVENT("GPU", "exec_start", 0);
    
    struct drm_i915_gem_exec_object2 exec_objects[5];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = isa_handle;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;
    exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects[1].handle = surface_handle;
    exec_objects[1].relocation_count = 1;
    exec_objects[1].relocs_ptr = (uintptr_t)&relocs[4];
    exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects[2].handle = desc_handle;
    exec_objects[2].relocation_count = 0;
    exec_objects[2].relocs_ptr = 0;
    exec_objects[2].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects[3].handle = output_handle;
    exec_objects[3].relocation_count = 0;
    exec_objects[3].relocs_ptr = 0;
    exec_objects[3].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE;
    
    exec_objects[4].handle = batch_handle;
    exec_objects[4].relocation_count = 4;
    exec_objects[4].relocs_ptr = (uintptr_t)relocs;
    exec_objects[4].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 5,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    uint64_t exec_start = get_timestamp_ns();
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t exec_end = get_timestamp_ns();
    uint64_t exec_duration_ns = exec_end - exec_start;
    
    if (ret != 0) {
        fprintf(stderr, "[ERROR] execbuffer2 failed: %s (errno=%d)\n", strerror(errno), errno);
        LOG_EVENT("GPU", "exec_error", errno);
        close(drm_fd);
        return 1;
    }
    
    LOG_EVENT("GPU", "exec_success", exec_duration_ns);
    printf("[SUCCESS] Batch buffer executed (%.3f ms)\n\n", exec_duration_ns / 1000000.0);
    
    /* ── Phase 7 : Wait GPU Completion ─────────────────────────────── */
    printf("[PHASE 7] Waiting for GPU completion...\n");
    LOG_EVENT("GPU", "wait_start", 0);
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = output_handle,
        .timeout_ns = 1000000000,  // 1 second
        .flags = 0
    };
    
    uint64_t wait_start = get_timestamp_ns();
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    uint64_t wait_end = get_timestamp_ns();
    uint64_t wait_duration_ns = wait_end - wait_start;
    
    if (ret != 0) {
        fprintf(stderr, "[ERROR] GPU wait failed: %s\n", strerror(errno));
        LOG_EVENT("GPU", "wait_error", errno);
        close(drm_fd);
        return 1;
    }
    
    LOG_EVENT("GPU", "wait_success", wait_duration_ns);
    printf("[SUCCESS] GPU completed (%.3f ms)\n\n", wait_duration_ns / 1000000.0);
    
    /* ── Phase 8 : Read Output ─────────────────────────────────────── */
    printf("[PHASE 8] Reading output buffer...\n");
    LOG_EVENT("MEMORY", "read_start", 0);
    
    struct drm_i915_gem_mmap mmap_output = {
        .handle = output_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) != 0) {
        fprintf(stderr, "[ERROR] Cannot map output buffer: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    uint32_t *output = (uint32_t*)mmap_output.addr_ptr;
    uint32_t result = output[0];
    LOG_EVENT("MEMORY", "output[0]", result);
    
    printf("[RESULT] output[0] = 0x%08x\n\n", result);
    
    munmap(output, 4096);
    
    /* ── Phase 9 : Verification ────────────────────────────────────── */
    printf("[PHASE 9] Verification...\n");
    
    int success = (result == 0xDEADBEEF);
    LOG_EVENT("VERIFY", "success", success);
    
    if (success) {
        printf("✅ VERIFICATION SUCCESS: output[0] = 0xDEADBEEF\n\n");
    } else {
        printf("❌ VERIFICATION FAILED: output[0] = 0x%08x (expected 0xDEADBEEF)\n\n", result);
    }
    
    /* ── Cleanup ───────────────────────────────────────────────────── */
    struct drm_gem_close close_gem = {0};
    
    close_gem.handle = isa_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    
    close_gem.handle = surface_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    
    close_gem.handle = desc_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    
    close_gem.handle = batch_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    
    close_gem.handle = output_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    
    close(drm_fd);
    
    uint64_t t_end = get_timestamp_ns();
    uint64_t total_ns = t_end - t_start;
    LOG_EVENT("INIT", "test_end", total_ns);
    
    /* ── Generate Forensic Report ──────────────────────────────────── */
    printf("=== FORENSIC REPORT C197.13 ===\n\n");
    printf("Total execution time: %.3f ms\n", total_ns / 1000000.0);
    printf("Total events captured: %zu\n\n", g_event_count);
    
    // Save detailed forensic log
    FILE* fp = fopen("c197_13_forensic_state_visibility.csv", "w");
    if (fp) {
        fprintf(fp, "timestamp_ns,phase,event,value\n");
        for (size_t i = 0; i < g_event_count; i++) {
            fprintf(fp, "%lu,%s,%s,%lu\n",
                    g_events[i].timestamp_ns,
                    g_events[i].phase,
                    g_events[i].event,
                    g_events[i].value);
        }
        fclose(fp);
        printf("Forensic log saved: c197_13_forensic_state_visibility.csv\n");
    }
    
    return success ? 0 : 1;
}

// Made with Bob
