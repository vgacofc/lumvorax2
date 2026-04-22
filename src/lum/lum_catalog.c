/*
 * LumVorax — LUM DBMS Phase 3 — Catalogue systeme
 * lum_catalog.c — Metadonnees tables/colonnes avec persistance
 *
 * Format fichier .lumcat (binaire) :
 *   [4 bytes] magic = 0x4C554D43 ("LUMC")
 *   [4 bytes] version = 1
 *   [4 bytes] n_tables
 *   [4 bytes] reserved
 *   [n_tables x sizeof(lum_table_def_t)] tables
 *
 * Cycle C69 — LUM DBMS Phase 3 — 2026-04-22
 */

#include "lum_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>

#define CAT_MAGIC   0x4C554D43u  /* "LUMC" */
#define CAT_VERSION 1u

/* Header binaire du fichier .lumcat */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t n_tables;
    uint32_t reserved;
} cat_file_header_t;

/* ── Creation / chargement ─────────────────────────────────────── */

lum_catalog_t* lum_catalog_open(const char* path) {
    lum_catalog_t* cat = (lum_catalog_t*)calloc(1, sizeof(lum_catalog_t));
    if (!cat) return NULL;
    strncpy(cat->catalog_path, path, sizeof(cat->catalog_path) - 1);
    cat->n_tables = 0;
    cat->dirty    = false;

    FILE* f = fopen(path, "rb");
    if (!f) {
        /* Nouveau catalogue : initialiser vide */
        fprintf(stderr, "[CATALOG] Nouveau catalogue : %s\n", path);
        return cat;
    }

    cat_file_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 || hdr.magic != CAT_MAGIC) {
        fprintf(stderr, "[CATALOG] ERREUR : fichier invalide %s\n", path);
        fclose(f);
        free(cat);
        return NULL;
    }

    cat->n_tables = (hdr.n_tables < LUM_CAT_MAX_TABLES) ? hdr.n_tables : LUM_CAT_MAX_TABLES;

    for (uint32_t i = 0; i < cat->n_tables; i++) {
        /* Lire la table sans les pointeurs (primary_index = NULL) */
        /* On lit seulement les champs POD (sans le pointeur B-Tree) */
        size_t pod_size = offsetof(lum_table_def_t, primary_index);
        if (fread(&cat->tables[i], pod_size, 1, f) != 1) {
            fprintf(stderr, "[CATALOG] ERREUR lecture table %u\n", i);
            break;
        }
        cat->tables[i].primary_index = NULL; /* reconstruire en memoire si besoin */
        cat->tables[i].in_use = true;
    }

    fclose(f);
    fprintf(stderr, "[CATALOG] Charge %u table(s) depuis %s\n", cat->n_tables, path);
    return cat;
}

bool lum_catalog_save(lum_catalog_t* cat) {
    if (!cat || !cat->dirty) return true;

    FILE* f = fopen(cat->catalog_path, "wb");
    if (!f) {
        fprintf(stderr, "[CATALOG] ERREUR ouverture ecriture %s: %s\n",
                cat->catalog_path, strerror(errno));
        return false;
    }

    cat_file_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic    = CAT_MAGIC;
    hdr.version  = CAT_VERSION;
    hdr.n_tables = cat->n_tables;

    fwrite(&hdr, sizeof(hdr), 1, f);

    size_t pod_size = offsetof(lum_table_def_t, primary_index);
    for (uint32_t i = 0; i < cat->n_tables; i++) {
        if (cat->tables[i].in_use)
            fwrite(&cat->tables[i], pod_size, 1, f);
    }

    fclose(f);
    cat->dirty = false;
    fprintf(stderr, "[CATALOG] Sauvegarde OK : %u table(s) -> %s\n",
            cat->n_tables, cat->catalog_path);
    return true;
}

void lum_catalog_close(lum_catalog_t* cat) {
    if (!cat) return;
    lum_catalog_save(cat);
    /* Liberer les index B-Tree en memoire */
    for (uint32_t i = 0; i < cat->n_tables; i++) {
        if (cat->tables[i].primary_index)
            lum_btree_destroy(cat->tables[i].primary_index);
    }
    free(cat);
}

/* ── Operations sur les tables ─────────────────────────────────── */

int lum_catalog_find_table(const lum_catalog_t* cat, const char* name) {
    if (!cat || !name) return -1;
    for (uint32_t i = 0; i < cat->n_tables; i++) {
        if (cat->tables[i].in_use &&
            strncmp(cat->tables[i].name, name, LUM_CAT_NAME_LEN) == 0)
            return (int)i;
    }
    return -1;
}

int lum_catalog_create_table(lum_catalog_t* cat,
                              const char* name,
                              const lum_col_def_t* cols,
                              uint32_t n_cols) {
    if (!cat || !name || !cols || n_cols == 0 || n_cols > LUM_CAT_MAX_COLS)
        return -1;
    if (cat->n_tables >= LUM_CAT_MAX_TABLES) {
        fprintf(stderr, "[CATALOG] ERREUR : max tables atteint (%d)\n", LUM_CAT_MAX_TABLES);
        return -1;
    }
    if (lum_catalog_find_table(cat, name) >= 0) {
        fprintf(stderr, "[CATALOG] ERREUR : table '%s' existe deja\n", name);
        return -1;
    }

    uint32_t idx = cat->n_tables;
    lum_table_def_t* t = &cat->tables[idx];
    memset(t, 0, sizeof(*t));

    strncpy(t->name, name, LUM_CAT_NAME_LEN - 1);
    t->n_cols = n_cols;
    memcpy(t->cols, cols, n_cols * sizeof(lum_col_def_t));
    t->n_rows          = 0;
    t->btree_root_page = UINT64_MAX;
    t->data_root_page  = UINT64_MAX;
    t->schema_version  = 1;
    t->in_use          = true;

    /* Calculer la taille d'une ligne */
    uint32_t row_size = 0;
    for (uint32_t i = 0; i < n_cols; i++)
        row_size += cols[i].size;
    t->row_size = row_size;

    /* Timestamp creation */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    t->created_ts_ns  = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    t->modified_ts_ns = t->created_ts_ns;

    /* Creer l'index B-Tree en memoire */
    t->primary_index = lum_btree_create();

    cat->n_tables++;
    cat->dirty = true;

    fprintf(stderr, "[CATALOG] Table '%s' creee : %u colonnes, row_size=%u bytes\n",
            name, n_cols, row_size);
    return (int)idx;
}

bool lum_catalog_drop_table(lum_catalog_t* cat, const char* name) {
    int idx = lum_catalog_find_table(cat, name);
    if (idx < 0) return false;
    if (cat->tables[idx].primary_index)
        lum_btree_destroy(cat->tables[idx].primary_index);
    memset(&cat->tables[idx], 0, sizeof(lum_table_def_t));
    /* Compacter */
    for (int i = idx; i < (int)cat->n_tables - 1; i++)
        cat->tables[i] = cat->tables[i + 1];
    cat->n_tables--;
    cat->dirty = true;
    fprintf(stderr, "[CATALOG] Table '%s' supprimee\n", name);
    return true;
}

void lum_catalog_list_tables(const lum_catalog_t* cat) {
    if (!cat) { printf("[CATALOG] NULL\n"); return; }
    printf("[CATALOG] %u table(s) :\n", cat->n_tables);
    for (uint32_t i = 0; i < cat->n_tables; i++) {
        const lum_table_def_t* t = &cat->tables[i];
        if (!t->in_use) continue;
        printf("  [%u] %-20s | %u col(s) | %"PRIu64" ligne(s) | row=%u B\n",
               i, t->name, t->n_cols, t->n_rows, t->row_size);
        for (uint32_t c = 0; c < t->n_cols; c++) {
            const char* type_str = "?";
            switch (t->cols[c].type) {
                case LUM_TYPE_INT64:   type_str = "INT64";   break;
                case LUM_TYPE_UINT64:  type_str = "UINT64";  break;
                case LUM_TYPE_FLOAT64: type_str = "FLOAT64"; break;
                case LUM_TYPE_BYTES:   type_str = "BYTES";   break;
                case LUM_TYPE_STRING:  type_str = "STRING";  break;
                case LUM_TYPE_HASH32:  type_str = "HASH32";  break;
                case LUM_TYPE_TS_NS:   type_str = "TS_NS";   break;
            }
            printf("    col[%u] %-16s %s(%u)%s%s\n",
                   c, t->cols[c].name, type_str, t->cols[c].size,
                   t->cols[c].is_primary_key ? " PK" : "",
                   t->cols[c].is_indexed     ? " IDX" : "");
        }
    }
}

bool lum_catalog_validate(const lum_catalog_t* cat) {
    if (!cat) return false;
    for (uint32_t i = 0; i < cat->n_tables; i++) {
        if (!cat->tables[i].in_use) continue;
        if (cat->tables[i].name[0] == '\0') return false;
        if (cat->tables[i].n_cols == 0) return false;
        if (cat->tables[i].n_cols > LUM_CAT_MAX_COLS) return false;
    }
    return true;
}
