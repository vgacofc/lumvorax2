/**
 * @file lum_sealevel.h
 * @brief LumVorax Sealevel - Runtime Parallèle pour Smart Contracts
 * 
 * Sealevel permet l'exécution parallèle de transactions en analysant
 * les dépendances entre comptes (lecture/écriture).
 * 
 * Performance cible : 100K+ tx/sec en parallèle
 * 
 * @version 1.0.0
 * @date 2026-05-02
 */

#ifndef LUM_SEALEVEL_H
#define LUM_SEALEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LUM_SEALEVEL_MAX_ACCOUNTS 256
#define LUM_SEALEVEL_MAX_THREADS 64

typedef struct {
    uint8_t account_id[32];
    bool is_writable;
} lum_account_ref_t;

typedef struct {
    uint8_t signature[64];
    uint8_t* bytecode;
    uint32_t bytecode_size;
    lum_account_ref_t accounts[LUM_SEALEVEL_MAX_ACCOUNTS];
    uint32_t num_accounts;
    uint64_t compute_units;
} lum_sealevel_tx_t;

typedef struct {
    uint32_t num_threads;
    pthread_t* threads;
    bool running;
    
    // Métriques
    uint64_t total_executed;
    uint64_t total_parallel;
    double avg_execution_time_ms;
} lum_sealevel_t;

lum_sealevel_t* lum_sealevel_init(uint32_t num_threads);
void lum_sealevel_free(lum_sealevel_t* vm);
bool lum_sealevel_execute_parallel(lum_sealevel_t* vm, lum_sealevel_tx_t** txs, uint32_t count);
void lum_sealevel_get_metrics(lum_sealevel_t* vm, uint64_t* executed, uint64_t* parallel, double* avg_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* LUM_SEALEVEL_H */

// Made with Bob
