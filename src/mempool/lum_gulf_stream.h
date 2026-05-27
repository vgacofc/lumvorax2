/**
 * @file lum_gulf_stream.h
 * @brief LumVorax Gulf Stream - Mempool Sans Cache Avancé
 * 
 * Gulf Stream permet de forwarder les transactions vers les leaders futurs
 * avant même que leur slot ne soit actif, éliminant la latence du mempool.
 * 
 * Caractéristiques :
 * - Forward transactions vers leaders futurs (2-4 slots à l'avance)
 * - Pas de cache mempool traditionnel
 * - Validation parallèle des transactions
 * - Priorisation par frais
 * 
 * Performance cible :
 * - Latence : <10ms pour forward
 * - Throughput : 100K+ tx/sec
 * 
 * @version 1.0.0
 * @date 2026-05-02
 * @author Bob (LumVorax Core Team)
 */

#ifndef LUM_GULF_STREAM_H
#define LUM_GULF_STREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "../consensus/lum_poh.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define LUM_GULF_STREAM_FORWARD_SLOTS 2     // Forward 2 slots à l'avance
#define LUM_GULF_STREAM_MAX_TX 1000000      // Max transactions en vol
#define LUM_GULF_STREAM_MAX_TX_SIZE 1232    // Taille max transaction

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

typedef struct {
    uint8_t signature[64];                  // Signature transaction
    uint8_t* data;                          // Données transaction
    uint32_t size;                          // Taille données
    uint64_t fee;                           // Frais transaction
    uint64_t target_slot;                   // Slot cible
    uint64_t timestamp_ns;                  // Timestamp soumission
    bool forwarded;                         // Déjà forwardé ?
} lum_gulf_stream_tx_t;

typedef struct {
    lum_poh_t* poh;                         // Référence PoH
    
    // Transactions en vol
    lum_gulf_stream_tx_t** pending_txs;
    uint32_t num_pending;
    uint32_t max_pending;
    
    // Leaders par slot
    uint8_t** leaders;                      // leaders[slot % 1024] = pubkey
    
    // Thread forwarding
    pthread_t forward_thread;
    bool running;
    
    // Synchronisation
    pthread_mutex_t mutex;
    
    // Métriques
    uint64_t total_submitted;
    uint64_t total_forwarded;
    uint64_t total_executed;
    double avg_forward_time_ms;
} lum_gulf_stream_t;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

lum_gulf_stream_t* lum_gulf_stream_init(lum_poh_t* poh);
void lum_gulf_stream_free(lum_gulf_stream_t* gs);
bool lum_gulf_stream_start(lum_gulf_stream_t* gs);
void lum_gulf_stream_stop(lum_gulf_stream_t* gs);

bool lum_gulf_stream_submit_tx(lum_gulf_stream_t* gs,
                                const uint8_t* signature,
                                const uint8_t* data,
                                uint32_t size,
                                uint64_t fee);

bool lum_gulf_stream_forward_tx(lum_gulf_stream_t* gs,
                                 lum_gulf_stream_tx_t* tx);

void lum_gulf_stream_set_leader(lum_gulf_stream_t* gs,
                                 uint64_t slot,
                                 const uint8_t* leader_pubkey);

void lum_gulf_stream_get_metrics(lum_gulf_stream_t* gs,
                                  uint64_t* submitted,
                                  uint64_t* forwarded,
                                  uint64_t* executed,
                                  double* avg_forward_ms);

#ifdef __cplusplus
}
#endif

#endif /* LUM_GULF_STREAM_H */

// Made with Bob
