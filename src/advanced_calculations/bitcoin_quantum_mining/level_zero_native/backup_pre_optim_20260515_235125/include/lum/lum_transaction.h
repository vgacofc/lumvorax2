/*
 * LUM DBMS — Transactions ACID (Phase 4)
 * Gestion complète des transactions avec isolation et durabilité
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 * Cycle : C143
 * Date : 2026-04-30
 */

#ifndef LUM_TRANSACTION_H
#define LUM_TRANSACTION_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/* Niveaux d'isolation */
typedef enum {
    LUM_ISOLATION_READ_UNCOMMITTED = 0,
    LUM_ISOLATION_READ_COMMITTED   = 1,
    LUM_ISOLATION_REPEATABLE_READ  = 2,
    LUM_ISOLATION_SERIALIZABLE     = 3
} lum_isolation_level_t;

/* États de transaction */
typedef enum {
    LUM_TX_STATE_IDLE = 0,
    LUM_TX_STATE_ACTIVE,
    LUM_TX_STATE_PREPARING,
    LUM_TX_STATE_COMMITTED,
    LUM_TX_STATE_ABORTED
} lum_tx_state_t;

/* Opération dans le WAL */
typedef enum {
    LUM_WAL_OP_INSERT = 0,
    LUM_WAL_OP_UPDATE = 1,
    LUM_WAL_OP_DELETE = 2,
    LUM_WAL_OP_BEGIN  = 3,
    LUM_WAL_OP_COMMIT = 4,
    LUM_WAL_OP_ABORT  = 5
} lum_wal_op_type_t;

/* Entrée Write-Ahead Log */
typedef struct {
    uint64_t lsn;                   /* Log Sequence Number */
    uint64_t tx_id;                 /* Transaction ID */
    uint64_t timestamp_ns;          /* Timestamp */
    lum_wal_op_type_t op_type;      /* Type d'opération */
    char table_name[64];            /* Nom de la table */
    uint64_t row_id;                /* ID de la ligne */
    void* old_data;                 /* Données avant (UPDATE/DELETE) */
    void* new_data;                 /* Données après (INSERT/UPDATE) */
    size_t data_size;               /* Taille des données */
    uint32_t crc32c;                /* Intégrité */
} lum_wal_entry_t;

/* Verrou (lock) */
typedef struct lum_lock {
    uint64_t tx_id;                 /* Transaction propriétaire */
    char table_name[64];            /* Table verrouillée */
    uint64_t row_id;                /* Ligne verrouillée (0 = table entière) */
    bool is_exclusive;              /* Verrou exclusif (write) ou partagé (read) */
    struct lum_lock* next;          /* Chaînage */
} lum_lock_t;

/* Transaction */
typedef struct {
    uint64_t tx_id;                 /* ID unique */
    lum_tx_state_t state;           /* État actuel */
    lum_isolation_level_t isolation;/* Niveau d'isolation */
    uint64_t start_ts_ns;           /* Timestamp début */
    uint64_t commit_ts_ns;          /* Timestamp commit */
    lum_wal_entry_t* wal_entries;   /* Entrées WAL */
    uint32_t num_wal_entries;       /* Nombre d'entrées */
    uint32_t wal_capacity;          /* Capacité buffer WAL */
    lum_lock_t* locks;              /* Verrous détenus */
    pthread_t thread_id;            /* Thread propriétaire */
} lum_transaction_t;

/* Gestionnaire de transactions */
typedef struct {
    lum_transaction_t** transactions; /* Table de hash des transactions */
    uint32_t max_transactions;      /* Nombre max de transactions */
    uint64_t next_tx_id;            /* Prochain ID de transaction */
    uint64_t next_lsn;              /* Prochain LSN */
    pthread_mutex_t lock;           /* Verrou global */
    int wal_fd;                     /* File descriptor WAL */
    char wal_path[256];             /* Chemin fichier WAL */
    bool enable_wal;                /* WAL activé */
} lum_tx_manager_t;

/* API publique */

/**
 * Initialise le gestionnaire de transactions
 * @param max_transactions Nombre max de transactions simultanées
 * @param wal_path Chemin du fichier WAL (NULL = désactivé)
 * @return Gestionnaire alloué ou NULL si erreur
 */
lum_tx_manager_t* lum_tx_manager_init(uint32_t max_transactions, const char* wal_path);

/**
 * Démarre une nouvelle transaction
 * @param manager Gestionnaire de transactions
 * @param isolation Niveau d'isolation
 * @return Transaction allouée ou NULL si erreur
 */
lum_transaction_t* lum_tx_begin(lum_tx_manager_t* manager, lum_isolation_level_t isolation);

/**
 * Commit une transaction
 * @param manager Gestionnaire de transactions
 * @param tx Transaction à committer
 * @return 0 si succès, -1 si erreur
 */
int lum_tx_commit(lum_tx_manager_t* manager, lum_transaction_t* tx);

/**
 * Rollback une transaction
 * @param manager Gestionnaire de transactions
 * @param tx Transaction à annuler
 * @return 0 si succès, -1 si erreur
 */
int lum_tx_rollback(lum_tx_manager_t* manager, lum_transaction_t* tx);

/**
 * Enregistre une opération dans le WAL
 * @param manager Gestionnaire de transactions
 * @param tx Transaction
 * @param op_type Type d'opération
 * @param table_name Nom de la table
 * @param row_id ID de la ligne
 * @param old_data Données avant (peut être NULL)
 * @param new_data Données après (peut être NULL)
 * @param data_size Taille des données
 * @return 0 si succès, -1 si erreur
 */
int lum_tx_log_operation(lum_tx_manager_t* manager, lum_transaction_t* tx,
                         lum_wal_op_type_t op_type, const char* table_name,
                         uint64_t row_id, const void* old_data, 
                         const void* new_data, size_t data_size);

/**
 * Acquiert un verrou
 * @param manager Gestionnaire de transactions
 * @param tx Transaction
 * @param table_name Nom de la table
 * @param row_id ID de la ligne (0 = table entière)
 * @param exclusive true = verrou exclusif, false = partagé
 * @return 0 si succès, -1 si erreur (deadlock ou timeout)
 */
int lum_tx_acquire_lock(lum_tx_manager_t* manager, lum_transaction_t* tx,
                        const char* table_name, uint64_t row_id, bool exclusive);

/**
 * Libère tous les verrous d'une transaction
 * @param manager Gestionnaire de transactions
 * @param tx Transaction
 * @return Nombre de verrous libérés
 */
int lum_tx_release_locks(lum_tx_manager_t* manager, lum_transaction_t* tx);

/**
 * Détecte les deadlocks
 * @param manager Gestionnaire de transactions
 * @return ID de la transaction à avorter, 0 si pas de deadlock
 */
uint64_t lum_tx_detect_deadlock(lum_tx_manager_t* manager);

/**
 * Récupère depuis le WAL (crash recovery)
 * @param manager Gestionnaire de transactions
 * @return Nombre de transactions récupérées, -1 si erreur
 */
int lum_tx_recover_from_wal(lum_tx_manager_t* manager);

/**
 * Flush le WAL sur disque
 * @param manager Gestionnaire de transactions
 * @return 0 si succès, -1 si erreur
 */
int lum_tx_flush_wal(lum_tx_manager_t* manager);

/**
 * Obtient les statistiques des transactions
 * @param manager Gestionnaire de transactions
 * @param active_count Pointeur pour recevoir le nombre de transactions actives
 * @param committed_count Pointeur pour recevoir le nombre de commits
 * @param aborted_count Pointeur pour recevoir le nombre d'aborts
 * @return 0 si succès, -1 si erreur
 */
int lum_tx_get_stats(lum_tx_manager_t* manager, uint32_t* active_count,
                     uint64_t* committed_count, uint64_t* aborted_count);

/**
 * Libère les ressources du gestionnaire
 * @param manager Gestionnaire de transactions
 */
void lum_tx_manager_destroy(lum_tx_manager_t* manager);

/* Macros utilitaires */
#define LUM_TX_TIMEOUT_MS 5000  /* Timeout acquisition verrou */
#define LUM_TX_MAX_RETRIES 3    /* Nombre max de retries deadlock */

#endif /* LUM_TRANSACTION_H */

// Made with Bob
