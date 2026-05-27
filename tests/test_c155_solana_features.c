/**
 * @file test_c155_solana_features.c
 * @brief Tests d'Intégration - 7 Fonctionnalités Solana
 * 
 * Tests complets pour :
 * 1. Proof of History (PoH)
 * 2. Tower BFT
 * 3. Turbine
 * 4. Gulf Stream
 * 5. Sealevel
 * 6. Cloudbreak
 * 7. Pipelining
 * 
 * @version 1.0.0
 * @date 2026-05-02
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "../src/consensus/lum_poh.h"
#include "../src/consensus/lum_tower_bft.h"
#include "../src/network/lum_turbine.h"
#include "../src/mempool/lum_gulf_stream.h"
#include "../src/vm/lum_sealevel.h"
#include "../src/storage/lum_cloudbreak.h"
#include "../src/pipeline/lum_pipeline.h"

#define TEST_DURATION_SEC 5
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET "\x1b[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf(ANSI_GREEN "✓" ANSI_RESET " %s\n", msg); \
        tests_passed++; \
    } else { \
        printf(ANSI_RED "✗" ANSI_RESET " %s\n", msg); \
        tests_failed++; \
    } \
} while(0)

/* ============================================================================
 * TEST 1 : PROOF OF HISTORY
 * ============================================================================ */

void test_poh(void) {
    printf("\n" ANSI_YELLOW "=== TEST 1: PROOF OF HISTORY ===" ANSI_RESET "\n");
    
    // Initialiser PoH
    lum_poh_t* poh = lum_poh_init(1000000, true);
    TEST_ASSERT(poh != NULL, "PoH init");
    
    // Démarrer générateur
    TEST_ASSERT(lum_poh_start(poh), "PoH start");
    
    // Attendre génération
    sleep(2);
    
    // Vérifier métriques
    double tps;
    uint64_t total_ticks;
    double uptime;
    lum_poh_get_metrics(poh, &tps, &total_ticks, &uptime);
    
    printf("  Ticks/sec: %.0f (cible: 1M)\n", tps);
    printf("  Total ticks: %lu\n", total_ticks);
    printf("  Uptime: %.2fs\n", uptime);
    
    TEST_ASSERT(tps > 100000, "PoH performance >100K ticks/sec");
    TEST_ASSERT(total_ticks > 200000, "PoH total ticks >200K");
    
    // Tester mix transaction
    uint8_t tx_hash[32] = {0};
    lum_poh_proof_t proof;
    TEST_ASSERT(lum_poh_mix_transaction(poh, tx_hash, &proof), "PoH mix transaction");
    
    // Arrêter
    lum_poh_stop(poh);
    lum_poh_free(poh);
}

/* ============================================================================
 * TEST 2 : TOWER BFT
 * ============================================================================ */

void test_tower_bft(void) {
    printf("\n" ANSI_YELLOW "=== TEST 2: TOWER BFT ===" ANSI_RESET "\n");
    
    // Initialiser PoH pour Tower BFT
    lum_poh_t* poh = lum_poh_init(1000000, false);
    lum_poh_start(poh);
    
    // Initialiser consensus
    lum_tower_consensus_t* consensus = lum_tower_consensus_init(poh);
    TEST_ASSERT(consensus != NULL, "Tower BFT init");
    
    // Ajouter validateurs
    uint8_t validator1[32] = {1};
    uint8_t validator2[32] = {2};
    uint8_t validator3[32] = {3};
    
    TEST_ASSERT(lum_tower_consensus_add_validator(consensus, validator1, 1000), "Add validator 1");
    TEST_ASSERT(lum_tower_consensus_add_validator(consensus, validator2, 1000), "Add validator 2");
    TEST_ASSERT(lum_tower_consensus_add_validator(consensus, validator3, 1000), "Add validator 3");
    
    // Soumettre votes
    uint8_t vote_hash[32] = {0xAA};
    TEST_ASSERT(lum_tower_consensus_submit_vote(consensus, validator1, 1, vote_hash), "Submit vote 1");
    TEST_ASSERT(lum_tower_consensus_submit_vote(consensus, validator2, 1, vote_hash), "Submit vote 2");
    
    // Vérifier supermajority (67% = 2/3)
    TEST_ASSERT(lum_tower_consensus_has_supermajority(consensus, 1), "Supermajority reached");
    
    // Métriques
    uint64_t votes, confirmations;
    lum_tower_consensus_get_metrics(consensus, &votes, &confirmations, NULL, NULL);
    printf("  Total votes: %lu\n", votes);
    printf("  Confirmations: %lu\n", confirmations);
    
    lum_tower_consensus_free(consensus);
    lum_poh_stop(poh);
    lum_poh_free(poh);
}

/* ============================================================================
 * TEST 3 : TURBINE
 * ============================================================================ */

void test_turbine(void) {
    printf("\n" ANSI_YELLOW "=== TEST 3: TURBINE ===" ANSI_RESET "\n");
    
    uint8_t node_id[32] = {0xFF};
    lum_turbine_t* turbine = lum_turbine_init(node_id, "127.0.0.1", 8000, 200);
    TEST_ASSERT(turbine != NULL, "Turbine init");
    
    // Ajouter nœuds
    uint8_t node2[32] = {0x01};
    uint8_t node3[32] = {0x02};
    TEST_ASSERT(lum_turbine_add_node(turbine, node2, "127.0.0.2", 8001), "Add node 2");
    TEST_ASSERT(lum_turbine_add_node(turbine, node3, "127.0.0.3", 8002), "Add node 3");
    
    // Reconstruire arbre
    TEST_ASSERT(lum_turbine_rebuild_tree(turbine), "Rebuild tree");
    
    // Broadcast bloc
    uint8_t block_data[1024] = {0xBB};
    TEST_ASSERT(lum_turbine_broadcast_block(turbine, 1, block_data, 1024), "Broadcast block");
    
    // Métriques
    uint64_t blocks_sent, blocks_received;
    lum_turbine_get_metrics(turbine, &blocks_sent, &blocks_received, NULL, NULL, NULL);
    printf("  Blocks sent: %lu\n", blocks_sent);
    
    lum_turbine_free(turbine);
}

/* ============================================================================
 * TEST 4 : GULF STREAM
 * ============================================================================ */

void test_gulf_stream(void) {
    printf("\n" ANSI_YELLOW "=== TEST 4: GULF STREAM ===" ANSI_RESET "\n");
    
    lum_poh_t* poh = lum_poh_init(1000000, false);
    lum_poh_start(poh);
    
    lum_gulf_stream_t* gs = lum_gulf_stream_init(poh);
    TEST_ASSERT(gs != NULL, "Gulf Stream init");
    TEST_ASSERT(lum_gulf_stream_start(gs), "Gulf Stream start");
    
    // Soumettre transactions
    uint8_t sig[64] = {0xCC};
    uint8_t tx_data[100] = {0xDD};
    
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT(lum_gulf_stream_submit_tx(gs, sig, tx_data, 100, 1000), "Submit tx");
    }
    
    sleep(1);
    
    // Métriques
    uint64_t submitted, forwarded;
    lum_gulf_stream_get_metrics(gs, &submitted, &forwarded, NULL, NULL);
    printf("  Submitted: %lu\n", submitted);
    printf("  Forwarded: %lu\n", forwarded);
    
    lum_gulf_stream_stop(gs);
    lum_gulf_stream_free(gs);
    lum_poh_stop(poh);
    lum_poh_free(poh);
}

/* ============================================================================
 * TEST 5 : SEALEVEL
 * ============================================================================ */

void test_sealevel(void) {
    printf("\n" ANSI_YELLOW "=== TEST 5: SEALEVEL ===" ANSI_RESET "\n");
    
    lum_sealevel_t* vm = lum_sealevel_init(8);
    TEST_ASSERT(vm != NULL, "Sealevel init");
    
    // Créer transactions
    lum_sealevel_tx_t* txs[100];
    for (int i = 0; i < 100; i++) {
        txs[i] = (lum_sealevel_tx_t*)calloc(1, sizeof(lum_sealevel_tx_t));
        txs[i]->num_accounts = 2;
        txs[i]->accounts[0].is_writable = (i % 2 == 0);
        txs[i]->accounts[1].is_writable = false;
    }
    
    // Exécuter en parallèle
    TEST_ASSERT(lum_sealevel_execute_parallel(vm, txs, 100), "Execute parallel");
    
    // Métriques
    uint64_t executed, parallel;
    lum_sealevel_get_metrics(vm, &executed, &parallel, NULL);
    printf("  Executed: %lu\n", executed);
    printf("  Parallel: %lu\n", parallel);
    
    for (int i = 0; i < 100; i++) free(txs[i]);
    lum_sealevel_free(vm);
}

/* ============================================================================
 * TEST 6 : CLOUDBREAK
 * ============================================================================ */

void test_cloudbreak(void) {
    printf("\n" ANSI_YELLOW "=== TEST 6: CLOUDBREAK ===" ANSI_RESET "\n");
    
    lum_cloudbreak_t* cb = lum_cloudbreak_init(4, "/tmp/cloudbreak_test");
    TEST_ASSERT(cb != NULL, "Cloudbreak init");
    
    // Store comptes
    uint8_t account1[32] = {0x11};
    uint8_t account2[32] = {0x22};
    uint8_t data1[100] = {0xAA};
    uint8_t data2[100] = {0xBB};
    
    TEST_ASSERT(lum_cloudbreak_store(cb, account1, data1, 100), "Store account 1");
    TEST_ASSERT(lum_cloudbreak_store(cb, account2, data2, 100), "Store account 2");
    
    // Load comptes
    uint8_t loaded[100];
    uint32_t size;
    TEST_ASSERT(lum_cloudbreak_load(cb, account1, loaded, &size), "Load account 1");
    TEST_ASSERT(size == 100, "Correct size");
    TEST_ASSERT(memcmp(loaded, data1, 100) == 0, "Correct data");
    
    // Métriques
    uint64_t reads, writes;
    lum_cloudbreak_get_metrics(cb, &reads, &writes, NULL, NULL);
    printf("  Reads: %lu\n", reads);
    printf("  Writes: %lu\n", writes);
    
    lum_cloudbreak_free(cb);
}

/* ============================================================================
 * TEST 7 : PIPELINING
 * ============================================================================ */

void test_pipeline(void) {
    printf("\n" ANSI_YELLOW "=== TEST 7: PIPELINING ===" ANSI_RESET "\n");
    
    lum_pipeline_t* pipeline = lum_pipeline_init();
    TEST_ASSERT(pipeline != NULL, "Pipeline init");
    TEST_ASSERT(lum_pipeline_start(pipeline), "Pipeline start");
    
    // Soumettre transactions
    uint8_t sig[64] = {0xEE};
    uint8_t data[100] = {0xFF};
    
    for (int i = 0; i < 1000; i++) {
        lum_pipeline_submit(pipeline, sig, data, 100);
    }
    
    sleep(2);
    
    // Métriques
    uint64_t fetched, verified, executed, written;
    lum_pipeline_get_metrics(pipeline, &fetched, &verified, &executed, &written, NULL);
    printf("  Fetched: %lu\n", fetched);
    printf("  Verified: %lu\n", verified);
    printf("  Executed: %lu\n", executed);
    printf("  Written: %lu\n", written);
    
    TEST_ASSERT(written > 0, "Pipeline processed transactions");
    
    lum_pipeline_stop(pipeline);
    lum_pipeline_free(pipeline);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LUMVORAX C155 - TESTS FONCTIONNALITÉS SOLANA            ║\n");
    printf("║  Objectif: Dépasser Solana (65K TPS → 100K+ TPS)         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    test_poh();
    test_tower_bft();
    test_turbine();
    test_gulf_stream();
    test_sealevel();
    test_cloudbreak();
    test_pipeline();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS TESTS                                          ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  " ANSI_GREEN "✓ Tests réussis : %3d" ANSI_RESET "                                ║\n", tests_passed);
    printf("║  " ANSI_RED "✗ Tests échoués : %3d" ANSI_RESET "                                ║\n", tests_failed);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return tests_failed > 0 ? 1 : 0;
}

// Made with Bob
