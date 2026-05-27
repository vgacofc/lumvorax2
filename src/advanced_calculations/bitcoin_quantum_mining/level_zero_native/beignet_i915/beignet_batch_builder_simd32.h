/*
 * BEIGNET BATCH BUILDER SIMD32 - Version automatique C498
 * 
 * DIFFÉRENCE CRITIQUE avec beignet_batch_builder.h:
 * - SIMD SIZE: DYNAMIQUE (8/16/32) au lieu de FIXE (8)
 * - THREAD COUNT: DYNAMIQUE depuis metadata au lieu de FIXE (64)
 * - THREAD WIDTH: CALCULÉ (work_group_size / simd_size)
 * 
 * Cette version implémente le système batch buffer AUTOMATIQUE C498
 */

#ifndef BEIGNET_BATCH_BUILDER_SIMD32_H
#define BEIGNET_BATCH_BUILDER_SIMD32_H

#include "beignet_batch_builder.h"

/**
 * GPGPU_WALKER Gen8/9 - Version SIMD32 DYNAMIQUE
 * 
 * CORRECTIONS C499:
 * - SIMD size: DYNAMIQUE (32 pour ce kernel)
 * - Thread count: DYNAMIQUE (256 pour ce kernel)
 * - Thread width: CALCULÉ (256/32 = 8)
 */
static inline void
beignet_batch_gpgpu_walker_gen9_simd32(beignet_batch_builder_t *builder,
                                        uint32_t simd_size,
                                        uint32_t thread_count)
{
    // Calcul right_mask pour SIMD dynamique
    size_t group_sz = builder->thread_width * builder->thread_height * builder->thread_depth;
    uint32_t shift = (group_sz & (simd_size - 1));
    shift = (shift == 0) ? simd_size : shift;
    uint32_t right_mask = (1 << shift) - 1;
    
    // Calcul SIMD bits pour DW4
    uint32_t simd_bits;
    switch (simd_size) {
        case 8:  simd_bits = 0; break;  // SIMD8
        case 16: simd_bits = 1; break;  // SIMD16
        case 32: simd_bits = 2; break;  // SIMD32 ✅
        default: simd_bits = 0; break;
    }
    
    beignet_batch_emit(builder, CMD_GPGPU_WALKER | 13);  // 15 DWORDs total
    
    // DW1: Interface Descriptor Offset (kernel index, toujours 0)
    beignet_batch_emit(builder, 0);
    
    // DW2: Indirect Data Length
    beignet_batch_emit(builder, 0);
    
    // DW3: Indirect Data Start Address
    beignet_batch_emit(builder, 0);
    
    // DW4: SIMD Size | Thread Width Counter Max
    // ✅ C499 FIX CRITIQUE: SIMD32 (bits 30-31 = 2) + thread_count dynamique
    beignet_batch_emit(builder, (simd_bits << 30) | (thread_count - 1));
    
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
    beignet_batch_emit(builder, 0xFFFFFFFF);
}

/**
 * Construction batch GPGPU COMPLET Gen9 avec SIMD32
 * 
 * Version C499: Utilise GPGPU_WALKER SIMD32 au lieu de SIMD8
 */
static inline uint32_t
beignet_build_gpgpu_batch_gen9_simd32(beignet_batch_builder_t *builder,
                                       uint32_t simd_size,
                                       uint32_t thread_count)
{
    // 1. PIPE_CONTROL initial
    beignet_batch_pipe_control(builder);
    
    // 2. PIPELINE_SELECT GPGPU
    beignet_batch_pipeline_select(builder);
    
    // 3. STATE_BASE_ADDRESS (avec relocations)
    beignet_batch_state_base_address_gen9(builder);
    
    // 4. MEDIA_VFE_STATE
    beignet_batch_media_vfe_state_gen9(builder);
    
    // 5. MEDIA_CURBE_LOAD (optionnel)
    beignet_batch_media_curbe_load(builder);
    
    // 6. MEDIA_INTERFACE_DESCRIPTOR_LOAD
    beignet_batch_media_idrt_load(builder);
    
    // 7. GPGPU_WALKER avec SIMD32 ✅
    beignet_batch_gpgpu_walker_gen9_simd32(builder, simd_size, thread_count);
    
    // 8. MEDIA_STATE_FLUSH
    beignet_batch_media_state_flush(builder);
    
    // 9. PIPE_CONTROL final
    beignet_batch_pipe_control(builder);
    
    // 10. MI_BATCH_BUFFER_END
    beignet_batch_emit(builder, MI_BATCH_BUFFER_END);
    
    return builder->batch_offset * 4;  // Retourne taille en bytes
}

#endif // BEIGNET_BATCH_BUILDER_SIMD32_H

// Made with Bob
