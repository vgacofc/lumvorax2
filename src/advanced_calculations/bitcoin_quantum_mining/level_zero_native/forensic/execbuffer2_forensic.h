#ifndef EXECBUFFER2_FORENSIC_H
#define EXECBUFFER2_FORENSIC_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ioctl.h>
#include <drm/i915_drm.h>

/**
 * EXECBUFFER2 FORENSIC ENGINE - C578
 * 
 * Objectif: Tracer et analyser soumissions EXECBUFFER2 complètes
 * Niveau: Forensic dynamique GPU runtime
 * 
 * Fonctionnalités:
 * - Capture paramètres EXECBUFFER2 complets
 * - Décodage exec_objects avec relocations
 * - Validation allocations GTT
 * - Corrélation timeline CPU/GPU
 * - Analyse causale soumission → exécution
 */

// ============================================================================
// STRUCTURES FORENSIC
// ============================================================================

/**
 * Paramètres EXECBUFFER2 décodés
 */
typedef struct {
    // Paramètres batch
    uint32_t buffer_count;
    uint32_t batch_start_offset;
    uint32_t batch_len;
    uint64_t flags;
    
    // Context et ring
    uint64_t context_id;
    uint32_t ring_id;
    
    // Cliprects (legacy)
    uint32_t num_cliprects;
    uint64_t cliprects_ptr;
    
    // DR registers (legacy)
    uint64_t DR1;
    uint64_t DR4;
    
    // Batch buffer handle
    uint32_t batch_buffer_handle;
    
    // Timeline (Gen12+)
    uint32_t exec_queue_id;
    uint64_t timeline_value;
    
    // Timestamps
    uint64_t timestamp_submit_ns;
    uint64_t timestamp_ioctl_start_ns;
    uint64_t timestamp_ioctl_end_ns;
    
    // Résultat
    int32_t ioctl_result;
    int32_t errno_value;
} execbuffer2_params_t;

/**
 * Exec Object décodé (buffer soumis)
 */
typedef struct {
    uint32_t handle;
    uint32_t relocation_count;
    uint64_t relocs_ptr;
    uint64_t alignment;
    uint64_t offset;  // GTT offset (OUT)
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
    
    // Informations additionnelles
    uint32_t size;
    bool is_batch;
    bool is_input;
    bool is_output;
    bool needs_relocation;
    bool gtt_allocated;
    
    // Forensic
    char name[64];
} exec_object2_decoded_t;

/**
 * Relocation décodée
 */
typedef struct {
    uint32_t target_handle;
    uint32_t delta;
    uint64_t offset;
    uint32_t read_domains;
    uint32_t write_domain;
    
    // Validation
    bool is_valid;
    bool was_applied;
    uint64_t resolved_address;
    
    // Forensic
    char description[128];
} relocation_decoded_t;

/**
 * Résultat forensic EXECBUFFER2
 */
typedef struct {
    // Paramètres
    execbuffer2_params_t params;
    
    // Buffers
    exec_object2_decoded_t *objects;
    uint32_t num_objects;
    
    // Relocations (toutes)
    relocation_decoded_t *relocations;
    uint32_t num_relocations;
    
    // Allocations GTT
    uint32_t buffers_allocated_gtt;
    uint64_t *gtt_offsets;
    
    // Timeline
    uint64_t latency_ioctl_ns;
    uint64_t latency_cpu_gpu_ns;
    uint64_t latency_gpu_execution_ns;
    
    // Seqno
    uint32_t seqno_before;
    uint32_t seqno_after;
    
    // Résultat
    bool success;
    char error_msg[256];
    
    // Statistiques
    uint32_t total_batch_size;
    uint32_t total_input_size;
    uint32_t total_output_size;
    uint32_t relocations_applied;
} execbuffer2_result_t;

/**
 * Configuration traçage EXECBUFFER2
 */
typedef struct {
    bool trace_params;
    bool trace_objects;
    bool trace_relocations;
    bool trace_gtt_allocations;
    bool trace_timeline;
    bool validate_relocations;
    bool capture_batch_content;
    bool capture_seqno;
    
    // Sortie
    FILE *log_file;
    bool wireshark_format;
} execbuffer2_trace_config_t;

// ============================================================================
// API PUBLIQUE
// ============================================================================

/**
 * Initialiser configuration traçage par défaut
 */
void execbuffer2_trace_config_init(execbuffer2_trace_config_t *config);

/**
 * Tracer soumission EXECBUFFER2 complète
 * 
 * @param drm_fd File descriptor DRM
 * @param args Arguments EXECBUFFER2
 * @param config Configuration traçage
 * @param result Résultat forensic (OUT)
 * @return true si succès
 */
bool trace_execbuffer2_submit(int drm_fd,
                              struct drm_i915_gem_execbuffer2 *args,
                              const execbuffer2_trace_config_t *config,
                              execbuffer2_result_t *result);

/**
 * Décoder exec_objects
 * 
 * @param objects Tableau exec_objects
 * @param count Nombre d'objets
 * @param decoded Tableau décodé (OUT)
 * @return true si succès
 */
bool decode_exec_objects(struct drm_i915_gem_exec_object2 *objects,
                        uint32_t count,
                        exec_object2_decoded_t *decoded);

/**
 * Décoder relocations d'un buffer
 * 
 * @param relocs Tableau relocations
 * @param count Nombre de relocations
 * @param decoded Tableau décodé (OUT)
 * @return true si succès
 */
bool decode_relocations(struct drm_i915_gem_relocation_entry *relocs,
                       uint32_t count,
                       relocation_decoded_t *decoded);

/**
 * Valider que relocations ont été appliquées
 * 
 * @param drm_fd File descriptor DRM
 * @param batch_handle Handle batch buffer
 * @param batch_size Taille batch
 * @param relocs Relocations attendues
 * @param reloc_count Nombre de relocations
 * @return true si toutes appliquées
 */
bool validate_relocations_applied(int drm_fd,
                                 uint32_t batch_handle,
                                 uint32_t batch_size,
                                 relocation_decoded_t *relocs,
                                 uint32_t reloc_count);

/**
 * Capturer allocations GTT de tous les buffers
 * 
 * @param drm_fd File descriptor DRM
 * @param objects Exec objects après EXECBUFFER2
 * @param count Nombre d'objets
 * @param gtt_offsets Offsets GTT (OUT)
 * @return Nombre de buffers alloués GTT
 */
uint32_t capture_gtt_allocations(int drm_fd,
                                 struct drm_i915_gem_exec_object2 *objects,
                                 uint32_t count,
                                 uint64_t *gtt_offsets);

/**
 * Capturer seqno avant/après soumission
 * 
 * @param drm_fd File descriptor DRM
 * @param ring_id Ring ID (0=RCS)
 * @param seqno_before Seqno avant (OUT)
 * @param seqno_after Seqno après (OUT)
 * @return true si succès
 */
bool capture_seqno(int drm_fd,
                  uint32_t ring_id,
                  uint32_t *seqno_before,
                  uint32_t *seqno_after);

/**
 * Corréler timeline CPU/GPU
 * 
 * @param result Résultat forensic
 * @param cpu_submit_ns Timestamp CPU submit
 * @param cpu_complete_ns Timestamp CPU complete
 * @return true si succès
 */
bool correlate_cpu_gpu_timeline(execbuffer2_result_t *result,
                               uint64_t cpu_submit_ns,
                               uint64_t cpu_complete_ns);

/**
 * Afficher résultat forensic format Wireshark
 * 
 * @param result Résultat forensic
 * @param fp File pointer sortie
 */
void print_execbuffer2_wireshark(const execbuffer2_result_t *result, FILE *fp);

/**
 * Afficher résultat forensic format humain
 * 
 * @param result Résultat forensic
 * @param fp File pointer sortie
 */
void print_execbuffer2_human(const execbuffer2_result_t *result, FILE *fp);

/**
 * Analyser échec EXECBUFFER2
 * 
 * @param result Résultat forensic
 * @param analysis Buffer analyse (OUT)
 * @param analysis_size Taille buffer
 */
void analyze_execbuffer2_failure(const execbuffer2_result_t *result,
                                char *analysis,
                                size_t analysis_size);

/**
 * Libérer résultat forensic
 * 
 * @param result Résultat à libérer
 */
void free_execbuffer2_result(execbuffer2_result_t *result);

// ============================================================================
// UTILITAIRES
// ============================================================================

/**
 * Obtenir timestamp nanoseconde
 */
static inline uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * Convertir flags exec_object en string
 */
const char* exec_object_flags_to_string(uint64_t flags);

/**
 * Convertir ring ID en nom
 */
const char* ring_id_to_name(uint32_t ring_id);

/**
 * Convertir read/write domains en string
 */
const char* domain_to_string(uint32_t domain);

#endif // EXECBUFFER2_FORENSIC_H

// Made with Bob
