/*
 * BEIGNET BATCH BUILDER - Extraction complète construction batch GPGPU
 * Source: Intel Beignet src/intel/intel_gpgpu.c + intel_batchbuffer.c
 * 
 * Ce module contient TOUTES les fonctions nécessaires pour construire
 * un batch GPGPU complet Gen9 avec relocations automatiques.
 */

#ifndef BEIGNET_BATCH_BUILDER_H
#define BEIGNET_BATCH_BUILDER_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <libdrm/intel_bufmgr.h>
#include <libdrm/i915_drm.h>

// ============================================================================
// COMMANDES HARDWARE GEN9
// ============================================================================

#define CMD_STATE_BASE_ADDRESS          (0x61010000)
#define CMD_MEDIA_STATE_POINTERS        (0x70000000)
#define CMD_MEDIA_INTERFACE_LOAD        (0x70020000)
#define CMD_GPGPU_WALKER                (0x71050000)
#define CMD_PIPE_CONTROL                (0x7a000000)
#define MI_BATCH_BUFFER_END             (0x0A000000)

#define BASE_ADDRESS_MODIFY             (1 << 0)

// Macro construction commandes
#define CMD(pipeline, op, sub) ((3 << 29) | \
                                ((pipeline) << 27) | \
                                ((op) << 24) | \
                                ((sub) << 16))

// ============================================================================
// STRUCTURES BATCH BUILDER
// ============================================================================

typedef struct {
    drm_intel_bo *batch_bo;
    drm_intel_bo *aux_buf_bo;
    drm_intel_bo *kernel_bo;
    drm_intel_bo *output_bo;
    
    uint32_t *batch_ptr;
    uint32_t batch_offset;
    uint32_t batch_size;
    
    // Offsets dans aux_buf
    uint32_t surface_heap_offset;
    uint32_t curbe_offset;
    uint32_t idrt_offset;
    uint32_t sampler_offset;
    
    // Configuration kernel
    uint32_t max_threads;
    uint32_t curbe_size;  // en DWORDs
    uint32_t kernel_offset;
    
    // Thread dimensions
    uint32_t thread_width;
    uint32_t thread_height;
    uint32_t thread_depth;
    
} beignet_batch_builder_t;

// ============================================================================
// FONCTIONS BATCH CONSTRUCTION
// ============================================================================

/**
 * Initialise le batch builder
 */
static inline void
beignet_batch_init(beignet_batch_builder_t *builder,
                   drm_intel_bo *batch_bo,
                   drm_intel_bo *aux_buf_bo,
                   drm_intel_bo *kernel_bo,
                   drm_intel_bo *output_bo)
{
    memset(builder, 0, sizeof(*builder));
    
    builder->batch_bo = batch_bo;
    builder->aux_buf_bo = aux_buf_bo;
    builder->kernel_bo = kernel_bo;
    builder->output_bo = output_bo;
    
    // Map batch buffer
    drm_intel_bo_map(batch_bo, 1);
    builder->batch_ptr = (uint32_t*)batch_bo->virtual;
    builder->batch_offset = 0;
    builder->batch_size = batch_bo->size / 4;  // en DWORDs
    
    // Configuration par défaut
    builder->surface_heap_offset = 0;
    builder->curbe_offset = 4096;
    builder->idrt_offset = 8192;
    builder->sampler_offset = 12288;
    
    builder->max_threads = 64;
    builder->curbe_size = 0;  // Pas de CURBE pour test simple
    builder->kernel_offset = 0;
    
    builder->thread_width = 1;
    builder->thread_height = 1;
    builder->thread_depth = 1;
}

/**
 * Émet un DWORD dans le batch
 */
static inline void
beignet_batch_emit(beignet_batch_builder_t *builder, uint32_t dword)
{
    assert(builder->batch_offset < builder->batch_size);
    builder->batch_ptr[builder->batch_offset++] = dword;
}

/**
 * Émet une relocation dans le batch
 */
static inline void
beignet_batch_emit_reloc(beignet_batch_builder_t *builder,
                         drm_intel_bo *bo,
                         uint32_t read_domains,
                         uint32_t write_domains,
                         uint32_t delta)
{
    uint32_t offset = builder->batch_offset * 4;
    
    drm_intel_bo_emit_reloc(builder->batch_bo,
                            offset,
                            bo,
                            delta,
                            read_domains,
                            write_domains);
    
    // Émet l'adresse temporaire (sera relocalisée)
    beignet_batch_emit(builder, bo->offset + delta);
}

/**
 * PIPE_CONTROL - Synchronisation GPU
 */
static inline void
beignet_batch_pipe_control(beignet_batch_builder_t *builder)
{
    beignet_batch_emit(builder, CMD_PIPE_CONTROL | (6 - 2));
    beignet_batch_emit(builder, (1 << 20) |  // CS Stall
                                (1 << 18) |  // TLB Invalidate
                                (1 << 6));   // Render Target Cache Flush
    beignet_batch_emit(builder, 0);  // Address low
    beignet_batch_emit(builder, 0);  // Address high
    beignet_batch_emit(builder, 0);  // Immediate data low
    beignet_batch_emit(builder, 0);  // Immediate data high
}

/**
 * PIPELINE_SELECT - Sélection pipeline GPGPU
 */
static inline void
beignet_batch_pipeline_select(beignet_batch_builder_t *builder)
{
    beignet_batch_emit(builder, 0x69040000 | (1 << 0));  // GPGPU pipeline
}

/**
 * STATE_BASE_ADDRESS Gen9 - COMPLET avec relocations
 */
static inline void
beignet_batch_state_base_address_gen9(beignet_batch_builder_t *builder)
{
    const uint32_t def_cc = 0x3;  // Cache control (L3 cacheable)
    
    // CMD_STATE_BASE_ADDRESS | 17 DWORDs
    beignet_batch_emit(builder, CMD_STATE_BASE_ADDRESS | 17);
    
    // General State Base Address (DW1-3)
    beignet_batch_emit(builder, 0 | (def_cc << 4) | BASE_ADDRESS_MODIFY);
    beignet_batch_emit(builder, 0);  // High 32 bits
    beignet_batch_emit(builder, 0 | (def_cc << 16));  // MOCS
    
    // Surface State Base Address (DW4-5) - RELOCATION vers aux_buf
    beignet_batch_emit_reloc(builder,
                             builder->aux_buf_bo,
                             I915_GEM_DOMAIN_SAMPLER,
                             I915_GEM_DOMAIN_SAMPLER,
                             builder->surface_heap_offset + 
                             (0 | (def_cc << 4) | BASE_ADDRESS_MODIFY));
    beignet_batch_emit(builder, 0);  // High 32 bits
    
    // Dynamic State Base Address (DW6-7) - RELOCATION vers aux_buf
    beignet_batch_emit_reloc(builder,
                             builder->aux_buf_bo,
                             I915_GEM_DOMAIN_RENDER,
                             I915_GEM_DOMAIN_RENDER,
                             0 | (def_cc << 4) | BASE_ADDRESS_MODIFY);
    beignet_batch_emit(builder, 0);  // High 32 bits
    
    // Indirect Object Base Address (DW8-9)
    beignet_batch_emit(builder, 0 | (def_cc << 4) | BASE_ADDRESS_MODIFY);
    beignet_batch_emit(builder, 0);
    
    // Instruction Base Address (DW10-11) - RELOCATION vers kernel
    beignet_batch_emit_reloc(builder,
                             builder->kernel_bo,
                             I915_GEM_DOMAIN_INSTRUCTION,
                             I915_GEM_DOMAIN_INSTRUCTION,
                             0 + (0 | (def_cc << 4) | BASE_ADDRESS_MODIFY));
    beignet_batch_emit(builder, 0);  // High 32 bits
    
    // Upper bounds (DW12-15)
    beignet_batch_emit(builder, 0xfffff000 | BASE_ADDRESS_MODIFY);
    beignet_batch_emit(builder, 0xfffff000 | BASE_ADDRESS_MODIFY);
    beignet_batch_emit(builder, 0xfffff000 | BASE_ADDRESS_MODIFY);
    beignet_batch_emit(builder, 0xfffff000 | BASE_ADDRESS_MODIFY);
    
    // Bindless Surface State Base Address (DW16-18)
    beignet_batch_emit(builder, (def_cc << 4) | BASE_ADDRESS_MODIFY);
    beignet_batch_emit(builder, 0);
    beignet_batch_emit(builder, 0xfffff000);
}

/**
 * MEDIA_VFE_STATE Gen8/9 - Configuration threads
 */
static inline void
beignet_batch_media_vfe_state_gen9(beignet_batch_builder_t *builder)
{
    // CMD_MEDIA_STATE_POINTERS | 9 DWORDs
    beignet_batch_emit(builder, CMD_MEDIA_STATE_POINTERS | (9 - 2));
    
    // Scratch Space Base Pointer (DW1-2) - Pas de scratch pour test simple
    beignet_batch_emit(builder, 0);
    beignet_batch_emit(builder, 0);
    
    // Max threads | URB entries | GPGPU mode (DW3)
    beignet_batch_emit(builder, 0 | 
                                ((builder->max_threads - 1) << 16) | 
                                (2 << 8) |   // URB entries (minimum 2)
                                0xc0);       // GPGPU mode + bypass gateway
    
    // Debug Object ID (DW4)
    beignet_batch_emit(builder, 0);
    
    // URB Entry Size | CURBE Size (DW5)
    beignet_batch_emit(builder, (2 << 16) | builder->curbe_size);
    
    // Scoreboard (DW6-8)
    beignet_batch_emit(builder, 0);
    beignet_batch_emit(builder, 0);
    beignet_batch_emit(builder, 0);
}

/**
 * MEDIA_CURBE_LOAD - Chargement CURBE (optionnel)
 */
static inline void
beignet_batch_media_curbe_load(beignet_batch_builder_t *builder)
{
    if (builder->curbe_size == 0) return;
    
    beignet_batch_emit(builder, CMD(2, 0, 1) | (4 - 2));
    beignet_batch_emit(builder, 0);  // MBZ
    beignet_batch_emit(builder, builder->curbe_size * 32);  // Length in bytes
    
    // RELOCATION vers CURBE dans aux_buf
    beignet_batch_emit_reloc(builder,
                             builder->aux_buf_bo,
                             I915_GEM_DOMAIN_INSTRUCTION,
                             0,
                             builder->curbe_offset);
}

/**
 * MEDIA_INTERFACE_DESCRIPTOR_LOAD - Chargement IDRT
 */
static inline void
beignet_batch_media_idrt_load(beignet_batch_builder_t *builder)
{
    beignet_batch_emit(builder, CMD(2, 0, 2) | (4 - 2));
    beignet_batch_emit(builder, 0);  // MBZ
    beignet_batch_emit(builder, 1 << 5);  // Interface Descriptor Total Length
    
    // Offset IDRT dans aux_buf (Gen8/9 utilise offset, pas relocation)
    beignet_batch_emit(builder, builder->idrt_offset);
}

/**
 * GPGPU_WALKER Gen8/9 - Lancement kernel
 * CORRECTION C488: DW1 = kernel index (0), DW4 = SIMD size + thread count
 * Source: Intel Beignet intel_gpgpu.c lignes 2283-2303
 */
static inline void
beignet_batch_gpgpu_walker_gen9(beignet_batch_builder_t *builder)
{
    // Calcul right_mask pour SIMD8
    uint32_t simd_sz = 8;  // SIMD8 pour kernel simple
    uint32_t thread_n = 64;  // Max threads
    size_t group_sz = builder->thread_width * builder->thread_height * builder->thread_depth;
    uint32_t shift = (group_sz & (simd_sz - 1));
    shift = (shift == 0) ? simd_sz : shift;
    uint32_t right_mask = (1 << shift) - 1;
    
    beignet_batch_emit(builder, CMD_GPGPU_WALKER | 13);  // 15 DWORDs total
    
    // DW1: Interface Descriptor Offset (kernel index, toujours 0)
    // CORRECTION C488 CRITIQUE: Ce n'est PAS l'offset IDRT mais l'index du kernel!
    beignet_batch_emit(builder, 0);
    
    // DW2: Indirect Data Length
    beignet_batch_emit(builder, 0);
    
    // DW3: Indirect Data Start Address
    beignet_batch_emit(builder, 0);
    
    // DW4: SIMD Size | Thread Width Counter Max
    // CORRECTION C488 CRITIQUE: Bit 30 = SIMD size (0=SIMD8, 1=SIMD16)
    // Bits 0-5 = thread count - 1
    beignet_batch_emit(builder, (0 << 30) | (thread_n - 1));  // SIMD8 | 63 threads
    
    // DW5: Thread Group ID Starting X Dimension
    beignet_batch_emit(builder, 0);
    
    // DW6: Reserved
    beignet_batch_emit(builder, 0);
    
    // DW7: Thread Group ID X Dimension
    beignet_batch_emit(builder, builder->thread_width);
    
    // DW8: Thread Group ID Starting Y Dimension
    beignet_batch_emit(builder, 0);
    
    // DW9: Reserved
    beignet_batch_emit(builder, 0);
    
    // DW10: Thread Group ID Y Dimension
    beignet_batch_emit(builder, builder->thread_height);
    
    // DW11: Thread Group ID Starting Z Dimension
    beignet_batch_emit(builder, 0);
    
    // DW12: Thread Group ID Z Dimension
    beignet_batch_emit(builder, builder->thread_depth);
    
    // DW13: Right Execution Mask
    beignet_batch_emit(builder, right_mask);
    
    // DW14: Bottom Execution Mask
    beignet_batch_emit(builder, 0xffffffff);
}

/**
 * MEDIA_STATE_FLUSH - Synchronisation GPU après GPGPU_WALKER
 * CRITIQUE: Cette commande est OBLIGATOIRE pour que le kernel s'exécute!
 * Source: Intel Beignet intel_gpgpu.c ligne 2305
 */
static inline void
beignet_batch_media_state_flush(beignet_batch_builder_t *builder)
{
    beignet_batch_emit(builder, CMD(2, 0, 4) | 0);  // MEDIA_STATE_FLUSH
    beignet_batch_emit(builder, 0);  // kernel index == 0
}

/**
 * Finalise le batch
 */
static inline void
beignet_batch_finalize(beignet_batch_builder_t *builder)
{
    // MI_BATCH_BUFFER_END
    beignet_batch_emit(builder, MI_BATCH_BUFFER_END);
    
    // Padding si nécessaire (alignement 8 bytes)
    if (builder->batch_offset & 1) {
        beignet_batch_emit(builder, 0);
    }
    
    // Unmap batch
    drm_intel_bo_unmap(builder->batch_bo);
}

/**
 * Construction batch GPGPU COMPLET Gen9
 */
static inline uint32_t
beignet_build_gpgpu_batch_gen9(beignet_batch_builder_t *builder)
{
    // 1. PIPE_CONTROL initial
    beignet_batch_pipe_control(builder);
    
    // 2. PIPELINE_SELECT GPGPU
    beignet_batch_pipeline_select(builder);
    
    // 3. STATE_BASE_ADDRESS avec relocations
    beignet_batch_state_base_address_gen9(builder);
    
    // 4. MEDIA_VFE_STATE
    beignet_batch_media_vfe_state_gen9(builder);
    
    // 5. MEDIA_CURBE_LOAD (optionnel)
    beignet_batch_media_curbe_load(builder);
    
    // 6. MEDIA_INTERFACE_DESCRIPTOR_LOAD
    beignet_batch_media_idrt_load(builder);
    
    // 7. GPGPU_WALKER
    beignet_batch_gpgpu_walker_gen9(builder);
    
    // 8. MEDIA_STATE_FLUSH (CORRECTION C487 - CRITIQUE!)
    beignet_batch_emit(builder, CMD(2, 0, 4) | 0);  // MEDIA_STATE_FLUSH
    beignet_batch_emit(builder, 0);  // kernel index == 0
    
    // 9. PIPE_CONTROL final
    beignet_batch_pipe_control(builder);
    
    // 10. MI_BATCH_BUFFER_END
    beignet_batch_finalize(builder);
    
    return builder->batch_offset * 4;  // Retourne taille en bytes
}

#endif // BEIGNET_BATCH_BUILDER_H

// Made with Bob
