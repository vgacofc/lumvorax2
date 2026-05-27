/*
 * CYCLE C197.14 - EXTRACTION BINAIRE OPENCL COMPLET
 * ==================================================
 * 
 * OBJECTIF : Extraire Kernel Binary Layout Intel complet depuis OpenCL
 *            et l'injecter dans DRM natif avec les 7 corrections C197.13
 * 
 * APPROCHE :
 * 1. Compiler kernel OpenCL minimal (write 0xDEADBEEF)
 * 2. Extraire binaire COMPLET via clGetProgramInfo(CL_PROGRAM_BINARIES)
 * 3. Analyser structure binaire (Header + Metadata + ISA)
 * 4. Injecter binaire dans DRM avec pipeline corrigé C197.13
 * 5. Valider output = 0xDEADBEEF
 * 
 * RÉSULTAT ATTENDU : output[0] = 0xDEADBEEF ✅
 */

#define _GNU_SOURCE
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
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
// ELF Section offsets (from readelf analysis)
#define TEXT_SECTION_OFFSET 0x40  // .text.test_write offset in ELF
#define TEXT_SECTION_SIZE 0x100   // 256 bytes ISA pure Gen9
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

/* ── Kernel OpenCL minimal ─────────────────────────────────────────── */
const char* kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"}\n";

/* ── PIPE_CONTROL Flags (Gen9) ─────────────────────────────────────── */
#define PIPE_CONTROL_DC_FLUSH              (1 << 5)
#define PIPE_CONTROL_INSTRUCTION_CACHE_INV (1 << 6)
#define PIPE_CONTROL_STATE_CACHE_INV       (1 << 16)
#define PIPE_CONTROL_RENDER_TARGET_FLUSH   (1 << 12)
#define PIPE_CONTROL_CS_STALL              (1 << 20)
#define PIPE_CONTROL_STALL_AT_SCOREBOARD   (1 << 1)

/* ── Helper Functions ──────────────────────────────────────────────── */

static void emit_pipe_control(uint32_t *batch, uint32_t flags) {
    batch[0] = (0x7A << 24) | (6 - 2);
    batch[1] = flags;
    batch[2] = 0;
    batch[3] = 0;
    batch[4] = 0;
    batch[5] = 0;
}

static void emit_mi_flush_dw(uint32_t *batch) {
    batch[0] = (0x26 << 23) | (2 << 0);
    batch[1] = (1 << 14);
    batch[2] = 0;
    batch[3] = 0;
}

static void emit_pipeline_select(uint32_t *batch) {
    batch[0] = (0x69 << 24) | (1 << 0);
    batch[1] = (1 << 0);
}

static void emit_state_base_address(uint32_t *batch, uint64_t isa_base, 
                                     uint64_t surface_base, uint64_t dynamic_base) {
    batch[0] = (0x61 << 24) | (16 - 2);
    batch[1] = 0;
    batch[2] = 0;
    batch[3] = (uint32_t)(surface_base & 0xFFFFFFFF) | 1;
    batch[4] = (uint32_t)(surface_base >> 32);
    batch[5] = (uint32_t)(dynamic_base & 0xFFFFFFFF) | 1;
    batch[6] = (uint32_t)(dynamic_base >> 32);
    batch[7] = 0;
    batch[8] = 0;
    batch[9] = (uint32_t)(isa_base & 0xFFFFFFFF) | 1;
    batch[10] = (uint32_t)(isa_base >> 32);
    batch[11] = 0xFFFFF000;
    batch[12] = 0xFFFFF000;
    batch[13] = 0xFFFFF000;
    batch[14] = 0xFFFFF000;
    batch[15] = 0;
}

static void emit_media_vfe_state(uint32_t *batch) {
    batch[0] = (0x70 << 24) | (0x00 << 16) | (8 - 2);
    batch[1] = 0;
    batch[2] = (64 << 16) | (1 << 8) | (1 << 0);
    batch[3] = 0;
    batch[4] = 0;
    batch[5] = 0;
    batch[6] = 0;
    batch[7] = 0;
}

static void emit_interface_descriptor_load(uint32_t *batch, uint32_t desc_offset) {
    batch[0] = (0x70 << 24) | (0x02 << 16) | (4 - 2);
    batch[1] = 0;
    batch[2] = 32;
    batch[3] = desc_offset;
}

static void emit_gpgpu_walker(uint32_t *batch) {
    batch[0] = (0x70 << 24) | (0x05 << 16) | (15 - 2);
    batch[1] = 0;
    batch[2] = 0;
    batch[3] = 0;
    batch[4] = 0;
    batch[5] = 0;
    batch[6] = 0;
    batch[7] = 1;
    batch[8] = 0;
    batch[9] = 1;
    batch[10] = 1;
    batch[11] = 0;
    batch[12] = 1;
    batch[13] = 1;
    batch[14] = 1;
}

/* ── Interface Descriptor ──────────────────────────────────────────── */
typedef struct {
    uint32_t dw0;
    uint32_t dw1;
    uint32_t dw2;
    uint32_t dw3;
    uint32_t dw4;
    uint32_t dw5;
    uint32_t dw6;
    uint32_t dw7;
} gen9_interface_descriptor_t;

/* ── Main ──────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== CYCLE C197.14 - OPENCL BINARY EXTRACTION ===\n\n");
    
    uint64_t t_start = get_timestamp_ns();
    LOG_EVENT("INIT", "test_start", 0);
    
    /* ── Phase 1 : Compiler Kernel OpenCL ──────────────────────────── */
    printf("[PHASE 1] Compiling OpenCL kernel...\n");
    LOG_EVENT("OPENCL", "compile_start", 0);
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetPlatformIDs = %d\n", err);
        return 1;
    }
    LOG_EVENT("OPENCL", "platform_id", (uint64_t)platform);
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetDeviceIDs = %d\n", err);
        return 1;
    }
    LOG_EVENT("OPENCL", "device_id", (uint64_t)device);
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateContext = %d\n", err);
        return 1;
    }
    LOG_EVENT("OPENCL", "context_created", (uint64_t)context);
    
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateProgramWithSource = %d\n", err);
        return 1;
    }
    LOG_EVENT("OPENCL", "program_created", (uint64_t)program);
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clBuildProgram = %d\n", err);
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    LOG_EVENT("OPENCL", "program_built", 0);
    
    printf("[SUCCESS] OpenCL kernel compiled\n\n");
    
    /* ── Phase 2 : Extraire Binaire Complet ────────────────────────── */
    printf("[PHASE 2] Extracting complete binary...\n");
    LOG_EVENT("OPENCL", "binary_extract_start", 0);
    
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetProgramInfo(BINARY_SIZES) = %d\n", err);
        return 1;
    }
    LOG_EVENT("OPENCL", "binary_size", binary_size);
    
    unsigned char* binary = malloc(binary_size);
    if (!binary) {
        fprintf(stderr, "[ERROR] Cannot allocate binary buffer\n");
        return 1;
    }
    
    unsigned char* binaries[1] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetProgramInfo(BINARIES) = %d\n", err);
        free(binary);
        return 1;
    }
    LOG_EVENT("OPENCL", "binary_extracted", binary_size);
    
    printf("[SUCCESS] Binary extracted (%zu bytes)\n", binary_size);
    
    // Sauvegarder binaire pour analyse
    FILE* f = fopen("c197_14_opencl_binary.bin", "wb");
    if (f) {
        fwrite(binary, 1, binary_size, f);
        fclose(f);
        printf("[INFO] Binary saved to c197_14_opencl_binary.bin\n\n");
    }
    
    /* ── Phase 3 : Analyser Structure Binaire ──────────────────────── */
    printf("[PHASE 3] Analyzing binary structure...\n");
    LOG_EVENT("ANALYSIS", "structure_start", 0);
    
    printf("[INFO] Binary size: %zu bytes\n", binary_size);
    printf("[INFO] First 64 bytes (hex):\n");
    for (size_t i = 0; i < 64 && i < binary_size; i++) {
        if (i % 16 == 0) printf("  %04zx: ", i);
        printf("%02x ", binary[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n\n");
    
    LOG_EVENT("ANALYSIS", "structure_analyzed", binary_size);
    
    /* ── Phase 4 : Open DRM ────────────────────────────────────────── */
    printf("[PHASE 4] Opening DRM device...\n");
    LOG_EVENT("DRM", "open_start", 0);
    
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        drm_fd = open("/dev/dri/card0", O_RDWR);
    }
    
    if (drm_fd < 0) {
        fprintf(stderr, "[ERROR] Cannot open DRM device: %s\n", strerror(errno));
        free(binary);
        return 1;
    }
    
    LOG_EVENT("DRM", "drm_fd", drm_fd);
    printf("[SUCCESS] DRM opened (fd=%d)\n\n", drm_fd);
    
    /* ── Phase 5 : Create i915 Context ─────────────────────────────── */
    printf("[PHASE 5] Creating i915 context...\n");
    LOG_EVENT("DRM", "context_create_start", 0);
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) != 0) {
        fprintf(stderr, "[ERROR] Cannot create context: %s\n", strerror(errno));
        close(drm_fd);
        free(binary);
        return 1;
    }
    
    uint32_t ctx_id = ctx_create.ctx_id;
    LOG_EVENT("DRM", "ctx_id", ctx_id);
    printf("[SUCCESS] Context created (ctx_id=%u)\n\n", ctx_id);
    
    /* ── Phase 6 : Create GEM Objects ──────────────────────────────── */
    printf("[PHASE 6] Creating GEM objects...\n");
    
    // ISA buffer (taille binaire OpenCL)
    size_t isa_size = (binary_size + 4095) & ~4095;  // Round up to 4KB
    struct drm_i915_gem_create create_isa = {
        .size = isa_size
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_isa) != 0) {
        fprintf(stderr, "[ERROR] Cannot create ISA buffer: %s\n", strerror(errno));
        close(drm_fd);
        free(binary);
        return 1;
    }
    uint32_t isa_handle = create_isa.handle;
    LOG_EVENT("GEM", "isa_handle", isa_handle);
    LOG_EVENT("GEM", "isa_size", isa_size);
    
    // Surface State buffer
    struct drm_i915_gem_create create_surface = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_surface) != 0) {
        fprintf(stderr, "[ERROR] Cannot create surface buffer: %s\n", strerror(errno));
        close(drm_fd);
        free(binary);
        return 1;
    }
    uint32_t surface_handle = create_surface.handle;
    LOG_EVENT("GEM", "surface_handle", surface_handle);
    
    // Interface Descriptor buffer
    struct drm_i915_gem_create create_desc = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_desc) != 0) {
        fprintf(stderr, "[ERROR] Cannot create descriptor buffer: %s\n", strerror(errno));
        close(drm_fd);
        free(binary);
        return 1;
    }
    uint32_t desc_handle = create_desc.handle;
    LOG_EVENT("GEM", "desc_handle", desc_handle);
    
    // Batch buffer
    struct drm_i915_gem_create create_batch = {
        .size = 65536
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) != 0) {
        fprintf(stderr, "[ERROR] Cannot create batch buffer: %s\n", strerror(errno));
        close(drm_fd);
        free(binary);
        return 1;
    }
    uint32_t batch_handle = create_batch.handle;
    LOG_EVENT("GEM", "batch_handle", batch_handle);
    
    // Output buffer
    struct drm_i915_gem_create create_output = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) != 0) {
        fprintf(stderr, "[ERROR] Cannot create output buffer: %s\n", strerror(errno));
        close(drm_fd);
        free(binary);
        return 1;
    }
    uint32_t output_handle = create_output.handle;
    LOG_EVENT("GEM", "output_handle", output_handle);
    
    printf("[SUCCESS] All GEM objects created\n\n");
    
    /* ── Phase 7 : Map and Fill Buffers ────────────────────────────── */
    printf("[PHASE 7] Mapping and filling buffers...\n");
    
    // Map ISA buffer et copier binaire OpenCL COMPLET
    struct drm_i915_gem_mmap mmap_isa = {
        .handle = isa_handle,
        .offset = 0,
        .size = isa_size,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_isa) != 0) {
        fprintf(stderr, "[ERROR] Cannot map ISA buffer: %s\n", strerror(errno));
        close(drm_fd);
        free(binary);
        return 1;
    }
    unsigned char *isa_ptr = (unsigned char*)mmap_isa.addr_ptr;
    
    // Extract .text.test_write section (ISA pure) from ELF binary
    // Skip ELF header (0x00-0x3F), copy only executable ISA code
    if (binary_size >= TEXT_SECTION_OFFSET + TEXT_SECTION_SIZE) {
        memcpy(isa_ptr, binary + TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE);
        munmap(isa_ptr, isa_size);
        LOG_EVENT("GEM", "isa_filled", TEXT_SECTION_SIZE);
        printf("[SUCCESS] ISA buffer filled with .text.test_write section (%d bytes)\n", TEXT_SECTION_SIZE);
    } else {
        fprintf(stderr, "[ERROR] Binary too small for .text section extraction\n");
        munmap(isa_ptr, isa_size);
        close(drm_fd);
        free(binary);
        return 1;
    }
    
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
        free(binary);
        return 1;
    }
    uint32_t *surface_ptr = (uint32_t*)mmap_surface.addr_ptr;
    memset(surface_ptr, 0, 4096);
    
    // Surface State
    surface_ptr[0] = 0x00000000;
    surface_ptr[1] = 0x00000000;
    surface_ptr[2] = 0x00000000;
    surface_ptr[3] = 0x00000FFF;
    surface_ptr[4] = 0x00000000;
    surface_ptr[5] = 0x00000000;
    surface_ptr[6] = 0x00000000;
    surface_ptr[7] = 0x00000000;
    
    // Binding Table
    surface_ptr[16] = 0;
    
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
        free(binary);
        return 1;
    }
    gen9_interface_descriptor_t *desc_ptr = (gen9_interface_descriptor_t*)mmap_desc.addr_ptr;
    memset(desc_ptr, 0, sizeof(gen9_interface_descriptor_t));
    
    // ✅ CORRECTIONS C197.13
    desc_ptr->dw0 = 0 >> 6;
    desc_ptr->dw1 = (1 << 18);
    desc_ptr->dw2 = 0;
    desc_ptr->dw3 = (1 << 0) | ((64 / 32) << 5);
    desc_ptr->dw4 = 0;
    desc_ptr->dw5 = (1 << 0);
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
        free(binary);
        return 1;
    }
    uint32_t *batch = (uint32_t*)mmap_batch.addr_ptr;
    memset(batch, 0, 65536);
    
    uint32_t *batch_ptr = batch;
    
    // Pipeline avec 7 corrections C197.13
    emit_pipeline_select(batch_ptr);
    batch_ptr += 2;
    
    emit_state_base_address(batch_ptr, 0, 0, 0);
    batch_ptr += 16;
    
    emit_pipe_control(batch_ptr, 
        PIPE_CONTROL_DC_FLUSH | 
        PIPE_CONTROL_INSTRUCTION_CACHE_INV | 
        PIPE_CONTROL_STATE_CACHE_INV);
    batch_ptr += 6;
    
    emit_media_vfe_state(batch_ptr);
    batch_ptr += 8;
    
    emit_pipe_control(batch_ptr, 
        PIPE_CONTROL_STATE_CACHE_INV | 
        PIPE_CONTROL_CS_STALL);
    batch_ptr += 6;
    
    emit_interface_descriptor_load(batch_ptr, 0);
    batch_ptr += 4;
    
    emit_gpgpu_walker(batch_ptr);
    batch_ptr += 15;
    
    emit_mi_flush_dw(batch_ptr);
    batch_ptr += 4;
    
    emit_pipe_control(batch_ptr, 
        PIPE_CONTROL_RENDER_TARGET_FLUSH | 
        PIPE_CONTROL_CS_STALL | 
        PIPE_CONTROL_STALL_AT_SCOREBOARD);
    batch_ptr += 6;
    
    batch_ptr[0] = (0x0A << 23);
    batch_ptr += 1;
    
    size_t batch_size = (uint8_t*)batch_ptr - (uint8_t*)batch;
    munmap(batch, 65536);
    LOG_EVENT("GEM", "batch_filled", batch_size);
    
    printf("[SUCCESS] All buffers filled (batch_size=%zu bytes)\n\n", batch_size);
    
    /* ── Phase 8 : Configure Relocations ───────────────────────────── */
    printf("[PHASE 8] Configuring relocations...\n");
    
    struct drm_i915_gem_relocation_entry relocs[5];
    memset(relocs, 0, sizeof(relocs));
    
    relocs[0].target_handle = isa_handle;
    relocs[0].delta = 0;
    relocs[0].offset = (2 + 16) * 4 + 9 * 4;
    relocs[0].presumed_offset = 0;
    relocs[0].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[0].write_domain = 0;
    
    relocs[1].target_handle = surface_handle;
    relocs[1].delta = 0;
    relocs[1].offset = (2 + 16) * 4 + 3 * 4;
    relocs[1].presumed_offset = 0;
    relocs[1].read_domains = I915_GEM_DOMAIN_SAMPLER;
    relocs[1].write_domain = 0;
    
    relocs[2].target_handle = desc_handle;
    relocs[2].delta = 0;
    relocs[2].offset = (2 + 16) * 4 + 5 * 4;
    relocs[2].presumed_offset = 0;
    relocs[2].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[2].write_domain = 0;
    
    relocs[3].target_handle = desc_handle;
    relocs[3].delta = 0;
    relocs[3].offset = (2 + 16 + 6 + 8 + 6) * 4 + 3 * 4;
    relocs[3].presumed_offset = 0;
    relocs[3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[3].write_domain = 0;
    
    relocs[4].target_handle = output_handle;
    relocs[4].delta = 0;
    relocs[4].offset = 1 * 4;
    relocs[4].presumed_offset = 0;
    relocs[4].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[4].write_domain = I915_GEM_DOMAIN_RENDER;
    
    LOG_EVENT("GEM", "relocations_configured", 5);
    printf("[SUCCESS] 5 relocations configured\n\n");
    
    /* ── Phase 9 : Execute Batch Buffer ────────────────────────────── */
    printf("[PHASE 9] Executing batch buffer...\n");
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
        free(binary);
        return 1;
    }
    
    LOG_EVENT("GPU", "exec_success", exec_duration_ns);
    printf("[SUCCESS] Batch buffer executed (%.3f ms)\n\n", exec_duration_ns / 1000000.0);
    
    /* ── Phase 10 : Wait GPU Completion ────────────────────────────── */
    printf("[PHASE 10] Waiting for GPU completion...\n");
    LOG_EVENT("GPU", "wait_start", 0);
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = output_handle,
        .timeout_ns = 1000000000,
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
        free(binary);
        return 1;
    }
    
    LOG_EVENT("GPU", "wait_success", wait_duration_ns);
    printf("[SUCCESS] GPU completed (%.3f ms)\n\n", wait_duration_ns / 1000000.0);
    
    /* ── Phase 11 : Read Output ────────────────────────────────────── */
    printf("[PHASE 11] Reading output buffer...\n");
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
        free(binary);
        return 1;
    }
    
    uint32_t *output = (uint32_t*)mmap_output.addr_ptr;
    uint32_t result = output[0];
    LOG_EVENT("MEMORY", "output[0]", result);
    
    printf("[RESULT] output[0] = 0x%08x\n\n", result);
    
    munmap(output, 4096);
    
    /* ── Phase 12 : Verification ───────────────────────────────────── */
    printf("[PHASE 12] Verification...\n");
    
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
    
    clReleaseProgram(program);
    clReleaseContext(context);
    close(drm_fd);
    free(binary);
    
    uint64_t t_end = get_timestamp_ns();
    uint64_t total_ns = t_end - t_start;
    LOG_EVENT("INIT", "test_end", total_ns);
    
    /* ── Generate Forensic Report ──────────────────────────────────── */
    printf("=== FORENSIC REPORT C197.14 ===\n\n");
    printf("Total execution time: %.3f ms\n", total_ns / 1000000.0);
    printf("Total events captured: %zu\n\n", g_event_count);
    
    // Save detailed forensic log
    FILE* fp = fopen("c197_14_forensic_opencl_binary.csv", "w");
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
        printf("Forensic log saved: c197_14_forensic_opencl_binary.csv\n");
    }
    
    return success ? 0 : 1;
}

// Made with Bob
