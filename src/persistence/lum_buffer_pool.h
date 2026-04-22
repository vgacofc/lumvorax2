/*
 * LumVorax — LUM DBMS Phase 3 — Buffer Pool
 * lum_buffer_pool.h — Buffer pool 8KB pages avec politique LRU
 *
 * IMPLEMENTATION 100% NATIVE.
 * Gere N_PAGES pages de PAGE_SIZE bytes en memoire.
 * Politique d'eviction : LRU (Least Recently Used).
 * Pages modifiees (dirty) : ecrites sur disque avant eviction.
 *
 * Utilisation :
 *   pool = lum_pool_create("monFichier.lum", 16)  // 16 pages x 8KB = 128KB
 *   page = lum_pool_pin(pool, page_id)             // charge la page
 *   lum_pool_mark_dirty(pool, page_id)             // si modification
 *   lum_pool_unpin(pool, page_id)                  // libere le verrou
 *   lum_pool_flush(pool)                           // flush toutes les dirty pages
 *   lum_pool_destroy(pool)                         // libere tout
 *
 * Cycle C69 — LUM DBMS Phase 3 — 2026-04-22
 */
#ifndef LUM_BUFFER_POOL_H
#define LUM_BUFFER_POOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

#define LUM_PAGE_SIZE       8192u    /* 8 KB — standard PostgreSQL */
#define LUM_POOL_MAX_PAGES  128u     /* Max pages en memoire simultanement */

/* Descripteur d'une frame (page en memoire) */
typedef struct {
    uint8_t   data[LUM_PAGE_SIZE]; /* Donnees de la page */
    uint64_t  page_id;             /* ID de la page sur disque (-1 = libre) */
    uint32_t  pin_count;           /* Nombre de threads qui utilisent cette page */
    bool      dirty;               /* Page modifiee non encore ecrite sur disque */
    uint64_t  last_access_tick;    /* Tick LRU */
} lum_page_frame_t;

/* Header de fichier LUM paginee */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /* 0x4C554D50 = "LUMP" */
    uint32_t version;        /* 1 = Phase 3 */
    uint64_t n_pages;        /* Nombre total de pages */
    uint64_t root_page;      /* Page racine (B-Tree ou catalog) */
    uint64_t free_list;      /* Premiere page libre */
    uint8_t  reserved[16];   /* Reserve pour extensions futures */
} lum_file_header_t;

/* Buffer pool complet */
typedef struct {
    int                fd;                         /* File descriptor du fichier */
    char               path[512];                  /* Chemin du fichier */
    lum_page_frame_t   frames[LUM_POOL_MAX_PAGES]; /* Frames en memoire */
    uint32_t           n_frames;                   /* Nombre de frames allouees */
    uint64_t           lru_clock;                  /* Compteur LRU global */
    uint64_t           n_disk_reads;               /* Statistiques */
    uint64_t           n_disk_writes;
    uint64_t           n_hits;                     /* Hits cache */
    uint64_t           n_misses;
    pthread_mutex_t    mutex;                      /* Protection multi-thread */
} lum_buffer_pool_t;

/* ── API publique ─────────────────────────────────────────────────── */

/* Cree un buffer pool attache a un fichier (cree si absent) */
lum_buffer_pool_t* lum_pool_create(const char* path, uint32_t n_frames);

/* Detruit le pool (flush + ferme fichier) */
void lum_pool_destroy(lum_buffer_pool_t* pool);

/* Charge une page en memoire et retourne son adresse. NULL si erreur.
 * La page est "pinnee" (ne peut pas etre evictee) jusqu'a unpin. */
uint8_t* lum_pool_pin(lum_buffer_pool_t* pool, uint64_t page_id);

/* Libere le pin sur une page (peut etre evictee ensuite) */
void lum_pool_unpin(lum_buffer_pool_t* pool, uint64_t page_id);

/* Marque la page comme modifiee (sera ecrite avant eviction) */
void lum_pool_mark_dirty(lum_buffer_pool_t* pool, uint64_t page_id);

/* Alloue une nouvelle page (retourne son ID) */
uint64_t lum_pool_alloc_page(lum_buffer_pool_t* pool);

/* Ecrit toutes les dirty pages sur disque */
bool lum_pool_flush(lum_buffer_pool_t* pool);

/* Statistiques du pool */
void lum_pool_stats(const lum_buffer_pool_t* pool,
                    uint64_t* hits, uint64_t* misses,
                    uint64_t* reads, uint64_t* writes);

#endif /* LUM_BUFFER_POOL_H */
