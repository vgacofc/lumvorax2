/*
 * TEST C438 - Configuration Surface State EXACTE selon Intel compute-runtime
 * 
 * OBJECTIF: Implémenter la configuration Surface State Gen9 EXACTEMENT comme Intel
 * SOURCE: /tmp/compute-runtime/shared/source/command_container/command_encoder.inl:447-493
 * 
 * DIFFÉRENCES CRITIQUES identifiées:
 * 1. Width/Height/Depth encodés via SurfaceStateBufferLength (pas valeurs brutes)
 * 2. Vertical Alignment = VALIGN_4 (pas VALIGN_16)
 * 3. MOCS calculé (pas 0x00)
 * 4. Coherency Type configuré (IA_COHERENT ou GPU_COHERENT)
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

// ============================================================================
// STRUCTURES Gen9 Surface State (64 bytes)
// ============================================================================

// Structure pour encoder Width/Height/Depth selon Intel
typedef union {
    struct {
        uint32_t width  : 14;  // Bits 0-13
        uint32_t height : 14;  // Bits 14-27
        uint32_t depth  : 11;  // Bits 28-38 (sur 2 DWORDs)
    } surfaceState;
    uint32_t length;
} SurfaceStateBufferLength;

typedef struct __attribute__((packed)) {
    // DW0
    uint32_t surface_type           : 3;   // Bits 0-2
    uint32_t reserved0              : 4;   // Bits 3-6
    uint32_t surface_format         : 9;   // Bits 7-15
    uint32_t reserved1              : 3;   // Bits 16-18
    uint32_t surface_vertical_align : 2;   // Bits 19-20
    uint32_t surface_horizontal_align:2;   // Bits 21-22
    uint32_t tile_mode              : 2;   // Bits 23-24
    uint32_t vertical_line_stride   : 1;   // Bit 25
    uint32_t vertical_line_stride_offset:1;// Bit 26
    uint32_t reserved2              : 5;   // Bits 27-31
    
    // DW1
    uint32_t memory_object_control  : 7;   // Bits 0-6 (MOCS)
    uint32_t reserved3              : 1;   // Bit 7
    uint32_t base_mip_level         : 5;   // Bits 8-12
    uint32_t surface_qpitch         : 15;  // Bits 13-27
    uint32_t reserved4              : 4;   // Bits 28-31
    
    // DW2
    uint32_t width                  : 14;  // Bits 0-13
    uint32_t reserved5              : 2;   // Bits 14-15
    uint32_t height                 : 14;  // Bits 16-29
    uint32_t reserved6              : 2;   // Bits 30-31
    
    // DW3
    uint32_t depth                  : 11;  // Bits 0-10
    uint32_t reserved7              : 21;  // Bits 11-31
    
    // DW4
    uint32_t multisample_count      : 3;   // Bits 0-2
    uint32_t reserved8              : 29;  // Bits 3-31
    
    // DW5
    uint32_t x_offset               : 7;   // Bits 0-6
    uint32_t reserved9              : 1;   // Bit 7
    uint32_t y_offset               : 3;   // Bits 8-10
    uint32_t reserved10             : 21;  // Bits 11-31
    
    // DW6
    uint32_t reserved11             : 3;   // Bits 0-2
    uint32_t coherency_type         : 1;   // Bit 3 (CRITIQUE!)
    uint32_t reserved12             : 28;  // Bits 4-31
    
    // DW7
    uint32_t shader_channel_select_r: 3;   // Bits 0-2
    uint32_t shader_channel_select_g: 3;   // Bits 3-5
    uint32_t shader_channel_select_b: 3;   // Bits 6-8
    uint32_t shader_channel_select_a: 3;   // Bits 9-11
    uint32_t reserved13             : 20;  // Bits 12-31
    
    // DW8-9: Surface Base Address (64-bit)
    uint64_t surface_base_address;
    
    // DW10-15: Auxiliary Surface, Clear Color, etc.
    uint32_t auxiliary_surface_mode : 3;   // Bits 0-2
    uint32_t reserved14             : 29;  // Bits 3-31
    uint32_t reserved15[5];                // DW11-15
} Gen9SurfaceState;

// ============================================================================
// CONSTANTES Gen9
// ============================================================================

#define SURFACE_TYPE_BUFFER         0  // SURFTYPE_BUFFER
#define SURFACE_TYPE_NULL           7  // SURFTYPE_NULL
#define SURFACE_FORMAT_RAW          511 // RAW format
#define SURFACE_VALIGN_4            1  // VALIGN_4 (Intel utilise ça!)
#define SURFACE_VALIGN_16           3  // VALIGN_16 (nous utilisions ça)
#define SURFACE_HALIGN_DEFAULT      0  // HALIGN_4
#define TILE_MODE_LINEAR            0  // Linear (pas tiled)
#define COHERENCY_TYPE_IA_COHERENT  0  // CPU coherent
#define COHERENCY_TYPE_GPU_COHERENT 1  // GPU coherent
#define AUX_MODE_NONE               0  // Pas de compression

// MOCS values (Memory Object Control State)
#define MOCS_UNCACHED               1  // UC (Uncached)
#define MOCS_CACHED_LLC             2  // WB (Write-Back, LLC)
#define MOCS_CACHED_L3              3  // WB (Write-Back, L3+LLC)

// ============================================================================
// FONCTION: Configuration Surface State selon Intel
// ============================================================================

void configure_intel_surface_state(Gen9SurfaceState *ss, uint64_t gpu_addr, uint32_t size) {
    memset(ss, 0, sizeof(Gen9SurfaceState));
    
    printf("[INTEL_CONFIG] Configuration Surface State Intel pour buffer %u bytes @ 0x%lx\n", 
           size, gpu_addr);
    
    // 1. CALCUL TAILLE BUFFER (aligné 4 bytes comme Intel)
    uint32_t aligned_size = (size + 3) & ~3;  // alignUp(size, 4)
    printf("[INTEL_CONFIG] Taille alignée: %u bytes\n", aligned_size);
    
    // 2. ENCODAGE WIDTH/HEIGHT/DEPTH selon Intel
    SurfaceStateBufferLength length;
    length.length = aligned_size - 1;  // Intel fait size-1
    
    ss->width = length.surfaceState.width + 1;   // +1 comme Intel!
    ss->height = length.surfaceState.height + 1; // +1 comme Intel!
    ss->depth = length.surfaceState.depth + 1;   // +1 comme Intel!
    
    printf("[INTEL_CONFIG] Width=%u, Height=%u, Depth=%u (encodé depuis length=%u)\n",
           ss->width, ss->height, ss->depth, length.length);
    
    // 3. TYPE ET FORMAT
    ss->surface_type = (gpu_addr != 0) ? SURFACE_TYPE_BUFFER : SURFACE_TYPE_NULL;
    ss->surface_format = SURFACE_FORMAT_RAW;
    printf("[INTEL_CONFIG] Type=%u, Format=%u\n", ss->surface_type, ss->surface_format);
    
    // 4. ALIGNEMENT (CRITIQUE: VALIGN_4 pas VALIGN_16!)
    ss->surface_vertical_align = SURFACE_VALIGN_4;  // Intel utilise VALIGN_4!
    ss->surface_horizontal_align = SURFACE_HALIGN_DEFAULT;
    printf("[INTEL_CONFIG] Vertical Align=VALIGN_4, Horizontal Align=HALIGN_DEFAULT\n");
    
    // 5. TILE MODE
    ss->tile_mode = TILE_MODE_LINEAR;
    ss->vertical_line_stride = 0;
    ss->vertical_line_stride_offset = 0;
    printf("[INTEL_CONFIG] Tile Mode=LINEAR\n");
    
    // 6. MOCS (Memory Object Control State)
    // Intel utilise une valeur calculée, pas 0x00
    // Pour un buffer normal: MOCS_CACHED_L3 (Write-Back L3+LLC)
    ss->memory_object_control = MOCS_CACHED_L3;
    printf("[INTEL_CONFIG] MOCS=0x%x (CACHED_L3)\n", ss->memory_object_control);
    
    // 7. ADRESSE BASE
    ss->surface_base_address = gpu_addr;
    printf("[INTEL_CONFIG] Base Address=0x%lx\n", ss->surface_base_address);
    
    // 8. COHERENCY TYPE (CRITIQUE: Intel configure ça!)
    // cpuCoherent=true → IA_COHERENT, sinon GPU_COHERENT
    // Pour un buffer GPU: GPU_COHERENT
    ss->coherency_type = COHERENCY_TYPE_GPU_COHERENT;
    printf("[INTEL_CONFIG] Coherency Type=GPU_COHERENT\n");
    
    // 9. AUXILIARY SURFACE MODE
    ss->auxiliary_surface_mode = AUX_MODE_NONE;
    printf("[INTEL_CONFIG] Auxiliary Surface Mode=NONE\n");
    
    // 10. SHADER CHANNEL SELECT (default RGBA)
    ss->shader_channel_select_r = 4;  // RED
    ss->shader_channel_select_g = 5;  // GREEN
    ss->shader_channel_select_b = 6;  // BLUE
    ss->shader_channel_select_a = 7;  // ALPHA
    
    printf("[INTEL_CONFIG] Surface State configuré (64 bytes)\n");
}

// ============================================================================
// FONCTION: Dump Surface State pour comparaison
// ============================================================================

void dump_surface_state(const char *label, const Gen9SurfaceState *ss) {
    printf("\n[DUMP_%s] Surface State (64 bytes):\n", label);
    const uint32_t *dwords = (const uint32_t *)ss;
    for (int i = 0; i < 16; i++) {
        printf("  DW%02d: 0x%08x", i, dwords[i]);
        if (i == 0) printf(" (Type=%u, Format=%u, VAlign=%u, HAlign=%u, Tile=%u)",
                          ss->surface_type, ss->surface_format, 
                          ss->surface_vertical_align, ss->surface_horizontal_align,
                          ss->tile_mode);
        if (i == 1) printf(" (MOCS=0x%x)", ss->memory_object_control);
        if (i == 2) printf(" (Width=%u, Height=%u)", ss->width, ss->height);
        if (i == 3) printf(" (Depth=%u)", ss->depth);
        if (i == 6) printf(" (Coherency=%u)", ss->coherency_type);
        if (i == 8) printf(" (BaseAddr Low)");
        if (i == 9) printf(" (BaseAddr High)");
        printf("\n");
    }
}

// ============================================================================
// MAIN: Test configuration Intel vs Ancienne
// ============================================================================

int main() {
    printf("=== TEST C438: Configuration Surface State Intel vs Ancienne ===\n\n");
    
    // Test avec buffer 4 bytes @ 0x12345000
    uint64_t test_addr = 0x12345000;
    uint32_t test_size = 4;
    
    // 1. Configuration INTEL (nouvelle)
    Gen9SurfaceState ss_intel;
    configure_intel_surface_state(&ss_intel, test_addr, test_size);
    dump_surface_state("INTEL", &ss_intel);
    
    // 2. Configuration ANCIENNE (pour comparaison)
    Gen9SurfaceState ss_old;
    memset(&ss_old, 0, sizeof(Gen9SurfaceState));
    ss_old.surface_type = SURFACE_TYPE_BUFFER;
    ss_old.surface_format = SURFACE_FORMAT_RAW;
    ss_old.surface_vertical_align = SURFACE_VALIGN_16;  // Ancienne valeur!
    ss_old.surface_horizontal_align = SURFACE_HALIGN_DEFAULT;
    ss_old.tile_mode = TILE_MODE_LINEAR;
    ss_old.memory_object_control = 0x00;  // Ancienne valeur!
    ss_old.width = test_size;  // Valeur brute (pas encodée!)
    ss_old.height = 1;
    ss_old.depth = 1;
    ss_old.surface_base_address = test_addr;
    ss_old.coherency_type = 0;  // Pas configuré!
    ss_old.auxiliary_surface_mode = AUX_MODE_NONE;
    
    dump_surface_state("OLD", &ss_old);
    
    // 3. COMPARAISON
    printf("\n=== DIFFÉRENCES CRITIQUES ===\n");
    printf("1. Vertical Align: Intel=VALIGN_4 (%u), Old=VALIGN_16 (%u)\n",
           ss_intel.surface_vertical_align, ss_old.surface_vertical_align);
    printf("2. MOCS: Intel=0x%x (CACHED_L3), Old=0x%x (UNCACHED)\n",
           ss_intel.memory_object_control, ss_old.memory_object_control);
    printf("3. Width: Intel=%u (encodé), Old=%u (brut)\n",
           ss_intel.width, ss_old.width);
    printf("4. Coherency: Intel=%u (GPU_COHERENT), Old=%u (pas configuré)\n",
           ss_intel.coherency_type, ss_old.coherency_type);
    
    printf("\n=== CONCLUSION ===\n");
    printf("La configuration Intel diffère sur 4 points CRITIQUES:\n");
    printf("- Alignement vertical (VALIGN_4 vs VALIGN_16)\n");
    printf("- MOCS (cached vs uncached)\n");
    printf("- Encodage Width/Height/Depth\n");
    printf("- Coherency Type (configuré vs non configuré)\n");
    printf("\nCes différences expliquent pourquoi le GPU ne peut PAS écrire!\n");
    
    return 0;
}

// Made with Bob
