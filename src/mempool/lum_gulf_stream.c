/**
 * @file lum_gulf_stream.c
 * @brief Implémentation Gulf Stream - Mempool Sans Cache
 */

#include "lum_gulf_stream.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

lum_gulf_stream_t* lum_gulf_stream_init(lum_poh_t* poh) {
    if (!poh) return NULL;
    
    lum_gulf_stream_t* gs = (lum_gulf_stream_t*)calloc(1, sizeof(lum_gulf_stream_t));
    if (!gs) return NULL;
    
    gs->poh = poh;
    gs->max_pending = LUM_GULF_STREAM_MAX_TX;
    gs->pending_txs = (lum_gulf_stream_tx_t**)calloc(gs->max_pending, sizeof(lum_gulf_stream_tx_t*));
    gs->leaders = (uint8_t**)calloc(1024, sizeof(uint8_t*));
    
    pthread_mutex_init(&gs->mutex, NULL);
    gs->running = false;
    
    return gs;
}

void lum_gulf_stream_free(lum_gulf_stream_t* gs) {
    if (!gs) return;
    
    if (gs->running) lum_gulf_stream_stop(gs);
    
    for (uint32_t i = 0; i < gs->num_pending; i++) {
        if (gs->pending_txs[i]) {
            free(gs->pending_txs[i]->data);
            free(gs->pending_txs[i]);
        }
    }
    
    for (uint32_t i = 0; i < 1024; i++) {
        free(gs->leaders[i]);
    }
    
    free(gs->pending_txs);
    free(gs->leaders);
    pthread_mutex_destroy(&gs->mutex);
    free(gs);
}

static void* forward_thread_func(void* arg) {
    lum_gulf_stream_t* gs = (lum_gulf_stream_t*)arg;
    
    while (gs->running) {
        pthread_mutex_lock(&gs->mutex);
        
        lum_poh_entry_t poh_entry;
        if (lum_poh_get_current(gs->poh, &poh_entry)) {
            uint64_t current_slot = poh_entry.slot;
            
            for (uint32_t i = 0; i < gs->num_pending; i++) {
                lum_gulf_stream_tx_t* tx = gs->pending_txs[i];
                if (tx && !tx->forwarded && tx->target_slot <= current_slot + LUM_GULF_STREAM_FORWARD_SLOTS) {
                    lum_gulf_stream_forward_tx(gs, tx);
                }
            }
        }
        
        pthread_mutex_unlock(&gs->mutex);
        usleep(1000);  // 1ms
    }
    
    return NULL;
}

bool lum_gulf_stream_start(lum_gulf_stream_t* gs) {
    if (!gs || gs->running) return false;
    
    gs->running = true;
    return pthread_create(&gs->forward_thread, NULL, forward_thread_func, gs) == 0;
}

void lum_gulf_stream_stop(lum_gulf_stream_t* gs) {
    if (!gs || !gs->running) return;
    
    gs->running = false;
    pthread_join(gs->forward_thread, NULL);
}

bool lum_gulf_stream_submit_tx(lum_gulf_stream_t* gs, const uint8_t* signature,
                                const uint8_t* data, uint32_t size, uint64_t fee) {
    if (!gs || !signature || !data || size > LUM_GULF_STREAM_MAX_TX_SIZE) return false;
    
    pthread_mutex_lock(&gs->mutex);
    
    if (gs->num_pending >= gs->max_pending) {
        pthread_mutex_unlock(&gs->mutex);
        return false;
    }
    
    lum_gulf_stream_tx_t* tx = (lum_gulf_stream_tx_t*)calloc(1, sizeof(lum_gulf_stream_tx_t));
    memcpy(tx->signature, signature, 64);
    tx->data = (uint8_t*)malloc(size);
    memcpy(tx->data, data, size);
    tx->size = size;
    tx->fee = fee;
    
    lum_poh_entry_t poh_entry;
    lum_poh_get_current(gs->poh, &poh_entry);
    tx->target_slot = poh_entry.slot + LUM_GULF_STREAM_FORWARD_SLOTS;
    tx->timestamp_ns = poh_entry.timestamp_ns;
    tx->forwarded = false;
    
    gs->pending_txs[gs->num_pending++] = tx;
    gs->total_submitted++;
    
    pthread_mutex_unlock(&gs->mutex);
    return true;
}

bool lum_gulf_stream_forward_tx(lum_gulf_stream_t* gs, lum_gulf_stream_tx_t* tx) {
    if (!gs || !tx || tx->forwarded) return false;
    
    // TODO: Envoyer au leader du slot cible
    tx->forwarded = true;
    gs->total_forwarded++;
    
    return true;
}

void lum_gulf_stream_set_leader(lum_gulf_stream_t* gs, uint64_t slot, const uint8_t* leader_pubkey) {
    if (!gs || !leader_pubkey) return;
    
    pthread_mutex_lock(&gs->mutex);
    
    uint32_t idx = slot % 1024;
    if (!gs->leaders[idx]) {
        gs->leaders[idx] = (uint8_t*)malloc(32);
    }
    memcpy(gs->leaders[idx], leader_pubkey, 32);
    
    pthread_mutex_unlock(&gs->mutex);
}

void lum_gulf_stream_get_metrics(lum_gulf_stream_t* gs, uint64_t* submitted,
                                  uint64_t* forwarded, uint64_t* executed, double* avg_forward_ms) {
    if (!gs) return;
    
    pthread_mutex_lock(&gs->mutex);
    if (submitted) *submitted = gs->total_submitted;
    if (forwarded) *forwarded = gs->total_forwarded;
    if (executed) *executed = gs->total_executed;
    if (avg_forward_ms) *avg_forward_ms = gs->avg_forward_time_ms;
    pthread_mutex_unlock(&gs->mutex);
}

// Made with Bob
