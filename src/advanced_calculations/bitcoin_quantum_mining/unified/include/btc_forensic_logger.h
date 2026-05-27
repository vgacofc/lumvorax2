/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_forensic_logger.h — Logging Forensique Unifié (OpenCL + Level Zero + CPU)
 *
 * CYCLE C198 PHASE 2 — Unification Totale des Backends
 * STANDARD_NAMES.md v4.5 §M-BTC-UNIFIED-C198
 *
 * OBJECTIFS :
 * 1. Logging forensique bit-level UNIFIÉ pour tous les backends
 * 2. Format .lum avec CRC32C + SHA-256 (compatible Level Zero)
 * 3. Ring buffer lock-free asynchrone (compatible OpenCL)
 * 4. Traçabilité nanoseconde de TOUTES les opérations
 * 5. Zéro dépendance backend-spécifique
 *
 * COMPATIBILITÉ :
 * - OpenCL (Intel NEO, AMD ROCm, NVIDIA)
 * - Level Zero (Intel Gen9+, Xe Graphics, Arc)
 * - CPU natif (fallback sans GPU)
 * - Futurs backends (Vulkan, CUDA, Metal)
 */

#ifndef BTC_FORENSIC_LOGGER_H
#define BTC_FORENSIC_LOGGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 1 : CONFIGURATION ET CONSTANTES
 * ══════════════════════════════════════════════════════════════════════ */

/* Capacité ring buffer par défaut (1 batch GPU = 262K événements) */
#define BTC_FORENSIC_BUFFER_DEFAULT     262144
#define BTC_FORENSIC_BUFFER_MIN         1024
#define BTC_FORENSIC_BUFFER_MAX         1048576

/* Magic numbers pour validation intégrité */
#define BTC_FORENSIC_MAGIC_HEADER       0x4C554D46  /* "LUMF" */
#define BTC_FORENSIC_MAGIC_EVENT        0x4C554D45  /* "LUME" */
#define BTC_FORENSIC_MAGIC_FOOTER       0x4C554D54  /* "LUMT" */

/* Codes retour */
#define BTC_FORENSIC_OK                 0
#define BTC_FORENSIC_ERROR_INIT         -1
#define BTC_FORENSIC_ERROR_MEMORY       -2
#define BTC_FORENSIC_ERROR_IO           -3
#define BTC_FORENSIC_ERROR_FULL         -4
#define BTC_FORENSIC_ERROR_INVALID      -5

/* Types d'événements forensiques */
typedef enum {
    BTC_FORENSIC_EVENT_INIT = 0,
    BTC_FORENSIC_EVENT_DEVICE_DETECT,
    BTC_FORENSIC_EVENT_MEMORY_ALLOC,
    BTC_FORENSIC_EVENT_MEMORY_FREE,
    BTC_FORENSIC_EVENT_KERNEL_COMPILE,
    BTC_FORENSIC_EVENT_KERNEL_LOAD,
    BTC_FORENSIC_EVENT_BATCH_START,
    BTC_FORENSIC_EVENT_BATCH_END,
    BTC_FORENSIC_EVENT_HASH_FOUND,
    BTC_FORENSIC_EVENT_HASH_NEAR_MISS,
    BTC_FORENSIC_EVENT_ERROR,
    BTC_FORENSIC_EVENT_CLEANUP,
    BTC_FORENSIC_EVENT_CUSTOM
} btc_forensic_event_type_t;

/* Niveaux de sévérité */
typedef enum {
    BTC_FORENSIC_LEVEL_DEBUG = 0,
    BTC_FORENSIC_LEVEL_INFO,
    BTC_FORENSIC_LEVEL_WARNING,
    BTC_FORENSIC_LEVEL_ERROR,
    BTC_FORENSIC_LEVEL_CRITICAL
} btc_forensic_level_t;

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 2 : STRUCTURES DE TRAÇABILITÉ BIT-LEVEL
 * ══════════════════════════════════════════════════════════════════════ */

/* Timestamp haute précision (nanoseconde) */
typedef struct {
    uint64_t sec;           /* Secondes depuis epoch (CLOCK_REALTIME) */
    uint64_t nsec;          /* Nanosecondes */
    uint64_t monotonic;     /* Nanosecondes depuis boot (CLOCK_MONOTONIC) */
} btc_forensic_timestamp_t;

/* Événement forensique bit-level (128 bytes alignés cache) */
typedef struct __attribute__((aligned(128))) {
    uint32_t magic;                     /* BTC_FORENSIC_MAGIC_EVENT */
    uint32_t event_id;                  /* ID unique séquentiel */
    btc_forensic_timestamp_t timestamp; /* Timestamp nanoseconde */
    btc_forensic_event_type_t type;     /* Type d'événement */
    btc_forensic_level_t level;         /* Niveau de sévérité */
    uint32_t thread_id;                 /* TID du thread */
    uint32_t backend_id;                /* 0=CPU, 1=OpenCL, 2=LevelZero */
    char description[64];               /* Description courte */
    uint64_t data[4];                   /* Données contextuelles */
    uint32_t crc32c;                    /* CRC32C pour intégrité */
    uint32_t reserved;                  /* Padding pour alignement */
} btc_forensic_event_t;

/* Header fichier .lum (64 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t magic;                     /* BTC_FORENSIC_MAGIC_HEADER */
    uint32_t version;                   /* Version format (1) */
    uint64_t creation_time;             /* Timestamp création */
    uint32_t backend_id;                /* Backend utilisé */
    uint32_t batch_size;                /* Taille batch */
    char hostname[32];                  /* Nom machine */
    uint32_t reserved[2];               /* Réservé futur */
} btc_forensic_header_t;

/* Footer fichier .lum (40 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t magic;                     /* BTC_FORENSIC_MAGIC_FOOTER */
    uint64_t total_events;              /* Nombre total d'événements */
    uint64_t dropped_events;            /* Événements perdus */
    uint32_t crc32c;                    /* CRC32C de tout le fichier */
    uint8_t sha256[32];                 /* SHA-256 de tout le fichier */
} btc_forensic_footer_t;

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 3 : RING BUFFER LOCK-FREE
 * ══════════════════════════════════════════════════════════════════════ */

/* Ring buffer forensique (lock-free, thread-safe) */
typedef struct {
    btc_forensic_event_t* events;       /* Buffer circulaire */
    size_t capacity;                    /* Capacité totale */
    volatile size_t write_index;        /* Index écriture (atomique) */
    volatile size_t read_index;         /* Index lecture (atomique) */
    volatile uint64_t total_events;     /* Compteur total */
    volatile uint64_t dropped_events;   /* Événements perdus */
    char log_file[256];                 /* Fichier .lum de sortie */
    int fd;                             /* File descriptor */
    bool async_mode;                    /* Mode asynchrone activé */
    void* flush_thread;                 /* pthread_t du thread flush */
    volatile bool shutdown;             /* Signal arrêt thread */
    uint32_t backend_id;                /* Backend actif */
} btc_forensic_buffer_t;

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 4 : API PUBLIQUE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Initialise le système de logging forensique
 * 
 * @param log_path Chemin fichier .lum (NULL = logs/forensic/btc_unified.lum)
 * @param capacity Capacité ring buffer (0 = défaut 262144)
 * @param async_mode true = thread flush asynchrone, false = flush synchrone
 * @param backend_id 0=CPU, 1=OpenCL, 2=LevelZero
 * @return BTC_FORENSIC_OK ou code erreur
 */
int btc_forensic_init(const char* log_path, size_t capacity, 
                      bool async_mode, uint32_t backend_id);

/**
 * Log un événement forensique (non-bloquant si buffer non plein)
 * 
 * @param type Type d'événement
 * @param level Niveau de sévérité
 * @param description Description courte (max 63 chars)
 * @param data Données contextuelles (4 × uint64_t)
 * @return BTC_FORENSIC_OK ou BTC_FORENSIC_ERROR_FULL
 */
int btc_forensic_log(btc_forensic_event_type_t type,
                     btc_forensic_level_t level,
                     const char* description,
                     const uint64_t data[4]);

/**
 * Log un événement avec timestamp custom (pour replay)
 * 
 * @param timestamp Timestamp à utiliser
 * @param type Type d'événement
 * @param level Niveau de sévérité
 * @param description Description courte
 * @param data Données contextuelles
 * @return BTC_FORENSIC_OK ou code erreur
 */
int btc_forensic_log_with_timestamp(btc_forensic_timestamp_t timestamp,
                                     btc_forensic_event_type_t type,
                                     btc_forensic_level_t level,
                                     const char* description,
                                     const uint64_t data[4]);

/**
 * Flush le buffer vers le fichier .lum (bloquant)
 * 
 * @return Nombre d'événements écrits ou code erreur négatif
 */
int btc_forensic_flush(void);

/**
 * Arrêt propre du système de logging
 * - Flush buffer final
 * - Écriture footer .lum
 * - Join thread asynchrone si actif
 * - Libération mémoire
 */
void btc_forensic_shutdown(void);

/**
 * Obtient un timestamp haute précision
 * 
 * @return Timestamp nanoseconde (REALTIME + MONOTONIC)
 */
btc_forensic_timestamp_t btc_forensic_get_timestamp(void);

/**
 * Calcule différence entre timestamps (nanosecondes)
 * 
 * @param start Timestamp début
 * @param end Timestamp fin
 * @return Différence en nanosecondes
 */
uint64_t btc_forensic_timestamp_diff_ns(btc_forensic_timestamp_t start,
                                         btc_forensic_timestamp_t end);

/**
 * Nombre d'événements en attente dans le buffer
 * 
 * @return Nombre d'événements non flushés
 */
size_t btc_forensic_pending(void);

/**
 * Statistiques du buffer forensique
 * 
 * @param total_events Pointeur vers compteur total (peut être NULL)
 * @param dropped_events Pointeur vers compteur perdus (peut être NULL)
 * @param pending_events Pointeur vers compteur en attente (peut être NULL)
 */
void btc_forensic_stats(uint64_t* total_events,
                        uint64_t* dropped_events,
                        size_t* pending_events);

/**
 * Valide l'intégrité d'un fichier .lum
 * 
 * @param log_path Chemin fichier .lum
 * @return true si intégrité OK, false sinon
 */
bool btc_forensic_validate_file(const char* log_path);

/**
 * Convertit type événement en string
 * 
 * @param type Type d'événement
 * @return String lisible
 */
const char* btc_forensic_event_type_string(btc_forensic_event_type_t type);

/**
 * Convertit niveau en string
 * 
 * @param level Niveau de sévérité
 * @return String lisible
 */
const char* btc_forensic_level_string(btc_forensic_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* BTC_FORENSIC_LOGGER_H */

// Made with Bob
