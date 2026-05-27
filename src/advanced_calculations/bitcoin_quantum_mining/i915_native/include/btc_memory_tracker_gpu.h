/**
 * @file btc_memory_tracker_gpu.h
 * @brief Memory Tracker RAM+GPU pour i915 Native Bitcoin Mining
 * 
 * PHASE 15W: Tracking complet des allocations mémoire RAM et GPU
 * 
 * Fonctionnalités:
 * - Tracking malloc/free (RAM)
 * - Tracking GEM create/close (GPU)
 * - Détection fuites mémoire
 * - Statistiques temps réel
 * - Export format .lum
 * 
 * @author LumVorax Team
 * @date 2026-05-11
 */

#ifndef BTC_MEMORY_TRACKER_GPU_H
#define BTC_MEMORY_TRACKER_GPU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES ET STRUCTURES
 * ============================================================================ */

/**
 * @brief Type d'allocation mémoire
 */
typedef enum {
    BTC_MEM_TYPE_RAM_MALLOC = 0,    /**< malloc() RAM */
    BTC_MEM_TYPE_RAM_CALLOC = 1,    /**< calloc() RAM */
    BTC_MEM_TYPE_RAM_REALLOC = 2,   /**< realloc() RAM */
    BTC_MEM_TYPE_GPU_GEM = 3,       /**< GEM buffer GPU */
    BTC_MEM_TYPE_GPU_MMAP = 4,      /**< mmap() GPU */
} btc_mem_type_t;

/**
 * @brief Entrée d'allocation mémoire
 */
typedef struct {
    void* ptr;                      /**< Pointeur alloué */
    size_t size;                    /**< Taille en bytes */
    btc_mem_type_t type;            /**< Type allocation */
    uint64_t timestamp_ns;          /**< Timestamp allocation (nanosec) */
    uint32_t gem_handle;            /**< Handle GEM (si GPU) */
    const char* file;               /**< Fichier source */
    int line;                       /**< Ligne source */
    bool freed;                     /**< Déjà libéré ? */
} btc_mem_entry_t;

/**
 * @brief Statistiques mémoire
 */
typedef struct {
    /* RAM */
    uint64_t ram_alloc_count;       /**< Nombre allocations RAM */
    uint64_t ram_free_count;        /**< Nombre libérations RAM */
    uint64_t ram_current_bytes;     /**< RAM actuellement allouée */
    uint64_t ram_peak_bytes;        /**< RAM pic */
    uint64_t ram_total_bytes;       /**< RAM totale allouée */
    
    /* GPU */
    uint64_t gpu_alloc_count;       /**< Nombre allocations GPU */
    uint64_t gpu_free_count;        /**< Nombre libérations GPU */
    uint64_t gpu_current_bytes;     /**< GPU actuellement allouée */
    uint64_t gpu_peak_bytes;        /**< GPU pic */
    uint64_t gpu_total_bytes;       /**< GPU totale allouée */
    
    /* Fuites */
    uint64_t ram_leaks_count;       /**< Nombre fuites RAM */
    uint64_t ram_leaks_bytes;       /**< Bytes fuites RAM */
    uint64_t gpu_leaks_count;       /**< Nombre fuites GPU */
    uint64_t gpu_leaks_bytes;       /**< Bytes fuites GPU */
} btc_mem_stats_t;

/**
 * @brief Tracker mémoire (opaque)
 */
typedef struct btc_mem_tracker btc_mem_tracker_t;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

/**
 * @brief Créer un tracker mémoire
 * 
 * @param max_entries Nombre max d'entrées (0 = illimité)
 * @return Tracker créé ou NULL si erreur
 */
btc_mem_tracker_t* btc_mem_tracker_create(size_t max_entries);

/**
 * @brief Détruire un tracker mémoire
 * 
 * @param tracker Tracker à détruire
 */
void btc_mem_tracker_destroy(btc_mem_tracker_t* tracker);

/**
 * @brief Enregistrer une allocation RAM
 * 
 * @param tracker Tracker
 * @param ptr Pointeur alloué
 * @param size Taille en bytes
 * @param type Type allocation (MALLOC/CALLOC/REALLOC)
 * @param file Fichier source
 * @param line Ligne source
 * @return true si succès, false si erreur
 */
bool btc_mem_track_alloc_ram(
    btc_mem_tracker_t* tracker,
    void* ptr,
    size_t size,
    btc_mem_type_t type,
    const char* file,
    int line
);

/**
 * @brief Enregistrer une libération RAM
 * 
 * @param tracker Tracker
 * @param ptr Pointeur libéré
 * @param file Fichier source
 * @param line Ligne source
 * @return true si succès, false si erreur (double free, ptr inconnu)
 */
bool btc_mem_track_free_ram(
    btc_mem_tracker_t* tracker,
    void* ptr,
    const char* file,
    int line
);

/**
 * @brief Enregistrer une allocation GPU (GEM)
 * 
 * @param tracker Tracker
 * @param gem_handle Handle GEM
 * @param size Taille en bytes
 * @param file Fichier source
 * @param line Ligne source
 * @return true si succès, false si erreur
 */
bool btc_mem_track_alloc_gpu(
    btc_mem_tracker_t* tracker,
    uint32_t gem_handle,
    size_t size,
    const char* file,
    int line
);

/**
 * @brief Enregistrer une libération GPU (GEM)
 * 
 * @param tracker Tracker
 * @param gem_handle Handle GEM
 * @param file Fichier source
 * @param line Ligne source
 * @return true si succès, false si erreur
 */
bool btc_mem_track_free_gpu(
    btc_mem_tracker_t* tracker,
    uint32_t gem_handle,
    const char* file,
    int line
);

/**
 * @brief Enregistrer un mapping GPU
 * 
 * @param tracker Tracker
 * @param ptr Pointeur mappé
 * @param gem_handle Handle GEM
 * @param size Taille en bytes
 * @param file Fichier source
 * @param line Ligne source
 * @return true si succès, false si erreur
 */
bool btc_mem_track_mmap_gpu(
    btc_mem_tracker_t* tracker,
    void* ptr,
    uint32_t gem_handle,
    size_t size,
    const char* file,
    int line
);

/**
 * @brief Enregistrer un unmapping GPU
 * 
 * @param tracker Tracker
 * @param ptr Pointeur unmappé
 * @param file Fichier source
 * @param line Ligne source
 * @return true si succès, false si erreur
 */
bool btc_mem_track_munmap_gpu(
    btc_mem_tracker_t* tracker,
    void* ptr,
    const char* file,
    int line
);

/**
 * @brief Obtenir les statistiques mémoire
 * 
 * @param tracker Tracker
 * @param stats Structure à remplir
 * @return true si succès, false si erreur
 */
bool btc_mem_get_stats(
    btc_mem_tracker_t* tracker,
    btc_mem_stats_t* stats
);

/**
 * @brief Détecter les fuites mémoire
 * 
 * @param tracker Tracker
 * @return Nombre de fuites détectées
 */
uint64_t btc_mem_detect_leaks(btc_mem_tracker_t* tracker);

/**
 * @brief Afficher les fuites mémoire
 * 
 * @param tracker Tracker
 * @param max_display Nombre max à afficher (0 = toutes)
 */
void btc_mem_print_leaks(btc_mem_tracker_t* tracker, size_t max_display);

/**
 * @brief Afficher les statistiques mémoire
 * 
 * @param tracker Tracker
 */
void btc_mem_print_stats(btc_mem_tracker_t* tracker);

/**
 * @brief Exporter les données vers fichier .lum
 * 
 * @param tracker Tracker
 * @param lum_path Chemin fichier .lum
 * @return true si succès, false si erreur
 */
bool btc_mem_export_lum(
    btc_mem_tracker_t* tracker,
    const char* lum_path
);

/**
 * @brief Réinitialiser le tracker
 * 
 * @param tracker Tracker
 */
void btc_mem_reset(btc_mem_tracker_t* tracker);

/* ============================================================================
 * MACROS HELPER
 * ============================================================================ */

/**
 * @brief Macro pour malloc() tracké
 * 
 * Usage: void* ptr = BTC_MALLOC(tracker, 1024);
 */
#define BTC_MALLOC(tracker, size) \
    ({ \
        void* __ptr = malloc(size); \
        if (__ptr && tracker) { \
            btc_mem_track_alloc_ram(tracker, __ptr, size, BTC_MEM_TYPE_RAM_MALLOC, __FILE__, __LINE__); \
        } \
        __ptr; \
    })

/**
 * @brief Macro pour calloc() tracké
 * 
 * Usage: void* ptr = BTC_CALLOC(tracker, 10, sizeof(int));
 */
#define BTC_CALLOC(tracker, nmemb, size) \
    ({ \
        void* __ptr = calloc(nmemb, size); \
        if (__ptr && tracker) { \
            btc_mem_track_alloc_ram(tracker, __ptr, (nmemb) * (size), BTC_MEM_TYPE_RAM_CALLOC, __FILE__, __LINE__); \
        } \
        __ptr; \
    })

/**
 * @brief Macro pour realloc() tracké
 * 
 * Usage: ptr = BTC_REALLOC(tracker, ptr, 2048);
 */
#define BTC_REALLOC(tracker, ptr, size) \
    ({ \
        void* __old_ptr = (ptr); \
        void* __new_ptr = realloc(__old_ptr, size); \
        if (tracker) { \
            if (__old_ptr) btc_mem_track_free_ram(tracker, __old_ptr, __FILE__, __LINE__); \
            if (__new_ptr) btc_mem_track_alloc_ram(tracker, __new_ptr, size, BTC_MEM_TYPE_RAM_REALLOC, __FILE__, __LINE__); \
        } \
        __new_ptr; \
    })

/**
 * @brief Macro pour free() tracké
 * 
 * Usage: BTC_FREE(tracker, ptr);
 */
#define BTC_FREE(tracker, ptr) \
    do { \
        if (ptr) { \
            if (tracker) btc_mem_track_free_ram(tracker, ptr, __FILE__, __LINE__); \
            free(ptr); \
            ptr = NULL; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* BTC_MEMORY_TRACKER_GPU_H */

// Made with Bob
