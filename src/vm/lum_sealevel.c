/**
 * @file lum_sealevel.c
 * @brief Implémentation Sealevel - Runtime Parallèle
 */

#include "lum_sealevel.h"
#include <stdlib.h>
#include <string.h>
#include <omp.h>

lum_sealevel_t* lum_sealevel_init(uint32_t num_threads) {
    lum_sealevel_t* vm = (lum_sealevel_t*)calloc(1, sizeof(lum_sealevel_t));
    if (!vm) return NULL;
    
    vm->num_threads = (num_threads > 0 && num_threads <= LUM_SEALEVEL_MAX_THREADS) ? 
                      num_threads : omp_get_max_threads();
    vm->running = false;
    
    return vm;
}

void lum_sealevel_free(lum_sealevel_t* vm) {
    if (!vm) return;
    free(vm);
}

static bool has_conflict(lum_sealevel_tx_t* tx1, lum_sealevel_tx_t* tx2) {
    for (uint32_t i = 0; i < tx1->num_accounts; i++) {
        for (uint32_t j = 0; j < tx2->num_accounts; j++) {
            if (memcmp(tx1->accounts[i].account_id, tx2->accounts[j].account_id, 32) == 0) {
                if (tx1->accounts[i].is_writable || tx2->accounts[j].is_writable) {
                    return true;  // Conflit lecture/écriture ou écriture/écriture
                }
            }
        }
    }
    return false;
}

bool lum_sealevel_execute_parallel(lum_sealevel_t* vm, lum_sealevel_tx_t** txs, uint32_t count) {
    if (!vm || !txs || count == 0) return false;
    
    // Analyser dépendances et créer groupes parallèles
    bool* executed = (bool*)calloc(count, sizeof(bool));
    uint64_t parallel_count = 0;
    
    #pragma omp parallel for num_threads(vm->num_threads)
    for (uint32_t i = 0; i < count; i++) {
        if (executed[i]) continue;
        
        bool can_execute = true;
        for (uint32_t j = 0; j < i; j++) {
            if (!executed[j] && has_conflict(txs[i], txs[j])) {
                can_execute = false;
                break;
            }
        }
        
        if (can_execute) {
            // Exécuter transaction (simulation)
            executed[i] = true;
            #pragma omp atomic
            parallel_count++;
        }
    }
    
    vm->total_executed += count;
    vm->total_parallel += parallel_count;
    
    free(executed);
    return true;
}

void lum_sealevel_get_metrics(lum_sealevel_t* vm, uint64_t* executed, 
                               uint64_t* parallel, double* avg_time_ms) {
    if (!vm) return;
    if (executed) *executed = vm->total_executed;
    if (parallel) *parallel = vm->total_parallel;
    if (avg_time_ms) *avg_time_ms = vm->avg_execution_time_ms;
}

// Made with Bob
