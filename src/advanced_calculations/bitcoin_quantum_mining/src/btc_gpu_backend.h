/**
 * @file btc_gpu_backend.h
 * @brief Interface abstraite unifiée pour backends GPU (OpenCL et Level Zero)
 * 
 * Ce fichier définit une interface abstraite permettant de supporter
 * plusieurs backends GPU de manière transparente :
 * - OpenCL (legacy, à éliminer progressivement)
 * - Level Zero (natif Intel Gen9, objectif 0% dépendance OpenCL)
 * 
 * @author Bob (LumVorax AI Agent)
 * @date 2026-05-12
 * @version 1.0
 * @cycle C220
 */

#ifndef BTC_GPU_BACKEND_H
#define BTC_GPU_BACKEND_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * TYPES ET STRUCTURES
 * ======================================================================== */

/**
 * @brief Type de backend GPU supporté
 */
typedef enum {
    BTC_GPU_BACKEND_OPENCL = 0,    /**< Backend OpenCL (legacy) */
    BTC_GPU_BACKEND_LEVELZERO = 1, /**< Backend Level Zero (natif) */
    BTC_GPU_BACKEND_AUTO = 2       /**< Détection automatique */
} btc_gpu_backend_type_t;

/**
 * @brief Handle opaque pour un contexte GPU backend
 * 
 * Ce handle encapsule soit un contexte OpenCL, soit un contexte Level Zero.
 * L'implémentation concrète est cachée dans les fichiers .c respectifs.
 */
typedef struct btc_gpu_backend_context btc_gpu_backend_context_t;

/**
 * @brief Handle opaque pour un buffer GPU
 */
typedef struct btc_gpu_buffer btc_gpu_buffer_t;

/**
 * @brief Handle opaque pour un kernel GPU
 */
typedef struct btc_gpu_kernel btc_gpu_kernel_t;

/**
 * @brief Handle opaque pour un événement GPU (profiling)
 */
typedef struct btc_gpu_event btc_gpu_event_t;

/**
 * @brief Statistiques GPU unifiées (indépendantes du backend)
 */
typedef struct {
    /* Métriques performance */
    uint64_t total_hashes;           /**< Total hashes calculés */
    uint64_t kernel_exec_ns;         /**< Temps exécution kernel (ns) */
    uint64_t gpu_queue_wait_ns;      /**< Temps attente queue (ns) */
    uint64_t gpu_transfer_ns;        /**< Temps transfert/setup (ns) */
    uint64_t gpu_total_pipeline_ns;  /**< Temps pipeline complet (ns) */
    
    /* Métriques utilisation */
    double gpu_utilization_pct;      /**< Utilisation GPU (%) */
    double hashrate_mhs;             /**< Hashrate instantané (MH/s) */
    
    /* Métriques mémoire */
    uint64_t memory_allocated_bytes; /**< Mémoire GPU allouée (bytes) */
    uint64_t memory_transferred_bytes; /**< Données transférées (bytes) */
    
    /* Compteurs */
    uint64_t kernel_launches;        /**< Nombre de lancements kernel */
    uint64_t errors;                 /**< Nombre d'erreurs */
} btc_gpu_stats_t;

/**
 * @brief Configuration backend GPU
 */
typedef struct {
    btc_gpu_backend_type_t backend_type; /**< Type de backend */
    uint32_t device_id;                  /**< ID device GPU (0 = défaut) */
    uint32_t work_group_size;            /**< Taille work-group (0 = auto) */
    bool enable_profiling;               /**< Activer profiling hardware */
    bool enable_forensic;                /**< Activer logs forensiques */
    const char *kernel_source_path;      /**< Chemin source kernel (optionnel) */
    const uint8_t *kernel_bytecode;      /**< Bytecode pré-compilé (optionnel) */
    size_t kernel_bytecode_size;         /**< Taille bytecode (bytes) */
} btc_gpu_config_t;

/* ========================================================================
 * INTERFACE PUBLIQUE
 * ======================================================================== */

/**
 * @brief Initialiser un contexte GPU backend
 * 
 * Cette fonction initialise le backend GPU spécifié (OpenCL ou Level Zero)
 * et retourne un handle opaque vers le contexte.
 * 
 * @param config Configuration backend
 * @param[out] ctx_out Handle contexte créé (NULL en cas d'erreur)
 * @return 0 si succès, code erreur sinon
 * 
 * @note Si backend_type = BTC_GPU_BACKEND_AUTO, détecte automatiquement
 *       le meilleur backend disponible (Level Zero prioritaire)
 * 
 * @example
 * btc_gpu_config_t config = {
 *     .backend_type = BTC_GPU_BACKEND_LEVELZERO,
 *     .device_id = 0,
 *     .work_group_size = 256,
 *     .enable_profiling = true,
 *     .enable_forensic = true,
 *     .kernel_bytecode = g_sha256_gen9_bytecode,
 *     .kernel_bytecode_size = sizeof(g_sha256_gen9_bytecode)
 * };
 * btc_gpu_backend_context_t *ctx = NULL;
 * int ret = btc_gpu_backend_init(&config, &ctx);
 */
int btc_gpu_backend_init(const btc_gpu_config_t *config, 
                         btc_gpu_backend_context_t **ctx_out);

/**
 * @brief Détruire un contexte GPU backend
 * 
 * @param ctx Handle contexte à détruire
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_backend_destroy(btc_gpu_backend_context_t *ctx);

/**
 * @brief Allouer un buffer GPU
 * 
 * @param ctx Handle contexte
 * @param size_bytes Taille buffer (bytes)
 * @param[out] buffer_out Handle buffer créé
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_buffer_alloc(btc_gpu_backend_context_t *ctx,
                         size_t size_bytes,
                         btc_gpu_buffer_t **buffer_out);

/**
 * @brief Libérer un buffer GPU
 * 
 * @param buffer Handle buffer à libérer
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_buffer_free(btc_gpu_buffer_t *buffer);

/**
 * @brief Écrire des données dans un buffer GPU
 * 
 * @param buffer Handle buffer destination
 * @param data Données source (CPU)
 * @param size_bytes Taille données (bytes)
 * @param blocking true = bloquant, false = asynchrone
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_buffer_write(btc_gpu_buffer_t *buffer,
                         const void *data,
                         size_t size_bytes,
                         bool blocking);

/**
 * @brief Lire des données depuis un buffer GPU
 * 
 * @param buffer Handle buffer source
 * @param[out] data Buffer destination (CPU)
 * @param size_bytes Taille données (bytes)
 * @param blocking true = bloquant, false = asynchrone
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_buffer_read(btc_gpu_buffer_t *buffer,
                        void *data,
                        size_t size_bytes,
                        bool blocking);

/**
 * @brief Créer un kernel GPU
 * 
 * @param ctx Handle contexte
 * @param kernel_name Nom du kernel (ex: "btc_sha256_mining")
 * @param[out] kernel_out Handle kernel créé
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_kernel_create(btc_gpu_backend_context_t *ctx,
                          const char *kernel_name,
                          btc_gpu_kernel_t **kernel_out);

/**
 * @brief Détruire un kernel GPU
 * 
 * @param kernel Handle kernel à détruire
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_kernel_destroy(btc_gpu_kernel_t *kernel);

/**
 * @brief Définir un argument kernel (buffer)
 * 
 * @param kernel Handle kernel
 * @param arg_index Index argument (0-based)
 * @param buffer Handle buffer
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_kernel_set_arg_buffer(btc_gpu_kernel_t *kernel,
                                  uint32_t arg_index,
                                  btc_gpu_buffer_t *buffer);

/**
 * @brief Définir un argument kernel (valeur scalaire)
 * 
 * @param kernel Handle kernel
 * @param arg_index Index argument (0-based)
 * @param value Pointeur vers valeur
 * @param size_bytes Taille valeur (bytes)
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_kernel_set_arg_value(btc_gpu_kernel_t *kernel,
                                 uint32_t arg_index,
                                 const void *value,
                                 size_t size_bytes);

/**
 * @brief Lancer un kernel GPU
 * 
 * @param kernel Handle kernel
 * @param global_work_size Taille globale (nombre total work-items)
 * @param local_work_size Taille locale (work-items par work-group, 0 = auto)
 * @param[out] event_out Handle événement (NULL si profiling désactivé)
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_kernel_launch(btc_gpu_kernel_t *kernel,
                          size_t global_work_size,
                          size_t local_work_size,
                          btc_gpu_event_t **event_out);

/**
 * @brief Attendre la fin d'un événement GPU
 * 
 * @param event Handle événement
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_event_wait(btc_gpu_event_t *event);

/**
 * @brief Obtenir les timestamps profiling d'un événement
 * 
 * @param event Handle événement
 * @param[out] queued_ns Timestamp queued (ns depuis epoch)
 * @param[out] submit_ns Timestamp submit (ns depuis epoch)
 * @param[out] start_ns Timestamp start (ns depuis epoch)
 * @param[out] end_ns Timestamp end (ns depuis epoch)
 * @return 0 si succès, code erreur sinon
 * 
 * @note Retourne 0 pour tous les timestamps si profiling désactivé
 */
int btc_gpu_event_get_profiling(btc_gpu_event_t *event,
                                uint64_t *queued_ns,
                                uint64_t *submit_ns,
                                uint64_t *start_ns,
                                uint64_t *end_ns);

/**
 * @brief Libérer un événement GPU
 * 
 * @param event Handle événement à libérer
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_event_release(btc_gpu_event_t *event);

/**
 * @brief Obtenir les statistiques GPU
 * 
 * @param ctx Handle contexte
 * @param[out] stats Statistiques GPU
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_backend_get_stats(btc_gpu_backend_context_t *ctx,
                              btc_gpu_stats_t *stats);

/**
 * @brief Réinitialiser les statistiques GPU
 * 
 * @param ctx Handle contexte
 * @return 0 si succès, code erreur sinon
 */
int btc_gpu_backend_reset_stats(btc_gpu_backend_context_t *ctx);

/**
 * @brief Obtenir le nom du backend actif
 * 
 * @param ctx Handle contexte
 * @return Nom du backend ("OpenCL", "Level Zero", ou "Unknown")
 */
const char* btc_gpu_backend_get_name(btc_gpu_backend_context_t *ctx);

/**
 * @brief Obtenir le type de backend actif
 * 
 * @param ctx Handle contexte
 * @return Type de backend
 */
btc_gpu_backend_type_t btc_gpu_backend_get_type(btc_gpu_backend_context_t *ctx);

/* ========================================================================
 * UTILITAIRES
 * ======================================================================== */

/**
 * @brief Détecter le meilleur backend disponible
 * 
 * @return Type de backend détecté (Level Zero prioritaire)
 * 
 * @note Ordre de priorité :
 *       1. Level Zero (si disponible)
 *       2. OpenCL (fallback)
 */
btc_gpu_backend_type_t btc_gpu_backend_detect(void);

/**
 * @brief Obtenir le nom d'un type de backend
 * 
 * @param type Type de backend
 * @return Nom du backend
 */
const char* btc_gpu_backend_type_to_string(btc_gpu_backend_type_t type);

/**
 * @brief Parser une variable d'environnement pour obtenir le type de backend
 * 
 * @param env_value Valeur variable (ex: "opencl", "levelzero", "auto")
 * @return Type de backend correspondant
 * 
 * @note Valeurs acceptées (case-insensitive) :
 *       - "opencl", "cl" → BTC_GPU_BACKEND_OPENCL
 *       - "levelzero", "l0", "ze" → BTC_GPU_BACKEND_LEVELZERO
 *       - "auto", NULL, "" → BTC_GPU_BACKEND_AUTO
 */
btc_gpu_backend_type_t btc_gpu_backend_parse_env(const char *env_value);

#ifdef __cplusplus
}
#endif

#endif /* BTC_GPU_BACKEND_H */

// Made with Bob
