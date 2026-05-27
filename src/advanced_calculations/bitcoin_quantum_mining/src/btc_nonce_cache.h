/*
 * LumVorax C256 - Nonce Cache System
 * Sauvegarde et récupération des nonces trouvés
 * 
 * OBJECTIF: Améliorer résilience en cas de crash
 * - Sauvegarder nonces trouvés en temps réel
 * - Éviter recalcul en cas de redémarrage
 * - Checkpoint automatique tous les N dispatches
 */

#ifndef BTC_NONCE_CACHE_H
#define BTC_NONCE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * CONSTANTES
 * ═══════════════════════════════════════════════════════════════════════════ */

#define NONCE_CACHE_MAGIC 0x4E4F4E43  /* "NONC" */
#define NONCE_CACHE_VERSION 1
#define MAX_CACHED_NONCES 10000       /* Maximum nonces en cache */
#define CACHE_CHECKPOINT_INTERVAL 100 /* Checkpoint tous les 100 dispatches */

/* ═══════════════════════════════════════════════════════════════════════════
 * STRUCTURES
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Entrée cache nonce
 */
typedef struct {
    uint32_t nonce;                   /* Nonce trouvé */
    uint32_t leading_zeros;           /* Nombre de zéros leading */
    uint8_t hash[32];                 /* Hash SHA-256 résultant */
    uint8_t block_header[80];         /* Block header Bitcoin */
    uint64_t timestamp_ns;            /* Timestamp découverte */
    uint32_t dispatch_id;             /* ID dispatch qui a trouvé */
    int gpu_index;                    /* Index GPU (-1 si CPU) */
} nonce_cache_entry_t;

/**
 * Header fichier cache
 */
typedef struct {
    uint32_t magic;                   /* NONCE_CACHE_MAGIC */
    uint32_t version;                 /* NONCE_CACHE_VERSION */
    uint32_t entry_count;             /* Nombre d'entrées */
    uint32_t max_entries;             /* Capacité max */
    uint64_t creation_time_ns;        /* Timestamp création */
    uint64_t last_update_ns;          /* Timestamp dernière MAJ */
    uint32_t total_dispatches;        /* Total dispatches effectués */
    uint64_t total_hashes;            /* Total hashes calculés */
    uint32_t checksum;                /* Checksum CRC32 */
    uint8_t reserved[64];             /* Réservé futur */
} nonce_cache_header_t;

/**
 * Contexte cache nonces
 */
typedef struct {
    char cache_file_path[256];        /* Chemin fichier cache */
    FILE* cache_file;                 /* File handle */
    nonce_cache_header_t header;      /* Header cache */
    nonce_cache_entry_t* entries;     /* Entrées en mémoire */
    uint32_t entry_count;             /* Nombre d'entrées actuelles */
    uint32_t max_entries;             /* Capacité max */
    bool auto_checkpoint;             /* Checkpoint automatique */
    uint32_t checkpoint_interval;     /* Intervalle checkpoint */
    uint32_t dispatches_since_checkpoint; /* Dispatches depuis dernier checkpoint */
    bool initialized;                 /* true si initialisé */
} nonce_cache_context_t;

/**
 * Configuration cache
 */
typedef struct {
    const char* cache_file_path;      /* Chemin fichier cache */
    uint32_t max_entries;             /* Capacité max (0 = défaut) */
    bool auto_checkpoint;             /* Activer checkpoint auto */
    uint32_t checkpoint_interval;     /* Intervalle checkpoint (0 = défaut) */
    bool load_existing;               /* Charger cache existant */
} nonce_cache_config_t;

/**
 * Statistiques cache
 */
typedef struct {
    uint32_t total_entries;           /* Total entrées */
    uint32_t total_dispatches;        /* Total dispatches */
    uint64_t total_hashes;            /* Total hashes */
    uint32_t best_leading_zeros;      /* Meilleur résultat */
    uint32_t best_nonce;              /* Meilleur nonce */
    uint64_t cache_size_bytes;        /* Taille cache (bytes) */
    uint64_t last_checkpoint_ns;      /* Timestamp dernier checkpoint */
} nonce_cache_stats_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * API PUBLIQUE
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialise le système de cache
 * 
 * @param ctx_out Contexte cache (alloué par fonction)
 * @param config Configuration
 * @return 0 si succès, -1 si erreur
 */
int btc_nonce_cache_init(nonce_cache_context_t** ctx_out,
                         const nonce_cache_config_t* config);

/**
 * Ajoute un nonce trouvé au cache
 * 
 * @param ctx Contexte cache
 * @param entry Entrée à ajouter
 * @return 0 si succès, -1 si erreur
 */
int btc_nonce_cache_add(nonce_cache_context_t* ctx,
                        const nonce_cache_entry_t* entry);

/**
 * Recherche un nonce dans le cache
 * 
 * @param ctx Contexte cache
 * @param nonce Nonce à rechercher
 * @param entry_out Entrée trouvée (optionnel)
 * @return true si trouvé, false sinon
 */
bool btc_nonce_cache_find(const nonce_cache_context_t* ctx,
                          uint32_t nonce,
                          nonce_cache_entry_t* entry_out);

/**
 * Vérifie si une plage de nonces a déjà été calculée
 * 
 * @param ctx Contexte cache
 * @param nonce_start Nonce début
 * @param nonce_count Nombre de nonces
 * @return true si déjà calculé, false sinon
 */
bool btc_nonce_cache_is_range_computed(const nonce_cache_context_t* ctx,
                                       uint32_t nonce_start,
                                       uint32_t nonce_count);

/**
 * Enregistre un dispatch effectué
 * Incrémente compteur et déclenche checkpoint si nécessaire
 * 
 * @param ctx Contexte cache
 * @param nonce_start Nonce début
 * @param nonce_count Nombre de nonces
 * @return 0 si succès, -1 si erreur
 */
int btc_nonce_cache_register_dispatch(nonce_cache_context_t* ctx,
                                      uint32_t nonce_start,
                                      uint32_t nonce_count);

/**
 * Force un checkpoint (sauvegarde sur disque)
 * 
 * @param ctx Contexte cache
 * @return 0 si succès, -1 si erreur
 */
int btc_nonce_cache_checkpoint(nonce_cache_context_t* ctx);

/**
 * Charge un cache existant depuis le disque
 * 
 * @param ctx Contexte cache
 * @return Nombre d'entrées chargées, -1 si erreur
 */
int btc_nonce_cache_load(nonce_cache_context_t* ctx);

/**
 * Obtient statistiques du cache
 * 
 * @param ctx Contexte cache
 * @param stats_out Statistiques
 * @return 0 si succès, -1 si erreur
 */
int btc_nonce_cache_get_stats(const nonce_cache_context_t* ctx,
                              nonce_cache_stats_t* stats_out);

/**
 * Obtient le meilleur nonce du cache
 * 
 * @param ctx Contexte cache
 * @param entry_out Meilleure entrée
 * @return 0 si succès, -1 si erreur ou cache vide
 */
int btc_nonce_cache_get_best(const nonce_cache_context_t* ctx,
                             nonce_cache_entry_t* entry_out);

/**
 * Exporte le cache au format JSON
 * 
 * @param ctx Contexte cache
 * @param json_path Chemin fichier JSON
 * @return 0 si succès, -1 si erreur
 */
int btc_nonce_cache_export_json(const nonce_cache_context_t* ctx,
                                const char* json_path);

/**
 * Affiche statistiques du cache
 * 
 * @param ctx Contexte cache
 */
void btc_nonce_cache_print_stats(const nonce_cache_context_t* ctx);

/**
 * Affiche les N meilleurs nonces
 * 
 * @param ctx Contexte cache
 * @param top_n Nombre de nonces à afficher
 */
void btc_nonce_cache_print_top(const nonce_cache_context_t* ctx, int top_n);

/**
 * Libère le contexte cache
 * Sauvegarde automatiquement avant fermeture
 * 
 * @param ctx Contexte à libérer
 */
void btc_nonce_cache_cleanup(nonce_cache_context_t* ctx);

/* ═══════════════════════════════════════════════════════════════════════════
 * UTILITAIRES
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Calcule checksum CRC32 d'un buffer
 * 
 * @param data Buffer
 * @param size Taille
 * @return Checksum CRC32
 */
uint32_t btc_nonce_cache_crc32(const void* data, size_t size);

/**
 * Vérifie intégrité du fichier cache
 * 
 * @param cache_path Chemin fichier cache
 * @return true si intègre, false sinon
 */
bool btc_nonce_cache_verify_integrity(const char* cache_path);

#ifdef __cplusplus
}
#endif

#endif /* BTC_NONCE_CACHE_H */

// Made with ❤️ by Bob - LumVorax C256

// Made with Bob
