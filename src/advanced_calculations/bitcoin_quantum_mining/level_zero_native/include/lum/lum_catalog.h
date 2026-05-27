/*
 * LumVorax — LUM DBMS Phase 3 — Catalogue systeme
 * lum_catalog.h — Metadonnees des tables et colonnes
 *
 * IMPLEMENTATION 100% NATIVE.
 * Le catalogue stocke les informations sur chaque table LUM :
 *   - nom de la table
 *   - colonnes (nom + type + taille)
 *   - nombre de lignes (approximatif)
 *   - page racine du B-Tree d'index
 *   - schema version
 *
 * Equivalent simplifie de pg_class + pg_attribute de PostgreSQL.
 * Maximum LUM_CAT_MAX_TABLES tables, LUM_CAT_MAX_COLS colonnes par table.
 *
 * Cycle C69 — LUM DBMS Phase 3 — 2026-04-22
 */
#ifndef LUM_CATALOG_H
#define LUM_CATALOG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "lum_btree.h"

#define LUM_CAT_MAX_TABLES  64
#define LUM_CAT_MAX_COLS    32
#define LUM_CAT_NAME_LEN    64

/* Types de colonnes supportes */
typedef enum {
    LUM_TYPE_INT64   = 1,   /* Entier 64 bits */
    LUM_TYPE_UINT64  = 2,   /* Entier non signe 64 bits */
    LUM_TYPE_FLOAT64 = 3,   /* Flottant 64 bits */
    LUM_TYPE_BYTES   = 4,   /* Blob binaire (taille fixe) */
    LUM_TYPE_STRING  = 5,   /* Chaine de caracteres (taille max fixe) */
    LUM_TYPE_HASH32  = 6,   /* Hash 32 bytes (SHA-256) */
    LUM_TYPE_TS_NS   = 7,   /* Timestamp nanosecondes */
} lum_col_type_t;

/* Description d'une colonne */
typedef struct {
    char           name[LUM_CAT_NAME_LEN];  /* Nom de la colonne */
    lum_col_type_t type;                    /* Type */
    uint32_t       size;                    /* Taille en bytes */
    bool           is_primary_key;          /* Cle primaire ? */
    bool           is_indexed;             /* Index B-Tree ? */
    bool           nullable;               /* Peut etre NULL ? */
} lum_col_def_t;

/* Description d'une table */
typedef struct {
    char          name[LUM_CAT_NAME_LEN];       /* Nom de la table */
    uint32_t      n_cols;                        /* Nombre de colonnes */
    lum_col_def_t cols[LUM_CAT_MAX_COLS];        /* Definitions colonnes */
    uint64_t      n_rows;                        /* Nombre de lignes */
    uint64_t      btree_root_page;               /* Page racine index B-Tree */
    uint64_t      data_root_page;               /* Page racine donnees */
    uint32_t      schema_version;               /* Version du schema */
    uint32_t      row_size;                     /* Taille d'une ligne en bytes */
    uint64_t      created_ts_ns;                /* Timestamp creation */
    uint64_t      modified_ts_ns;               /* Timestamp derniere modif */
    lum_btree_t*  primary_index;               /* Index B-Tree primaire (en RAM) */
    bool          in_use;                      /* Entree valide ? */
} lum_table_def_t;

/* Catalogue systeme */
typedef struct {
    lum_table_def_t tables[LUM_CAT_MAX_TABLES]; /* Tableau des tables */
    uint32_t        n_tables;                    /* Nombre de tables actives */
    char            catalog_path[512];           /* Fichier catalogue (.lumcat) */
    bool            dirty;                       /* Catalogue modifie ? */
} lum_catalog_t;

/* ── API publique ─────────────────────────────────────────────────── */

/* Cree/charge un catalogue depuis un fichier */
lum_catalog_t* lum_catalog_open(const char* path);

/* Sauvegarde le catalogue sur disque */
bool lum_catalog_save(lum_catalog_t* cat);

/* Ferme le catalogue et libere la memoire */
void lum_catalog_close(lum_catalog_t* cat);

/* Cree une nouvelle table dans le catalogue */
int lum_catalog_create_table(lum_catalog_t* cat,
                              const char* name,
                              const lum_col_def_t* cols,
                              uint32_t n_cols);

/* Trouve une table par nom (-1 si non trouvee) */
int lum_catalog_find_table(const lum_catalog_t* cat, const char* name);

/* Supprime une table du catalogue */
bool lum_catalog_drop_table(lum_catalog_t* cat, const char* name);

/* Liste toutes les tables (affichage) */
void lum_catalog_list_tables(const lum_catalog_t* cat);

/* Valide la coherence du catalogue */
bool lum_catalog_validate(const lum_catalog_t* cat);

#endif /* LUM_CATALOG_H */
