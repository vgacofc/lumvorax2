/*
 * LumVorax Bitcoin Mining - Gen9 Native Minimal Test
 * C223 - Version 100% native i915 DRM (0% OpenCL)
 * 
 * Programme minimal pour tester l'API Gen9 Native
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "btc_gen9_native_runner.h"

/* Configuration test */
#define TEST_DURATION_SEC 60
#define BATCH_SIZE 2621440  /* 2.6M nonces (solution limite 9 dispatches Gen9) */
#define WORK_GROUP_SIZE 256

/* Structure pour les résultats */
typedef struct {
    uint32_t nonce;
    uint32_t leading_zeros;
    uint64_t timestamp_ns;
} btc_result_t;

/* Fonction utilitaire : timestamp nanoseconde - Supprimée car déjà définie dans btc_gen9_native_runner.h */

/* Fonction utilitaire : affichage hashrate */
static void print_hashrate(uint64_t total_hashes, uint64_t elapsed_ns) {
    double elapsed_sec = (double)elapsed_ns / 1e9;
    double hashrate_mhs = ((double)total_hashes / elapsed_sec) / 1e6;
    printf("[HASHRATE] %.2f MH/s (%.0f hashes en %.2f sec)\n",
           hashrate_mhs, (double)total_hashes, elapsed_sec);
}

int main(int argc, char** argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("LumVorax Bitcoin Mining - Gen9 Native Test C223\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Architecture : Application → i915 DRM → GPU Gen9\n");
    printf("Dépendance OpenCL : 0%% (objectif atteint)\n");
    printf("Durée test : %d secondes\n", TEST_DURATION_SEC);
    printf("Batch size : %u nonces\n", BATCH_SIZE);
    printf("═══════════════════════════════════════════════════════════════\n\n");

    /* Configuration Gen9 Native */
    btc_gen9_config_t config = {
        .batch_size = BATCH_SIZE,
        .work_group_size = WORK_GROUP_SIZE,
        .kernel_path = "btc_sha256_opt.bin",  /* C255v8n FIX: Chemin relatif depuis level_zero_native/ */
        .log_path = "logs/forensic/gen9_native_test_c223.log",
        .enable_profiling = true
    };

    /* Initialisation Gen9 Native */
    printf("[INIT] Initialisation GPU Gen9 Native...\n");
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret != 0 || ctx == NULL) {
        fprintf(stderr, "[ERROR] Échec initialisation Gen9 Native (ret=%d)\n", ret);
        return 1;
    }
    printf("[INIT] ✓ GPU Gen9 Native initialisé\n\n");

    /* Données de test Bitcoin (block header simplifié) */
    uint8_t block_header[80];
    memset(block_header, 0, 80);
    /* Version */
    block_header[0] = 0x01;
    block_header[1] = 0x00;
    block_header[2] = 0x00;
    block_header[3] = 0x00;
    /* Prev block hash (exemple) */
    memset(&block_header[4], 0xAA, 32);
    /* Merkle root (exemple) */
    memset(&block_header[36], 0xBB, 32);
    /* Timestamp */
    uint32_t timestamp = (uint32_t)time(NULL);
    memcpy(&block_header[68], &timestamp, 4);
    /* Bits (difficulté) */
    uint32_t bits = 0x1d00ffff;  /* Difficulté Bitcoin testnet */
    memcpy(&block_header[72], &bits, 4);
    /* Nonce sera rempli par le GPU */

    printf("[TEST] Démarrage mining GPU...\n");
    printf("[TEST] Target : recherche leading zeros maximaux\n\n");

    uint64_t start_time = get_timestamp_ns();
    uint64_t total_hashes = 0;
    uint32_t dispatch_count = 0;
    uint32_t best_leading_zeros = 0;
    uint32_t best_nonce = 0;

    /* Boucle de mining */
    while (1) {
        uint64_t current_time = get_timestamp_ns();
        uint64_t elapsed_ns = current_time - start_time;
        
        /* Vérifier timeout */
        if (elapsed_ns >= (uint64_t)TEST_DURATION_SEC * 1000000000ULL) {
            break;
        }

        /* Exécuter batch GPU */
        uint32_t start_nonce = dispatch_count * BATCH_SIZE;
        btc_result_t result;
        memset(&result, 0, sizeof(result));

        ret = btc_gen9_execute_mining(ctx, block_header, start_nonce, BATCH_SIZE,
                                      &result.nonce, &result.leading_zeros);
        
        if (ret != 0) {
            fprintf(stderr, "[WARNING] Dispatch %u échoué (ret=%d)\n", dispatch_count, ret);
            dispatch_count++;  /* C255v8n FIX: Incrémenter même en cas d'échec pour éviter boucle infinie */
            continue;
        }

        dispatch_count++;
        total_hashes += BATCH_SIZE;

        /* Vérifier si meilleur résultat */
        if (result.leading_zeros > best_leading_zeros) {
            best_leading_zeros = result.leading_zeros;
            best_nonce = result.nonce;
            printf("[FOUND] Dispatch %u : %u leading zeros (nonce=0x%08X)\n",
                   dispatch_count, result.leading_zeros, result.nonce);
        }

        /* Afficher progression toutes les 10 dispatches */
        if (dispatch_count % 10 == 0) {
            print_hashrate(total_hashes, elapsed_ns);
        }
    }

    /* Résultats finaux */
    uint64_t total_time = get_timestamp_ns() - start_time;
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("RÉSULTATS FINAUX C223\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Durée totale : %.2f secondes\n", (double)total_time / 1e9);
    printf("Dispatches GPU : %u\n", dispatch_count);
    printf("Total hashes : %lu\n", total_hashes);
    print_hashrate(total_hashes, total_time);
    printf("Meilleur résultat : %u leading zeros (nonce=0x%08X)\n",
           best_leading_zeros, best_nonce);
    printf("═══════════════════════════════════════════════════════════════\n");

    /* Afficher métriques Gen9 */
    printf("\n[METRICS] Métriques GPU Gen9 Native :\n");
    btc_gen9_print_metrics(ctx);

    /* Cleanup */
    printf("\n[CLEANUP] Libération ressources GPU...\n");
    btc_gen9_cleanup(ctx);
    printf("[CLEANUP] ✓ Cleanup terminé\n");

    return 0;
}

// Made with Bob
