/*
 * LumVorax — LUM DBMS Phase 3 — Buffer Pool natif
 * lum_buffer_pool.c — Gestion de pages 8KB en memoire avec LRU
 */
#define _POSIX_C_SOURCE 200809L
/*
 *
 * Politique LRU (Least Recently Used) :
 *   Quand toutes les frames sont pleines et qu'une nouvelle page est demandee,
 *   on evicte la frame avec le plus petit last_access_tick (la moins recemment utilisee).
 *   Les pages "dirty" (modifiees) sont ecrites sur disque avant eviction.
 *   Les pages "pinnees" (pin_count > 0) ne peuvent jamais etre evictees.
 *
 * Cycle C69 — LUM DBMS Phase 3 — 2026-04-22
 */

#include "lum_buffer_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>

#define POOL_INVALID_PAGE UINT64_MAX

/* ── Utilitaires I/O ─────────────────────────────────────────────── */

/* Ecrit une page sur disque */
static bool _write_page(lum_buffer_pool_t* pool, lum_page_frame_t* frame) {
    if (frame->page_id == POOL_INVALID_PAGE) return false;
    off_t offset = (off_t)sizeof(lum_file_header_t)
                 + (off_t)frame->page_id * (off_t)LUM_PAGE_SIZE;
    ssize_t w = pwrite(pool->fd, frame->data, LUM_PAGE_SIZE, offset);
    if (w != (ssize_t)LUM_PAGE_SIZE) {
        fprintf(stderr, "[BUF_POOL] ERREUR ecriture page %"PRIu64": %s\n",
                frame->page_id, strerror(errno));
        return false;
    }
    pool->n_disk_writes++;
    frame->dirty = false;
    return true;
}

/* Lit une page depuis le disque */
static bool _read_page(lum_buffer_pool_t* pool, uint64_t page_id, uint8_t* buf) {
    off_t offset = (off_t)sizeof(lum_file_header_t)
                 + (off_t)page_id * (off_t)LUM_PAGE_SIZE;
    ssize_t r = pread(pool->fd, buf, LUM_PAGE_SIZE, offset);
    if (r == 0) {
        /* Page au-dela de la fin : initialiser a zero */
        memset(buf, 0, LUM_PAGE_SIZE);
        pool->n_disk_reads++;
        return true;
    }
    if (r != (ssize_t)LUM_PAGE_SIZE) {
        fprintf(stderr, "[BUF_POOL] ERREUR lecture page %"PRIu64" (r=%zd): %s\n",
                page_id, r, strerror(errno));
        return false;
    }
    pool->n_disk_reads++;
    return true;
}

/* ── Politique LRU : trouver une frame disponible ───────────────── */

/* Cherche une frame libre (page_id = POOL_INVALID_PAGE) */
static int _find_free_frame(lum_buffer_pool_t* pool) {
    for (uint32_t i = 0; i < pool->n_frames; i++) {
        if (pool->frames[i].page_id == POOL_INVALID_PAGE)
            return (int)i;
    }
    return -1;
}

/* Trouve la frame a evicter (LRU non-pinnee) */
static int _find_victim_frame(lum_buffer_pool_t* pool) {
    int    best_idx  = -1;
    uint64_t best_tick = UINT64_MAX;
    for (uint32_t i = 0; i < pool->n_frames; i++) {
        if (pool->frames[i].pin_count > 0) continue; /* pinnee, intouchable */
        if (pool->frames[i].last_access_tick < best_tick) {
            best_tick = pool->frames[i].last_access_tick;
            best_idx  = (int)i;
        }
    }
    return best_idx;
}

/* ── API publique ─────────────────────────────────────────────────── */

lum_buffer_pool_t* lum_pool_create(const char* path, uint32_t n_frames) {
    if (!path || n_frames == 0 || n_frames > LUM_POOL_MAX_PAGES)
        n_frames = 16;

    lum_buffer_pool_t* pool = (lum_buffer_pool_t*)calloc(1, sizeof(lum_buffer_pool_t));
    if (!pool) return NULL;

    /* Ouvrir ou creer le fichier */
    pool->fd = open(path, O_RDWR | O_CREAT, 0644);
    if (pool->fd < 0) {
        fprintf(stderr, "[BUF_POOL] ERREUR ouverture fichier %s: %s\n",
                path, strerror(errno));
        free(pool);
        return NULL;
    }

    strncpy(pool->path, path, sizeof(pool->path) - 1);
    pool->n_frames  = n_frames;
    pool->lru_clock = 1;
    pthread_mutex_init(&pool->mutex, NULL);

    /* Initialiser toutes les frames comme libres */
    for (uint32_t i = 0; i < n_frames; i++)
        pool->frames[i].page_id = POOL_INVALID_PAGE;

    /* Lire ou initialiser le header */
    struct stat st;
    fstat(pool->fd, &st);
    if (st.st_size < (off_t)sizeof(lum_file_header_t)) {
        /* Nouveau fichier : ecrire le header */
        lum_file_header_t hdr;
        memset(&hdr, 0, sizeof(hdr));
        hdr.magic    = 0x4C554D50u; /* "LUMP" */
        hdr.version  = 1;
        hdr.n_pages  = 0;
        hdr.root_page = POOL_INVALID_PAGE;
        hdr.free_list = POOL_INVALID_PAGE;
        if (pwrite(pool->fd, &hdr, sizeof(hdr), 0) != (ssize_t)sizeof(hdr)) {
            fprintf(stderr, "[BUF_POOL] ERREUR ecriture header: %s\n", strerror(errno));
        }
    }

    fprintf(stderr, "[BUF_POOL] Pool cree : %s (%u frames x %u bytes = %u KB)\n",
            path, n_frames, LUM_PAGE_SIZE, n_frames * LUM_PAGE_SIZE / 1024);
    return pool;
}

void lum_pool_destroy(lum_buffer_pool_t* pool) {
    if (!pool) return;
    pthread_mutex_lock(&pool->mutex);
    lum_pool_flush(pool); /* flush dirty pages */
    close(pool->fd);
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    free(pool);
}

uint8_t* lum_pool_pin(lum_buffer_pool_t* pool, uint64_t page_id) {
    if (!pool || page_id == POOL_INVALID_PAGE) return NULL;
    pthread_mutex_lock(&pool->mutex);

    /* Chercher la page dans le cache */
    for (uint32_t i = 0; i < pool->n_frames; i++) {
        if (pool->frames[i].page_id == page_id) {
            pool->frames[i].pin_count++;
            pool->frames[i].last_access_tick = ++pool->lru_clock;
            pool->n_hits++;
            pthread_mutex_unlock(&pool->mutex);
            return pool->frames[i].data;
        }
    }

    /* Cache miss : charger depuis le disque */
    pool->n_misses++;

    /* Trouver une frame libre ou a evicter */
    int frame_idx = _find_free_frame(pool);
    if (frame_idx < 0) {
        frame_idx = _find_victim_frame(pool);
        if (frame_idx < 0) {
            fprintf(stderr, "[BUF_POOL] ERREUR : toutes les frames pinnees !\n");
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }
        /* Evicter : si dirty, ecrire sur disque */
        if (pool->frames[frame_idx].dirty)
            _write_page(pool, &pool->frames[frame_idx]);
    }

    /* Charger la nouvelle page */
    pool->frames[frame_idx].page_id = page_id;
    pool->frames[frame_idx].dirty   = false;
    pool->frames[frame_idx].pin_count = 1;
    pool->frames[frame_idx].last_access_tick = ++pool->lru_clock;
    _read_page(pool, page_id, pool->frames[frame_idx].data);

    pthread_mutex_unlock(&pool->mutex);
    return pool->frames[frame_idx].data;
}

void lum_pool_unpin(lum_buffer_pool_t* pool, uint64_t page_id) {
    if (!pool) return;
    pthread_mutex_lock(&pool->mutex);
    for (uint32_t i = 0; i < pool->n_frames; i++) {
        if (pool->frames[i].page_id == page_id && pool->frames[i].pin_count > 0) {
            pool->frames[i].pin_count--;
            break;
        }
    }
    pthread_mutex_unlock(&pool->mutex);
}

void lum_pool_mark_dirty(lum_buffer_pool_t* pool, uint64_t page_id) {
    if (!pool) return;
    pthread_mutex_lock(&pool->mutex);
    for (uint32_t i = 0; i < pool->n_frames; i++) {
        if (pool->frames[i].page_id == page_id) {
            pool->frames[i].dirty = true;
            break;
        }
    }
    pthread_mutex_unlock(&pool->mutex);
}

uint64_t lum_pool_alloc_page(lum_buffer_pool_t* pool) {
    if (!pool) return POOL_INVALID_PAGE;
    pthread_mutex_lock(&pool->mutex);

    /* Lire le header pour connaitre le nombre de pages */
    lum_file_header_t hdr;
    if (pread(pool->fd, &hdr, sizeof(hdr), 0) != (ssize_t)sizeof(hdr)) {
        pthread_mutex_unlock(&pool->mutex);
        return POOL_INVALID_PAGE;
    }

    uint64_t new_page = hdr.n_pages;
    hdr.n_pages++;
    if (pwrite(pool->fd, &hdr, sizeof(hdr), 0) != (ssize_t)sizeof(hdr)) {
        pthread_mutex_unlock(&pool->mutex);
        return POOL_INVALID_PAGE;
    }

    /* Etendre le fichier avec une page vide */
    uint8_t zero_page[LUM_PAGE_SIZE] = {0};
    off_t offset = (off_t)sizeof(hdr) + (off_t)new_page * LUM_PAGE_SIZE;
    ssize_t written = pwrite(pool->fd, zero_page, LUM_PAGE_SIZE, offset);
    (void)written; /* Intentionally ignore return value - best effort write */

    pthread_mutex_unlock(&pool->mutex);
    return new_page;
}

bool lum_pool_flush(lum_buffer_pool_t* pool) {
    if (!pool) return false;
    bool ok = true;
    for (uint32_t i = 0; i < pool->n_frames; i++) {
        if (pool->frames[i].dirty && pool->frames[i].page_id != POOL_INVALID_PAGE)
            ok &= _write_page(pool, &pool->frames[i]);
    }
    fsync(pool->fd);
    return ok;
}

void lum_pool_stats(const lum_buffer_pool_t* pool,
                    uint64_t* hits, uint64_t* misses,
                    uint64_t* reads, uint64_t* writes) {
    if (!pool) return;
    if (hits)   *hits   = pool->n_hits;
    if (misses) *misses = pool->n_misses;
    if (reads)  *reads  = pool->n_disk_reads;
    if (writes) *writes = pool->n_disk_writes;
}
