/*
 * LumVorax — LUM DBMS Phase 4 — Mini-langage LUMQ
 * lum_query.c — Parser + Executor complet
 *
 * Cycle C69 — LUM DBMS Phase 4 — 2026-04-22
 */

#include "lum_query.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>

/* ── Utilitaires parsing ────────────────────────────────────────── */

/* Copie un mot depuis src vers dst, avance src apres le mot */
static int _next_token(const char** src, char* dst, size_t max_len) {
    while (**src && isspace((unsigned char)**src)) (*src)++;
    if (!**src) { dst[0] = '\0'; return 0; }
    size_t i = 0;
    while (**src && !isspace((unsigned char)**src) && i < max_len - 1) {
        dst[i++] = (char)toupper((unsigned char)**src);
        (*src)++;
    }
    dst[i] = '\0';
    return (int)i;
}

/* Meme chose mais en preservant la casse */
static int _next_token_raw(const char** src, char* dst, size_t max_len) {
    while (**src && isspace((unsigned char)**src)) (*src)++;
    if (!**src) { dst[0] = '\0'; return 0; }
    size_t i = 0;
    /* Supporte chaines entre parentheses */
    if (**src == '(') {
        (*src)++;
        while (**src && **src != ')' && i < max_len - 1)
            dst[i++] = *(*src)++;
        if (**src == ')') (*src)++;
        dst[i] = '\0';
        return (int)i;
    }
    while (**src && !isspace((unsigned char)**src) &&
           **src != ',' && **src != ')' && i < max_len - 1) {
        dst[i++] = *(*src)++;
    }
    if (**src == ',') (*src)++;
    dst[i] = '\0';
    return (int)i;
}

static uint64_t _parse_uint64(const char* s, bool* ok) {
    if (!s || !*s) { if (ok) *ok = false; return 0; }
    char* end;
    errno = 0;
    uint64_t v = strtoull(s, &end, 0);
    if (ok) *ok = (errno == 0 && end != s && *end == '\0');
    return v;
}

/* ── Parser LUMQ ────────────────────────────────────────────────── */

lumq_ast_t lumq_parse(const char* query) {
    lumq_ast_t ast;
    memset(&ast, 0, sizeof(ast));
    ast.cmd   = LUMQ_UNKNOWN;
    ast.valid = false;

    if (!query || !*query) {
        strncpy(ast.error_msg, "Requete vide", sizeof(ast.error_msg) - 1);
        return ast;
    }

    const char* p = query;
    char tok[128] = {0};

    _next_token(&p, tok, sizeof(tok)); /* Verbe principal */

    /* ── INSERT INTO <table> VALUES (<key>, <value>) ── */
    if (strcmp(tok, "INSERT") == 0) {
        ast.cmd = LUMQ_INSERT;
        _next_token(&p, tok, sizeof(tok)); /* INTO */
        _next_token(&p, ast.table_name, sizeof(ast.table_name)); /* table */
        _next_token(&p, tok, sizeof(tok)); /* VALUES */
        /* Parser VALUES (key, value) ou VALUES key value */
        char sk[64], sv[64];
        _next_token_raw(&p, sk, sizeof(sk));
        _next_token_raw(&p, sv, sizeof(sv));
        /* Nettoyer parentheses/virgules restantes */
        char* cp;
        if ((cp = strchr(sk, '(')) != NULL) memmove(sk, cp + 1, strlen(cp));
        if ((cp = strchr(sk, ',')) != NULL) *cp = '\0';
        if ((cp = strchr(sv, ')')) != NULL) *cp = '\0';
        bool ok_k = false, ok_v = false;
        ast.key   = _parse_uint64(sk, &ok_k);
        ast.value = _parse_uint64(sv, &ok_v);
        if (!ok_k || !ok_v) {
            snprintf(ast.error_msg, sizeof(ast.error_msg),
                     "INSERT : cle='%s' ou valeur='%s' invalide", sk, sv);
            return ast;
        }
        ast.valid = true;
        return ast;
    }

    /* ── FIND IN <table> WHERE key = <k> ── */
    if (strcmp(tok, "FIND") == 0) {
        ast.cmd = LUMQ_FIND;
        _next_token(&p, tok, sizeof(tok)); /* IN */
        _next_token(&p, ast.table_name, sizeof(ast.table_name));
        _next_token(&p, tok, sizeof(tok)); /* WHERE */
        _next_token(&p, tok, sizeof(tok)); /* key */
        _next_token(&p, tok, sizeof(tok)); /* = */
        char sk[64];
        _next_token_raw(&p, sk, sizeof(sk));
        bool ok = false;
        ast.key = _parse_uint64(sk, &ok);
        if (!ok) { snprintf(ast.error_msg, sizeof(ast.error_msg),
                    "FIND : cle '%s' invalide", sk); return ast; }
        ast.valid = true;
        return ast;
    }

    /* ── RANGE IN <table> WHERE key BETWEEN <lo> AND <hi> ── */
    if (strcmp(tok, "RANGE") == 0) {
        ast.cmd = LUMQ_RANGE;
        _next_token(&p, tok, sizeof(tok)); /* IN */
        _next_token(&p, ast.table_name, sizeof(ast.table_name));
        _next_token(&p, tok, sizeof(tok)); /* WHERE */
        _next_token(&p, tok, sizeof(tok)); /* key */
        _next_token(&p, tok, sizeof(tok)); /* BETWEEN */
        char slo[64], shi[64];
        _next_token_raw(&p, slo, sizeof(slo));
        _next_token(&p, tok, sizeof(tok)); /* AND */
        _next_token_raw(&p, shi, sizeof(shi));
        bool ok1 = false, ok2 = false;
        ast.range_lo = _parse_uint64(slo, &ok1);
        ast.range_hi = _parse_uint64(shi, &ok2);
        if (!ok1 || !ok2 || ast.range_lo > ast.range_hi) {
            snprintf(ast.error_msg, sizeof(ast.error_msg),
                     "RANGE : bornes invalides [%s, %s]", slo, shi);
            return ast;
        }
        ast.valid = true;
        return ast;
    }

    /* ── COUNT IN <table> ── */
    if (strcmp(tok, "COUNT") == 0) {
        ast.cmd = LUMQ_COUNT;
        _next_token(&p, tok, sizeof(tok)); /* IN */
        _next_token(&p, ast.table_name, sizeof(ast.table_name));
        ast.valid = (ast.table_name[0] != '\0');
        return ast;
    }

    /* ── DELETE FROM <table> WHERE key = <k> ── */
    if (strcmp(tok, "DELETE") == 0) {
        ast.cmd = LUMQ_DELETE;
        _next_token(&p, tok, sizeof(tok)); /* FROM */
        _next_token(&p, ast.table_name, sizeof(ast.table_name));
        _next_token(&p, tok, sizeof(tok)); /* WHERE */
        _next_token(&p, tok, sizeof(tok)); /* key */
        _next_token(&p, tok, sizeof(tok)); /* = */
        char sk[64];
        _next_token_raw(&p, sk, sizeof(sk));
        bool ok = false;
        ast.key = _parse_uint64(sk, &ok);
        if (!ok) { snprintf(ast.error_msg, sizeof(ast.error_msg),
                    "DELETE : cle '%s' invalide", sk); return ast; }
        ast.valid = true;
        return ast;
    }

    /* ── DUMP TABLE <table> ── */
    if (strcmp(tok, "DUMP") == 0) {
        ast.cmd = LUMQ_DUMP;
        _next_token(&p, tok, sizeof(tok)); /* TABLE */
        _next_token(&p, ast.table_name, sizeof(ast.table_name));
        ast.valid = (ast.table_name[0] != '\0');
        return ast;
    }

    /* ── CREATE TABLE <name> (...) ── */
    if (strcmp(tok, "CREATE") == 0) {
        ast.cmd = LUMQ_CREATE;
        _next_token(&p, tok, sizeof(tok)); /* TABLE */
        _next_token(&p, ast.table_name, sizeof(ast.table_name));
        /* Capturer le reste comme definition des colonnes */
        while (*p && isspace((unsigned char)*p)) p++;
        strncpy(ast.create_cols, p, sizeof(ast.create_cols) - 1);
        ast.valid = (ast.table_name[0] != '\0');
        return ast;
    }

    /* ── DROP TABLE <name> ── */
    if (strcmp(tok, "DROP") == 0) {
        ast.cmd = LUMQ_DROP;
        _next_token(&p, tok, sizeof(tok)); /* TABLE */
        _next_token(&p, ast.table_name, sizeof(ast.table_name));
        ast.valid = (ast.table_name[0] != '\0');
        return ast;
    }

    /* ── LIST TABLES ── */
    if (strcmp(tok, "LIST") == 0) {
        ast.cmd   = LUMQ_LIST;
        ast.valid = true;
        return ast;
    }

    /* ── SHOW STATS ── */
    if (strcmp(tok, "SHOW") == 0) {
        ast.cmd   = LUMQ_STATS;
        ast.valid = true;
        return ast;
    }

    snprintf(ast.error_msg, sizeof(ast.error_msg),
             "Commande inconnue : '%s'", tok);
    return ast;
}

/* ── Executor LUMQ ──────────────────────────────────────────────── */

/* Callback pour RANGE : comptage + affichage */
typedef struct { uint64_t count; } range_cb_data_t;

static void _range_count_cb(uint64_t key, uint64_t val, void* ud) {
    range_cb_data_t* d = (range_cb_data_t*)ud;
    d->count++;
    printf("  key=%-20"PRIu64" val=%"PRIu64"\n", key, val);
}

/* Cherche ou cree un index B-Tree pour une table */
static lum_btree_t* _get_table_index(lumq_ctx_t* ctx, const char* name) {
    int idx = lum_catalog_find_table(ctx->catalog, name);
    if (idx < 0) return NULL;
    lum_table_def_t* t = &ctx->catalog->tables[idx];
    if (!t->primary_index) {
        t->primary_index = lum_btree_create();
    }
    return t->primary_index;
}

lumq_result_t lumq_execute(lumq_ctx_t* ctx, const lumq_ast_t* ast) {
    lumq_result_t res;
    memset(&res, 0, sizeof(res));
    res.ok    = false;
    res.value = UINT64_MAX;

    if (!ast || !ast->valid) {
        snprintf(res.message, sizeof(res.message),
                 "ERREUR parsing : %s", ast ? ast->error_msg : "AST NULL");
        return res;
    }

    switch (ast->cmd) {

    case LUMQ_INSERT: {
        lum_btree_t* idx = _get_table_index(ctx, ast->table_name);
        if (!idx) {
            snprintf(res.message, sizeof(res.message),
                     "Table '%s' non trouvee", ast->table_name);
            return res;
        }
        lum_btree_insert(idx, ast->key, ast->value);
        /* Mise a jour n_rows dans le catalogue */
        int tidx = lum_catalog_find_table(ctx->catalog, ast->table_name);
        if (tidx >= 0) {
            ctx->catalog->tables[tidx].n_rows = lum_btree_count(idx);
            /* Timestamp modification */
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ctx->catalog->tables[tidx].modified_ts_ns =
                (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
        }
        res.ok = true;
        res.rows_affected = 1;
        snprintf(res.message, sizeof(res.message),
                 "INSERT OK : table=%s key=%"PRIu64" val=%"PRIu64,
                 ast->table_name, ast->key, ast->value);
        return res;
    }

    case LUMQ_FIND: {
        lum_btree_t* idx = _get_table_index(ctx, ast->table_name);
        if (!idx) {
            snprintf(res.message, sizeof(res.message),
                     "Table '%s' non trouvee", ast->table_name);
            return res;
        }
        uint64_t found = lum_btree_search(idx, ast->key);
        if (found == LUM_BTREE_NOT_FOUND) {
            snprintf(res.message, sizeof(res.message),
                     "FIND : cle %"PRIu64" non trouvee dans '%s'",
                     ast->key, ast->table_name);
            res.ok = true; /* pas d'erreur, juste absent */
            res.count = 0;
        } else {
            res.ok    = true;
            res.value = found;
            res.count = 1;
            snprintf(res.message, sizeof(res.message),
                     "FIND OK : key=%"PRIu64" -> val=%"PRIu64, ast->key, found);
        }
        return res;
    }

    case LUMQ_RANGE: {
        lum_btree_t* idx = _get_table_index(ctx, ast->table_name);
        if (!idx) {
            snprintf(res.message, sizeof(res.message),
                     "Table '%s' non trouvee", ast->table_name);
            return res;
        }
        range_cb_data_t cbd = {0};
        printf("[LUMQ] RANGE [%"PRIu64", %"PRIu64"] dans '%s' :\n",
               ast->range_lo, ast->range_hi, ast->table_name);
        lum_btree_range(idx, ast->range_lo, ast->range_hi, _range_count_cb, &cbd);
        res.ok    = true;
        res.count = cbd.count;
        snprintf(res.message, sizeof(res.message),
                 "RANGE OK : %"PRIu64" resultat(s) dans [%"PRIu64", %"PRIu64"]",
                 cbd.count, ast->range_lo, ast->range_hi);
        return res;
    }

    case LUMQ_COUNT: {
        lum_btree_t* idx = _get_table_index(ctx, ast->table_name);
        if (!idx) {
            snprintf(res.message, sizeof(res.message),
                     "Table '%s' non trouvee", ast->table_name);
            return res;
        }
        res.ok    = true;
        res.count = lum_btree_count(idx);
        snprintf(res.message, sizeof(res.message),
                 "COUNT '%s' = %"PRIu64" lignes", ast->table_name, res.count);
        return res;
    }

    case LUMQ_DELETE: {
        lum_btree_t* idx = _get_table_index(ctx, ast->table_name);
        if (!idx) {
            snprintf(res.message, sizeof(res.message),
                     "Table '%s' non trouvee", ast->table_name);
            return res;
        }
        bool deleted = lum_btree_delete(idx, ast->key);
        if (deleted) {
            int tidx = lum_catalog_find_table(ctx->catalog, ast->table_name);
            if (tidx >= 0)
                ctx->catalog->tables[tidx].n_rows = lum_btree_count(idx);
        }
        res.ok = true;
        res.rows_affected = deleted ? 1 : 0;
        snprintf(res.message, sizeof(res.message),
                 "DELETE %s : key=%"PRIu64" dans '%s'",
                 deleted ? "OK" : "absent", ast->key, ast->table_name);
        return res;
    }

    case LUMQ_DUMP: {
        lum_btree_t* idx = _get_table_index(ctx, ast->table_name);
        if (!idx) {
            snprintf(res.message, sizeof(res.message),
                     "Table '%s' non trouvee", ast->table_name);
            return res;
        }
        printf("[LUMQ] DUMP table '%s' (max 50 entrees) :\n", ast->table_name);
        lum_btree_dump(idx, 50);
        res.ok    = true;
        res.count = lum_btree_count(idx);
        snprintf(res.message, sizeof(res.message),
                 "DUMP OK : %"PRIu64" entrees total", res.count);
        return res;
    }

    case LUMQ_CREATE: {
        /* Parser simple des colonnes : "col1 TYPE, col2 TYPE, ..." */
        lum_col_def_t cols[LUM_CAT_MAX_COLS];
        uint32_t n_cols = 0;
        char buf[512];
        strncpy(buf, ast->create_cols, sizeof(buf) - 1);
        /* Supprimer parentheses */
        char* start = buf;
        if (*start == '(') start++;
        char* end = strrchr(start, ')');
        if (end) *end = '\0';

        char* token = strtok(start, ",");
        while (token && n_cols < LUM_CAT_MAX_COLS) {
            while (isspace((unsigned char)*token)) token++;
            memset(&cols[n_cols], 0, sizeof(cols[n_cols]));
            /* Format: "colname TYPE" */
            char cname[LUM_CAT_NAME_LEN] = {0};
            char ctype[32] = {0};
            if (sscanf(token, "%63s %31s", cname, ctype) == 2) {
                strncpy(cols[n_cols].name, cname, LUM_CAT_NAME_LEN - 1);
                /* Detecter le type */
                if (strstr(ctype, "INT64"))        cols[n_cols].type = LUM_TYPE_INT64,  cols[n_cols].size = 8;
                else if (strstr(ctype, "UINT64"))   cols[n_cols].type = LUM_TYPE_UINT64, cols[n_cols].size = 8;
                else if (strstr(ctype, "FLOAT64"))  cols[n_cols].type = LUM_TYPE_FLOAT64,cols[n_cols].size = 8;
                else if (strstr(ctype, "HASH32"))   cols[n_cols].type = LUM_TYPE_HASH32, cols[n_cols].size = 32;
                else if (strstr(ctype, "TS_NS"))    cols[n_cols].type = LUM_TYPE_TS_NS,  cols[n_cols].size = 8;
                else if (strstr(ctype, "STRING"))   cols[n_cols].type = LUM_TYPE_STRING, cols[n_cols].size = 64;
                else                               cols[n_cols].type = LUM_TYPE_BYTES,  cols[n_cols].size = 8;
                if (n_cols == 0) cols[n_cols].is_primary_key = true; /* Premiere col = PK */
                n_cols++;
            }
            token = strtok(NULL, ",");
        }
        if (n_cols == 0) {
            snprintf(res.message, sizeof(res.message),
                     "CREATE : aucune colonne valide dans '%s'", ast->create_cols);
            return res;
        }
        int tidx = lum_catalog_create_table(ctx->catalog, ast->table_name, cols, n_cols);
        if (tidx < 0) {
            snprintf(res.message, sizeof(res.message),
                     "CREATE FAILED : table '%s'", ast->table_name);
            return res;
        }
        res.ok = true;
        snprintf(res.message, sizeof(res.message),
                 "CREATE TABLE '%s' OK (%u colonne(s))", ast->table_name, n_cols);
        return res;
    }

    case LUMQ_DROP: {
        bool ok = lum_catalog_drop_table(ctx->catalog, ast->table_name);
        res.ok = ok;
        snprintf(res.message, sizeof(res.message),
                 ok ? "DROP TABLE '%s' OK" : "DROP TABLE '%s' non trouvee",
                 ast->table_name);
        return res;
    }

    case LUMQ_LIST:
        lum_catalog_list_tables(ctx->catalog);
        res.ok    = true;
        res.count = ctx->catalog->n_tables;
        snprintf(res.message, sizeof(res.message),
                 "%u table(s) dans le catalogue", ctx->catalog->n_tables);
        return res;

    case LUMQ_STATS:
        printf("[LUMQ] Statistiques DBMS :\n");
        printf("  Tables       : %u\n", ctx->catalog->n_tables);
        for (uint32_t i = 0; i < ctx->catalog->n_tables; i++) {
            lum_table_def_t* t = &ctx->catalog->tables[i];
            if (!t->in_use) continue;
            uint64_t cnt = t->primary_index ? lum_btree_count(t->primary_index) : 0;
            printf("  [%s] lignes=%"PRIu64" index_hauteur=%d\n",
                   t->name, cnt,
                   t->primary_index ? t->primary_index->height : -1);
        }
        res.ok = true;
        snprintf(res.message, sizeof(res.message), "STATS OK");
        return res;

    default:
        snprintf(res.message, sizeof(res.message), "Commande non implementee");
        return res;
    }
}

/* ── Context DBMS ────────────────────────────────────────────────── */

lumq_ctx_t* lumq_ctx_create(const char* data_dir) {
    lumq_ctx_t* ctx = (lumq_ctx_t*)calloc(1, sizeof(lumq_ctx_t));
    if (!ctx) return NULL;

    strncpy(ctx->data_dir, data_dir ? data_dir : ".", sizeof(ctx->data_dir) - 1);

    /* Construire le chemin du catalogue */
    char cat_path[512];
    snprintf(cat_path, sizeof(cat_path), "%s/lum_system.lumcat", ctx->data_dir);

    ctx->catalog = lum_catalog_open(cat_path);
    if (!ctx->catalog) {
        free(ctx);
        return NULL;
    }

    printf("[LUMQ] Contexte DBMS initialise : data_dir=%s\n", ctx->data_dir);
    return ctx;
}

void lumq_ctx_destroy(lumq_ctx_t* ctx) {
    if (!ctx) return;
    lum_catalog_save(ctx->catalog);
    lum_catalog_close(ctx->catalog);
    free(ctx);
}

lumq_result_t lumq_run(lumq_ctx_t* ctx, const char* query) {
    lumq_ast_t ast = lumq_parse(query);
    return lumq_execute(ctx, &ast);
}

void lumq_print_result(const lumq_result_t* r) {
    if (!r) return;
    printf("[LUMQ] %s | %s", r->ok ? "OK" : "ERREUR", r->message);
    if (r->value != UINT64_MAX && r->count > 0)
        printf(" | val=%"PRIu64, r->value);
    if (r->count > 0)
        printf(" | count=%"PRIu64, r->count);
    if (r->rows_affected > 0)
        printf(" | rows=%"PRIu64, r->rows_affected);
    printf("\n");
}

void lumq_repl(lumq_ctx_t* ctx) {
    char line[1024];
    printf("[LUMQ] REPL interactif LumVorax DBMS (CTRL+D pour quitter)\n");
    printf("[LUMQ] Commandes : INSERT, FIND, RANGE, COUNT, DELETE, DUMP, "
           "CREATE, DROP, LIST, SHOW STATS\n");
    while (1) {
        printf("lumq> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        /* Supprimer le newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (!line[0] || line[0] == '#') continue;
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) break;
        lumq_result_t res = lumq_run(ctx, line);
        lumq_print_result(&res);
    }
}
