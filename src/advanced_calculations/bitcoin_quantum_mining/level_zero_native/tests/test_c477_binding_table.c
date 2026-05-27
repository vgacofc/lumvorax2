/*
 * TEST C477 - BINDING TABLE + SURFACE STATE
 *
 * Base: test_c476_kernel_execution.c
 * Ajout: Binding Table + Surface State Gen7 (infrastructure Beignet complète)
 *
 * Objectif: Premier 0xDEADBEEF écrit par GPU natif i915 ✅
 *
 * Nouveautés C477:
 * - Buffer surface_heap (8KB): binding_table + surface_states
 * - gen7_surface_state_t configuré pour output buffer
 * - IDRT.binding_table_pointer lié à surface_heap
 * - 2 relocations supplémentaires (surface→output, idrt→surface_heap)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
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

/* Commandes Gen9 */
#define CMD(PIPELINE,OP,SUB_OP) ((3 << 29) | ((PIPELINE) << 27) | ((OP) << 24) | ((SUB_OP) << 16))
#define CMD_PIPE_CONTROL                CMD(3, 2, 0)
#define CMD_PIPELINE_SELECT             CMD(1, 1, 4)
#define CMD_MEDIA_STATE_POINTERS        CMD(2, 0, 0)
#define CMD_CURBE_LOAD                  CMD(2, 0, 1)
#define CMD_IDRT_LOAD                   CMD(2, 0, 2)
#define CMD_GPGPU_WALKER                CMD(2, 1, 5)
#define CMD_MEDIA_STATE_FLUSH           CMD(2, 0, 4)

#define PIPELINE_SELECT_GPGPU           2

/* Interface Descriptor Gen8 (Beignet intel_structs.h) */
typedef struct {
    struct {
        uint32_t pad6:6;
        uint32_t kernel_start_pointer:26;
    } desc0;
    struct {
        uint32_t kernel_start_pointer_high:16;
        uint32_t pad6:16;
    } desc1;
    struct {
        uint32_t pad:7;
        uint32_t software_exception:1;
        uint32_t pad2:3;
        uint32_t maskstack_exception:1;
        uint32_t pad3:1;
        uint32_t illegal_opcode_exception:1;
        uint32_t pad4:2;
        uint32_t floating_point_mode:1;
        uint32_t thread_priority:1;
        uint32_t single_program_flow:1;
        uint32_t denorm_mode:1;
        uint32_t thread_preemption_disable:1;
        uint32_t pad5:11;
    } desc2;
    struct {
        uint32_t pad:2;
        uint32_t sampler_count:3;
        uint32_t sampler_state_pointer:27;
    } desc3;
    struct {
        uint32_t binding_table_entry_count:5;
        uint32_t binding_table_pointer:11;
        uint32_t pad:16;
    } desc4;
    struct {
        uint32_t curbe_read_offset:16;
        uint32_t curbe_read_len:16;
    } desc5;
    struct {
        uint32_t group_threads_num:10;
        uint32_t pad:6;
        uint32_t barrier_enable:1;
        uint32_t slm_sz:5;
        uint32_t pad2:2;
        uint32_t rounding_mode:2;
        uint32_t pad3:6;
    } desc6;
    struct {
        uint32_t cross_thread_curbe_read_len:8;
        uint32_t pad:24;
    } desc7;
} gen8_interface_descriptor_t;

/* Surface State Gen7 (Beignet intel_structs.h) - 8 DWORDs */
typedef struct {
    struct {
        uint32_t cube_pos_z:1;
        uint32_t cube_neg_z:1;
        uint32_t cube_pos_y:1;
        uint32_t cube_neg_y:1;
        uint32_t cube_pos_x:1;
        uint32_t cube_neg_x:1;
        uint32_t media_boundary_pixel_mode:2;
        uint32_t render_cache_rw_mode:1;
        uint32_t pad1:1;
        uint32_t surface_array_spacing:1;
        uint32_t vertical_line_stride_offset:1;
        uint32_t vertical_line_stride:1;
        uint32_t tile_walk:1;
        uint32_t tiled_surface:1;
        uint32_t horizontal_alignment:1;
        uint32_t vertical_alignment:2;
        uint32_t surface_format:9;     // 0 = RAW
        uint32_t pad0:1;
        uint32_t surface_array:1;
        uint32_t surface_type:3;       // 0 = SURFTYPE_BUFFER
    } ss0;
    struct {
        uint32_t base_addr;            // Relocalisé vers output_bo
    } ss1;
    struct {
        uint32_t width:14;             // Buffer size - 1
        uint32_t pad1:2;
        uint32_t height:14;
        uint32_t pad0:2;
    } ss2;
    struct {
        uint32_t pitch:18;             // Buffer size - 1
        uint32_t pad0:3;
        uint32_t depth:11;
    } ss3;
    struct {
        uint32_t pad;
    } ss4;
    struct {
        uint32_t mip_count:4;
        uint32_t surface_min_load:4;
        uint32_t pad2:6;
        uint32_t coherence_type:1;
        uint32_t stateless_force_write_thru:1;
        uint32_t cache_control:4;
        uint32_t pad1:12;
    } ss5;
    struct {
        uint32_t pad;
    } ss6;
    struct {
        uint32_t pad;
    } ss7;
} gen7_surface_state_t;

/* Surface Heap: Binding Table + Surface States */
typedef struct {
    uint32_t binding_table[256];                    // Offsets vers surface states
    gen7_surface_state_t surface_states[256];       // Surface states
} surface_heap_t;

#define BINDING_TABLE_SIZE (256 * sizeof(uint32_t))
#define SURFACE_STATE_SIZE sizeof(gen7_surface_state_t)

/* Include batch complet */
#include "../include/gen9_null_state_batch.h"

/* Relocations from kernel */
static const uint32_t gen9_null_state_relocs[] = {
    0x000007a8, 0x000007b4, 0x000007bc, 0x000007cc
};

#define BATCH_SIZE (sizeof(gen9_null_state_batch))
#define NUM_RELOCS (sizeof(gen9_null_state_relocs) / sizeof(gen9_null_state_relocs[0]))

/* Kernel bytecode GPU Gen9 pur - Section .text extraite */
#include "test_c475_minimal_compute_text.h"
#define minimal_kernel kernel_text
#define minimal_kernel_size KERNEL_TEXT_SIZE

int main(void) {
    FORENSIC_LOG("🚀 TEST C477 - BINDING TABLE + SURFACE STATE");
    FORENSIC_LOG("📊 État: 10%% - Démarrage");
    
    /* Ouvrir DRM */
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        FORENSIC_LOG("❌ Impossible d'ouvrir /dev/dri/renderD128: %s", strerror(errno));
        return 1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", drm_fd);
    FORENSIC_LOG("📊 État: 15%%");
    
    /* Créer contexte GPU */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        FORENSIC_LOG("❌ GEM_CONTEXT_CREATE failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte GPU créé: id=%u", ctx_id);
    
    /* Créer buffers: batch + 4 relocs + kernel + output + idrt + surface_heap */
    struct drm_i915_gem_create create_batch = {.size = 16384};
    struct drm_i915_gem_create create_kernel = {.size = 4096};
    struct drm_i915_gem_create create_output = {.size = 4096};
    struct drm_i915_gem_create create_idrt = {.size = 4096};
    struct drm_i915_gem_create create_surface_heap = {.size = 8192};  // Binding table + surface states
    struct drm_i915_gem_create create_reloc[4];
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0 ||
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0 ||
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0 ||
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_idrt) < 0 ||
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_surface_heap) < 0) {
        FORENSIC_LOG("❌ Buffer creation failed");
        close(drm_fd);
        return 1;
    }
    
    uint32_t batch_bo = create_batch.handle;
    uint32_t kernel_bo = create_kernel.handle;
    uint32_t output_bo = create_output.handle;
    uint32_t idrt_bo = create_idrt.handle;
    uint32_t surface_heap_bo = create_surface_heap.handle;
    uint32_t reloc_bo[4];
    
    for (int i = 0; i < 4; i++) {
        create_reloc[i].size = 4096;
        if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_reloc[i]) < 0) {
            FORENSIC_LOG("❌ Reloc buffer %d creation failed", i);
            close(drm_fd);
            return 1;
        }
        reloc_bo[i] = create_reloc[i].handle;
    }
    
    FORENSIC_LOG("✅ Buffers créés: batch=%u kernel=%u output=%u idrt=%u surface_heap=%u relocs=[%u,%u,%u,%u]",
                 batch_bo, kernel_bo, output_bo, idrt_bo, surface_heap_bo,
                 reloc_bo[0], reloc_bo[1], reloc_bo[2], reloc_bo[3]);
    FORENSIC_LOG("📊 État: 25%%");
    
    /* Mapper et écrire kernel */
    struct drm_i915_gem_mmap_offset mmap_kernel = {
        .handle = kernel_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_kernel) < 0) {
        FORENSIC_LOG("❌ MMAP_OFFSET kernel failed");
        close(drm_fd);
        return 1;
    }
    
    uint32_t* kernel_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_kernel.offset);
    if (kernel_ptr == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap kernel failed");
        close(drm_fd);
        return 1;
    }
    
    memcpy(kernel_ptr, minimal_kernel, minimal_kernel_size);
    munmap(kernel_ptr, 4096);
    FORENSIC_LOG("✅ Kernel ISA Gen9 RÉEL écrit: %d bytes", (int)minimal_kernel_size);
    FORENSIC_LOG("📊 État: 30%%");
    
    /* Mapper et configurer surface_heap (binding table + surface state) */
    struct drm_i915_gem_mmap_offset mmap_surface = {
        .handle = surface_heap_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_surface) < 0) {
        FORENSIC_LOG("❌ MMAP_OFFSET surface_heap failed");
        close(drm_fd);
        return 1;
    }
    
    surface_heap_t* surface_heap = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_surface.offset);
    if (surface_heap == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap surface_heap failed");
        close(drm_fd);
        return 1;
    }
    
    /* Configuration binding table: entry 0 pointe vers surface state 0 */
    memset(surface_heap, 0, sizeof(surface_heap_t));
    surface_heap->binding_table[0] = BINDING_TABLE_SIZE;  // Offset vers premier surface state
    
    /* Configuration surface state pour output buffer */
    gen7_surface_state_t* ss = &surface_heap->surface_states[0];
    ss->ss0.surface_type = 0;        // SURFTYPE_BUFFER
    ss->ss0.surface_format = 0;      // RAW
    ss->ss0.tiled_surface = 0;
    ss->ss0.horizontal_alignment = 0;
    ss->ss0.vertical_alignment = 0;
    ss->ss1.base_addr = 0;           // Sera relocalisé vers output_bo
    ss->ss2.width = 4095;            // 4096 - 1
    ss->ss2.height = 0;
    ss->ss3.pitch = 4095;            // 4096 - 1
    ss->ss3.depth = 0;
    ss->ss5.cache_control = 0;
    
    munmap(surface_heap, 8192);
    FORENSIC_LOG("✅ Surface heap configuré: binding_table[0]=%u", BINDING_TABLE_SIZE);
    FORENSIC_LOG("📊 État: 40%%");
    
    /* Mapper et construire IDRT */
    struct drm_i915_gem_mmap_offset mmap_idrt = {
        .handle = idrt_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_idrt) < 0) {
        FORENSIC_LOG("❌ MMAP_OFFSET idrt failed");
        close(drm_fd);
        return 1;
    }
    
    gen8_interface_descriptor_t* idrt_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_idrt.offset);
    if (idrt_ptr == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap idrt failed");
        close(drm_fd);
        return 1;
    }
    
    /* Configuration IDRT (Beignet intel_gpgpu_build_idrt_gen9) */
    memset(idrt_ptr, 0, sizeof(gen8_interface_descriptor_t));
    idrt_ptr->desc0.kernel_start_pointer = 0; // Sera relocalisé
    idrt_ptr->desc2.single_program_flow = 0;
    idrt_ptr->desc2.floating_point_mode = 0;
    idrt_ptr->desc3.sampler_state_pointer = 0;
    idrt_ptr->desc4.binding_table_entry_count = 1;  // 1 buffer (output)
    idrt_ptr->desc4.binding_table_pointer = 0;      // Offset/32 dans surface_heap (sera relocalisé)
    idrt_ptr->desc5.curbe_read_len = 0;             // Pas de CURBE
    idrt_ptr->desc5.curbe_read_offset = 0;
    idrt_ptr->desc6.group_threads_num = 1; // 1 thread
    idrt_ptr->desc6.barrier_enable = 0;
    idrt_ptr->desc6.slm_sz = 0;
    idrt_ptr->desc6.rounding_mode = 0;
    
    munmap(idrt_ptr, 4096);
    FORENSIC_LOG("✅ Interface Descriptor configuré");
    FORENSIC_LOG("📊 État: 50%%");
    
    /* Mapper batch buffer */
    struct drm_i915_gem_mmap_offset mmap_batch = {
        .handle = batch_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_batch) < 0) {
        FORENSIC_LOG("❌ MMAP_OFFSET batch failed");
        close(drm_fd);
        return 1;
    }
    
    uint32_t* batch = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_batch.offset);
    if (batch == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap batch failed");
        close(drm_fd);
        return 1;
    }
    
    /* Copier batch null_state */
    memcpy(batch, gen9_null_state_batch, BATCH_SIZE);
    uint32_t offset = BATCH_SIZE / 4;
    FORENSIC_LOG("✅ Batch null_state copié: %zu bytes (%u DWORDs)", BATCH_SIZE, offset);
    
    /* Construire séquence GPGPU complète */
    
    // 1. PIPE_CONTROL
    batch[offset++] = CMD_PIPE_CONTROL | (5-2);
    batch[offset++] = 0x00100000;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // 2. PIPELINE_SELECT GPGPU
    batch[offset++] = CMD_PIPELINE_SELECT | PIPELINE_SELECT_GPGPU;
    
    // 3. MEDIA_VFE_STATE
    batch[offset++] = CMD_MEDIA_STATE_POINTERS | (9-2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0 | ((64-1) << 16) | (2 << 8) | 0xc0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // 4. CURBE_LOAD (vide)
    batch[offset++] = CMD_CURBE_LOAD | (4-2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // 5. IDRT_LOAD (avec relocation)
    uint32_t idrt_load_offset = offset * 4 + 12; // Position du 4ème DWORD
    batch[offset++] = CMD_IDRT_LOAD | (4-2);
    batch[offset++] = 0;
    batch[offset++] = 1 << 5; // 1 descriptor
    batch[offset++] = 0; // Sera relocalisé vers idrt_bo
    
    // 6. GPGPU_WALKER
    batch[offset++] = CMD_GPGPU_WALKER | 13;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = (0 << 30) | 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0xFF;
    batch[offset++] = 0xFFFFFFFF;
    
    // 7. MEDIA_STATE_FLUSH
    batch[offset++] = CMD_MEDIA_STATE_FLUSH | 0;
    batch[offset++] = 0;
    
    // 8. PIPE_CONTROL final
    batch[offset++] = CMD_PIPE_CONTROL | (5-2);
    batch[offset++] = 0x00100000;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    uint32_t batch_len = offset * 4;
    munmap(batch, 16384);
    
    FORENSIC_LOG("✅ Batch complet: %u DWORDs (%u bytes)", offset, batch_len);
    FORENSIC_LOG("   - Null state: 960 DWORDs");
    FORENSIC_LOG("   - GPGPU: %u DWORDs", offset - 960);
    FORENSIC_LOG("📊 État: 65%%");
    
    /* Préparer relocations (8 total) */
    struct drm_i915_gem_relocation_entry relocs[NUM_RELOCS + 4];
    memset(relocs, 0, sizeof(relocs));
    
    // Relocs null_state (4)
    for (size_t i = 0; i < NUM_RELOCS; i++) {
        relocs[i].target_handle = reloc_bo[i];
        relocs[i].offset = gen9_null_state_relocs[i];
        relocs[i].read_domains = I915_GEM_DOMAIN_RENDER;
    }
    
    // Reloc 4: IDRT kernel_start_pointer (dans IDRT buffer)
    relocs[NUM_RELOCS].target_handle = kernel_bo;
    relocs[NUM_RELOCS].delta = 0;
    relocs[NUM_RELOCS].offset = 0; // desc0 offset dans idrt_bo
    relocs[NUM_RELOCS].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[NUM_RELOCS].write_domain = 0;
    
    // Reloc 5: IDRT_LOAD offset (dans batch)
    relocs[NUM_RELOCS+1].target_handle = idrt_bo;
    relocs[NUM_RELOCS+1].delta = 0;
    relocs[NUM_RELOCS+1].offset = idrt_load_offset;
    relocs[NUM_RELOCS+1].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[NUM_RELOCS+1].write_domain = 0;
    
    // Reloc 6: Surface state ss1.base_addr → output_bo (dans surface_heap)
    relocs[NUM_RELOCS+2].target_handle = output_bo;
    relocs[NUM_RELOCS+2].delta = 0;
    relocs[NUM_RELOCS+2].offset = BINDING_TABLE_SIZE + offsetof(gen7_surface_state_t, ss1);
    relocs[NUM_RELOCS+2].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[NUM_RELOCS+2].write_domain = I915_GEM_DOMAIN_RENDER;
    
    // Reloc 7: IDRT binding_table_pointer → surface_heap (dans IDRT)
    relocs[NUM_RELOCS+3].target_handle = surface_heap_bo;
    relocs[NUM_RELOCS+3].delta = 0;
    relocs[NUM_RELOCS+3].offset = offsetof(gen8_interface_descriptor_t, desc4);
    relocs[NUM_RELOCS+3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[NUM_RELOCS+3].write_domain = 0;
    
    FORENSIC_LOG("✅ Relocations préparées: %zu total", NUM_RELOCS + 4);
    FORENSIC_LOG("   - Null state: 4");
    FORENSIC_LOG("   - IDRT→kernel: 1");
    FORENSIC_LOG("   - Batch→IDRT: 1");
    FORENSIC_LOG("   - Surface→output: 1");
    FORENSIC_LOG("   - IDRT→surface_heap: 1");
    FORENSIC_LOG("📊 État: 75%%");
    
    /* EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_objects[9];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    for (int i = 0; i < 4; i++) {
        exec_objects[i].handle = reloc_bo[i];
    }
    exec_objects[4].handle = kernel_bo;
    exec_objects[5].handle = output_bo;
    exec_objects[5].flags = EXEC_OBJECT_WRITE;
    exec_objects[6].handle = surface_heap_bo;
    exec_objects[6].relocation_count = 1;
    exec_objects[6].relocs_ptr = (uint64_t)&relocs[NUM_RELOCS+2];
    exec_objects[7].handle = idrt_bo;
    exec_objects[7].relocation_count = 2;
    exec_objects[7].relocs_ptr = (uint64_t)&relocs[NUM_RELOCS];
    exec_objects[8].handle = batch_bo;
    exec_objects[8].relocation_count = NUM_RELOCS + 1;
    exec_objects[8].relocs_ptr = (uint64_t)relocs;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 9,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch (null_state + GPGPU + kernel)...");
    FORENSIC_LOG("📊 État: 80%%");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double submit_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
    
    FORENSIC_LOG("✅ EXECBUFFER2 réussi! (%.3f ms)", submit_ms);
    
    /* Attendre GPU idle */
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = 2000000000
    };
    
    FORENSIC_LOG("⏳ Attente GPU idle...");
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double wait_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
    
    FORENSIC_LOG("✅ GPU idle confirmé (%.3f ms)", wait_ms);
    FORENSIC_LOG("📊 État: 90%%");
    
    /* Vérifier output */
    struct drm_i915_gem_mmap_offset mmap_output = {
        .handle = output_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_output) < 0) {
        FORENSIC_LOG("❌ MMAP_OFFSET output failed");
        close(drm_fd);
        return 1;
    }
    
    uint32_t* output = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_output.offset);
    if (output == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap output failed");
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("");
    FORENSIC_LOG("📊 RÉSULTATS OUTPUT:");
    FORENSIC_LOG("output[0] = 0x%08X (attendu: 0xDEADBEEF)", output[0]);
    FORENSIC_LOG("output[1] = 0x%08X (attendu: 0xCAFEBABE)", output[1]);
    FORENSIC_LOG("output[2] = 0x%08X (attendu: 0x12345678)", output[2]);
    FORENSIC_LOG("output[3] = 0x%08X (attendu: 0xABCDEF00)", output[3]);
    
    int success = (output[0] == 0xDEADBEEF);
    uint32_t out0 = output[0];
    munmap(output, 4096);
    
    FORENSIC_LOG("📊 État: 100%%");
    FORENSIC_LOG("");
    
    if (success) {
        FORENSIC_LOG("🎉 TEST C476 RÉUSSI!");
        FORENSIC_LOG("✅ Kernel GPU exécuté!");
        FORENSIC_LOG("✅ Output = 0xDEADBEEF");
        FORENSIC_LOG("✅ Premier calcul GPU natif i915!");
    } else {
        FORENSIC_LOG("⚠️  TEST C476 PARTIEL");
        FORENSIC_LOG("✅ Batch stable");
        FORENSIC_LOG("❌ Kernel non exécuté (output = 0x%08X)", out0);
        FORENSIC_LOG("💡 Debugging requis: ISA kernel ou relocations");
    }
    
    close(drm_fd);
    return success ? 0 : 1;
}

// Made with Bob
