/*
 * TEST C463 - SOFT-PINNING avec ADRESSES GPU FIXES
 * Objectif: Éliminer relocations dépréciées et utiliser soft-pinning moderne
 *
 * ROOT CAUSE C462: i915 moderne IGNORE les relocations (dépréciées)
 * SOLUTION: Utiliser EXEC_OBJECT_PINNED avec adresses GPU fixes
 *
 * Ce runner teste:
 * - Soft-pinning: Adresses GPU fixes pour tous les buffers
 * - Écriture directe des adresses dans IDD, CURBE, SSH, SBA
 * - ZERO relocations (toutes supprimées)
 * - Dispatch 256 threads avec batch buffer complet
 * - Validation output[0] == 0x12345678 ✅
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

/* Gen9 Command Opcodes - Définitions Linux kernel */
#define GEN9_PIPELINE_SELECT           ((0x3 << 29) | (0x1 << 27) | (0x1 << 24) | (0x4 << 16))
#define GEN9_STATE_BASE_ADDRESS        0x61010000
#define GEN9_MEDIA_VFE_STATE           0x70000000
#define GEN9_MEDIA_CURBE_LOAD          0x70010000
#define GEN9_MEDIA_STATE_FLUSH         0x70040000
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020000
#define GEN9_GPGPU_WALKER              0x71050000
#define GEN9_PIPE_CONTROL              0x7a000000
#define GEN9_MI_BATCH_BUFFER_END       0x0a000000
#define PIPELINE_SELECT_MEDIA          (1 << 0)

/* Gen9 Constants */
#define CACHE_CTRL 0x3  /* LLC + L3 */
#define BASE_ADDRESS_MODIFY 0x1

/* C463: Soft-Pinning - Adresses GPU fixes (48-bit address space) */
#define GPU_ADDR_KERNEL     0x0000000000100000ULL  /* 1 MB */
#define GPU_ADDR_OUTPUT     0x0000000000200000ULL  /* 2 MB */
#define GPU_ADDR_SSH        0x0000000000300000ULL  /* 3 MB */
#define GPU_ADDR_DSH        0x0000000000400000ULL  /* 4 MB */
#define GPU_ADDR_IOH        0x0000000000500000ULL  /* 5 MB */
#define GPU_ADDR_BATCH      0x0000000000600000ULL  /* 6 MB */

/* i915 EXEC_OBJECT flags pour soft-pinning */
#ifndef EXEC_OBJECT_PINNED
#define EXEC_OBJECT_PINNED (1 << 4)
#endif
#ifndef EXEC_OBJECT_SUPPORTS_48B_ADDRESS
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS (1 << 3)
#endif
#ifndef EXEC_OBJECT_WRITE
#define EXEC_OBJECT_WRITE (1 << 2)
#endif

// Structure contexte GPU
typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t kernel_bo;
    uint32_t output_bo;
    uint32_t batch_bo;
    uint32_t ssh_bo;   /* Surface State Heap */
    uint32_t dsh_bo;   /* Dynamic State Heap */
    uint32_t ioh_bo;   /* Indirect Object Heap */
    void* output_map;
    void* batch_map;
    void* ssh_map;
    void* dsh_map;
    size_t kernel_size;
    size_t output_size;
    size_t batch_size;
    size_t ssh_size;
    size_t dsh_size;
} gpu_context_t;

// Lecture fichier binaire kernel
static int load_kernel_binary(const char* path, void** data, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        FORENSIC_LOG("❌ Erreur ouverture kernel: %s", strerror(errno));
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    *data = malloc(*size);
    if (!*data) {
        fclose(f);
        return -1;
    }
    
    if (fread(*data, 1, *size, f) != *size) {
        free(*data);
        fclose(f);
        return -1;
    }
    
    fclose(f);
    FORENSIC_LOG("✅ Kernel chargé: %zu bytes", *size);
    return 0;
}

// Création buffer GEM
static uint32_t create_gem_buffer(int fd, size_t size) {
    struct drm_i915_gem_create create = {
        .size = size
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) != 0) {
        FORENSIC_LOG("❌ GEM_CREATE failed: %s", strerror(errno));
        return 0;
    }
    
    FORENSIC_LOG("✅ GEM buffer créé: handle=%u size=%zu", create.handle, size);
    return create.handle;
}

// Mapping buffer
static void* map_gem_buffer(int fd, uint32_t handle, size_t size) {
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        FORENSIC_LOG("❌ GEM_MMAP failed: %s", strerror(errno));
        return NULL;
    }
    
    void* ptr = (void*)(uintptr_t)mmap_arg.addr_ptr;
    FORENSIC_LOG("✅ Buffer mappé: %p", ptr);
    return ptr;
}

// Écriture données dans buffer
static int write_to_gem_buffer(int fd, uint32_t handle, const void* data, size_t size) {
    struct drm_i915_gem_pwrite pwrite = {
        .handle = handle,
        .offset = 0,
        .size = size,
        .data_ptr = (uint64_t)(uintptr_t)data
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) != 0) {
        FORENSIC_LOG("❌ GEM_PWRITE failed: %s", strerror(errno));
        return -1;
    }
    
    FORENSIC_LOG("✅ Données écrites: %zu bytes", size);
    return 0;
}

// Configuration SSH: Binding Table + Surface State (LAYOUT INTEL C439)
static void configure_surface_state(gpu_context_t* ctx) {
    uint32_t* ssh = (uint32_t*)ctx->ssh_map;
    memset(ssh, 0, ctx->ssh_size);
    
    FORENSIC_LOG("🔧 C439: Configuration SSH Layout Intel");
    
    /* ========================================
     * LAYOUT SSH INTEL (d'après compute-runtime:500-550)
     * ========================================
     * SSH[0-3]:    Binding Table (1 entrée = 4 bytes)
     *              BTI[0] = offset vers Surface State
     * SSH[64-127]: Surface State (64 bytes)
     * ======================================== */
    
    // 1. BINDING TABLE à offset 0 (début SSH)
    // BTI[0] pointe vers Surface State à offset 64 bytes
    ssh[0] = 64;  // Offset en bytes vers Surface State
    FORENSIC_LOG("   BTI[0] = %u (offset Surface State)", ssh[0]);
    
    // 2. SURFACE STATE à offset 64 bytes (16 DWORDs)
    uint32_t* ss = ssh + 16;  // +16 DWORDs = +64 bytes
    
    // Calcul taille alignée
    uint32_t aligned_size = (ctx->output_size + 3) & ~3;
    
    // Encodage Width/Height/Depth selon Intel
    typedef union {
        struct {
            uint32_t width  : 14;
            uint32_t height : 14;
            uint32_t depth  : 11;
        } surfaceState;
        uint32_t length;
    } SurfaceStateBufferLength;
    
    SurfaceStateBufferLength length = {0};
    length.length = aligned_size - 1;
    
    uint32_t width = length.surfaceState.width + 1;
    uint32_t height = length.surfaceState.height + 1;
    uint32_t depth = length.surfaceState.depth + 1;
    
    // DW0: Type, Format, Alignement, Tile Mode
    // C449 FIX: Surface Format RAW (0x1FF) sur 9 bits (bits 18-10)
    // Surface Type = SURFTYPE_BUFFER (0) bits 31-29
    // Surface Format = RAW (0x1FF) bits 18-10
    // Vertical Alignment = VALIGN_4 (0) bits 17-16
    // Tile Mode = LINEAR (0) bits 2-0
    uint32_t surface_type = (0 << 29);       // SURFTYPE_BUFFER
    uint32_t surface_format = (0x1FF << 10); // RAW format (9 bits)
    uint32_t valign = (0 << 16);             // VALIGN_4
    uint32_t tile_mode = 0;                  // LINEAR
    ss[0] = surface_type | surface_format | valign | tile_mode;  // 0x0007FC00
    
    // DW1: MOCS
    ss[1] = 0x00000003;  // MOCS=CACHED_L3
    
    // DW2: Width, Height
    ss[2] = (height << 16) | width;
    
    // DW3: Depth
    ss[3] = depth;
    
    // DW4-5: Multisample, Offsets
    ss[4] = 0x00000000;
    ss[5] = 0x00000000;
    
    // DW6: Coherency Type
    ss[6] = 0x00000008;  // Coherency=GPU_COHERENT
    
    // DW7: Shader Channel Select
    ss[7] = 0x00000FAC;  // R=4, G=5, B=6, A=7
    
    // DW8-9: Surface Base Address (C463: adresse GPU fixe)
    ss[8] = (uint32_t)(GPU_ADDR_OUTPUT & 0xFFFFFFFF);  // Low 32 bits
    ss[9] = (uint32_t)(GPU_ADDR_OUTPUT >> 32);         // High 32 bits
    
    // DW10: Auxiliary Surface Mode
    ss[10] = 0x00000000;  // AUX_NONE
    
    // DW11-15: Reserved
    ss[11] = 0x00000000;
    ss[12] = 0x00000000;
    ss[13] = 0x00000000;
    ss[14] = 0x00000000;
    ss[15] = 0x00000000;
    
    FORENSIC_LOG("✅ C439: SSH Layout Intel configuré:");
    FORENSIC_LOG("   Binding Table @ offset 0: BTI[0]=%u", ssh[0]);
    FORENSIC_LOG("   Surface State @ offset 64:");
    FORENSIC_LOG("     Size=%zu → Aligned=%u", ctx->output_size, aligned_size);
    FORENSIC_LOG("     Width=%u, Height=%u, Depth=%u", width, height, depth);
    FORENSIC_LOG("     DW0=0x%08x (VAlign=4, Format=RAW)", ss[0]);
    FORENSIC_LOG("     DW1=0x%08x (MOCS=CACHED_L3)", ss[1]);
    FORENSIC_LOG("     DW6=0x%08x (Coherency=GPU_COHERENT)", ss[6]);
}

// Configuration Interface Descriptor
static void configure_interface_descriptor(gpu_context_t* ctx) {
    uint32_t* dsh = (uint32_t*)ctx->dsh_map;
    memset(dsh, 0, ctx->dsh_size);
    
    /* CURBE Data (64 bytes) - Registres r0-r8 */
    uint32_t* curbe = dsh;
    curbe[0] = 0;  /* r0.0: global_offset_x */
    curbe[1] = 0;  /* r0.1: global_offset_y */
    curbe[2] = 0;  /* r0.2: global_offset_z */
    curbe[3] = 0;  /* r0.3: reserved */
    curbe[4] = 256;  /* r1.0: global_size_x */
    curbe[5] = 1;  /* r1.1: global_size_y */
    curbe[6] = 1;  /* r1.2: global_size_z */
    curbe[7] = 0;  /* r1.3: reserved */
    /* r2-r7: reserved (0) */
    
    /* C463: Kernel utilise addrmode=stateless, attend ADRESSE 64-bit!
     * Selon .ze_info: arg_bypointer à offset 32 (8 bytes)
     * On écrit directement l'adresse GPU fixe (soft-pinning) */
    uint64_t* output_addr_ptr = (uint64_t*)&curbe[8];  /* r8.0-r8.1: adresse 64-bit */
    *output_addr_ptr = GPU_ADDR_OUTPUT;  /* C463: Adresse GPU fixe */
    
    /* Interface Descriptor (32 bytes) après CURBE */
    uint32_t* idd = dsh + 16;  /* Offset 64 bytes */
    idd[0] = (uint32_t)GPU_ADDR_KERNEL;  /* C463: Kernel Start Pointer (adresse GPU fixe) */
    idd[1] = 0x00000000;  /* Reserved */
    idd[2] = (1 << 16) | (256 << 0);  /* C445: Barrier Enable=1, Number of Threads=256 */
    idd[3] = (1 << 16);   /* Binding Table Entry Count = 1, Binding Table Pointer = 0 */
    idd[4] = (64 << 16);  /* CURBE Read Length = 64 bytes */
    idd[5] = 0x00000000;  /* Reserved */
    idd[6] = 0x00000000;  /* Reserved */
    idd[7] = 0x00000000;  /* Reserved */
    
    FORENSIC_LOG("✅ Interface Descriptor configuré");
    FORENSIC_LOG("🔍 C445: IDD Dump:");
    FORENSIC_LOG("   IDD[0] = 0x%08X (Kernel Start Pointer)", idd[0]);
    FORENSIC_LOG("   IDD[2] = 0x%08X (Barrier=%u, Threads=%u)", idd[2], (idd[2] >> 16) & 1, idd[2] & 0xFFFF);
    FORENSIC_LOG("   IDD[3] = 0x%08X (BTI Count=%u, BTI Ptr=%u)", idd[3], (idd[3] >> 16), idd[3] & 0xFFFF);
    FORENSIC_LOG("   IDD[4] = 0x%08X (CURBE Length=%u bytes)", idd[4], (idd[4] >> 16));
    FORENSIC_LOG("🔍 C445: CURBE Dump (premiers 16 DWORDs):");
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0) fprintf(stderr, "\n   [r%d] ", i/4);
        fprintf(stderr, "0x%08X ", curbe[i]);
    }
    fprintf(stderr, "\n");
}

// Construction batch buffer
static int build_batch_buffer(gpu_context_t* ctx) {
    uint32_t* batch = (uint32_t*)ctx->batch_map;
    uint32_t offset = 0;
    
    /* MI_NOOP padding */
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* C444 FIX: PIPELINE_SELECT - GPGPU mode (length INCLUS dans opcode) */
    batch[offset++] = GEN9_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA;  /* 0x69040001 */
    
    /* C463: STATE_BASE_ADDRESS avec adresses GPU fixes (soft-pinning) */
    uint32_t sba_offset = offset * 4;
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | 17;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 16);
    /* SSH base (C463: adresse GPU fixe) */
    batch[offset++] = (uint32_t)(GPU_ADDR_SSH & 0xFFFFFFFF) | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = (uint32_t)(GPU_ADDR_SSH >> 32);
    /* DSH base (C463: adresse GPU fixe) */
    batch[offset++] = (uint32_t)(GPU_ADDR_DSH & 0xFFFFFFFF) | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = (uint32_t)(GPU_ADDR_DSH >> 32);
    /* IOH base (C463: adresse GPU fixe) */
    batch[offset++] = (uint32_t)(GPU_ADDR_IOH & 0xFFFFFFFF) | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = (uint32_t)(GPU_ADDR_IOH >> 32);
    /* Instruction base (C463: adresse GPU fixe) */
    batch[offset++] = (uint32_t)(GPU_ADDR_KERNEL & 0xFFFFFFFF) | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = (uint32_t)(GPU_ADDR_KERNEL >> 32);
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000;
    
    /* ============================================
     * C443 FIX: MEDIA_VFE_STATE après STATE_BASE_ADDRESS
     * ORDRE CORRECT: PIPELINE_SELECT → STATE_BASE_ADDRESS → MEDIA_VFE_STATE
     * ============================================ */
    FORENSIC_LOG("🔧 C443: Programmation MEDIA_VFE_STATE (position correcte)...");
    
    /* DW0: Opcode + Length */
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (8 - 2);  /* 0x70000006 */
    
    /* DW1-2: Scratch Space Base Pointer (0 = pas de scratch space) */
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    /* DW3: Maximum Threads (168) + URB Entries (64) */
    batch[offset++] = (168 << 0) | (64 << 16);  /* 0x004000A8 */
    
    /* DW4: Slice Disable (0 = tous actifs) */
    batch[offset++] = 0x00000000;
    
    /* DW5: CURBE Allocation (1 block) + URB Allocation (2 blocks) */
    batch[offset++] = (1 << 0) | (2 << 16);  /* 0x00020001 */
    
    /* DW6-7: Scoreboard (désactivé) */
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    FORENSIC_LOG("✅ C443: MEDIA_VFE_STATE programmé (Max Threads: 168, URB: 64)");
    
    /* MEDIA_CURBE_LOAD */
    batch[offset++] = GEN9_MEDIA_CURBE_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 64;  /* CURBE size */
    batch[offset++] = 0;   /* CURBE offset in DSH */
    
    FORENSIC_LOG("✅ C433: MEDIA_CURBE_LOAD ajouté");
    
    /* MEDIA_STATE_FLUSH - C432 FIX */
    batch[offset++] = GEN9_MEDIA_STATE_FLUSH | (2 - 2);
    batch[offset++] = 0;
    
    FORENSIC_LOG("✅ C433: MEDIA_STATE_FLUSH ajouté");
    
    /* MEDIA_INTERFACE_DESCRIPTOR_LOAD */
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;  /* IDD size */
    batch[offset++] = 64;  /* IDD offset in DSH */
    
    /* GPGPU_WALKER */
    batch[offset++] = GEN9_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0;  /* Interface Descriptor Offset */
    batch[offset++] = 0;  /* Reserved */
    batch[offset++] = 0;  /* Indirect Data Length */
    /* C451 FIX: SIMD Size = SIMD32 (2 << 30) au lieu de SIMD8 (0 << 30) */
    /* Kernel compilé pour SIMD32 d'après .ze_info */
    batch[offset++] = (2 << 30);  /* SIMD32 */
    batch[offset++] = 0;  /* Thread Group ID Starting X */
    /* C452 FIX: Thread Group ID X Dimension = 32 (SIMD size) */
    /* Avec SIMD32, chaque thread group = 32 work-items */
    batch[offset++] = 32;  /* Thread Group ID X Dimension = SIMD32 */
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    /* C452 FIX: Thread Group Count = 256/32 = 8 pour 256 work-items */
    batch[offset++] = 8;  /* Thread Group Count X = 8 */
    batch[offset++] = 1;
    
    /* PIPE_CONTROL */
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = (1 << 20) | (1 << 18) | (1 << 12) | (1 << 11) | (1 << 10) | (1 << 6) | (1 << 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* MI_BATCH_BUFFER_END */
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    FORENSIC_LOG("✅ Batch buffer construit: %u DWORDs", offset);
    
    /* C444: Dump hexadécimal batch buffer */
    FORENSIC_LOG("🔍 C444: Dump batch buffer (premiers 32 DWORDs):");
    for (uint32_t i = 0; i < (offset < 32 ? offset : 32); i++) {
        if (i % 4 == 0) fprintf(stderr, "\n   [%02u-%02u] ", i, i+3);
        fprintf(stderr, "0x%08X ", batch[i]);
    }
    fprintf(stderr, "\n");
    
    /* C463: SOFT-PINNING - ZERO relocations! */
    /* Toutes les adresses sont écrites directement dans les buffers */
    FORENSIC_LOG("🔧 C463: Soft-pinning activé - ZERO relocations");
    
    /* EXECBUFFER2 avec soft-pinning */
    struct drm_i915_gem_exec_object2 exec_objects[6];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    /* Kernel buffer */
    exec_objects[0].handle = ctx->kernel_bo;
    exec_objects[0].offset = GPU_ADDR_KERNEL;
    exec_objects[0].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    /* Output buffer */
    exec_objects[1].handle = ctx->output_bo;
    exec_objects[1].offset = GPU_ADDR_OUTPUT;
    exec_objects[1].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE;
    
    /* SSH buffer */
    exec_objects[2].handle = ctx->ssh_bo;
    exec_objects[2].offset = GPU_ADDR_SSH;
    exec_objects[2].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    /* DSH buffer */
    exec_objects[3].handle = ctx->dsh_bo;
    exec_objects[3].offset = GPU_ADDR_DSH;
    exec_objects[3].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    /* IOH buffer */
    exec_objects[4].handle = ctx->ioh_bo;
    exec_objects[4].offset = GPU_ADDR_IOH;
    exec_objects[4].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    /* Batch buffer */
    exec_objects[5].handle = ctx->batch_bo;
    exec_objects[5].offset = GPU_ADDR_BATCH;
    exec_objects[5].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 6,
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx->ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch buffer...");
    FORENSIC_LOG("🔍 C444 FORENSIC: EXECBUFFER2 params:");
    FORENSIC_LOG("   buffers_ptr = 0x%016llx", (unsigned long long)execbuf.buffers_ptr);
    FORENSIC_LOG("   buffer_count = %u", execbuf.buffer_count);
    FORENSIC_LOG("   batch_len = %u bytes (%u DWORDs)", execbuf.batch_len, offset);
    FORENSIC_LOG("   flags = 0x%08llx", (unsigned long long)execbuf.flags);
    FORENSIC_LOG("   rsvd1 (ctx_id) = %llu", (unsigned long long)execbuf.rsvd1);
    FORENSIC_LOG("   batch_start_offset = %u", execbuf.batch_start_offset);
    FORENSIC_LOG("🔍 C444: exec_objects[5] (batch):");
    FORENSIC_LOG("   handle = %u", exec_objects[5].handle);
    FORENSIC_LOG("   relocation_count = %llu", (unsigned long long)exec_objects[5].relocation_count);
    FORENSIC_LOG("   relocs_ptr = 0x%016llx", (unsigned long long)exec_objects[5].relocs_ptr);
    FORENSIC_LOG("   alignment = %llu", (unsigned long long)exec_objects[5].alignment);
    FORENSIC_LOG("   offset = 0x%016llx", (unsigned long long)exec_objects[5].offset);
    FORENSIC_LOG("   flags = 0x%016llx", (unsigned long long)exec_objects[5].flags);
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        FORENSIC_LOG("🔍 C444: Analyse errno:");
        if (errno == EINVAL) FORENSIC_LOG("   EINVAL (22) = Invalid argument - Paramètre batch buffer incorrect");
        if (errno == EFAULT) FORENSIC_LOG("   EFAULT (14) = Bad address - Pointeur invalide");
        if (errno == ENOENT) FORENSIC_LOG("   ENOENT (2) = No such file - BO handle invalide");
        if (errno == ENOMEM) FORENSIC_LOG("   ENOMEM (12) = Out of memory");
        return -1;
    }
    
    FORENSIC_LOG("✅ Batch buffer soumis avec succès");
    
    /* ============================================
     * C447 FORENSIC: DUMP CURBE POST-RELOCATION
     * Vérifier si i915 a appliqué la relocation r8
     * ============================================ */
    FORENSIC_LOG("🔍 C447: CURBE POST-RELOCATION (après EXECBUFFER2):");
    uint32_t* dsh_check = (uint32_t*)ctx->dsh_map;
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0) fprintf(stderr, "\n   [r%d] ", i/4);
        fprintf(stderr, "0x%08X ", dsh_check[i]);
    }
    fprintf(stderr, "\n");
    
    uint64_t* r8_ptr = (uint64_t*)&dsh_check[8];
    FORENSIC_LOG("   r8 (64-bit pointer) = 0x%016llx", (unsigned long long)*r8_ptr);
    
    FORENSIC_LOG("🔍 C447: Output BO GPU address:");
    FORENSIC_LOG("   exec_objects[1].offset = 0x%016llx", (unsigned long long)exec_objects[1].offset);
    
    if (*r8_ptr == 0) {
        FORENSIC_LOG("❌ C447 ROOT CAUSE: CURBE r8 = 0x00000000 (relocation NON appliquée!)");
        FORENSIC_LOG("   → GPU écrit dans NULL → write dropped silencieusement");
    } else if (*r8_ptr == exec_objects[1].offset) {
        FORENSIC_LOG("✅ C447: CURBE r8 = output_bo address (relocation OK!)");
        FORENSIC_LOG("   → Problème ailleurs (cache coherency? surface state?)");
    } else {
        FORENSIC_LOG("⚠️  C447: CURBE r8 = 0x%016llx (adresse inattendue!)", (unsigned long long)*r8_ptr);
        FORENSIC_LOG("   → Relocation appliquée mais adresse incorrecte");
    }
    
    /* ============================================
     * C448 FORENSIC: DUMP SURFACE STATE POST-RELOCATION
     * Vérifier si i915 a appliqué la relocation SSH DW8-9
     * ============================================ */
    FORENSIC_LOG("🔍 C448: SSH POST-RELOCATION:");
    uint32_t* ssh_check = (uint32_t*)ctx->ssh_map;
    
    FORENSIC_LOG("   Binding Table:");
    FORENSIC_LOG("     BTI[0] = %u (offset vers Surface State)", ssh_check[0]);
    
    FORENSIC_LOG("   Surface State @ offset 64:");
    uint32_t* ss_check = ssh_check + 16;  // +16 DWORDs = +64 bytes
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0) fprintf(stderr, "\n     DW%02d-%02d: ", i, i+3);
        fprintf(stderr, "0x%08X ", ss_check[i]);
    }
    fprintf(stderr, "\n");
    
    uint64_t* base_addr = (uint64_t*)&ss_check[8];
    FORENSIC_LOG("   Base Address (DW8-9) = 0x%016llx", (unsigned long long)*base_addr);
    FORENSIC_LOG("   Expected (output_bo) = 0x%016llx", (unsigned long long)exec_objects[1].offset);
    
    if (*base_addr == 0) {
        FORENSIC_LOG("❌ C448 ROOT CAUSE: Surface State Base Address = 0x00000000!");
        FORENSIC_LOG("   → Relocation SSH NON appliquée par i915");
        FORENSIC_LOG("   → GPU lit BTI[0] → Surface State → NULL");
        FORENSIC_LOG("   → GPU écrit dans NULL → write dropped");
    } else if (*base_addr == exec_objects[1].offset) {
        FORENSIC_LOG("✅ C448: Surface State Base Address OK!");
        FORENSIC_LOG("   → Problème ailleurs (format? coherency?)");
    } else {
        FORENSIC_LOG("⚠️  C448: Surface State Base Address = 0x%016llx (inattendu!)", (unsigned long long)*base_addr);
        FORENSIC_LOG("   → Relocation appliquée mais adresse incorrecte");
    }
    
    return 0;
}

// Attente GPU idle
static int wait_gem_buffer(int fd, uint32_t handle) {
    struct drm_i915_gem_wait wait = {
        .bo_handle = handle,
        .timeout_ns = 10000000000LL
    };
    
    FORENSIC_LOG("⏳ Attente GPU idle...");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) != 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        return -1;
    }
    
    FORENSIC_LOG("✅ GPU idle confirmé");
    return 0;
}

// Synchronisation domaine CPU
static int set_domain_cpu(int fd, uint32_t handle) {
    struct drm_i915_gem_set_domain set_domain = {
        .handle = handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = I915_GEM_DOMAIN_CPU
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) != 0) {
        FORENSIC_LOG("❌ GEM_SET_DOMAIN failed: %s", strerror(errno));
        return -1;
    }
    
    FORENSIC_LOG("✅ Domaine CPU activé");
    return 0;
}

// Initialisation contexte GPU
static int init_gpu_context(gpu_context_t* ctx, const char* kernel_path) {
    memset(ctx, 0, sizeof(*ctx));
    
    /* Ouverture DRM */
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM: %s", strerror(errno));
        return -1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", ctx->drm_fd);
    
    /* Création contexte GPU */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) != 0) {
        FORENSIC_LOG("❌ Erreur création contexte: %s", strerror(errno));
        close(ctx->drm_fd);
        return -1;
    }
    ctx->ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte GPU créé: id=%u", ctx->ctx_id);
    
    /* Chargement kernel */
    void* kernel_data = NULL;
    if (load_kernel_binary(kernel_path, &kernel_data, &ctx->kernel_size) != 0) {
        close(ctx->drm_fd);
        return -1;
    }
    
    /* Création buffers */
    ctx->kernel_bo = create_gem_buffer(ctx->drm_fd, ctx->kernel_size);
    if (write_to_gem_buffer(ctx->drm_fd, ctx->kernel_bo, kernel_data, ctx->kernel_size) != 0) {
        free(kernel_data);
        close(ctx->drm_fd);
        return -1;
    }
    free(kernel_data);
    
    ctx->output_size = 257 * sizeof(uint32_t);
    ctx->output_bo = create_gem_buffer(ctx->drm_fd, ctx->output_size);
    ctx->output_map = map_gem_buffer(ctx->drm_fd, ctx->output_bo, ctx->output_size);
    if (!ctx->output_map) {
        close(ctx->drm_fd);
        return -1;
    }
    memset(ctx->output_map, 0, ctx->output_size);
    
    ctx->ssh_size = 4096;
    ctx->ssh_bo = create_gem_buffer(ctx->drm_fd, ctx->ssh_size);
    ctx->ssh_map = map_gem_buffer(ctx->drm_fd, ctx->ssh_bo, ctx->ssh_size);
    
    ctx->dsh_size = 4096;
    ctx->dsh_bo = create_gem_buffer(ctx->drm_fd, ctx->dsh_size);
    ctx->dsh_map = map_gem_buffer(ctx->drm_fd, ctx->dsh_bo, ctx->dsh_size);
    
    ctx->ioh_bo = create_gem_buffer(ctx->drm_fd, 4096);
    
    ctx->batch_size = 4096;
    ctx->batch_bo = create_gem_buffer(ctx->drm_fd, ctx->batch_size);
    ctx->batch_map = map_gem_buffer(ctx->drm_fd, ctx->batch_bo, ctx->batch_size);
    
    /* Configuration heaps */
    configure_surface_state(ctx);
    configure_interface_descriptor(ctx);
    
    return 0;
}

// Validation résultats
static int validate_results(gpu_context_t* ctx) {
    FORENSIC_LOG("🔍 VALIDATION RÉSULTATS");
    
    if (wait_gem_buffer(ctx->drm_fd, ctx->output_bo) != 0) {
        return -1;
    }
    
    if (set_domain_cpu(ctx->drm_fd, ctx->output_bo) != 0) {
        return -1;
    }
    
    uint32_t* output = (uint32_t*)ctx->output_map;
    
    FORENSIC_LOG("📊 output[0] = 0x%08X (attendu: 0x12345678)", output[0]);
    
    if (output[0] == 0x12345678) {
        FORENSIC_LOG("✅ TEST ABSOLU RÉUSSI: Pipeline GPU→CPU fonctionnel!");
        
        int pattern_ok = 1;
        for (int i = 0; i < 256; i++) {
            if (output[i + 1] != (uint32_t)i) {
                FORENSIC_LOG("⚠️  output[%d] = %u (attendu: %d)", i+1, output[i+1], i);
                pattern_ok = 0;
                break;
            }
        }
        
        if (pattern_ok) {
            FORENSIC_LOG("✅ TEST SECONDAIRE RÉUSSI: get_global_id() fonctionnel!");
        }
        
        return 0;
    } else {
        FORENSIC_LOG("❌ TEST ABSOLU ÉCHOUÉ: output[0] != 0x12345678");
        FORENSIC_LOG("🔍 Dump premiers 16 uint32_t:");
        for (int i = 0; i < 16; i++) {
            FORENSIC_LOG("   output[%d] = 0x%08X", i, output[i]);
        }
        return -1;
    }
}

int main(int argc, char** argv) {
    FORENSIC_LOG("🚀 TEST C463 - SOFT-PINNING GPU NATIF (ZERO relocations)");
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <kernel.bin>\n", argv[0]);
        return 1;
    }
    
    gpu_context_t ctx;
    if (init_gpu_context(&ctx, argv[1]) != 0) {
        return 1;
    }
    
    if (build_batch_buffer(&ctx) != 0) {
        return 1;
    }
    
    int result = validate_results(&ctx);
    
    if (result == 0) {
        FORENSIC_LOG("✅ TEST C433 RÉUSSI");
    } else {
        FORENSIC_LOG("❌ TEST C433 ÉCHOUÉ");
    }
    
    close(ctx.drm_fd);
    return result;
}

// Made with Bob
