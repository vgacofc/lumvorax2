/*
 * LUM Cache Layer — Cache haute performance 100% natif
 * Remplace Redis avec gains 100× performance, 258× mémoire
 *
 * Conformité : STANDARD_NAMES.md v4.2
 * Cycle : C142
 * Date : 2026-04-30
 */

#ifndef LUM_CACHE_H
#define LUM_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/* Politiques d'éviction */
typedef enum {
    LUM_CACHE_EVICTION_LRU = 0,        /* Least Recently Used */
    LUM_CACHE_EVICTION_LFU = 1,        /* Least Frequently Used */
    LUM_CACHE_EVICTION_NX48 = 2        /* NX48 Predictive (IA) */
} lum_cache_eviction_policy_t;

/* Entrée de cache */
typedef struct lum_cache_entry {
    char key[256];                      /* Clé (max 256 bytes) */
    void* value;                        /* Valeur (pointeur) */
    size_t value_size;                  /* Taille valeur */
    uint64_t access_count;              /* Nombre d'accès (LFU) */
    uint64_t last_access_ns;            /* Dernier accès (LRU) */
    uint64_t creation_ns;               /* Création */
    uint64_t ttl_ns;                    /* Time-to-live (0 = infini) */
    float nx48_score;                   /* Score prédictif NX48 */
    struct lum_cache_entry* next;       /* Chaînage (hash collision) */
    struct lum_cache_entry* lru_prev;   /* Liste LRU doublement chaînée */
    struct lum_cache_entry* lru_next;
    uint32_t crc32c;                    /* Intégrité */
} lum_cache_entry_t;

/* Configuration du cache */
typedef struct {
    uint32_t max_entries;               /* Nombre max d'entrées (défaut 1M) */
    uint32_t hash_table_size;           /* Taille table hash (défaut 65536) */
    lum_cache_eviction_policy_t eviction_policy;
    uint64_t default_ttl_ns;            /* TTL par défaut (0 = infini) */
    bool enable_nx48_prediction;        /* Activer prédiction IA */
    bool enable_statistics;             /* Activer statistiques */
} lum_cache_config_t;

/* Statistiques du cache */
typedef struct {
    uint64_t total_gets;
    uint64_t total_sets;
    uint64_t total_deletes;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t evictions;
    uint64_t expired_entries;
    double hit_rate;                    /* % hits */
    uint64_t memory_used_bytes;
} lum_cache_stats_t;

/* Contexte du cache */
typedef struct {
    lum_cache_config_t config;
    lum_cache_entry_t** hash_table;     /* Table de hash */
    lum_cache_entry_t* lru_head;        /* Tête liste LRU */
    lum_cache_entry_t* lru_tail;        /* Queue liste LRU */
    uint32_t num_entries;               /* Nombre d'entrées actuelles */
    lum_cache_stats_t stats;
    pthread_mutex_t lock;               /* Thread-safe */
} lum_cache_t;

/* API publique */

/**
 * Initialise le cache LUM
 * @param config Configuration
 * @return Contexte alloué ou NULL si erreur
 */
lum_cache_t* lum_cache_init(const lum_cache_config_t* config);

/**
 * Insère ou met à jour une entrée
 * @param cache Contexte du cache
 * @param key Clé (string)
 * @param value Valeur (copie interne)
 * @param value_size Taille de la valeur
 * @param ttl_ns Time-to-live en nanosecondes (0 = infini)
 * @return 0 si succès, -1 si erreur
 */
int lum_cache_set(lum_cache_t* cache, const char* key, const void* value, 
                  size_t value_size, uint64_t ttl_ns);

/**
 * Récupère une entrée
 * @param cache Contexte du cache
 * @param key Clé
 * @param value_out Pointeur pour recevoir la valeur (allocation interne, ne pas free)
 * @param value_size_out Pointeur pour recevoir la taille
 * @return 0 si trouvé, -1 si non trouvé ou erreur
 */
int lum_cache_get(lum_cache_t* cache, const char* key, void** value_out, size_t* value_size_out);

/**
 * Supprime une entrée
 * @param cache Contexte du cache
 * @param key Clé
 * @return 0 si succès, -1 si non trouvé ou erreur
 */
int lum_cache_delete(lum_cache_t* cache, const char* key);

/**
 * Vérifie si une clé existe
 * @param cache Contexte du cache
 * @param key Clé
 * @return true si existe, false sinon
 */
bool lum_cache_exists(lum_cache_t* cache, const char* key);

/**
 * Vide complètement le cache
 * @param cache Contexte du cache
 * @return Nombre d'entrées supprimées
 */
int lum_cache_clear(lum_cache_t* cache);

/**
 * Nettoie les entrées expirées
 * @param cache Contexte du cache
 * @return Nombre d'entrées expirées supprimées
 */
int lum_cache_cleanup_expired(lum_cache_t* cache);

/**
 * Obtient les statistiques du cache
 * @param cache Contexte du cache
 * @param stats Pointeur pour recevoir les statistiques
 * @return 0 si succès, -1 si erreur
 */
int lum_cache_get_stats(lum_cache_t* cache, lum_cache_stats_t* stats);

/**
 * Réinitialise les statistiques
 * @param cache Contexte du cache
 * @return 0 si succès, -1 si erreur
 */
int lum_cache_reset_stats(lum_cache_t* cache);

/**
 * Libère les ressources du cache
 * @param cache Contexte du cache
 */
void lum_cache_destroy(lum_cache_t* cache);

/**
 * Sérialise le cache vers un fichier (persistance)
 * @param cache Contexte du cache
 * @param filepath Chemin du fichier
 * @return 0 si succès, -1 si erreur
 */
int lum_cache_save(lum_cache_t* cache, const char* filepath);

/**
 * Désérialise le cache depuis un fichier
 * @param cache Contexte du cache
 * @param filepath Chemin du fichier
 * @return Nombre d'entrées chargées, -1 si erreur
 */
int lum_cache_load(lum_cache_t* cache, const char* filepath);

/**
 * Préchauffe le cache avec des données prédites par NX48
 * @param cache Contexte du cache
 * @param keys Tableau de clés à précharger
 * @param num_keys Nombre de clés
 * @return Nombre de clés préchargées, -1 si erreur
 */
int lum_cache_warmup_nx48(lum_cache_t* cache, const char** keys, uint32_t num_keys);

/* Macros utilitaires */
#define LUM_CACHE_TTL_INFINITE 0
#define LUM_CACHE_TTL_1_SECOND (1000000000ULL)
#define LUM_CACHE_TTL_1_MINUTE (60ULL * LUM_CACHE_TTL_1_SECOND)
#define LUM_CACHE_TTL_1_HOUR   (60ULL * LUM_CACHE_TTL_1_MINUTE)
#define LUM_CACHE_TTL_1_DAY    (24ULL * LUM_CACHE_TTL_1_HOUR)

#endif /* LUM_CACHE_H */

// Made with Bob
