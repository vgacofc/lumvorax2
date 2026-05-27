/*
 * LumVorax — LUM DBMS Phase 4 — Mini-langage LUMQ
 * lum_query.h — Parser + Executor de requetes LUMQ
 *
 * LUMQ = LUM Query Language (implementation 100% native)
 * Syntaxe inspiree de SQL mais simplifiee pour les donnees LUM.
 *
 * Commandes supportees :
 *
 *   INSERT INTO <table> VALUES (<k1>, <k2>, ...)
 *   FIND IN <table> WHERE key = <k>
 *   RANGE IN <table> WHERE key BETWEEN <lo> AND <hi>
 *   COUNT IN <table>
 *   DELETE FROM <table> WHERE key = <k>
 *   DUMP TABLE <table>
 *   CREATE TABLE <name> (col1 TYPE, col2 TYPE, ...)
 *   DROP TABLE <name>
 *   LIST TABLES
 *   SHOW STATS
 *
 * Cycle C69 — LUM DBMS Phase 4 — 2026-04-22
 */
#ifndef LUM_QUERY_H
#define LUM_QUERY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "lum_catalog.h"
#include "lum_btree.h"

/* Types de commandes LUMQ */
typedef enum {
    LUMQ_INSERT      = 1,
    LUMQ_FIND        = 2,
    LUMQ_RANGE       = 3,
    LUMQ_COUNT       = 4,
    LUMQ_DELETE      = 5,
    LUMQ_DUMP        = 6,
    LUMQ_CREATE      = 7,
    LUMQ_DROP        = 8,
    LUMQ_LIST        = 9,
    LUMQ_STATS       = 10,
    LUMQ_UNKNOWN     = 0,
} lumq_cmd_type_t;

/* AST d'une requete parsee */
typedef struct {
    lumq_cmd_type_t  cmd;
    char             table_name[64];
    uint64_t         key;         /* Cle primaire */
    uint64_t         value;       /* Valeur a inserer */
    uint64_t         range_lo;    /* Borne inferieure pour RANGE */
    uint64_t         range_hi;    /* Borne superieure pour RANGE */
    char             create_cols[512]; /* Colonnes pour CREATE TABLE */
    char             error_msg[128];   /* Message d'erreur si parsing echoue */
    bool             valid;            /* Parsing OK ? */
} lumq_ast_t;

/* Resultat d'execution d'une requete */
typedef struct {
    bool     ok;              /* Execution OK ? */
    uint64_t value;           /* Valeur trouvee (FIND) */
    uint64_t count;           /* Nombre de resultats (COUNT, RANGE) */
    uint64_t rows_affected;   /* Lignes modifiees (INSERT, DELETE) */
    char     message[256];    /* Message humain */
} lumq_result_t;

/* Contexte DBMS (regroupe catalog + index pour les requetes) */
typedef struct {
    lum_catalog_t* catalog;
    char           data_dir[256]; /* Repertoire ou stocker les fichiers .lum */
} lumq_ctx_t;

/* ── API publique ─────────────────────────────────────────────────── */

/* Cree un contexte DBMS */
lumq_ctx_t* lumq_ctx_create(const char* data_dir);

/* Detruit le contexte */
void lumq_ctx_destroy(lumq_ctx_t* ctx);

/* Parse une commande LUMQ en AST */
lumq_ast_t lumq_parse(const char* query);

/* Execute un AST sur le contexte DBMS */
lumq_result_t lumq_execute(lumq_ctx_t* ctx, const lumq_ast_t* ast);

/* Parse + execute (convenience) */
lumq_result_t lumq_run(lumq_ctx_t* ctx, const char* query);

/* Affiche un resultat de maniere lisible */
void lumq_print_result(const lumq_result_t* result);

/* REPL interactif (lit depuis stdin) */
void lumq_repl(lumq_ctx_t* ctx);

#endif /* LUM_QUERY_H */
