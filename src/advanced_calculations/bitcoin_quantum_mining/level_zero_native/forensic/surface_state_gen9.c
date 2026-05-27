/**
 * LUMVORAX SURFACE STATE ENGINE - Gen9 Implementation
 * 
 * Implémentation complète décodage/génération Surface States Gen9
 * 
 * CYCLE: C577
 * PRIORITÉ: ABSOLUE
 */

#define _GNU_SOURCE
#include "surface_state_gen9.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// UTILITAIRES BIT MANIPULATION
// ============================================================================

static inline uint32_t extract_bits(uint32_t value, uint32_t start, uint32_t end) {
    uint32_t mask = ((1U << (end - start + 1)) - 1);
    return (value >> start) & mask;
}

static inline uint32_t set_bits(uint32_t value, uint32_t start, uint32_t end, uint32_t data) {
    uint32_t mask = ((1U << (end - start + 1)) - 1);
    value &= ~(mask << start);
    value |= (data & mask) << start;
    return value;
}

static inline uint64_t extract_bits_64(uint64_t value, uint32_t start, uint32_t end) {
    uint64_t mask = ((1ULL << (end - start + 1)) - 1);
    return (value >> start) & mask;
}

// ============================================================================
// CONVERSION STRING
// ============================================================================

const char* surface_format_to_string(surface_format_t format) {
    switch (format) {
        case SURFACEFORMAT_R32_UINT: return "R32_UINT";
        case SURFACEFORMAT_R32_SINT: return "R32_SINT";
        case SURFACEFORMAT_R32_FLOAT: return "R32_FLOAT";
        case SURFACEFORMAT_R32G32_UINT: return "R32G32_UINT";
        case SURFACEFORMAT_R32G32_SINT: return "R32G32_SINT";
        case SURFACEFORMAT_R32G32_FLOAT: return "R32G32_FLOAT";
        case SURFACEFORMAT_R32G32B32A32_UINT: return "R32G32B32A32_UINT";
        case SURFACEFORMAT_R32G32B32A32_SINT: return "R32G32B32A32_SINT";
        case SURFACEFORMAT_R32G32B32A32_FLOAT: return "R32G32B32A32_FLOAT";
        case SURFACEFORMAT_RAW: return "RAW";
        default: return "UNKNOWN";
    }
}

const char* surface_type_to_string(surface_type_t type) {
    switch (type) {
        case SURFTYPE_1D: return "1D";
        case SURFTYPE_2D: return "2D";
        case SURFTYPE_3D: return "3D";
        case SURFTYPE_CUBE: return "CUBE";
        case SURFTYPE_BUFFER: return "BUFFER";
        case SURFTYPE_STRBUF: return "STRBUF";
        case SURFTYPE_NULL: return "NULL";
        default: return "UNKNOWN";
    }
}

const char* tile_mode_to_string(tile_mode_t mode) {
    switch (mode) {
        case TILE_LINEAR: return "LINEAR";
        case TILE_WMAJOR: return "W-MAJOR";
        case TILE_XMAJOR: return "X-MAJOR";
        case TILE_YMAJOR: return "Y-MAJOR";
        default: return "UNKNOWN";
    }
}

const char* mocs_to_string(mocs_t mocs) {
    switch (mocs) {
        case MOCS_UNCACHED: return "UNCACHED";
        case MOCS_CACHED_LLC: return "CACHED_LLC";
        case MOCS_CACHED_L3: return "CACHED_L3";
        case MOCS_CACHED_LLC_L3: return "CACHED_LLC_L3";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// DÉCODAGE SURFACE STATE
// ============================================================================

bool decode_surface_state_gen9(const uint32_t *data, surface_state_t *state) {
    if (!data || !state) {
        return false;
    }
    
    memset(state, 0, sizeof(surface_state_t));
    memcpy(state->raw_data, data, SURFACE_STATE_SIZE_BYTES);
    
    // DWord 0
    state->surface_type = (surface_type_t)extract_bits(data[0], 29, 31);
    state->surface_format = (surface_format_t)extract_bits(data[0], 18, 27);
    state->tile_mode = (tile_mode_t)extract_bits(data[0], 12, 13);
    state->is_array = extract_bits(data[0], 28, 28);
    state->surface_vertical_alignment = extract_bits(data[0], 16, 17);
    state->surface_horizontal_alignment = extract_bits(data[0], 14, 15);
    
    // DWord 1 + DWord 8 (bits 32-47 de l'adresse)
    uint64_t addr_low = data[1] & 0xFFFFF000ULL;  // Bits 12-31
    uint64_t addr_high = ((uint64_t)extract_bits(data[8], 0, 15)) << 32;  // Bits 32-47
    state->surface_base_address = addr_low | addr_high;
    state->mocs = (mocs_t)extract_bits(data[1], 24, 30);
    
    // DWord 2
    state->width = extract_bits(data[2], 0, 13);
    state->height = extract_bits(data[2], 16, 29);
    
    // DWord 3
    state->depth = extract_bits(data[3], 21, 31);
    state->surface_pitch = extract_bits(data[3], 0, 17);
    
    // DWord 4
    state->render_target_view_extent = extract_bits(data[4], 21, 31);
    state->minimum_array_element = extract_bits(data[4], 0, 10);
    
    // DWord 5
    state->mip_count = extract_bits(data[5], 20, 23);
    state->surface_min_lod = extract_bits(data[5], 4, 7);
    state->mip_tail_start_lod = extract_bits(data[5], 8, 11);
    
    // DWord 6
    state->y_offset = extract_bits(data[6], 20, 22);
    state->x_offset = extract_bits(data[6], 25, 31);
    
    // DWord 7
    state->resource_min_lod = extract_bits(data[7], 0, 11);
    state->shader_channel_select_r = extract_bits(data[7], 16, 18);
    state->shader_channel_select_g = extract_bits(data[7], 19, 21);
    state->shader_channel_select_b = extract_bits(data[7], 22, 24);
    state->shader_channel_select_a = extract_bits(data[7], 25, 27);
    
    // DWords 8-9: Aux Surface
    uint64_t aux_addr_low = data[9] & 0xFFFFF000ULL;
    uint64_t aux_addr_high = ((uint64_t)extract_bits(data[8], 16, 31)) << 32;
    state->aux_surface_base_address = aux_addr_low | aux_addr_high;
    state->aux_surface_pitch = extract_bits(data[9], 3, 11);
    state->aux_surface_qpitch = extract_bits(data[9], 16, 30);
    
    // DWords 10-11: Clear Color
    state->clear_color[0] = data[10];
    state->clear_color[1] = data[11];
    state->clear_color[2] = data[12];
    state->clear_color[3] = data[13];
    
    // DWords 12-15: Reserved
    state->reserved[0] = data[12];
    state->reserved[1] = data[13];
    state->reserved[2] = data[14];
    state->reserved[3] = data[15];
    
    state->is_valid = true;
    return true;
}

// ============================================================================
// GÉNÉRATION SURFACE STATE
// ============================================================================

bool generate_surface_state_gen9(const surface_config_t *config, uint32_t *data) {
    if (!config || !data) {
        return false;
    }
    
    memset(data, 0, SURFACE_STATE_SIZE_BYTES);
    
    // DWord 0
    data[0] = set_bits(data[0], 29, 31, config->type);
    data[0] = set_bits(data[0], 18, 27, config->format);
    data[0] = set_bits(data[0], 12, 13, config->tiling);
    
    // Alignement par défaut (4 pour horizontal, 4 pour vertical)
    data[0] = set_bits(data[0], 14, 15, 1);  // HALIGN_4
    data[0] = set_bits(data[0], 16, 17, 1);  // VALIGN_4
    
    // DWord 1: Adresse base (bits 12-31) + MOCS
    data[1] = (uint32_t)(config->base_address & 0xFFFFF000ULL);
    data[1] = set_bits(data[1], 24, 30, config->mocs);
    
    // DWord 2: Width + Height
    data[2] = set_bits(data[2], 0, 13, config->width - 1);
    data[2] = set_bits(data[2], 16, 29, config->height - 1);
    
    // DWord 3: Depth + Pitch
    data[3] = set_bits(data[3], 21, 31, config->depth - 1);
    data[3] = set_bits(data[3], 0, 17, config->pitch - 1);
    
    // DWord 4: Render Target View Extent
    if (config->is_render_target) {
        data[4] = set_bits(data[4], 21, 31, config->depth - 1);
    }
    
    // DWord 5: Mip Count = 0 (pas de mipmaps)
    data[5] = 0;
    
    // DWord 6: Offsets = 0
    data[6] = 0;
    
    // DWord 7: Shader Channel Select (RGBA = 0,1,2,3)
    data[7] = set_bits(data[7], 16, 18, 4);  // Red = SCS_RED
    data[7] = set_bits(data[7], 19, 21, 5);  // Green = SCS_GREEN
    data[7] = set_bits(data[7], 22, 24, 6);  // Blue = SCS_BLUE
    data[7] = set_bits(data[7], 25, 27, 7);  // Alpha = SCS_ALPHA
    
    // DWord 8: Adresse base bits 32-47
    data[8] = (uint32_t)((config->base_address >> 32) & 0xFFFF);
    
    // DWords 9-15: Zéro (pas de compression, pas de clear color)
    for (int i = 9; i < 16; i++) {
        data[i] = 0;
    }
    
    return true;
}

// ============================================================================
// VALIDATION SURFACE STATE
// ============================================================================

bool validate_surface_state_gen9(const surface_state_t *state) {
    if (!state || !state->is_valid) {
        return false;
    }
    
    // Vérifier type surface valide
    if (state->surface_type > SURFTYPE_NULL) {
        snprintf((char*)state->error_msg, sizeof(state->error_msg),
                 "Invalid surface type: %u", state->surface_type);
        return false;
    }
    
    // Vérifier format surface valide
    if (state->surface_format > 0x1FF) {
        snprintf((char*)state->error_msg, sizeof(state->error_msg),
                 "Invalid surface format: 0x%03X", state->surface_format);
        return false;
    }
    
    // Vérifier tiling mode valide
    if (state->tile_mode > TILE_YMAJOR) {
        snprintf((char*)state->error_msg, sizeof(state->error_msg),
                 "Invalid tile mode: %u", state->tile_mode);
        return false;
    }
    
    // Vérifier dimensions non nulles (sauf pour SURFTYPE_NULL)
    // ROOT CAUSE #94: Validation dimensions trop stricte pour SURFTYPE_BUFFER
    // Pour BUFFER: height peut être 0 (1D buffer), seul width doit être non-nul
    if (state->surface_type != SURFTYPE_NULL) {
        if (state->width == 0) {
            snprintf((char*)state->error_msg, sizeof(state->error_msg),
                     "Invalid width: %u", state->width + 1);
            return false;
        }
        // Pour surfaces 2D/3D, vérifier aussi height
        if (state->surface_type == SURFTYPE_2D || state->surface_type == SURFTYPE_3D) {
            if (state->height == 0) {
                snprintf((char*)state->error_msg, sizeof(state->error_msg),
                         "Invalid height for 2D/3D surface: %u", state->height + 1);
                return false;
            }
        }
    }
    
    // Vérifier pitch aligné sur 64 bytes pour tiling
    if (state->tile_mode != TILE_LINEAR) {
        if ((state->surface_pitch + 1) % 64 != 0) {
            snprintf((char*)state->error_msg, sizeof(state->error_msg),
                     "Pitch not aligned to 64 bytes: %u", state->surface_pitch + 1);
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// AFFICHAGE WIRESHARK
// ============================================================================

void print_surface_state_wireshark(const surface_state_t *state, FILE *fp) {
    if (!state || !fp) {
        return;
    }
    
    fprintf(fp, "╔════════════════════════════════════════════════════════════════╗\n");
    fprintf(fp, "║           RENDER_SURFACE_STATE (16 DWords = 64 bytes)         ║\n");
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 0
    fprintf(fp, "║ DWord 0: 0x%08X                                           ║\n", state->raw_data[0]);
    fprintf(fp, "║   Surface Type       : %s (bits 29-31)                    ║\n", 
            surface_type_to_string(state->surface_type));
    fprintf(fp, "║   Surface Format     : %s (bits 18-27)                    ║\n",
            surface_format_to_string(state->surface_format));
    fprintf(fp, "║   Tile Mode          : %s (bits 12-13)                    ║\n",
            tile_mode_to_string(state->tile_mode));
    fprintf(fp, "║   Is Array           : %s (bit 28)                        ║\n",
            state->is_array ? "YES" : "NO");
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 1
    fprintf(fp, "║ DWord 1: 0x%08X                                           ║\n", state->raw_data[1]);
    fprintf(fp, "║   Base Address [31:12]: 0x%08llX                          ║\n",
            (unsigned long long)(state->surface_base_address & 0xFFFFF000ULL));
    fprintf(fp, "║   MOCS               : %s (bits 24-30)                    ║\n",
            mocs_to_string(state->mocs));
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 2
    fprintf(fp, "║ DWord 2: 0x%08X                                           ║\n", state->raw_data[2]);
    fprintf(fp, "║   Width              : %u pixels (bits 0-13)              ║\n",
            state->width + 1);
    fprintf(fp, "║   Height             : %u pixels (bits 16-29)             ║\n",
            state->height + 1);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 3
    fprintf(fp, "║ DWord 3: 0x%08X                                           ║\n", state->raw_data[3]);
    fprintf(fp, "║   Depth              : %u (bits 21-31)                    ║\n",
            state->depth + 1);
    fprintf(fp, "║   Surface Pitch      : %u bytes (bits 0-17)               ║\n",
            state->surface_pitch + 1);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 4
    fprintf(fp, "║ DWord 4: 0x%08X                                           ║\n", state->raw_data[4]);
    fprintf(fp, "║   RT View Extent     : %u (bits 21-31)                    ║\n",
            state->render_target_view_extent);
    fprintf(fp, "║   Min Array Element  : %u (bits 0-10)                     ║\n",
            state->minimum_array_element);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 5
    fprintf(fp, "║ DWord 5: 0x%08X                                           ║\n", state->raw_data[5]);
    fprintf(fp, "║   Mip Count          : %u (bits 20-23)                    ║\n",
            state->mip_count);
    fprintf(fp, "║   Surface Min LOD    : %u (bits 4-7)                      ║\n",
            state->surface_min_lod);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 6
    fprintf(fp, "║ DWord 6: 0x%08X                                           ║\n", state->raw_data[6]);
    fprintf(fp, "║   Y Offset           : %u (bits 20-22)                    ║\n",
            state->y_offset);
    fprintf(fp, "║   X Offset           : %u (bits 25-31)                    ║\n",
            state->x_offset);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWord 7
    fprintf(fp, "║ DWord 7: 0x%08X                                           ║\n", state->raw_data[7]);
    fprintf(fp, "║   Resource Min LOD   : %u (bits 0-11)                     ║\n",
            state->resource_min_lod);
    fprintf(fp, "║   Channel Select R   : %u (bits 16-18)                    ║\n",
            state->shader_channel_select_r);
    fprintf(fp, "║   Channel Select G   : %u (bits 19-21)                    ║\n",
            state->shader_channel_select_g);
    fprintf(fp, "║   Channel Select B   : %u (bits 22-24)                    ║\n",
            state->shader_channel_select_b);
    fprintf(fp, "║   Channel Select A   : %u (bits 25-27)                    ║\n",
            state->shader_channel_select_a);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWords 8-9: Aux Surface
    fprintf(fp, "║ DWords 8-9: Aux Surface                                        ║\n");
    fprintf(fp, "║   Aux Base Address   : 0x%016llX                      ║\n",
            (unsigned long long)state->aux_surface_base_address);
    fprintf(fp, "║   Aux Pitch          : %u bytes                               ║\n",
            state->aux_surface_pitch);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // DWords 10-13: Clear Color
    fprintf(fp, "║ DWords 10-13: Clear Color                                      ║\n");
    fprintf(fp, "║   R: 0x%08X  G: 0x%08X                                ║\n",
            state->clear_color[0], state->clear_color[1]);
    fprintf(fp, "║   B: 0x%08X  A: 0x%08X                                ║\n",
            state->clear_color[2], state->clear_color[3]);
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // Validation
    fprintf(fp, "║ VALIDATION: %s                                              ║\n",
            state->is_valid ? "✓ VALID" : "✗ INVALID");
    if (!state->is_valid && state->error_msg[0]) {
        fprintf(fp, "║ Error: %-56s ║\n", state->error_msg);
    }
    
    fprintf(fp, "╚════════════════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// COMPARAISON SURFACE STATES
// ============================================================================

uint32_t compare_surface_states(const surface_state_t *state1,
                                 const surface_state_t *state2,
                                 FILE *fp) {
    if (!state1 || !state2 || !fp) {
        return 0;
    }
    
    uint32_t diff_count = 0;
    
    fprintf(fp, "╔════════════════════════════════════════════════════════════════╗\n");
    fprintf(fp, "║              SURFACE STATE DIFFERENTIAL ANALYSIS               ║\n");
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    // Comparer champs critiques
    if (state1->surface_type != state2->surface_type) {
        fprintf(fp, "║ DIFF: Surface Type: %s vs %s                              ║\n",
                surface_type_to_string(state1->surface_type),
                surface_type_to_string(state2->surface_type));
        diff_count++;
    }
    
    if (state1->surface_format != state2->surface_format) {
        fprintf(fp, "║ DIFF: Surface Format: %s vs %s                            ║\n",
                surface_format_to_string(state1->surface_format),
                surface_format_to_string(state2->surface_format));
        diff_count++;
    }
    
    if (state1->tile_mode != state2->tile_mode) {
        fprintf(fp, "║ DIFF: Tile Mode: %s vs %s                                 ║\n",
                tile_mode_to_string(state1->tile_mode),
                tile_mode_to_string(state2->tile_mode));
        diff_count++;
    }
    
    if (state1->width != state2->width || state1->height != state2->height) {
        fprintf(fp, "║ DIFF: Dimensions: %ux%u vs %ux%u                          ║\n",
                state1->width + 1, state1->height + 1,
                state2->width + 1, state2->height + 1);
        diff_count++;
    }
    
    if (state1->surface_pitch != state2->surface_pitch) {
        fprintf(fp, "║ DIFF: Pitch: %u vs %u bytes                               ║\n",
                state1->surface_pitch + 1, state2->surface_pitch + 1);
        diff_count++;
    }
    
    if (state1->surface_base_address != state2->surface_base_address) {
        fprintf(fp, "║ DIFF: Base Address: 0x%016llX vs 0x%016llX        ║\n",
                (unsigned long long)state1->surface_base_address,
                (unsigned long long)state2->surface_base_address);
        diff_count++;
    }
    
    if (state1->mocs != state2->mocs) {
        fprintf(fp, "║ DIFF: MOCS: %s vs %s                                      ║\n",
                mocs_to_string(state1->mocs),
                mocs_to_string(state2->mocs));
        diff_count++;
    }
    
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    fprintf(fp, "║ TOTAL DIFFERENCES: %u                                          ║\n", diff_count);
    fprintf(fp, "╚════════════════════════════════════════════════════════════════╝\n");
    
    return diff_count;
}

// ============================================================================
// EXTRACTION DEPUIS BATCH
// ============================================================================

uint32_t extract_surface_states_from_batch(const uint32_t *batch_data,
                                            uint32_t batch_size,
                                            surface_state_t *states,
                                            uint32_t max_states) {
    if (!batch_data || !states || batch_size < SURFACE_STATE_SIZE_BYTES) {
        return 0;
    }
    
    uint32_t count = 0;
    uint32_t dwords = batch_size / 4;
    
    // Chercher patterns Surface State (heuristique)
    for (uint32_t i = 0; i < dwords - SURFACE_STATE_SIZE_DWORDS && count < max_states; i++) {
        // Vérifier si c'est potentiellement un Surface State
        uint32_t dword0 = batch_data[i];
        uint32_t surface_type = extract_bits(dword0, 29, 31);
        uint32_t surface_format = extract_bits(dword0, 18, 27);
        
        // Heuristique: type valide + format valide
        if (surface_type <= SURFTYPE_NULL && surface_format <= 0x1FF) {
            if (decode_surface_state_gen9(&batch_data[i], &states[count])) {
                if (validate_surface_state_gen9(&states[count])) {
                    count++;
                    i += SURFACE_STATE_SIZE_DWORDS - 1;  // Skip décodé
                }
            }
        }
    }
    
    return count;
}

// ============================================================================
// GÉNÉRATION MINING OUTPUT
// ============================================================================

bool generate_mining_output_surface_state(uint32_t buffer_size,
                                          uint64_t base_address,
                                          uint32_t *data) {
    if (!data || buffer_size == 0) {
        return false;
    }
    
    surface_config_t config = {
        .type = SURFTYPE_BUFFER,
        .format = SURFACEFORMAT_R32_UINT,
        .tiling = TILE_LINEAR,
        .width = buffer_size / 4,  // En DWords
        .height = 1,
        .depth = 1,
        .pitch = buffer_size,
        .base_address = base_address,
        .mocs = MOCS_CACHED_LLC_L3,
        .enable_compression = false,
        .is_render_target = true,
        .is_read_only = false
    };
    
    return generate_surface_state_gen9(&config, data);
}

// ============================================================================
// ANALYSE CAUSALE
// ============================================================================

void analyze_surface_state_failure(const surface_state_t *state, FILE *fp) {
    if (!state || !fp) {
        return;
    }
    
    fprintf(fp, "╔════════════════════════════════════════════════════════════════╗\n");
    fprintf(fp, "║           SURFACE STATE FAILURE ROOT CAUSE ANALYSIS            ║\n");
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    
    if (!state->is_valid) {
        fprintf(fp, "║ STATUS: INVALID                                                ║\n");
        fprintf(fp, "║ Error: %-56s ║\n", state->error_msg);
        fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    }
    
    // Analyser causes potentielles
    fprintf(fp, "║ POTENTIAL ROOT CAUSES:                                         ║\n");
    
    if (state->surface_base_address == 0) {
        fprintf(fp, "║ ✗ Base address is NULL (0x0000000000000000)                   ║\n");
        fprintf(fp, "║   → Surface not allocated in GTT                               ║\n");
        fprintf(fp, "║   → Missing relocation entry                                   ║\n");
    }
    
    if (state->surface_base_address == 0x1001) {
        fprintf(fp, "║ ✗ Base address is symbolic (0x0000000000001001)               ║\n");
        fprintf(fp, "║   → Relocation not resolved                                    ║\n");
        fprintf(fp, "║   → EXECBUFFER2 not called yet                                 ║\n");
    }
    
    if (state->width == 0 || state->height == 0) {
        fprintf(fp, "║ ✗ Invalid dimensions: %ux%u                                   ║\n",
                state->width + 1, state->height + 1);
        fprintf(fp, "║   → Surface State not initialized                              ║\n");
    }
    
    if (state->surface_pitch == 0) {
        fprintf(fp, "║ ✗ Pitch is zero                                                ║\n");
        fprintf(fp, "║   → Surface State corrupted                                    ║\n");
    }
    
    if (state->tile_mode != TILE_LINEAR && (state->surface_pitch + 1) % 64 != 0) {
        fprintf(fp, "║ ✗ Pitch not aligned to 64 bytes for tiled surface             ║\n");
        fprintf(fp, "║   → Hardware will reject surface                               ║\n");
    }
    
    if (state->mocs == MOCS_UNCACHED) {
        fprintf(fp, "║ ⚠ MOCS is UNCACHED                                             ║\n");
        fprintf(fp, "║   → Performance impact (no LLC/L3 caching)                     ║\n");
    }
    
    fprintf(fp, "╠════════════════════════════════════════════════════════════════╣\n");
    fprintf(fp, "║ RECOMMENDED ACTIONS:                                           ║\n");
    fprintf(fp, "║ 1. Verify buffer allocated via GEM_CREATE                      ║\n");
    fprintf(fp, "║ 2. Add relocation entry for surface base address              ║\n");
    fprintf(fp, "║ 3. Ensure Surface State in batch before GPGPU_WALKER          ║\n");
    fprintf(fp, "║ 4. Use MOCS_CACHED_LLC_L3 for optimal performance             ║\n");
    fprintf(fp, "╚════════════════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// CALCULS UTILITAIRES
// ============================================================================

uint64_t calculate_surface_size(const surface_state_t *state) {
    if (!state) {
        return 0;
    }
    
    // Note: width n'est pas utilisé car pitch contient déjà la largeur alignée
    uint64_t height = state->height + 1;
    uint64_t depth = state->depth + 1;
    uint64_t pitch = state->surface_pitch + 1;
    
    // Taille = pitch * height * depth
    return pitch * height * depth;
}

bool check_surface_alignment(const surface_state_t *state) {
    if (!state) {
        return false;
    }
    
    // Vérifier alignement adresse base (4KB pour Gen9)
    if (state->surface_base_address % 4096 != 0) {
        return false;
    }
    
    // Vérifier alignement pitch selon tiling
    uint32_t pitch = state->surface_pitch + 1;
    switch (state->tile_mode) {
        case TILE_LINEAR:
            return (pitch % 64 == 0);  // 64 bytes
        case TILE_XMAJOR:
        case TILE_YMAJOR:
            return (pitch % 128 == 0);  // 128 bytes
        default:
            return true;
    }
}

// Made with Bob
