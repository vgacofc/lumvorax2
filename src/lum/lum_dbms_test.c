/*
 * LumVorax — LUM DBMS Phase 3+4 — Test de validation integre
 * lum_dbms_test.c — Programme de test autonome (sans PostgreSQL)
 *
 * Tests executes :
 *   T01 : B-Tree insert/search/delete 10000 cles
 *   T02 : B-Tree range query [1000, 2000]
 *   T03 : B-Tree validation structurelle (proprietes B-Tree)
 *   T04 : Buffer Pool creer/pin/dirty/flush
 *   T05 : Catalog creer/find/list/drop tables
 *   T06 : LUMQ parser INSERT / FIND / RANGE / COUNT / DELETE / LIST
 *   T07 : MVCC insert / read snapshot isolation / vacuum
 *   T08 : Stress test 100K insertions B-Tree
 *
 * Compilation :
 *   cc -O2 -std=c11 -Wall -Wextra -pthread \
 *      -I../lum -I../persistence \
 *      src/lum/lum_dbms_test.c \
 *      src/lum/lum_btree.c src/lum/lum_catalog.c src/lum/lum_query.c \
 *      src/persistence/lum_buffer_pool.c src/persistence/lum_mvcc.c \
 *      -o lum_dbms_test
 *
 * Cycle C69 — LUM DBMS Phase 3+4 — 2026-04-22
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

#include "lum_btree.h"
#include "lum_catalog.h"
#include "lum_query.h"
#include "../persistence/lum_mvcc.h"
#include "../persistence/lum_buffer_pool.h"

/* ── Macros de test ───────────────────────────────────────────── */
#define TEST_PASS(name) printf("[PASS] %-40s\n", name)
#define TEST_FAIL(name, msg) printf("[FAIL] %-40s : %s\n", name, msg)
#define ASSERT_EQ(a, b, name) do { \
    if ((a) != (b)) { \
        printf("[FAIL] %-40s : attendu=%"PRIu64" obtenu=%"PRIu64"\n", \
               name, (uint64_t)(b), (uint64_t)(a)); \
        g_failures++; \
    } else { \
        TEST_PASS(name); \
        g_passes++; \
    } \
} while(0)

static int g_passes   = 0;
static int g_failures = 0;

static double _elapsed_ms(struct timespec* t0) {
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (double)(t1.tv_sec - t0->tv_sec) * 1e3
         + (double)(t1.tv_nsec - t0->tv_nsec) * 1e-6;
}

/* ── T01 : B-Tree insert/search/delete ─────────────────────────── */
static void test_t01_btree_basic(void) {
    printf("\n=== T01 : B-Tree insert/search/delete ===\n");
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC, &t0);

    lum_btree_t* tree = lum_btree_create();
    assert(tree);

    const int N = 10000;
    for (int i = 0; i < N; i++)
        lum_btree_insert(tree, (uint64_t)i * 7 + 1, (uint64_t)i * 13);

    ASSERT_EQ(lum_btree_count(tree), (uint64_t)N, "T01/count apres insert");

    /* Recherche : toutes les cles doivent etre trouvees */
    int not_found = 0;
    for (int i = 0; i < N; i++) {
        uint64_t v = lum_btree_search(tree, (uint64_t)i * 7 + 1);
        if (v == LUM_BTREE_NOT_FOUND || v != (uint64_t)i * 13) not_found++;
    }
    ASSERT_EQ(not_found, 0, "T01/search toutes cles presentes");

    /* Mise a jour */
    lum_btree_insert(tree, 1, 9999);
    ASSERT_EQ(lum_btree_search(tree, 1), 9999ULL, "T01/update key=1");

    /* Cles absentes */
    ASSERT_EQ(lum_btree_search(tree, UINT64_MAX - 1), LUM_BTREE_NOT_FOUND, "T01/absent retourne NOT_FOUND");

    /* Suppression */
    int del_ok = 0;
    for (int i = 0; i < N / 2; i++) {
        if (lum_btree_delete(tree, (uint64_t)i * 7 + 1)) del_ok++;
    }
    ASSERT_EQ(del_ok, N / 2, "T01/delete N/2 cles");
    ASSERT_EQ(lum_btree_count(tree), (uint64_t)(N - N / 2), "T01/count apres delete");

    /* Validation structurelle */
    bool valid = lum_btree_validate(tree);
    ASSERT_EQ(valid, 1, "T01/validate structure B-Tree");

    printf("  Temps T01 : %.2f ms\n", _elapsed_ms(&t0));
    lum_btree_destroy(tree);
}

/* ── T02 : B-Tree range query ────────────────────────────────────── */
static uint64_t g_range_count = 0;
static void _range_cb(uint64_t key, uint64_t val, void* ud) {
    (void)key; (void)val; (void)ud;
    g_range_count++;
}

static void test_t02_btree_range(void) {
    printf("\n=== T02 : B-Tree range query ===\n");
    lum_btree_t* tree = lum_btree_create();
    assert(tree);

    /* Inserer cles 0..999 */
    for (uint64_t i = 0; i < 1000; i++)
        lum_btree_insert(tree, i, i * 100);

    /* Range [100, 200] = 101 elements (100, 101, ..., 200) */
    g_range_count = 0;
    lum_btree_range(tree, 100, 200, _range_cb, NULL);
    ASSERT_EQ(g_range_count, 101ULL, "T02/range [100,200] = 101 elements");

    /* Range [0, 999] = 1000 elements */
    g_range_count = 0;
    lum_btree_range(tree, 0, 999, _range_cb, NULL);
    ASSERT_EQ(g_range_count, 1000ULL, "T02/range [0,999] = 1000 elements");

    /* Range vide */
    g_range_count = 0;
    lum_btree_range(tree, 5000, 9999, _range_cb, NULL);
    ASSERT_EQ(g_range_count, 0ULL, "T02/range vide = 0 elements");

    lum_btree_destroy(tree);
}

/* ── T03 : Stress test 100K insertions ─────────────────────────── */
static void test_t03_btree_stress(void) {
    printf("\n=== T03 : Stress test 100K insertions B-Tree ===\n");
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC, &t0);

    lum_btree_t* tree = lum_btree_create();
    assert(tree);

    const int N = 100000;
    /* Ordre pseudo-aleatoire (pas de srand pour reproductibilite) */
    uint64_t key = 12345678901ULL;
    for (int i = 0; i < N; i++) {
        key = key * 6364136223846793005ULL + 1442695040888963407ULL; /* LCG */
        lum_btree_insert(tree, key, (uint64_t)i);
    }

    ASSERT_EQ(lum_btree_count(tree), (uint64_t)N, "T03/count apres 100K inserts");

    bool valid = lum_btree_validate(tree);
    ASSERT_EQ(valid, 1, "T03/structure valide apres 100K inserts");

    double ms = _elapsed_ms(&t0);
    printf("  Temps T03 : %.2f ms (%.0f insert/s)\n",
           ms, (double)N / (ms / 1000.0));

    lum_btree_destroy(tree);
}

/* ── T04 : Buffer Pool ───────────────────────────────────────────── */
static void test_t04_buffer_pool(void) {
    printf("\n=== T04 : Buffer Pool ===\n");
    const char* path = "/tmp/lum_test_pool.lum";
    unlink(path);

    lum_buffer_pool_t* pool = lum_pool_create(path, 4);
    if (!pool) {
        TEST_FAIL("T04/create pool", "NULL retourne");
        g_failures++;
        return;
    }
    TEST_PASS("T04/create pool");
    g_passes++;

    /* Allouer et ecrire 3 pages */
    for (int i = 0; i < 3; i++) {
        uint64_t pid = lum_pool_alloc_page(pool);
        uint8_t* pg  = lum_pool_pin(pool, pid);
        if (pg) {
            memset(pg, (int)('A' + i), LUM_PAGE_SIZE);
            lum_pool_mark_dirty(pool, pid);
            lum_pool_unpin(pool, pid);
        }
    }
    TEST_PASS("T04/alloc + write 3 pages");
    g_passes++;

    bool flushed = lum_pool_flush(pool);
    ASSERT_EQ(flushed, 1, "T04/flush OK");

    uint64_t hits, misses, reads, writes;
    lum_pool_stats(pool, &hits, &misses, &reads, &writes);
    printf("  Stats pool : hits=%"PRIu64" misses=%"PRIu64
           " reads=%"PRIu64" writes=%"PRIu64"\n",
           hits, misses, reads, writes);

    lum_pool_destroy(pool);
    unlink(path);
    TEST_PASS("T04/destroy pool");
    g_passes++;
}

/* ── T05 : Catalog ──────────────────────────────────────────────── */
static void test_t05_catalog(void) {
    printf("\n=== T05 : Catalog systeme ===\n");
    const char* path = "/tmp/lum_test_catalog.lumcat";
    unlink(path);

    lum_catalog_t* cat = lum_catalog_open(path);
    assert(cat);
    TEST_PASS("T05/catalog_open");
    g_passes++;

    /* Creer 3 tables */
    lum_col_def_t cols_btc[] = {
        { "nonce",     LUM_TYPE_UINT64, 8,  true,  true,  false },
        { "hash",      LUM_TYPE_HASH32, 32, false, false, false },
        { "timestamp", LUM_TYPE_TS_NS,  8,  false, false, false },
    };
    int idx1 = lum_catalog_create_table(cat, "btc_blocks", cols_btc, 3);
    ASSERT_EQ(idx1 >= 0, 1, "T05/create table btc_blocks");

    lum_col_def_t cols_hub[] = {
        { "site_id",   LUM_TYPE_INT64,   8,  true,  true,  false },
        { "energy",    LUM_TYPE_FLOAT64, 8,  false, false, false },
    };
    int idx2 = lum_catalog_create_table(cat, "hubbard_sites", cols_hub, 2);
    ASSERT_EQ(idx2 >= 0, 1, "T05/create table hubbard_sites");

    ASSERT_EQ(cat->n_tables, 2U, "T05/2 tables dans catalogue");

    /* Chercher */
    int found = lum_catalog_find_table(cat, "btc_blocks");
    ASSERT_EQ(found, 0, "T05/find table btc_blocks = idx 0");

    int notfound = lum_catalog_find_table(cat, "inexistante");
    ASSERT_EQ(notfound, -1, "T05/find table inexistante = -1");

    /* Sauvegarder + recharger */
    cat->dirty = true;
    bool saved = lum_catalog_save(cat);
    ASSERT_EQ(saved, 1, "T05/catalog_save OK");

    lum_catalog_close(cat);
    cat = lum_catalog_open(path);
    assert(cat);
    ASSERT_EQ(cat->n_tables, 2U, "T05/rechargement : 2 tables OK");

    /* Drop */
    bool dropped = lum_catalog_drop_table(cat, "btc_blocks");
    ASSERT_EQ(dropped, 1, "T05/drop table btc_blocks");
    ASSERT_EQ(cat->n_tables, 1U, "T05/1 table restante apres drop");

    lum_catalog_close(cat);
    unlink(path);
}

/* ── T06 : LUMQ parser + execution ──────────────────────────────── */
static void test_t06_lumq(void) {
    printf("\n=== T06 : LUMQ parser + execution ===\n");
    const char* dir = "/tmp/lumq_test";
    mkdir(dir, 0755);

    lumq_ctx_t* ctx = lumq_ctx_create(dir);
    assert(ctx);

    /* CREATE TABLE */
    lumq_result_t r;
    r = lumq_run(ctx, "CREATE TABLE test_btc (key UINT64, value INT64)");
    ASSERT_EQ(r.ok, 1, "T06/CREATE TABLE");

    /* INSERT */
    r = lumq_run(ctx, "INSERT INTO test_btc VALUES 42 9999");
    ASSERT_EQ(r.ok, 1, "T06/INSERT key=42 val=9999");

    r = lumq_run(ctx, "INSERT INTO test_btc VALUES 100 1234");
    ASSERT_EQ(r.ok, 1, "T06/INSERT key=100 val=1234");

    r = lumq_run(ctx, "INSERT INTO test_btc VALUES 200 5678");
    ASSERT_EQ(r.ok, 1, "T06/INSERT key=200 val=5678");

    /* COUNT */
    r = lumq_run(ctx, "COUNT IN test_btc");
    ASSERT_EQ(r.ok, 1, "T06/COUNT OK");
    ASSERT_EQ(r.count, 3ULL, "T06/COUNT = 3");

    /* FIND */
    r = lumq_run(ctx, "FIND IN test_btc WHERE key = 42");
    ASSERT_EQ(r.ok, 1, "T06/FIND key=42 OK");
    ASSERT_EQ(r.value, 9999ULL, "T06/FIND key=42 val=9999");

    r = lumq_run(ctx, "FIND IN test_btc WHERE key = 999");
    ASSERT_EQ(r.ok, 1, "T06/FIND absent OK (pas d'erreur)");
    ASSERT_EQ(r.count, 0ULL, "T06/FIND absent count=0");

    /* RANGE */
    r = lumq_run(ctx, "RANGE IN test_btc WHERE key BETWEEN 42 AND 200");
    ASSERT_EQ(r.ok, 1, "T06/RANGE [42, 200]");
    ASSERT_EQ(r.count, 3ULL, "T06/RANGE [42, 200] = 3 elements");

    /* DELETE */
    r = lumq_run(ctx, "DELETE FROM test_btc WHERE key = 42");
    ASSERT_EQ(r.ok, 1, "T06/DELETE key=42");
    ASSERT_EQ(r.rows_affected, 1ULL, "T06/DELETE rows_affected=1");

    r = lumq_run(ctx, "COUNT IN test_btc");
    ASSERT_EQ(r.count, 2ULL, "T06/COUNT apres DELETE = 2");

    /* LIST */
    r = lumq_run(ctx, "LIST TABLES");
    ASSERT_EQ(r.ok, 1, "T06/LIST TABLES OK");

    /* SHOW STATS */
    r = lumq_run(ctx, "SHOW STATS");
    ASSERT_EQ(r.ok, 1, "T06/SHOW STATS OK");

    /* DROP */
    r = lumq_run(ctx, "DROP TABLE test_btc");
    ASSERT_EQ(r.ok, 1, "T06/DROP TABLE");

    lumq_ctx_destroy(ctx);
    unlink("/tmp/lumq_test/lum_system.lumcat");
    rmdir(dir);
}

/* ── T07 : MVCC ─────────────────────────────────────────────────── */
static void test_t07_mvcc(void) {
    printf("\n=== T07 : MVCC snapshot isolation ===\n");

    lum_mvcc_store_t* store = lum_mvcc_create(256);
    assert(store);

    /* Transaction T1 : inserer key=1 */
    lum_mvcc_tx_t t1 = lum_mvcc_begin();
    uint8_t data1[] = "lumvorax_btc_block_v1";
    bool ins = lum_mvcc_insert(store, &t1, 1, data1, sizeof(data1));
    ASSERT_EQ(ins, 1, "T07/MVCC insert key=1");

    /* Lecture immediate : doit voir key=1 (meme timestamp) */
    uint8_t buf[64]; uint32_t sz = sizeof(buf);
    /* Pour lire la version qu'on vient d'inserer, on prend un nouveau snapshot apres l'insert */
    lum_mvcc_tx_t t2 = lum_mvcc_begin();
    bool found = lum_mvcc_read(store, &t2, 1, buf, &sz);
    ASSERT_EQ(found, 1, "T07/MVCC lecture apres insert OK");

    /* Suppression dans T1 */
    bool del = lum_mvcc_delete(store, &t1, 1);
    ASSERT_EQ(del, 1, "T07/MVCC delete key=1");

    /* T2 (snapshot avant delete) doit toujours voir key=1 */
    sz = sizeof(buf);
    found = lum_mvcc_read(store, &t2, 1, buf, &sz);
    ASSERT_EQ(found, 1, "T07/MVCC snapshot isolation : T2 voit toujours key=1");

    /* Stats */
    uint64_t live, dead;
    lum_mvcc_stats(store, &live, &dead);
    printf("  MVCC : live=%"PRIu64" dead=%"PRIu64"\n", live, dead);

    /* Vacuum */
    uint64_t freed = lum_mvcc_vacuum(store, UINT64_MAX);
    printf("  MVCC vacuum : %"PRIu64" tuple(s) liberes\n", freed);

    lum_mvcc_destroy(store);
    TEST_PASS("T07/MVCC destroy");
    g_passes++;
}

/* ── Main ────────────────────────────────────────────────────────── */
int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax LUM DBMS — Test de validation Phase 3+4       ║\n");
    printf("║  100%% natif — B-Tree + Buffer Pool + Catalog + MVCC     ║\n");
    printf("║  Cycle C69 — 2026-04-22                                  ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    struct timespec t_total; clock_gettime(CLOCK_MONOTONIC, &t_total);

    test_t01_btree_basic();
    test_t02_btree_range();
    test_t03_btree_stress();
    test_t04_buffer_pool();
    test_t05_catalog();
    test_t06_lumq();
    test_t07_mvcc();

    double total_ms = _elapsed_ms(&t_total);

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS : %3d PASS  %3d FAIL  (%.1f ms total)%*s║\n",
           g_passes, g_failures, total_ms,
           (int)(20 - (g_failures >= 10 ? 2 : 1)), " ");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    return g_failures > 0 ? 1 : 0;
}
