/*
 * LumVorax — LUM DBMS Phase 4 — MVCC natif
 * lum_mvcc.c — Multi-Version Concurrency Control avec Snapshot Isolation
 */
#define _POSIX_C_SOURCE 200809L
/*
 *
 * Cycle C69 — LUM DBMS Phase 4 — 2026-04-22
 */

#include "lum_mvcc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

/* ── Timestamp nanoseconde ──────────────────────────────────────── */
static lum_ts_t _now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (lum_ts_t)ts.tv_sec * 1000000000ULL + (lum_ts_t)ts.tv_nsec;
}

/* ── Hash de la cle pour bucket ─────────────────────────────────── */
static uint32_t _key_bucket(uint64_t key, uint32_t n_buckets) {
    /* FNV-1a 64 bits */
    uint64_t h = 14695981039346656037ULL;
    h ^= (key & 0xFF); h *= 1099511628211ULL;
    h ^= (key >> 8  ) & 0xFF; h *= 1099511628211ULL;
    h ^= (key >> 16 ) & 0xFF; h *= 1099511628211ULL;
    h ^= (key >> 24 ) & 0xFF; h *= 1099511628211ULL;
    return (uint32_t)(h % (uint64_t)n_buckets);
}

/* ── Creation / destruction ─────────────────────────────────────── */

lum_mvcc_store_t* lum_mvcc_create(uint32_t n_buckets) {
    if (n_buckets == 0) n_buckets = 1024;
    lum_mvcc_store_t* s = (lum_mvcc_store_t*)calloc(1, sizeof(lum_mvcc_store_t));
    if (!s) return NULL;
    s->buckets   = (lum_mvcc_tuple_t**)calloc(n_buckets, sizeof(lum_mvcc_tuple_t*));
    if (!s->buckets) { free(s); return NULL; }
    s->n_buckets = n_buckets;
    s->n_tuples  = 0;
    s->n_dead    = 0;
    pthread_mutex_init(&s->mutex, NULL);
    return s;
}

static void _free_chain(lum_mvcc_tuple_t* t) {
    while (t) {
        lum_mvcc_tuple_t* next = t->next_version;
        free(t);
        t = next;
    }
}

void lum_mvcc_destroy(lum_mvcc_store_t* store) {
    if (!store) return;
    pthread_mutex_lock(&store->mutex);
    for (uint32_t i = 0; i < store->n_buckets; i++)
        _free_chain(store->buckets[i]);
    free(store->buckets);
    pthread_mutex_unlock(&store->mutex);
    pthread_mutex_destroy(&store->mutex);
    free(store);
}

/* ── Transaction ────────────────────────────────────────────────── */

lum_mvcc_tx_t lum_mvcc_begin(void) {
    lum_mvcc_tx_t tx;
    tx.snapshot_ts = _now_ns();
    tx.aborted     = false;
    tx.n_reads     = 0;
    tx.n_writes    = 0;
    return tx;
}

/* ── Visibilite d'un tuple depuis un snapshot ───────────────────── */
/* Un tuple est visible si :
 *   xmin <= snapshot_ts  (cree avant le snapshot)
 *   xmax > snapshot_ts   (pas encore supprime au moment du snapshot)
 * OU en attente de commit (xmin == snapshot_ts = "cree dans cette tx") */
static bool _is_visible(const lum_mvcc_tuple_t* t, lum_ts_t snapshot_ts) {
    return (t->xmin <= snapshot_ts) && (t->xmax > snapshot_ts);
}

/* ── Insertion / mise a jour ───────────────────────────────────── */

bool lum_mvcc_insert(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx,
                     uint64_t key, const uint8_t* data, uint32_t data_size) {
    if (!store || !tx || tx->aborted) return false;
    if (data_size > LUM_MVCC_MAX_DATA) data_size = LUM_MVCC_MAX_DATA;

    pthread_mutex_lock(&store->mutex);
    lum_ts_t now = _now_ns();

    uint32_t bkt = _key_bucket(key, store->n_buckets);
    /* Si la cle existe et est vivante : marquer xmax pour "mise a jour" */
    lum_mvcc_tuple_t* cur = store->buckets[bkt];
    while (cur) {
        if (cur->key == key && cur->xmax == LUM_MVCC_INFINITY) {
            cur->xmax = now;  /* Invalider l'ancienne version */
            store->n_dead++;
            break;
        }
        cur = cur->next_version;
    }

    /* Creer la nouvelle version */
    lum_mvcc_tuple_t* t = (lum_mvcc_tuple_t*)calloc(1, sizeof(lum_mvcc_tuple_t));
    if (!t) { pthread_mutex_unlock(&store->mutex); return false; }
    t->key       = key;
    t->xmin      = now;
    t->xmax      = LUM_MVCC_INFINITY;
    t->data_size = data_size;
    if (data && data_size > 0) memcpy(t->data, data, data_size);
    /* Inserer en tete du bucket */
    t->next_version          = store->buckets[bkt];
    store->buckets[bkt]      = t;
    store->n_tuples++;
    tx->n_writes++;

    pthread_mutex_unlock(&store->mutex);
    return true;
}

/* ── Lecture ────────────────────────────────────────────────────── */

bool lum_mvcc_read(lum_mvcc_store_t* store, const lum_mvcc_tx_t* tx,
                   uint64_t key, uint8_t* data_out, uint32_t* size_out) {
    if (!store || !tx || tx->aborted) return false;

    pthread_mutex_lock(&store->mutex);
    uint32_t bkt = _key_bucket(key, store->n_buckets);
    lum_mvcc_tuple_t* cur = store->buckets[bkt];
    while (cur) {
        if (cur->key == key && _is_visible(cur, tx->snapshot_ts)) {
            if (data_out && size_out) {
                uint32_t sz = cur->data_size < *size_out ? cur->data_size : *size_out;
                memcpy(data_out, cur->data, sz);
                *size_out = cur->data_size;
            }
            pthread_mutex_unlock(&store->mutex);
            ((lum_mvcc_tx_t*)tx)->n_reads++;
            return true;
        }
        cur = cur->next_version;
    }
    pthread_mutex_unlock(&store->mutex);
    return false;
}

/* ── Suppression ────────────────────────────────────────────────── */

bool lum_mvcc_delete(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx, uint64_t key) {
    if (!store || !tx || tx->aborted) return false;

    pthread_mutex_lock(&store->mutex);
    lum_ts_t now = _now_ns();
    uint32_t bkt = _key_bucket(key, store->n_buckets);
    lum_mvcc_tuple_t* cur = store->buckets[bkt];
    bool found = false;
    while (cur) {
        if (cur->key == key && cur->xmax == LUM_MVCC_INFINITY) {
            cur->xmax = now;
            store->n_dead++;
            found = true;
            tx->n_writes++;
            break;
        }
        cur = cur->next_version;
    }
    pthread_mutex_unlock(&store->mutex);
    return found;
}

/* ── Commit / Abort ─────────────────────────────────────────────── */

bool lum_mvcc_commit(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx) {
    (void)store;
    if (!tx || tx->aborted) return false;
    /* Dans cette implementation simplifiee, le commit est immediat
     * (les ecritures sont visibles des l'insertion avec xmin=now).
     * Une implementation complete aurait un log de commit 2PC. */
    return true;
}

void lum_mvcc_abort(lum_mvcc_store_t* store, lum_mvcc_tx_t* tx) {
    if (!store || !tx) return;
    tx->aborted = true;
    /* Annuler les ecritures de cette transaction : marquer xmax = 0 */
    /* Note : dans une impl complete, on trackait les tuples modifies.
     * Ici on fait un vacuum des tuples crees apres tx->snapshot_ts. */
    pthread_mutex_lock(&store->mutex);
    for (uint32_t i = 0; i < store->n_buckets; i++) {
        lum_mvcc_tuple_t* cur = store->buckets[i];
        while (cur) {
            /* Si cree apres le snapshot : invalider */
            if (cur->xmin > tx->snapshot_ts && cur->xmax == LUM_MVCC_INFINITY) {
                cur->xmax = 0; /* Marque comme annule */
                store->n_dead++;
            }
            cur = cur->next_version;
        }
    }
    pthread_mutex_unlock(&store->mutex);
}

/* ── Vacuum ─────────────────────────────────────────────────────── */

uint64_t lum_mvcc_vacuum(lum_mvcc_store_t* store, lum_ts_t older_than) {
    if (!store) return 0;
    uint64_t freed = 0;
    pthread_mutex_lock(&store->mutex);
    for (uint32_t i = 0; i < store->n_buckets; i++) {
        lum_mvcc_tuple_t** prev = &store->buckets[i];
        lum_mvcc_tuple_t*  cur  = store->buckets[i];
        while (cur) {
            /* Supprimer si mort et plus ancien que older_than */
            bool dead = (cur->xmax != LUM_MVCC_INFINITY && cur->xmax < older_than);
            if (dead) {
                *prev = cur->next_version;
                free(cur);
                cur = *prev;
                freed++;
                if (store->n_dead > 0) store->n_dead--;
            } else {
                prev = &cur->next_version;
                cur  = cur->next_version;
            }
        }
    }
    pthread_mutex_unlock(&store->mutex);
    return freed;
}

uint64_t lum_mvcc_count_live(lum_mvcc_store_t* store) {
    if (!store) return 0;
    lum_ts_t now = _now_ns();
    uint64_t count = 0;
    pthread_mutex_lock(&store->mutex);
    for (uint32_t i = 0; i < store->n_buckets; i++) {
        lum_mvcc_tuple_t* cur = store->buckets[i];
        while (cur) {
            if (cur->xmin <= now && cur->xmax == LUM_MVCC_INFINITY) count++;
            cur = cur->next_version;
        }
    }
    pthread_mutex_unlock(&store->mutex);
    return count;
}

void lum_mvcc_stats(const lum_mvcc_store_t* store,
                    uint64_t* live, uint64_t* dead) {
    if (!store) return;
    if (live) *live = store->n_tuples;
    if (dead) *dead = store->n_dead;
}
