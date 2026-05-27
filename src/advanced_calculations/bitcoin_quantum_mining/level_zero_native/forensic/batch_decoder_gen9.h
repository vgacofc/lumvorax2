/**
 * LUMVORAX BATCH DECODER ENGINE - Gen9 Architecture
 * 
 * "Wireshark du GPU" - Décodage complet batch buffers Intel Gen9
 * 
 * Transforme:
 *   0x61010000 → PIPELINE_SELECT (3D mode)
 *   0x78050006 → STATE_BASE_ADDRESS (7 DWords)
 * 
 * Avec validation complète, analyse causale, et détection anomalies.
 * 
 * CYCLE: C576
 * PRIORITÉ: #2 (Critique pour Surface States)
 * NIVEAU: Forensic maximal
 */

#ifndef LUMVORAX_BATCH_DECODER_GEN9_H
#define LUMVORAX_BATCH_DECODER_GEN9_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// ============================================================================
// STRUCTURES DE DONNÉES
// ============================================================================

/**
 * Type de commande GPU Gen9
 */
typedef enum {
    CMD_UNKNOWN = 0,
    
    // MI Commands (Machine Instructions)
    CMD_MI_NOOP = 0x00,
    CMD_MI_BATCH_BUFFER_END = 0x0A,
    CMD_MI_LOAD_REGISTER_IMM = 0x22,
    CMD_MI_LOAD_REGISTER_MEM = 0x29,
    CMD_MI_STORE_DATA_IMM = 0x20,
    CMD_MI_STORE_REGISTER_MEM = 0x24,
    CMD_MI_ATOMIC = 0x2F,
    CMD_MI_FLUSH_DW = 0x26,
    
    // Pipeline Commands
    CMD_PIPELINE_SELECT = 0x69,
    CMD_STATE_BASE_ADDRESS = 0x61,
    CMD_STATE_SIP = 0x02,
    
    // Media Commands
    CMD_MEDIA_VFE_STATE = 0x70,
    CMD_MEDIA_CURBE_LOAD = 0x71,
    CMD_MEDIA_INTERFACE_DESCRIPTOR_LOAD = 0x72,
    CMD_MEDIA_STATE_FLUSH = 0x04,
    CMD_GPGPU_WALKER = 0x05,
    
    // 3D Commands
    CMD_3DSTATE_BINDING_TABLE_POINTERS = 0x78,
    CMD_3DSTATE_SAMPLER_STATE_POINTERS = 0x79,
    
    // Compute Commands
    CMD_COMPUTE_WALKER = 0x15,
    
} gpu_command_type_t;

/**
 * Champ décodé d'une commande
 */
typedef struct {
    const char *name;           // Nom du champ
    uint32_t value;             // Valeur brute
    uint32_t bit_start;         // Bit de début
    uint32_t bit_end;           // Bit de fin
    const char *description;    // Description sémantique
    bool is_address;            // Est une adresse GTT
    bool is_reserved;           // Champ réservé (doit être 0)
    bool is_valid;              // Validation réussie
} decoded_field_t;

/**
 * Commande décodée complète
 */
typedef struct {
    uint32_t offset;                    // Offset dans batch (bytes)
    uint32_t dword_count;               // Nombre de DWords
    gpu_command_type_t type;            // Type de commande
    const char *name;                   // Nom lisible
    uint32_t opcode;                    // Opcode brut
    
    // Données brutes
    uint32_t raw_data[32];              // Max 32 DWords par commande
    
    // Champs décodés
    decoded_field_t fields[64];         // Max 64 champs
    uint32_t field_count;
    
    // Validation
    bool is_valid;                      // Commande valide
    char error_msg[256];                // Message d'erreur si invalide
    
    // Analyse causale
    bool requires_relocation;           // Nécessite relocation
    uint32_t relocation_targets[8];     // Offsets des relocations
    uint32_t relocation_count;
    
    bool accesses_memory;               // Accède à la mémoire
    bool modifies_state;                // Modifie l'état GPU
    bool is_synchronization;            // Point de synchronisation
    
} decoded_command_t;

/**
 * Batch buffer décodé complet
 */
typedef struct {
    // Métadonnées
    uint32_t size_bytes;                // Taille totale
    uint32_t command_count;             // Nombre de commandes
    
    // Commandes décodées
    decoded_command_t commands[1024];   // Max 1024 commandes
    
    // Statistiques
    uint32_t mi_command_count;
    uint32_t pipeline_command_count;
    uint32_t media_command_count;
    uint32_t compute_command_count;
    
    // Validation globale
    bool is_valid;
    bool has_end_marker;                // Contient MI_BATCH_BUFFER_END
    uint32_t error_count;
    uint32_t warning_count;
    
    // Analyse causale
    uint32_t memory_access_count;
    uint32_t state_change_count;
    uint32_t sync_point_count;
    
    // Timeline
    uint64_t decode_timestamp_ns;
    
} decoded_batch_t;

/**
 * Configuration du décodeur
 */
typedef struct {
    bool verbose;                       // Mode verbeux
    bool validate_reserved_bits;        // Valider bits réservés
    bool decode_addresses;              // Décoder adresses GTT
    bool analyze_causality;             // Analyse causale
    bool detect_anomalies;              // Détection anomalies
    FILE *output_file;                  // Fichier de sortie (NULL = stdout)
} decoder_config_t;

// ============================================================================
// API PUBLIQUE
// ============================================================================

/**
 * Initialise le décodeur avec configuration
 */
void batch_decoder_init(decoder_config_t *config);

/**
 * Décode un batch buffer complet
 * 
 * @param batch_data Données brutes du batch
 * @param size_bytes Taille en bytes
 * @param output Structure de sortie
 * @return true si décodage réussi
 */
bool batch_decoder_decode(
    const uint32_t *batch_data,
    uint32_t size_bytes,
    decoded_batch_t *output
);

/**
 * Affiche un batch décodé (format lisible)
 */
void batch_decoder_print(const decoded_batch_t *batch);

/**
 * Affiche un batch décodé (format JSON)
 */
void batch_decoder_print_json(const decoded_batch_t *batch);

/**
 * Affiche un batch décodé (format Wireshark-like)
 */
void batch_decoder_print_wireshark(const decoded_batch_t *batch);

/**
 * Valide un batch décodé
 * 
 * @return Nombre d'erreurs détectées
 */
uint32_t batch_decoder_validate(decoded_batch_t *batch);

/**
 * Compare deux batches (OpenCL vs natif)
 * 
 * Génère rapport différentiel détaillé
 */
void batch_decoder_compare(
    const decoded_batch_t *batch_opencl,
    const decoded_batch_t *batch_native,
    FILE *output
);

/**
 * Analyse causale d'un batch
 * 
 * Identifie:
 * - Dépendances mémoire
 * - Points de synchronisation
 * - Modifications d'état critiques
 * - Anomalies potentielles
 */
void batch_decoder_analyze_causality(decoded_batch_t *batch);

/**
 * Détecte anomalies dans un batch
 * 
 * @return Nombre d'anomalies détectées
 */
uint32_t batch_decoder_detect_anomalies(decoded_batch_t *batch);

/**
 * Génère graphe de dépendances (format DOT)
 */
void batch_decoder_generate_dependency_graph(
    const decoded_batch_t *batch,
    const char *output_path
);

// ============================================================================
// DÉCODEURS SPÉCIALISÉS PAR COMMANDE
// ============================================================================

/**
 * Décode MI_BATCH_BUFFER_END
 */
bool decode_mi_batch_buffer_end(
    const uint32_t *data,
    decoded_command_t *cmd
);

/**
 * Décode STATE_BASE_ADDRESS (critique pour GTT)
 */
bool decode_state_base_address(
    const uint32_t *data,
    decoded_command_t *cmd
);

/**
 * Décode MEDIA_VFE_STATE
 */
bool decode_media_vfe_state(
    const uint32_t *data,
    decoded_command_t *cmd
);

/**
 * Décode MEDIA_INTERFACE_DESCRIPTOR_LOAD (IDRT)
 */
bool decode_media_idrt_load(
    const uint32_t *data,
    decoded_command_t *cmd
);

/**
 * Décode GPGPU_WALKER
 */
bool decode_gpgpu_walker(
    const uint32_t *data,
    decoded_command_t *cmd
);

/**
 * Décode MI_LOAD_REGISTER_MEM
 */
bool decode_mi_load_register_mem(
    const uint32_t *data,
    decoded_command_t *cmd
);

/**
 * Décode MI_STORE_DATA_IMM
 */
bool decode_mi_store_data_imm(
    const uint32_t *data,
    decoded_command_t *cmd
);

/**
 * Décode PIPELINE_SELECT
 */
bool decode_pipeline_select(
    const uint32_t *data,
    decoded_command_t *cmd
);

// ============================================================================
// UTILITAIRES
// ============================================================================

/**
 * Extrait un champ de bits
 */
static inline uint32_t extract_bits(uint32_t value, uint32_t start, uint32_t end) {
    uint32_t mask = ((1U << (end - start + 1)) - 1);
    return (value >> start) & mask;
}

/**
 * Identifie le type de commande depuis l'opcode
 */
gpu_command_type_t identify_command_type(uint32_t dword0);

/**
 * Obtient le nom d'une commande
 */
const char* get_command_name(gpu_command_type_t type);

/**
 * Obtient la longueur d'une commande (en DWords)
 */
uint32_t get_command_length(uint32_t dword0);

/**
 * Valide un champ réservé
 */
bool validate_reserved_field(uint32_t value, uint32_t expected);

/**
 * Formate une adresse GTT
 */
void format_gtt_address(uint64_t address, char *buffer, size_t size);

#endif // LUMVORAX_BATCH_DECODER_GEN9_H

// Made with Bob
