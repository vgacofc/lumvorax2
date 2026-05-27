// I915_BATCH_BUILDER_AUTO.H - Système Automatique de Construction Batch Buffers
// Objectif: Reproduire le système automatique d'OpenCL pour i915 DRM
// Inspiré de: intel/compute-runtime

#ifndef I915_BATCH_BUILDER_AUTO_H
#define I915_BATCH_BUILDER_AUTO_H

#include <stdint.h>
#include <stdbool.h>
#include <libdrm/i915_drm.h>
#include <libdrm/drm.h>

// ============================================================================
// STRUCTURES DE BASE
// ============================================================================

// Buffer Object (BO) - Représente un buffer GPU
typedef struct {
    uint32_t handle;           // Handle GEM
    uint64_t size;             // Taille en bytes
    uint64_t gtt_offset;       // Offset GTT après pinning
    void *cpu_ptr;             // Pointeur CPU (si mappé)
    bool is_mapped;            // Buffer mappé?
    bool is_pinned;            // Buffer pinné?
    uint32_t flags;            // Flags (EXEC_OBJECT_*)
} i915_bo_t;

// Relocation Entry - Représente une relocation à appliquer
typedef struct {
    uint32_t target_bo_index;  // Index du BO cible dans la liste
    uint64_t offset;           // Offset dans le batch où patcher
    uint64_t delta;            // Delta à ajouter à l'adresse GTT
    uint32_t read_domains;     // Domaines de lecture
    uint32_t write_domain;     // Domaine d'écriture
} i915_relocation_t;

// State Base Address Configuration
typedef struct {
    bool general_state_enable;
    uint64_t general_state_base;
    
    bool surface_state_enable;
    uint64_t surface_state_base;
    
    bool dynamic_state_enable;
    uint64_t dynamic_state_base;
    
    bool indirect_object_enable;
    uint64_t indirect_object_base;
    
    bool instruction_enable;
    uint64_t instruction_base;
    
    bool bindless_surface_enable;
    uint64_t bindless_surface_base;
    
    uint32_t general_buffer_size;
    uint32_t dynamic_buffer_size;
    uint32_t indirect_buffer_size;
    uint32_t instruction_buffer_size;
} state_base_address_config_t;

// MEDIA_VFE_STATE Configuration
typedef struct {
    uint32_t scratch_space_base_low;
    uint32_t scratch_space_base_high;
    uint32_t max_threads;
    uint32_t urb_entries;
    uint32_t urb_entry_size;
    uint32_t curbe_allocation_size;
    uint32_t scoreboard_mask;
} media_vfe_state_config_t;

// Interface Descriptor (IDRT) Configuration
typedef struct {
    uint64_t kernel_start_pointer;
    uint32_t sampler_count;
    uint64_t sampler_state_pointer;
    uint32_t binding_table_entry_count;
    uint64_t binding_table_pointer;
    uint32_t curbe_read_offset;
    uint32_t curbe_read_length;
    uint32_t barrier_enable;
    uint32_t slm_size;
    uint32_t number_of_threads;
    uint32_t cross_thread_constant_data_read_length;
} interface_descriptor_config_t;

// GPGPU_WALKER Configuration
typedef struct {
    uint32_t interface_descriptor_offset;
    uint32_t indirect_data_length;
    uint64_t indirect_data_start_address;
    uint32_t thread_group_id_start_x;
    uint32_t thread_group_id_start_y;
    uint32_t thread_group_id_start_z;
    uint32_t thread_group_id_x_dimension;
    uint32_t thread_group_id_y_dimension;
    uint32_t thread_group_id_z_dimension;
    uint32_t thread_width_counter_max;
    uint32_t thread_height_counter_max;
    uint32_t thread_depth_counter_max;
    uint32_t right_execution_mask;
    uint32_t bottom_execution_mask;
} gpgpu_walker_config_t;

// Batch Builder Context - État complet du builder
typedef struct {
    int drm_fd;                        // File descriptor DRM
    
    // Buffers
    i915_bo_t *bos;                    // Liste des BOs
    uint32_t bo_count;                 // Nombre de BOs
    uint32_t bo_capacity;              // Capacité allouée
    
    // Batch buffer
    i915_bo_t batch_bo;                // BO du batch
    uint32_t *batch_ptr;               // Pointeur dans le batch
    uint32_t batch_offset;             // Offset courant (en DWords)
    uint32_t batch_capacity;           // Capacité (en DWords)
    
    // Relocations
    i915_relocation_t *relocations;    // Liste des relocations
    uint32_t relocation_count;         // Nombre de relocations
    uint32_t relocation_capacity;      // Capacité allouée
    
    // Configuration
    state_base_address_config_t sba_config;
    media_vfe_state_config_t vfe_config;
    interface_descriptor_config_t idrt_config;
    gpgpu_walker_config_t walker_config;
    
    // État
    bool is_initialized;
    bool is_finalized;
    uint32_t pipeline_select_mode;     // 0=3D, 2=GPGPU
    
} i915_batch_builder_t;

// ============================================================================
// API PUBLIQUE - GESTION DU BUILDER
// ============================================================================

// Initialiser le batch builder
i915_batch_builder_t* i915_batch_builder_create(int drm_fd, uint32_t batch_size);

// Détruire le batch builder
void i915_batch_builder_destroy(i915_batch_builder_t *builder);

// Reset le builder pour réutilisation
void i915_batch_builder_reset(i915_batch_builder_t *builder);

// ============================================================================
// API PUBLIQUE - GESTION DES BUFFERS
// ============================================================================

// Créer et enregistrer un nouveau BO
uint32_t i915_batch_builder_create_bo(
    i915_batch_builder_t *builder,
    uint64_t size,
    uint32_t flags
);

// Mapper un BO en mémoire CPU
void* i915_batch_builder_map_bo(
    i915_batch_builder_t *builder,
    uint32_t bo_index
);

// Unmapper un BO
void i915_batch_builder_unmap_bo(
    i915_batch_builder_t *builder,
    uint32_t bo_index
);

// ============================================================================
// API PUBLIQUE - CONSTRUCTION DU BATCH
// ============================================================================

// Émettre PIPELINE_SELECT
void i915_batch_emit_pipeline_select(
    i915_batch_builder_t *builder,
    uint32_t mode  // 0=3D, 2=GPGPU
);

// Émettre STATE_BASE_ADDRESS (automatique avec relocations)
void i915_batch_emit_state_base_address(
    i915_batch_builder_t *builder,
    const state_base_address_config_t *config
);

// Émettre 3DSTATE_BINDING_TABLE_POOL_ALLOC
void i915_batch_emit_binding_table_pool_alloc(
    i915_batch_builder_t *builder,
    uint32_t bo_index,
    uint32_t pool_size
);

// Émettre MEDIA_VFE_STATE
void i915_batch_emit_media_vfe_state(
    i915_batch_builder_t *builder,
    const media_vfe_state_config_t *config
);

// Émettre MEDIA_CURBE_LOAD
void i915_batch_emit_media_curbe_load(
    i915_batch_builder_t *builder,
    uint32_t bo_index,
    uint32_t offset,
    uint32_t length
);

// Émettre MEDIA_INTERFACE_DESCRIPTOR_LOAD
void i915_batch_emit_media_interface_descriptor_load(
    i915_batch_builder_t *builder,
    uint32_t bo_index,
    uint32_t offset,
    uint32_t length
);

// Émettre GPGPU_WALKER
void i915_batch_emit_gpgpu_walker(
    i915_batch_builder_t *builder,
    const gpgpu_walker_config_t *config
);

// Émettre MEDIA_STATE_FLUSH
void i915_batch_emit_media_state_flush(
    i915_batch_builder_t *builder
);

// Émettre PIPE_CONTROL
void i915_batch_emit_pipe_control(
    i915_batch_builder_t *builder,
    uint32_t flags
);

// Émettre MI_BATCH_BUFFER_END
void i915_batch_emit_batch_buffer_end(
    i915_batch_builder_t *builder
);

// ============================================================================
// API PUBLIQUE - EXÉCUTION
// ============================================================================

// Finaliser le batch (appliquer relocations, etc.)
bool i915_batch_builder_finalize(i915_batch_builder_t *builder);

// Exécuter le batch
bool i915_batch_builder_execute(
    i915_batch_builder_t *builder,
    uint64_t timeout_ns
);

// Attendre la fin de l'exécution
bool i915_batch_builder_wait(
    i915_batch_builder_t *builder,
    uint64_t timeout_ns
);

// ============================================================================
// API PUBLIQUE - HELPERS
// ============================================================================

// Ajouter une relocation automatiquement
void i915_batch_add_relocation(
    i915_batch_builder_t *builder,
    uint32_t target_bo_index,
    uint64_t batch_offset,
    uint64_t delta,
    uint32_t read_domains,
    uint32_t write_domain
);

// Obtenir l'adresse GTT d'un BO (après pinning)
uint64_t i915_batch_get_bo_gtt_address(
    i915_batch_builder_t *builder,
    uint32_t bo_index
);

// Dump du batch pour debugging
void i915_batch_builder_dump(
    i915_batch_builder_t *builder,
    const char *filename
);

// ============================================================================
// API PUBLIQUE - CONFIGURATION HELPERS
// ============================================================================

// Créer configuration STATE_BASE_ADDRESS par défaut
state_base_address_config_t i915_create_default_sba_config(
    uint32_t heap_bo_index
);

// Créer configuration MEDIA_VFE_STATE par défaut
media_vfe_state_config_t i915_create_default_vfe_config(void);

// Créer configuration Interface Descriptor par défaut
interface_descriptor_config_t i915_create_default_idrt_config(
    uint64_t kernel_offset,
    uint64_t binding_table_offset,
    uint32_t curbe_length
);

// Créer configuration GPGPU_WALKER par défaut
gpgpu_walker_config_t i915_create_default_walker_config(
    uint32_t threads_x,
    uint32_t threads_y,
    uint32_t threads_z
);

#endif // I915_BATCH_BUILDER_AUTO_H

// Made with Bob
