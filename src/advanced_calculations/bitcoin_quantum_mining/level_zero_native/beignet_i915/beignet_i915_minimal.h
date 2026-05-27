/*
 * BEIGNET I915 MINIMAL - Extraction modules Intel Beignet
 * 
 * Code source: Intel Beignet (2012-2019)
 * License: LGPL 2.1
 * 
 * Modules extraits SANS dépendances LLVM/OpenCL:
 * - Structures Gen7/Gen9
 * - Fonction setup_bti avec dri_bo_emit_reloc()
 * 
 * Dépendances: libdrm, libdrm_intel uniquement
 */

#ifndef __BEIGNET_I915_MINIMAL_H__
#define __BEIGNET_I915_MINIMAL_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "intel_structs_minimal.h"
#include <libdrm/intel_bufmgr.h>

// ============================================================================
// STRUCTURES GEN7 (Extrait intel_structs.h)
// ============================================================================

/* Gen7 Surface State (8 DWORDs = 32 bytes) */
typedef struct gen7_surface_state {
  struct {
    uint32_t cube_pos_z:1;
    uint32_t cube_neg_z:1;
    uint32_t cube_pos_y:1;
    uint32_t cube_neg_y:1;
    uint32_t cube_pos_x:1;
    uint32_t cube_neg_x:1;
    uint32_t pad2:2;
    uint32_t render_cache_read_mode:1;
    uint32_t pad1:1;
    uint32_t surface_array_spacing:1;
    uint32_t vert_line_stride_ofs:1;
    uint32_t vert_line_stride:1;
    uint32_t tile_walk:1;
    uint32_t tiled_surface:1;
    uint32_t horizontal_alignment:1;
    uint32_t vertical_alignment:2;
    uint32_t surface_format:9;     /**< BRW_SURFACEFORMAT_x */
    uint32_t pad0:1;
    uint32_t is_array:1;
    uint32_t surface_type:3;       /**< BRW_SURFACE_1D/2D/3D/CUBE */
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
    uint32_t pad1:12;
    uint32_t y_offset:4;
    uint32_t pad0:1;
    uint32_t x_offset:7;
  } ss5;

  struct {
    uint32_t pad; /* Multisample Control Surface stuff */
  } ss6;

  struct {
    uint32_t resource_min_lod:12;
    uint32_t pad0:4;
    uint32_t shader_chanel_select_a:3;
    uint32_t shader_chanel_select_b:3;
    uint32_t shader_chanel_select_g:3;
    uint32_t shader_chanel_select_r:3;
    uint32_t alpha_clear_color:1;
    uint32_t blue_clear_color:1;
    uint32_t green_clear_color:1;
    uint32_t red_clear_color:1;
  } ss7;
} gen7_surface_state_t;

/* Surface Heap: Binding Table + Surface States */
typedef struct surface_heap {
  uint32_t binding_table[256];           // 1024 bytes
  char surface[256*sizeof(gen7_surface_state_t)];  // 8192 bytes
} surface_heap_t;  // Total: 9216 bytes

// ============================================================================
// DEFINES (Extrait intel_defines.h)
// ============================================================================

#define I965_SURFACE_BUFFER     4
#define I965_SURFACEFORMAT_RAW  0x1FF
#define I965_SURFACEFORMAT_R32G32B32A32_UINT  0x0C0

// ============================================================================
// FONCTION SETUP BTI GEN7 (Extrait intel_gpgpu.c lignes 1039-1067)
// ============================================================================

/**
 * Configure Binding Table Entry + Surface State Gen7
 * 
 * Cette fonction utilise dri_bo_emit_reloc() de libdrm_intel pour
 * enregistrer automatiquement les relocations, résolvant le problème
 * EXECBUFFER2 Invalid argument de C479.
 * 
 * @param aux_buf_bo Buffer contenant surface_heap
 * @param surface_heap_offset Offset du surface_heap dans aux_buf_bo
 * @param buf Buffer à lier (ex: output buffer)
 * @param internal_offset Offset dans buf
 * @param size Taille du buffer
 * @param index Index dans binding table (0-255)
 * @param format Format surface (I965_SURFACEFORMAT_RAW, etc.)
 */
static inline void
beignet_i915_setup_bti_gen7(drm_intel_bo *aux_buf_bo,
                             uint32_t surface_heap_offset,
                             drm_intel_bo *buf,
                             uint32_t internal_offset,
                             size_t size,
                             unsigned char index,
                             uint32_t format)
{
  assert(size <= (2ul<<30));
  size_t s = size - 1;
  
  // Map aux_buf pour accéder au surface_heap
  drm_intel_bo_map(aux_buf_bo, 1);
  surface_heap_t *heap = (surface_heap_t*)((char*)aux_buf_bo->virtual + surface_heap_offset);
  
  // Pointeur vers surface state
  gen7_surface_state_t *ss0 = (gen7_surface_state_t*)
      &heap->surface[index * sizeof(gen7_surface_state_t)];
  
  // Configuration Surface State Gen7
  memset(ss0, 0, sizeof(gen7_surface_state_t));
  ss0->ss0.surface_type = I965_SURFACE_BUFFER;
  ss0->ss0.surface_format = format;
  ss0->ss2.width  = s & 0x7f;           /* bits 6:0 of sz */
  ss0->ss2.height = (s >> 7) & 0x3fff;  /* bits 20:7 of sz */
  ss0->ss3.depth  = (s >> 21) & 0x3ff;  /* bits 30:21 of sz */
  ss0->ss5.mip_count = 0;  // Cache control simplifié
  
  // Configuration Binding Table
  heap->binding_table[index] = offsetof(surface_heap_t, surface) + 
                               index * sizeof(gen7_surface_state_t);
  
  // Adresse temporaire (sera relocalisée)
  ss0->ss1.base_addr = buf->offset + internal_offset;
  
  drm_intel_bo_unmap(aux_buf_bo);
  
  // RELOCATION AUTOMATIQUE via libdrm_intel
  // C'est LA solution au problème EXECBUFFER2 de C479!
  uint32_t reloc_offset = surface_heap_offset +
                          offsetof(surface_heap_t, surface) +
                          index * sizeof(gen7_surface_state_t) +
                          offsetof(gen7_surface_state_t, ss1);
  
  drm_intel_bo_emit_reloc(aux_buf_bo,
                          reloc_offset,
                          buf,
                          internal_offset,
                          I915_GEM_DOMAIN_RENDER,
                          I915_GEM_DOMAIN_RENDER);
}

#endif /* __BEIGNET_I915_MINIMAL_H__ */

// Made with Bob
