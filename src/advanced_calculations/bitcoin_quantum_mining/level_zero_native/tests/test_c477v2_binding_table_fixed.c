/*
 * TEST C477v2 - BINDING TABLE + SURFACE STATE (FORMAT INTEL BEIGNET EXACT)
 *
 * Base: test_c477_binding_table.c
 * Correction: Utilisation format EXACT Intel Beignet pour surface_heap
 *
 * Source: /tmp/beignet/src/intel/intel_gpgpu.c lignes 58-62
 * typedef struct surface_heap {
 *   uint32_t binding_table[256];           // 1024 bytes
 *   char surface[256*sizeof(gen_surface_state_t)];  // 8192 bytes
 * } surface_heap_t;  // Total: 9216 bytes
 *
 * Objectif: Premier 0xDEADBEEF écrit par GPU natif i915 ✅
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

/* Surface State Gen7 (8 DWORDs = 32 bytes) */
typedef struct {
    struct {
        uint32_t cube_pos_z:1;
        uint32_t cube_neg_z:1;
        uint32_t cube_pos_y:1;
        uint32_t cube_neg_y:1;
        uint32_t cube_pos_x:1;
        uint32_t cube_neg_x:1;
        uint32_t pad2:2;
        uint32_t render_cache_rw_mode:1;
        uint32_t pad1:1;
        uint32_t surface_array_spacing:1;
        uint32_t vert_line_stride_ofs:1;
        uint32_t vert_line_stride:1;
        uint32_t tile_walk:1;
        uint32_t tiled_surface:1;
        uint32_t horizontal_alignment:1;
        uint32_t vertical_alignment:2;
        uint32_t surface_format:9;
        uint32_t pad0:1;
        uint32_t is_array:1;
        uint32_t surface_type:3;
    } ss0;
    struct {
        uint32_t base_addr;
    } ss1;
    struct {
        uint32_t width:14;
        uint32_t pad1:2;
        uint32_t height:14;
        uint32_t pad0:2;
    } ss2;
    struct {
        uint32_t pitch:18;
        uint32_t pad:3;
        uint32_t depth:11;
    } ss3;
    struct {
        uint32_t multisample_position_palette_index:3;
        uint32_t num_multisamples:3;
        uint32_t multisampled_surface_storage_format:1;
        uint32_t render_target_view_extent:11;
        uint32_t min_array_elt:11;
        uint32_t rotation:2;
        uint32_t pad0:1;
    } ss4;
    struct {
        uint32_t mip_count:4;
        uint32_t min_lod:4;
        uint32_t pad1:6;
        uint32_t coherence_type:1;
        uint32_t stateless_force_write_thru:1;
        uint32_t cache_control:4;
        uint32_t y_offset:4;
        uint32_t pad0:1;
        uint32_t x_offset:7;
    } ss5;
    uint32_t ss6;
    struct {
        uint32_t resource_min_lod:12;
        uint32_t pad0:4;
        uint32_t shader_chanel_select_a:3;
        uint32_t shader_chanel_select_b:3;
        uint32_t shader_chanel_select_g:3;
        uint32_t shader_chanel_select_r:3;
        uint32_t pad1:4;
    } ss7;
} gen7_surface_state_t;

/* Surface Heap Intel Beignet EXACT (9216 bytes) */
typedef struct {
    uint32_t binding_table[256];                        // 1024 bytes
    char surface[256 * sizeof(gen7_surface_state_t)];   // 8192 bytes (256 * 32)
} surface_heap_t;

#define BINDING_TABLE_SIZE (256 * sizeof(uint32_t))  // 1024 bytes

/* Interface Descriptor Gen8 */
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

#include "../include/gen9_null_state_batch.h"

/* Kernel ISA Gen9 - Section .text extraite */
static const unsigned char kernel_text[] = {
    0x01,0x00,0x60,0x00,0x0c,0x02,0x40,0x20,0x00,0x00,0x20,0x00,
    0x00,0x00,0x00,0x00,0x06,0x80,0x00,0x00,0x04,0x00,0x00,0x30,
    0x00,0x10,0x00,0x16,0xc0,0x04,0xc0,0x04,0x01,0x00,0x00,0x00,
    0x0c,0x47,0x80,0x20,0xef,0xbe,0xad,0xde,0xbe,0xba,0xfe,0xca,
    0x01,0x00,0x60,0x00,0x6c,0x26,0x70,0x20,0x00,0x00,0x00,0x00,
    0x40,0xc8,0x00,0x00,0x01,0x00,0x00,0x00,0x0c,0x47,0x88,0x20,
    0x78,0x56,0x34,0x12,0x00,0xef,0xcd,0xab,0x01,0x4d,0x00,0x20,
    0x07,0x7f,0x02,0x00,0x01,0xcd,0x02,0x20,0x00,0x08,0x04,0x00,
    0x40,0x00,0x60,0x00,0x0c,0x43,0xc0,0x20,0x60,0x00,0x00,0x1a,
    0x70,0x00,0x8d,0x00,0x01,0xc3,0x02,0x60,0x05,0x08,0x04,0x00,
    0x33,0x00,0x60,0x0c,0x14,0x80,0x00,0x00,0xc1,0x00,0x00,0x00,
    0xff,0x81,0x06,0x04,0x31,0x00,0x00,0x07,0x04,0x02,0x00,0x20,
    0xe0,0x0f,0x00,0x06,0x10,0x00,0x00,0x82,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
#define KERNEL_TEXT_SIZE 320

/* Relocations null_state (depuis gen9_renderstate.c) */
static const struct {
    uint32_t offset;
    uint32_t index;
} gen9_null_state_relocs[4] = {
    {0x00000004, 0},
    {0x00000028, 1},
    {0x0000004c, 2},
    {0x00000070, 3}
};

int main() {
    FORENSIC_LOG("🚀 TEST C477v2 - BINDING TABLE FORMAT INTEL BEIGNET EXACT");
    FORENSIC_LOG("📊 État: 10%% - Démarrage");
    
    /* Ouvrir DRM */
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        FORENSIC_LOG("❌ Impossible d'ouvrir /dev/dri/card1");
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
    struct drm_i915_gem_create create_surface_heap = {.size = 16384};  // 16KB pour 9216 bytes + marge
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
    
    memcpy(kernel_ptr, kernel_text, KERNEL_TEXT_SIZE);
    munmap(kernel_ptr, 4096);
    FORENSIC_LOG("✅ Kernel ISA Gen9 RÉEL écrit: %d bytes", KERNEL_TEXT_SIZE);
    FORENSIC_LOG("📊 État: 30%%");
    
    /* Mapper et configurer surface_heap (FORMAT INTEL BEIGNET EXACT) */
    struct drm_i915_gem_mmap_offset mmap_surface = {
        .handle = surface_heap_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_surface) < 0) {
        FORENSIC_LOG("❌ MMAP_OFFSET surface_heap failed");
        close(drm_fd);
        return 1;
    }
    
    surface_heap_t* surface_heap = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_surface.offset);
    if (surface_heap == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap surface_heap failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    /* Configuration binding table: entry 0 pointe vers surface state 0 (MÉTHODE INTEL BEIGNET) */
    memset(surface_heap, 0, sizeof(surface_heap_t));
    surface_heap->binding_table[0] = offsetof(surface_heap_t, surface) + 0 * sizeof(gen7_surface_state_t);
    
    /* Configuration surface state pour output buffer (MÉTHODE INTEL BEIGNET) */
    gen7_surface_state_t* ss = (gen7_surface_state_t*)&surface_heap->surface[0 * sizeof(gen7_surface_state_t)];
    memset(ss, 0, sizeof(gen7_surface_state_t));
    ss->ss0.surface_type = 0;        // I965_SURFACE_BUFFER
    ss->ss0.surface_format = 0;      // I965_SURFACEFORMAT_RAW
    ss->ss2.width = 4095;            // (4096 - 1) & 0x7f = 127
    ss->ss2.height = (4095 >> 7) & 0x3fff;  // bits 20:7
    ss->ss3.depth = (4095 >> 21) & 0x3ff;   // bits 30:21
    ss->ss3.pitch = 4095;
    ss->ss5.cache_control = 0;
    ss->ss1.base_addr = 0;           // Sera relocalisé vers output_bo
    
    munmap(surface_heap, 16384);
    FORENSIC_LOG("✅ Surface heap configuré (FORMAT INTEL): binding_table[0]=%u", 
                 (uint32_t)(offsetof(surface_heap_t, surface)));
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
    
    memset(idrt_ptr, 0, sizeof(gen8_interface_descriptor_t));
    idrt_ptr->desc0.kernel_start_pointer = 0;  // Relocalisé vers kernel_bo
    idrt_ptr->desc2.floating_point_mode = 1;
    idrt_ptr->desc2.single_program_flow = 1;
    idrt_ptr->desc3.sampler_count = 0;
    idrt_ptr->desc3.sampler_state_pointer = 0;
    idrt_ptr->desc4.binding_table_entry_count = 1;  // 1 entrée dans binding table
    idrt_ptr->desc4.binding_table_pointer = 0;      // Relocalisé vers surface_heap_bo
    idrt_ptr->desc5.curbe_read_len = 0;
    idrt_ptr->desc6.group_threads_num = 1;
    idrt_ptr->desc6.barrier_enable = 0;
    idrt_ptr->desc6.slm_sz = 0;
    idrt_ptr->desc7.cross_thread_curbe_read_len = 0;
    
    munmap(idrt_ptr, 4096);
    FORENSIC_LOG("✅ IDRT configuré: binding_table_entry_count=1");
    FORENSIC_LOG("📊 État: 50%%");
    
    /* Construire batch buffer complet */
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
    
    uint32_t offset = 0;
    
    /* 1. Batch null_state complet (960 DWORDs) */
    memcpy(&batch[offset], gen9_null_state_batch, sizeof(gen9_null_state_batch));
    offset += sizeof(gen9_null_state_batch) / 4;
    
    /* 2. PIPE_CONTROL (flush) */
    batch[offset++] = CMD_PIPE_CONTROL | (6 - 2);
    batch[offset++] = (1 << 21) | (1 << 20);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 3. PIPELINE_SELECT GPGPU */
    batch[offset++] = CMD_PIPELINE_SELECT | (1 - 2);
    batch[offset++] = PIPELINE_SELECT_GPGPU;
    
    /* 4. MEDIA_VFE_STATE */
    batch[offset++] = (CMD_MEDIA_STATE_POINTERS & 0xFFFF0000) | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = (1 << 16);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 5. CURBE_LOAD */
    batch[offset++] = CMD_CURBE_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 6. IDRT_LOAD (relocalisé vers idrt_bo) */
    uint32_t idrt_load_offset = offset;
    batch[offset++] = CMD_IDRT_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;
    batch[offset++] = 0;  // Relocalisé
    
    /* 7. GPGPU_WALKER */
    batch[offset++] = CMD_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    
    /* 8. MEDIA_STATE_FLUSH */
    batch[offset++] = CMD_MEDIA_STATE_FLUSH | (2 - 2);
    batch[offset++] = 0;
    
    /* 9. PIPE_CONTROL final */
    batch[offset++] = CMD_PIPE_CONTROL | (6 - 2);
    batch[offset++] = (1 << 21) | (1 << 20);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 10. MI_BATCH_BUFFER_END */
    batch[offset++] = 0x0A000000;
    
    uint32_t batch_size = offset * 4;
    munmap(batch, 16384);
    FORENSIC_LOG("✅ Batch construit: %u DWORDs (%u bytes)", offset, batch_size);
    FORENSIC_LOG("📊 État: 70%%");
    
    /* Configurer relocations (8 total) */
    struct drm_i915_gem_relocation_entry relocs[8];
    memset(relocs, 0, sizeof(relocs));
    
    /* Relocs 0-3: null_state → reloc_bo[0-3] */
    for (int i = 0; i < 4; i++) {
        relocs[i].target_handle = reloc_bo[i];
        relocs[i].delta = 0;
        relocs[i].offset = gen9_null_state_relocs[i].offset;
        relocs[i].presumed_offset = 0;
        relocs[i].read_domains = I915_GEM_DOMAIN_RENDER;
        relocs[i].write_domain = I915_GEM_DOMAIN_RENDER;
    }
    
    /* Reloc 4: IDRT.desc0 → kernel_bo (dans idrt_bo) */
    relocs[4].target_handle = kernel_bo;
    relocs[4].delta = 0;
    relocs[4].offset = offsetof(gen8_interface_descriptor_t, desc0);
    relocs[4].presumed_offset = 0;
    relocs[4].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[4].write_domain = 0;
    
    /* Reloc 5: IDRT.desc4.binding_table_pointer → surface_heap_bo (dans idrt_bo) */
    relocs[5].target_handle = surface_heap_bo;
    relocs[5].delta = 0;
    relocs[5].offset = offsetof(gen8_interface_descriptor_t, desc4);
    relocs[5].presumed_offset = 0;
    relocs[5].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[5].write_domain = 0;
    
    /* Reloc 6: surface_state.ss1 → output_bo (dans surface_heap_bo) */
    relocs[6].target_handle = output_bo;
    relocs[6].delta = 0;
    relocs[6].offset = offsetof(surface_heap_t, surface) + 0 * sizeof(gen7_surface_state_t) + offsetof(gen7_surface_state_t, ss1);
    relocs[6].presumed_offset = 0;
    relocs[6].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[6].write_domain = I915_GEM_DOMAIN_RENDER;
    
    /* Reloc 7: Batch IDRT_LOAD → idrt_bo (dans batch_bo) */
    relocs[7].target_handle = idrt_bo;
    relocs[7].delta = 0;
    relocs[7].offset = idrt_load_offset * 4 + 12;
    relocs[7].presumed_offset = 0;
    relocs[7].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[7].write_domain = 0;
    
    FORENSIC_LOG("✅ 8 relocations configurées");
    FORENSIC_LOG("📊 État: 80%%");
    
    /* Préparer EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_objects[9];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    for (int i = 0; i < 4; i++) {
        exec_objects[i].handle = reloc_bo[i];
        exec_objects[i].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
        exec_objects[i].offset = 0x100000 + i * 0x1000;
    }
    
    exec_objects[4].handle = kernel_bo;
    exec_objects[4].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[4].offset = 0x200000;
    
    exec_objects[5].handle = output_bo;
    exec_objects[5].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[5].offset = 0x300000;
    
    exec_objects[6].handle = idrt_bo;
    exec_objects[6].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[6].offset = 0x400000;
    exec_objects[6].relocation_count = 2;
    exec_objects[6].relocs_ptr = (uintptr_t)&relocs[4];  // relocs 4-5 (desc0→kernel, desc4→surface_heap)
    
    exec_objects[7].handle = surface_heap_bo;
    exec_objects[7].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[7].offset = 0x500000;
    exec_objects[7].relocation_count = 1;
    exec_objects[7].relocs_ptr = (uintptr_t)&relocs[6];  // reloc 6 (ss1→output)
    
    exec_objects[8].handle = batch_bo;
    exec_objects[8].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[8].offset = 0x600000;
    exec_objects[8].relocation_count = 5;
    exec_objects[8].relocs_ptr = (uintptr_t)&relocs[0];  // relocs 0-3 (null_state) + reloc 7 (IDRT_LOAD→idrt)
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 9,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double exec_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    FORENSIC_LOG("✅ EXECBUFFER2 réussi: %.3f ms", exec_time * 1000);
    FORENSIC_LOG("📊 État: 90%%");
    
    /* Attendre GPU idle */
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = 1000000000,
        .flags = 0
    };
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double wait_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    FORENSIC_LOG("✅ GPU idle: %.3f ms", wait_time * 1000);
    
    /* Lire résultat */
    struct drm_i915_gem_mmap_offset mmap_output = {
        .handle = output_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_output) < 0) {
        FORENSIC_LOG("❌ MMAP_OFFSET output failed");
        close(drm_fd);
        return 1;
    }
    
    uint32_t* output = mmap(NULL, 4096, PROT_READ, MAP_SHARED, drm_fd, mmap_output.offset);
    if (output == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap output failed");
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("📊 État: 100%%");
    FORENSIC_LOG("🎯 RÉSULTAT GPU:");
    FORENSIC_LOG("   output[0] = 0x%08X %s", output[0], output[0] == 0xDEADBEEF ? "✅ SUCCÈS!" : "");
    FORENSIC_LOG("   output[1] = 0x%08X %s", output[1], output[1] == 0xCAFEBABE ? "✅" : "");
    FORENSIC_LOG("   output[2] = 0x%08X %s", output[2], output[2] == 0x12345678 ? "✅" : "");
    FORENSIC_LOG("   output[3] = 0x%08X %s", output[3], output[3] == 0xABCDEF00 ? "✅" : "");
    
    if (output[0] == 0xDEADBEEF) {
        FORENSIC_LOG("🎉 PREMIER KERNEL GPU NATIF i915 RÉUSSI!");
    }
    
    munmap(output, 4096);
    close(drm_fd);
    return 0;
}

// Made with Bob
