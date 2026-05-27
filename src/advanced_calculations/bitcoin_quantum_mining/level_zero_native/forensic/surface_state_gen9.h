/**
 * LUMVORAX SURFACE STATE ENGINE - Gen9 Architecture
 * 
 * Décodage et génération RENDER_SURFACE_STATE pour allocation GTT
 * 
 * Surface State = Descripteur mémoire GPU (16 DWords = 64 bytes)
 * Contient: format, dimensions, pitch, tiling, adresse base
 * 
 * CRITIQUE: Sans Surface States corrects, output buffer JAMAIS alloué GTT
 * 
 * CYCLE: C577
 * PRIORITÉ: ABSOLUE (bloque allocation GTT)
 * NIVEAU: Forensic maximal
 */

#ifndef LUMVORAX_SURFACE_STATE_GEN9_H
#define LUMVORAX_SURFACE_STATE_GEN9_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// ============================================================================
// CONSTANTES GEN9
// ============================================================================

#define SURFACE_STATE_SIZE_DWORDS 16
#define SURFACE_STATE_SIZE_BYTES 64

// Surface Types (DWord 0, bits 29-31)
typedef enum {
    SURFTYPE_1D = 0,
    SURFTYPE_2D = 1,
    SURFTYPE_3D = 2,
    SURFTYPE_CUBE = 3,
    SURFTYPE_BUFFER = 4,
    SURFTYPE_STRBUF = 5,
    SURFTYPE_NULL = 7
} surface_type_t;

// Surface Formats (DWord 0, bits 18-27)
typedef enum {
    SURFACEFORMAT_R32_UINT = 0x0C2,
    SURFACEFORMAT_R32_SINT = 0x0C3,
    SURFACEFORMAT_R32_FLOAT = 0x0C4,
    SURFACEFORMAT_R32G32_UINT = 0x0C5,
    SURFACEFORMAT_R32G32_SINT = 0x0C6,
    SURFACEFORMAT_R32G32_FLOAT = 0x0C7,
    SURFACEFORMAT_R32G32B32A32_UINT = 0x0C8,
    SURFACEFORMAT_R32G32B32A32_SINT = 0x0C9,
    SURFACEFORMAT_R32G32B32A32_FLOAT = 0x0CA,
    SURFACEFORMAT_RAW = 0x1FF
} surface_format_t;

// Tiling Modes (DWord 0, bits 12-13)
typedef enum {
    TILE_LINEAR = 0,
    TILE_WMAJOR = 1,
    TILE_XMAJOR = 2,
    TILE_YMAJOR = 3
} tile_mode_t;

// Memory Object Control State (DWord 1, bits 24-30)
typedef enum {
    MOCS_UNCACHED = 0x00,
    MOCS_CACHED_LLC = 0x02,
    MOCS_CACHED_L3 = 0x03,
    MOCS_CACHED_LLC_L3 = 0x07
} mocs_t;

// ============================================================================
// STRUCTURES DE DONNÉES
// ============================================================================

/**
 * Surface State décodé (16 DWords)
 */
typedef struct {
    // DWord 0
    surface_type_t surface_type;
    surface_format_t surface_format;
    tile_mode_t tile_mode;
    bool is_array;
    uint32_t surface_vertical_alignment;
    uint32_t surface_horizontal_alignment;
    
    // DWord 1
    uint64_t surface_base_address;  // 48 bits
    mocs_t mocs;
    
    // DWord 2
    uint32_t width;   // Surface width - 1
    uint32_t height;  // Surface height - 1
    
    // DWord 3
    uint32_t depth;   // Surface depth - 1
    uint32_t surface_pitch;  // Pitch - 1 (bytes)
    
    // DWord 4
    uint32_t render_target_view_extent;
    uint32_t minimum_array_element;
    
    // DWord 5
    uint32_t mip_count;
    uint32_t surface_min_lod;
    uint32_t mip_tail_start_lod;
    
    // DWord 6
    uint32_t y_offset;
    uint32_t x_offset;
    
    // DWord 7
    uint32_t resource_min_lod;
    uint32_t shader_channel_select_r;
    uint32_t shader_channel_select_g;
    uint32_t shader_channel_select_b;
    uint32_t shader_channel_select_a;
    
    // DWords 8-9: Aux Surface (compression)
    uint64_t aux_surface_base_address;
    uint32_t aux_surface_pitch;
    uint32_t aux_surface_qpitch;
    
    // DWords 10-11: Clear Color
    uint32_t clear_color[4];
    
    // DWords 12-15: Reserved
    uint32_t reserved[4];
    
    // Métadonnées
    bool is_valid;
    char error_msg[256];
    uint32_t raw_data[SURFACE_STATE_SIZE_DWORDS];
} surface_state_t;

/**
 * Configuration pour génération Surface State
 */
typedef struct {
    surface_type_t type;
    surface_format_t format;
    tile_mode_t tiling;
    
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t pitch;
    
    uint64_t base_address;
    mocs_t mocs;
    
    bool enable_compression;
    bool is_render_target;
    bool is_read_only;
} surface_config_t;

// ============================================================================
// API PUBLIQUE
// ============================================================================

/**
 * Décode un Surface State Gen9 (16 DWords)
 * 
 * @param data Pointeur vers 16 DWords
 * @param state Structure à remplir
 * @return true si décodage réussi
 */
bool decode_surface_state_gen9(const uint32_t *data, surface_state_t *state);

/**
 * Génère un Surface State Gen9 à partir d'une configuration
 * 
 * @param config Configuration surface
 * @param data Buffer 16 DWords à remplir
 * @return true si génération réussie
 */
bool generate_surface_state_gen9(const surface_config_t *config, uint32_t *data);

/**
 * Valide un Surface State Gen9
 * 
 * @param state Surface State à valider
 * @return true si valide
 */
bool validate_surface_state_gen9(const surface_state_t *state);

/**
 * Affiche un Surface State en format Wireshark
 * 
 * @param state Surface State décodé
 * @param fp Fichier de sortie
 */
void print_surface_state_wireshark(const surface_state_t *state, FILE *fp);

/**
 * Compare deux Surface States
 * 
 * @param state1 Premier Surface State
 * @param state2 Deuxième Surface State
 * @param fp Fichier pour rapport différentiel
 * @return Nombre de différences
 */
uint32_t compare_surface_states(const surface_state_t *state1,
                                 const surface_state_t *state2,
                                 FILE *fp);

/**
 * Extrait Surface States d'un batch buffer
 * 
 * @param batch_data Données batch buffer
 * @param batch_size Taille en bytes
 * @param states Buffer pour Surface States trouvés
 * @param max_states Capacité buffer
 * @return Nombre de Surface States trouvés
 */
uint32_t extract_surface_states_from_batch(const uint32_t *batch_data,
                                            uint32_t batch_size,
                                            surface_state_t *states,
                                            uint32_t max_states);

/**
 * Génère Surface State pour buffer output Bitcoin mining
 * 
 * @param buffer_size Taille buffer en bytes
 * @param base_address Adresse base GTT (0 = symbolique)
 * @param data Buffer 16 DWords à remplir
 * @return true si génération réussie
 */
bool generate_mining_output_surface_state(uint32_t buffer_size,
                                          uint64_t base_address,
                                          uint32_t *data);

/**
 * Analyse causale: pourquoi Surface State invalide?
 * 
 * @param state Surface State à analyser
 * @param fp Fichier pour rapport
 */
void analyze_surface_state_failure(const surface_state_t *state, FILE *fp);

// ============================================================================
// UTILITAIRES
// ============================================================================

/**
 * Convertit format surface en string
 */
const char* surface_format_to_string(surface_format_t format);

/**
 * Convertit type surface en string
 */
const char* surface_type_to_string(surface_type_t type);

/**
 * Convertit tiling mode en string
 */
const char* tile_mode_to_string(tile_mode_t mode);

/**
 * Convertit MOCS en string
 */
const char* mocs_to_string(mocs_t mocs);

/**
 * Calcule taille surface en bytes
 */
uint64_t calculate_surface_size(const surface_state_t *state);

/**
 * Vérifie alignement surface
 */
bool check_surface_alignment(const surface_state_t *state);

#endif // LUMVORAX_SURFACE_STATE_GEN9_H

// Made with Bob
