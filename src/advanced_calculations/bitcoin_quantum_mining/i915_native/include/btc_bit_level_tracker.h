/**
 * LumVorax C198 Phase 15U — Bit-Level Tracker Header
 * 
 * Traçabilité bit-level complète pour i915 DRM native
 * Format .lum : Header + Événements binaires
 * 
 * Objectif : Tracker TOUTES les opérations GPU/RAM au niveau bit
 */

#ifndef BTC_BIT_LEVEL_TRACKER_H
#define BTC_BIT_LEVEL_TRACKER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// ═══════════════════════════════════════════════════════════════════════════
// CONSTANTES FORMAT .LUM
// ═══════════════════════════════════════════════════════════════════════════

#define BTC_LUM_MAGIC           0x4C554D00  // "LUM\0"
#define BTC_LUM_VERSION         1
#define BTC_LUM_HEADER_SIZE     64          // bytes
#define BTC_LUM_MAX_PATH        256         // bytes

// ═══════════════════════════════════════════════════════════════════════════
// TYPES D'ÉVÉNEMENTS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    BTC_LUM_EVENT_GEM_CREATE    = 0x01,  // Création buffer GEM
    BTC_LUM_EVENT_GEM_CLOSE     = 0x02,  // Destruction buffer GEM
    BTC_LUM_EVENT_GPU_MMAP      = 0x03,  // Mapping mémoire GPU
    BTC_LUM_EVENT_GPU_MUNMAP    = 0x04,  // Unmapping mémoire GPU
    BTC_LUM_EVENT_BATCH_WRITE   = 0x05,  // Écriture batch buffer
    BTC_LUM_EVENT_EXECBUFFER2   = 0x06,  // Soumission GPU
    BTC_LUM_EVENT_GPU_WAIT      = 0x07,  // Attente completion
    BTC_LUM_EVENT_CTX_CREATE    = 0x08,  // Création contexte
    BTC_LUM_EVENT_CTX_DESTROY   = 0x09,  // Destruction contexte
    BTC_LUM_EVENT_RAM_ALLOC     = 0x0A,  // Allocation RAM (malloc)
    BTC_LUM_EVENT_RAM_FREE      = 0x0B,  // Libération RAM (free)
    BTC_LUM_EVENT_IOCTL_CALL    = 0x0C,  // Appel ioctl()
    BTC_LUM_EVENT_IOCTL_RESULT  = 0x0D,  // Résultat ioctl()
    BTC_LUM_EVENT_ERROR         = 0xFF   // Erreur critique
} btc_lum_event_type;

// ═══════════════════════════════════════════════════════════════════════════
// STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Header fichier .lum (64 bytes)
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;              // 0x4C554D00 ("LUM\0")
    uint32_t version;            // Version format (1)
    uint64_t timestamp_start;    // Timestamp début (nanosec)
    uint64_t timestamp_end;      // Timestamp fin (nanosec)
    char module[16];             // Nom module ("i915_native")
    char arch[16];               // Architecture ("Gen9")
    uint32_t event_count;        // Nombre événements
    uint32_t reserved;           // Réservé (padding)
} btc_lum_header;

/**
 * Événement générique (taille variable)
 */
typedef struct __attribute__((packed)) {
    uint64_t timestamp;          // Timestamp événement (nanosec)
    uint8_t event_type;          // Type événement (btc_lum_event_type)
    uint8_t reserved[3];         // Padding
    uint32_t data_size;          // Taille données (bytes)
    // Suivi de data_size bytes de données spécifiques
} btc_lum_event_header;

/**
 * Données GEM_CREATE
 */
typedef struct __attribute__((packed)) {
    uint32_t gem_handle;         // Handle GEM
    uint64_t size;               // Taille buffer (bytes)
    uint64_t gpu_addr;           // Adresse GPU virtuelle
} btc_lum_gem_create_data;

/**
 * Données GEM_CLOSE
 */
typedef struct __attribute__((packed)) {
    uint32_t gem_handle;         // Handle GEM
} btc_lum_gem_close_data;

/**
 * Données GPU_MMAP
 */
typedef struct __attribute__((packed)) {
    uint32_t gem_handle;         // Handle GEM
    uint64_t cpu_addr;           // Adresse CPU (mmap)
    uint64_t size;               // Taille mapping (bytes)
} btc_lum_gpu_mmap_data;

/**
 * Données GPU_MUNMAP
 */
typedef struct __attribute__((packed)) {
    uint64_t cpu_addr;           // Adresse CPU (munmap)
    uint64_t size;               // Taille unmapping (bytes)
} btc_lum_gpu_munmap_data;

/**
 * Données BATCH_WRITE
 */
typedef struct __attribute__((packed)) {
    uint32_t gem_handle;         // Handle GEM batch buffer
    uint64_t offset;             // Offset dans buffer
    uint32_t size;               // Taille données écrites
    // Suivi de 'size' bytes de données batch
} btc_lum_batch_write_data;

/**
 * Données EXECBUFFER2
 */
typedef struct __attribute__((packed)) {
    uint32_t ctx_id;             // ID contexte GPU
    uint32_t gem_handle;         // Handle GEM batch buffer
    uint64_t batch_offset;       // Offset batch dans buffer
    uint32_t batch_len;          // Longueur batch (bytes)
} btc_lum_execbuffer2_data;

/**
 * Données GPU_WAIT
 */
typedef struct __attribute__((packed)) {
    uint32_t ctx_id;             // ID contexte GPU
    uint64_t timeout_ns;         // Timeout (nanosec)
    int32_t result;              // Résultat (0=succès, errno si échec)
} btc_lum_gpu_wait_data;

/**
 * Données CTX_CREATE
 */
typedef struct __attribute__((packed)) {
    uint32_t ctx_id;             // ID contexte créé
} btc_lum_ctx_create_data;

/**
 * Données CTX_DESTROY
 */
typedef struct __attribute__((packed)) {
    uint32_t ctx_id;             // ID contexte détruit
} btc_lum_ctx_destroy_data;

/**
 * Données RAM_ALLOC
 */
typedef struct __attribute__((packed)) {
    uint64_t ptr;                // Adresse RAM allouée
    uint64_t size;               // Taille allocation (bytes)
    char location[64];           // Fichier:ligne
} btc_lum_ram_alloc_data;

/**
 * Données RAM_FREE
 */
typedef struct __attribute__((packed)) {
    uint64_t ptr;                // Adresse RAM libérée
} btc_lum_ram_free_data;

/**
 * Données IOCTL_CALL
 */
typedef struct __attribute__((packed)) {
    uint32_t request;            // Code ioctl (ex: DRM_IOCTL_I915_GEM_EXECBUFFER2)
    int32_t fd;                  // File descriptor
} btc_lum_ioctl_call_data;

/**
 * Données IOCTL_RESULT
 */
typedef struct __attribute__((packed)) {
    uint32_t request;            // Code ioctl
    int32_t result;              // Résultat (0=succès, -1=échec)
    int32_t errno_val;           // Valeur errno si échec
} btc_lum_ioctl_result_data;

/**
 * Données ERROR
 */
typedef struct __attribute__((packed)) {
    int32_t errno_val;           // Code erreur
    char message[128];           // Message erreur
} btc_lum_error_data;

/**
 * Contexte tracker
 */
typedef struct {
    int fd;                      // File descriptor fichier .lum
    char filepath[BTC_LUM_MAX_PATH]; // Chemin fichier
    btc_lum_header header;       // Header fichier
    uint32_t event_count;        // Compteur événements
    bool is_open;                // Fichier ouvert ?
    uint64_t start_time_ns;      // Timestamp début
} btc_lum_tracker;

// ═══════════════════════════════════════════════════════════════════════════
// API PUBLIQUE
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Créer tracker et ouvrir fichier .lum
 * 
 * @param filepath Chemin fichier .lum à créer
 * @return Pointeur tracker, NULL si échec
 */
btc_lum_tracker* btc_lum_create(const char *filepath);

/**
 * Fermer tracker et finaliser fichier .lum
 * 
 * @param tracker Pointeur tracker
 */
void btc_lum_close(btc_lum_tracker *tracker);

/**
 * Logger événement GEM_CREATE
 */
void btc_lum_log_gem_create(btc_lum_tracker *tracker, uint32_t gem_handle, 
                            uint64_t size, uint64_t gpu_addr);

/**
 * Logger événement GEM_CLOSE
 */
void btc_lum_log_gem_close(btc_lum_tracker *tracker, uint32_t gem_handle);

/**
 * Logger événement GPU_MMAP
 */
void btc_lum_log_gpu_mmap(btc_lum_tracker *tracker, uint32_t gem_handle,
                          uint64_t cpu_addr, uint64_t size);

/**
 * Logger événement GPU_MUNMAP
 */
void btc_lum_log_gpu_munmap(btc_lum_tracker *tracker, uint64_t cpu_addr, 
                            uint64_t size);

/**
 * Logger événement BATCH_WRITE
 */
void btc_lum_log_batch_write(btc_lum_tracker *tracker, uint32_t gem_handle,
                             uint64_t offset, const void *data, uint32_t size);

/**
 * Logger événement EXECBUFFER2
 */
void btc_lum_log_execbuffer2(btc_lum_tracker *tracker, uint32_t ctx_id,
                             uint32_t gem_handle, uint64_t batch_offset,
                             uint32_t batch_len);

/**
 * Logger événement GPU_WAIT
 */
void btc_lum_log_gpu_wait(btc_lum_tracker *tracker, uint32_t ctx_id,
                          uint64_t timeout_ns, int32_t result);

/**
 * Logger événement CTX_CREATE
 */
void btc_lum_log_ctx_create(btc_lum_tracker *tracker, uint32_t ctx_id);

/**
 * Logger événement CTX_DESTROY
 */
void btc_lum_log_ctx_destroy(btc_lum_tracker *tracker, uint32_t ctx_id);

/**
 * Logger événement RAM_ALLOC
 */
void btc_lum_log_ram_alloc(btc_lum_tracker *tracker, uint64_t ptr, 
                           uint64_t size, const char *location);

/**
 * Logger événement RAM_FREE
 */
void btc_lum_log_ram_free(btc_lum_tracker *tracker, uint64_t ptr);

/**
 * Logger événement IOCTL_CALL
 */
void btc_lum_log_ioctl_call(btc_lum_tracker *tracker, uint32_t request, 
                            int32_t fd);

/**
 * Logger événement IOCTL_RESULT
 */
void btc_lum_log_ioctl_result(btc_lum_tracker *tracker, uint32_t request,
                              int32_t result, int32_t errno_val);

/**
 * Logger événement ERROR
 */
void btc_lum_log_error(btc_lum_tracker *tracker, int32_t errno_val,
                       const char *message);

/**
 * Obtenir timestamp actuel (nanosec depuis epoch)
 */
uint64_t btc_lum_get_timestamp_ns(void);

#endif // BTC_BIT_LEVEL_TRACKER_H

// Made with Bob
