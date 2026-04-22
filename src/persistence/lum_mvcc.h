/*
 * LumVorax — LUM DBMS Phase 4 — MVCC basique
 * lum_mvcc.h — Multi-Version Concurrency Control avec timestamps
 *
 * IMPLEMENTATION 100% NATIVE.
 * Chaque enregistrement a :
 *   xmin : timestamp de creation (version visible si xmin <= snapshot_ts)
 *   xmax : timestamp de suppression (version visible si xmax > snapshot_ts)
 *   key  : cle primaire
 *   data : donnees de la ligne
 *
 * Les transactions lisent un snapshot au moment de debut (snapshot_ts = now).
 * Une version est "visible" si : xmin <= snapshot_ts < xmax
 *
 * MVCC garantit :
 *   - Les lectures ne bloquent pas les ecritures
 *   - Les ecritures ne bloquent pas les lectures
 *   - Isolation niveau "Snapshot Isolation"
 *
 * Simplifications (vs PostgreSQL) :
 *   - Pas de vacuum automatique (versions mortes restent jusqu'a vacuum manuel)
 *   - Pas de row-level locks (mutex global par table)
 *   - Pas de 2PC (single node seulement)
 *
 * Cycle C69 — LUM DBMS Phase 4 — 2026-04-22
 */
#ifndef LUM_MVCC_H
#define LUM_MVCC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

/* Timestamp MVCC : nanosecondes depuis epoch */
typedef uint64_t lum_ts_t;

/* Valeur speciale "toujours visible" */
#define LUM_MVCC_INFINITY  UINT64_MAX
#define LUM_MVCC_BORN      0ULL

/* Taille max des donnees d'une ligne MVCC */
#define LUM_MVCC_MAX_DATA  256

/* Une version d'une ligne (tuple) */
typedef struct lum_mvcc_tuple {
    uint64_t               key;                     /* Cle primaire */
    lum_ts_t               xmin;                    /* Timestamp creation */
    lum_ts_t               xmax;                    /* Timestamp suppression (INFINITY = vivant) */
    uint32_t               data_size;               /* Taille des donnees */
    uint8_t                data[LUM_MVCC_MAX_DATA]; /* Donnees de la ligne */
    struct lum_mvcc_tuple* next_version;            /* Chaine des versions */
} lum_mvcc_tuple_t;

/* Table MVCC : liste chainee de tuples par cle */
typedef struct {
    lum_mvcc_tuple_t** buckets;  /* Hash table des tuples par cle */
    uint32_t           n_buckets;/* Nombre de buckets */
    uint64_t           n_tuples; /* Nombre de tuples vivants */
    uint64_t           n_dead;   /* Nombre de tuples morts (vacuum necessaire) */
    pthread_mutex_t    mutex;    /* Protection concurrence */
} lum_mvcc_store_t;

/* Contexte de transaction */
typedef struct {
    lum_ts_t  snapshot_ts; /* Timestamp snapshot au debut de la transaction */
    bool      aborted;     /* Transaction annulee ? */
    uint64_t  n_reads;     /* Nombre de lectures */
    uint64_t  n_writes;    /* Nombre d'ecritures */
} lum_mvcc_tx_t;

/* ── API publique ─────────────────────────────────────────────────── */

/* Cree un store MVCC pour N buckets */
lum_mvcc_store_t* lum_mvcc_create(uint32_t n_buckets);

/* Detruit le store et libere toute la memoire */
void lum_mvcc_destroy(lum_mvcc_store_t* store);

/* Commence une nouvelle transaction (capture le snapshot timestamp) */
lum_mvcc_tx_t lum_mvcc_begin(void);

/* Insere ou met a jour une cle dans la transaction (copy-on-write) */
bool lum_mvcc_insert(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx,
                     uint64_t key, const uint8_t* data, uint32_t data_size);

/* Lit une cle visible dans le snapshot de la transaction */
bool lum_mvcc_read(lum_mvcc_store_t* store, const lum_mvcc_tx_t* tx,
                   uint64_t key, uint8_t* data_out, uint32_t* size_out);

/* Supprime une cle (marque xmax = now) dans la transaction */
bool lum_mvcc_delete(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx, uint64_t key);

/* Commit la transaction (rend les ecritures visibles) */
bool lum_mvcc_commit(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx);

/* Annule la transaction (supprime les versions en cours) */
void lum_mvcc_abort(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx);

/* Vacuum : supprime les tuples morts (xmax != INFINITY et anciens) */
uint64_t lum_mvcc_vacuum(lum_mvcc_store_t* store, lum_ts_t older_than);

/* Compte les tuples vivants */
uint64_t lum_mvcc_count_live(lum_mvcc_store_t* store);

/* Stats du store */
void lum_mvcc_stats(const lum_mvcc_store_t* store,
                    uint64_t* live, uint64_t* dead);

#endif /* LUM_MVCC_H */
