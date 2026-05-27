/**
 * BATCH GENERATOR AUTOMATIQUE i915 DRM NATIF
 * 
 * Objectif: Reproduire le mécanisme automatique d'OpenCL pour générer
 * des batch buffers i915 DRM de manière automatique, basé sur le
 * reverse engineering complet d'OpenCL (C545, C585).
 * 
 * Architecture: 100% i915 DRM natif, AUCUNE dépendance externe
 * 
 * Basé sur:
 * - RAPPORT_C585: Capture OpenCL EXECBUFFER2 (12 buffers, flags 0x800)
 * - RAPPORT_C545: Structure batch 320 bytes (13 commandes)
 * - RAPPORT_C595: Plan d'action corrigé
 * 
 * Date: 2026-05-26
 * Cycle: C596 - Début développement système automatique
 */

#ifndef BATCH_GENERATOR_AUTO_H
#define BATCH_GENERATOR_AUTO_H

#include <stdint.h>
#include <stdbool.h>
#include <xf86drm.h>
#include <i915_drm.h>

/**
 * Configuration OpenCL validée (C585)
 */
#define OPENCL_BUFFER_COUNT 12
#define OPENCL_BATCH_SIZE 320
#define OPENCL_FLAGS 0x800  // I915_EXEC_NO_RELOC
#define OPENCL_OBJECT_FLAGS 0x18  // EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS

/**
 * Structure pour un buffer i915 avec soft-pinning
 */
typedef struct {
    uint32_t handle;           // GEM handle
    uint64_t offset;           // GTT offset (soft-pinned)
    uint64_t size;             // Taille buffer
    uint32_t flags;            // EXEC_OBJECT_* flags
    void *cpu_addr;            // Adresse CPU mappée
    const char *name;          // Nom debug
} auto_buffer_t;

/**
 * Types de buffers identifiés dans OpenCL (C585)
 */
typedef enum {
    BUFFER_TYPE_BATCH = 0,     // Batch buffer principal
    BUFFER_TYPE_HEAP,          // Heap (kernel ISA + structures)
    BUFFER_TYPE_OUTPUT,        // Buffer output résultats
    BUFFER_TYPE_SURFACE_STATE, // Surface states
    BUFFER_TYPE_BINDING_TABLE, // Binding table
    BUFFER_TYPE_IDRT,          // Interface Descriptor Table
    BUFFER_TYPE_CURBE,         // Cross-thread constants
    BUFFER_TYPE_SCRATCH,       // Scratch space
    BUFFER_TYPE_KERNEL,        // Kernel ISA
    BUFFER_TYPE_DYNAMIC_STATE, // Dynamic state
    BUFFER_TYPE_GENERAL_STATE, // General state
    BUFFER_TYPE_INDIRECT       // Indirect objects
} buffer_type_t;

/**
 * Configuration générateur automatique
 */
typedef struct {
    int drm_fd;                // File descriptor DRM
    uint32_t ctx_id;           // Context i915
    
    // Buffers automatiques (12 comme OpenCL)
    auto_buffer_t buffers[OPENCL_BUFFER_COUNT];
    uint32_t buffer_count;
    
    // Batch buffer automatique
    uint32_t *batch_data;      // Données batch (320 bytes)
    uint32_t batch_dwords;     // Nombre de DWords
    
    // Configuration kernel
    uint32_t global_work_size; // Nombre total work-items
    uint32_t local_work_size;  // Taille work-group
    uint32_t num_threads;      // Threads EU à dispatcher
    
    // Kernel ISA
    void *kernel_isa;          // Code kernel Gen9
    uint32_t kernel_size;      // Taille kernel
    
    // Flags automatiques (comme OpenCL)
    uint64_t exec_flags;       // I915_EXEC_NO_RELOC, etc.
    uint32_t object_flags;     // EXEC_OBJECT_PINNED, etc.
    
    // État génération
    bool initialized;
    bool batch_generated;
    
} batch_generator_t;

/**
 * Structure STATE_BASE_ADDRESS (6 variantes progressives identifiées C545)
 */
typedef struct {
    uint32_t opcode;           // 0x61010008 ou variantes
    uint64_t general_state_base;
    uint64_t surface_state_base;
    uint64_t dynamic_state_base;
    uint64_t indirect_object_base;
    uint64_t instruction_base;
    uint32_t modify_enable;    // Bits enable pour chaque base
} state_base_address_t;

/**
 * Structure MEDIA_VFE_STATE (C545: 167 threads max, CURBE 1922)
 */
typedef struct {
    uint32_t opcode;           // 0x70020000
    uint32_t scratch_space;
    uint32_t max_threads;      // 167 pour Gen9 UHD 620
    uint32_t num_urb_entries;
    uint32_t urb_entry_size;
    uint32_t curbe_size;       // 1922 bytes identifié
    uint32_t scoreboard_mask;
} media_vfe_state_t;

/**
 * Structure MEDIA_INTERFACE_DESCRIPTOR_LOAD
 */
typedef struct {
    uint32_t opcode;           // 0x61010011 (19 DWords identifié C594)
    uint64_t idrt_offset;      // Offset Interface Descriptor Table
    uint32_t idrt_length;
} media_idl_t;

/**
 * Structure GPGPU_WALKER (dispatch compute)
 */
typedef struct {
    uint32_t opcode;           // 0x61020001 ou 0x71050000
    uint32_t thread_width;
    uint32_t thread_height;
    uint32_t thread_depth;
    uint32_t thread_group_id_x;
    uint32_t thread_group_id_y;
    uint32_t thread_group_id_z;
    uint32_t right_execution_mask;
    uint32_t bottom_execution_mask;
} gpgpu_walker_t;

/**
 * API Publique - Générateur Automatique
 */

/**
 * Initialiser générateur automatique
 * Crée automatiquement les 12 buffers avec soft-pinning comme OpenCL
 */
int batch_generator_init(batch_generator_t *gen, int drm_fd);

/**
 * Configurer kernel à exécuter
 */
int batch_generator_set_kernel(batch_generator_t *gen, 
                                void *kernel_isa, 
                                uint32_t kernel_size,
                                uint32_t global_work_size,
                                uint32_t local_work_size);

/**
 * Générer automatiquement batch buffer complet (320 bytes comme OpenCL)
 * Génère les 13 commandes identifiées en C545:
 * 1. PIPELINE_SELECT GPGPU
 * 2-7. STATE_BASE_ADDRESS (6 variantes)
 * 8-10. 3DSTATE_BINDING_TABLE_POOL_ALLOC (3x)
 * 11. MEDIA_VFE_STATE
 * 12. MEDIA_INTERFACE_DESCRIPTOR_LOAD
 * 13. GPGPU_WALKER
 * 14. PIPE_CONTROL
 */
int batch_generator_generate(batch_generator_t *gen);

/**
 * Soumettre batch au GPU avec configuration automatique
 * Configure automatiquement:
 * - 12 buffers avec soft-pinning (flags 0x18)
 * - flags EXECBUFFER2 (0x800 = I915_EXEC_NO_RELOC)
 * - context i915
 * - batch_len = 320 bytes
 */
int batch_generator_submit(batch_generator_t *gen);

/**
 * Attendre fin exécution GPU
 */
int batch_generator_wait(batch_generator_t *gen);

/**
 * Lire résultats depuis buffer output
 */
int batch_generator_read_results(batch_generator_t *gen, void *output, size_t size);

/**
 * Nettoyer générateur
 */
void batch_generator_cleanup(batch_generator_t *gen);

/**
 * Fonctions internes - Génération automatique des commandes
 */

/**
 * CYCLE C597 - FONCTIONS GÉNÉRATION COMMANDES BATCH
 * Basé sur reverse engineering OpenCL (C545, C585)
 */

// Génération STATE_BASE_ADDRESS complet (19 DWords)
int generate_state_base_address(batch_generator_t *gen, uint32_t *batch, uint32_t *idx);

// Génération MEDIA_VFE_STATE (9 DWords)
int generate_media_vfe_state(batch_generator_t *gen, uint32_t *batch, uint32_t *idx);

/**
 * Générer PIPE_CONTROL automatiquement (À IMPLÉMENTER C599)
 */
int generate_pipe_control(batch_generator_t *gen,
                          uint32_t *batch,
                          uint32_t *offset);

/**
 * Allouer et configurer automatiquement les 12 buffers
 * Avec soft-pinning automatique (flags 0x18)
 */
int allocate_buffers_auto(batch_generator_t *gen);

/**
 * Calculer automatiquement offsets GTT pour soft-pinning
 * Évite collisions, respecte alignements
 */
uint64_t calculate_gtt_offset_auto(batch_generator_t *gen, 
                                    buffer_type_t type,
                                    uint64_t size);

/**
 * Génération MEDIA_INTERFACE_DESCRIPTOR_LOAD (C598)
 * Basé sur reverse engineering OpenCL C545 @ 0x00EC
 * Configure Interface Descriptor Table (IDRT)
 */
int generate_media_interface_descriptor_load(batch_generator_t *gen,
                                              uint32_t *batch,
                                              uint32_t *idx);

/**
 * Génération GPGPU_WALKER (C598)
 * Basé sur reverse engineering OpenCL C545 @ 0x0110
 * Dispatch compute threads sur GPU
 */
int generate_gpgpu_walker(batch_generator_t *gen,
                          uint32_t *batch,
                          uint32_t *idx,
                          uint32_t thread_x,
                          uint32_t thread_y,
                          uint32_t thread_z);

/**
 * CYCLE C600 - CONFIGURATION GPU STRUCTURES
 * Configuration IDRT, Surface State, Binding Table pour exécution kernel réelle
 */

/**
 * Configurer Interface Descriptor Table (IDRT)
 * Structure 32 bytes contenant:
 * - Kernel start pointer (GTT offset)
 * - Sampler state pointer
 * - Binding table pointer
 * - CURBE read length
 * - Number of threads
 * - Barrier enable, etc.
 */
int configure_idrt(batch_generator_t *gen);

/**
 * Configurer Surface State pour buffer output
 * Structure 64 bytes (16 DWords) contenant:
 * - Surface base address (GTT offset output buffer)
 * - Surface type (BUFFER)
 * - Surface format (RAW)
 * - Width, Height, Depth
 * - Pitch, etc.
 */
int configure_surface_state(batch_generator_t *gen, uint32_t surface_index);

/**
 * Configurer Binding Table
 * Table de pointeurs vers Surface States
 * Chaque entrée = offset relatif vers un Surface State
 */
int configure_binding_table(batch_generator_t *gen);

/**
 * Utilitaires
 */
const char* buffer_type_name(buffer_type_t type);
void dump_batch_hex(uint32_t *batch, uint32_t dwords);
void dump_execbuffer2_config(batch_generator_t *gen);

#endif // BATCH_GENERATOR_AUTO_H

// Made with Bob
