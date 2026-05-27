/*
 * LumVorax C256 - Multi-GPU Manager
 * Détection et gestion de plusieurs GPUs Intel Gen9
 * 
 * OBJECTIF: Distribuer le mining Bitcoin sur tous les GPUs disponibles
 * GAIN ESTIMÉ: Linéaire avec nombre de GPUs (2 GPUs = 2x hashrate)
 */

#ifndef BTC_MULTI_GPU_MANAGER_H
#define BTC_MULTI_GPU_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * CONSTANTES
 * ═══════════════════════════════════════════════════════════════════════════ */

#define MAX_GPUS 8                    /* Maximum 8 GPUs supportés */
#define DRM_DEVICE_PATH_MAX 256       /* Taille max chemin device */

/* ═══════════════════════════════════════════════════════════════════════════
 * STRUCTURES
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Information sur un GPU détecté
 */
typedef struct {
    char device_path[DRM_DEVICE_PATH_MAX];  /* /dev/dri/renderD128, etc. */
    int fd;                                  /* File descriptor (-1 si non ouvert) */
    char driver_name[32];                    /* "i915", etc. */
    int driver_major;                        /* Version driver */
    int driver_minor;
    int driver_patch;
    uint32_t device_id;                      /* PCI device ID (ex: 0x5916 pour UHD 620) */
    char device_name[128];                   /* Nom GPU */
    bool is_gen9;                            /* true si Gen9 (Skylake/Kabylake) */
    bool is_available;                       /* true si GPU utilisable */
    uint64_t total_hashes;                   /* Hashes calculés sur ce GPU */
    uint64_t total_time_ns;                  /* Temps GPU total */
    double current_hashrate_mhs;             /* Hashrate actuel (MH/s) */
} gpu_info_t;

/**
 * Contexte multi-GPU
 */
typedef struct {
    gpu_info_t gpus[MAX_GPUS];              /* Liste des GPUs */
    int gpu_count;                           /* Nombre de GPUs détectés */
    int active_gpu_count;                    /* Nombre de GPUs actifs */
    bool initialized;                        /* true si initialisé */
    
    /* Statistiques globales */
    uint64_t total_hashes_all_gpus;         /* Total hashes tous GPUs */
    uint64_t start_time_ns;                  /* Timestamp début */
    double global_hashrate_mhs;              /* Hashrate global (MH/s) */
} multi_gpu_context_t;

/**
 * Configuration dispatch multi-GPU
 */
typedef struct {
    uint32_t nonce_start;                    /* Nonce de départ global */
    uint32_t nonce_count;                    /* Nombre total de nonces */
    uint8_t block_header[80];                /* Bitcoin block header */
    bool enable_load_balancing;              /* Équilibrage charge dynamique */
} multi_gpu_dispatch_config_t;

/**
 * Résultat dispatch multi-GPU
 */
typedef struct {
    uint32_t best_nonce;                     /* Meilleur nonce trouvé */
    uint32_t leading_zeros;                  /* Nombre de zéros leading */
    int gpu_index;                           /* Index GPU qui a trouvé */
    uint64_t total_hashes;                   /* Total hashes calculés */
    double elapsed_sec;                      /* Temps écoulé */
    double hashrate_mhs;                     /* Hashrate global */
} multi_gpu_result_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * API PUBLIQUE
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialise le gestionnaire multi-GPU
 * Détecte tous les GPUs Intel disponibles dans /dev/dri/
 * 
 * @param ctx_out Contexte multi-GPU (alloué par fonction)
 * @return Nombre de GPUs détectés, -1 si erreur
 */
int btc_multi_gpu_init(multi_gpu_context_t** ctx_out);

/**
 * Obtient informations sur un GPU spécifique
 * 
 * @param ctx Contexte multi-GPU
 * @param gpu_index Index du GPU (0 à gpu_count-1)
 * @return Pointeur vers gpu_info_t, NULL si index invalide
 */
const gpu_info_t* btc_multi_gpu_get_info(const multi_gpu_context_t* ctx, int gpu_index);

/**
 * Active un GPU pour le mining
 * Ouvre le device DRM et initialise le contexte
 * 
 * @param ctx Contexte multi-GPU
 * @param gpu_index Index du GPU à activer
 * @return 0 si succès, -1 si erreur
 */
int btc_multi_gpu_activate(multi_gpu_context_t* ctx, int gpu_index);

/**
 * Désactive un GPU
 * Ferme le device DRM et libère les ressources
 * 
 * @param ctx Contexte multi-GPU
 * @param gpu_index Index du GPU à désactiver
 * @return 0 si succès, -1 si erreur
 */
int btc_multi_gpu_deactivate(multi_gpu_context_t* ctx, int gpu_index);

/**
 * Active tous les GPUs détectés
 * 
 * @param ctx Contexte multi-GPU
 * @return Nombre de GPUs activés, -1 si erreur
 */
int btc_multi_gpu_activate_all(multi_gpu_context_t* ctx);

/**
 * Exécute dispatch mining sur tous les GPUs actifs
 * Distribue automatiquement les nonces entre les GPUs
 * 
 * STRATÉGIE:
 * - GPU 0: nonces [start, start + count/N)
 * - GPU 1: nonces [start + count/N, start + 2*count/N)
 * - etc.
 * 
 * @param ctx Contexte multi-GPU
 * @param config Configuration dispatch
 * @param result Résultat (meilleur nonce trouvé)
 * @return 0 si succès, -1 si erreur
 */
int btc_multi_gpu_dispatch(multi_gpu_context_t* ctx,
                           const multi_gpu_dispatch_config_t* config,
                           multi_gpu_result_t* result);

/**
 * Obtient statistiques globales multi-GPU
 * 
 * @param ctx Contexte multi-GPU
 * @param total_hashes_out Total hashes tous GPUs
 * @param hashrate_mhs_out Hashrate global (MH/s)
 * @return 0 si succès, -1 si erreur
 */
int btc_multi_gpu_get_stats(const multi_gpu_context_t* ctx,
                            uint64_t* total_hashes_out,
                            double* hashrate_mhs_out);

/**
 * Affiche informations sur tous les GPUs détectés
 * 
 * @param ctx Contexte multi-GPU
 */
void btc_multi_gpu_print_info(const multi_gpu_context_t* ctx);

/**
 * Affiche statistiques de performance multi-GPU
 * 
 * @param ctx Contexte multi-GPU
 */
void btc_multi_gpu_print_stats(const multi_gpu_context_t* ctx);

/**
 * Libère le contexte multi-GPU
 * Ferme tous les GPUs actifs
 * 
 * @param ctx Contexte à libérer
 */
void btc_multi_gpu_cleanup(multi_gpu_context_t* ctx);

/* ═══════════════════════════════════════════════════════════════════════════
 * FONCTIONS UTILITAIRES
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Détecte tous les devices DRM dans /dev/dri/
 * 
 * @param devices_out Buffer pour stocker les chemins (ex: "/dev/dri/renderD128")
 * @param max_devices Taille max du buffer
 * @return Nombre de devices détectés
 */
int btc_multi_gpu_scan_devices(char devices[][DRM_DEVICE_PATH_MAX], int max_devices);

/**
 * Vérifie si un device DRM est un GPU Intel Gen9
 * 
 * @param device_path Chemin device (ex: "/dev/dri/renderD128")
 * @param device_id_out Device ID PCI (optionnel)
 * @return true si Gen9, false sinon
 */
bool btc_multi_gpu_is_gen9(const char* device_path, uint32_t* device_id_out);

#ifdef __cplusplus
}
#endif

#endif /* BTC_MULTI_GPU_MANAGER_H */

// Made with ❤️ by Bob - LumVorax C256

// Made with Bob
