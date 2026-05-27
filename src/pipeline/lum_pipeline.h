/**
 * @file lum_pipeline.h
 * @brief LumVorax Pipeline - Architecture TPU-Style 4 Étapes
 * 
 * Pipeline de traitement continu des transactions en 4 étapes parallèles :
 * 1. Fetch - Récupération transactions
 * 2. SigVerify - Vérification signatures (GPU/TPU)
 * 3. Banking - Exécution transactions
 * 4. Write - Écriture état
 * 
 * Performance cible : 100K+ tx/sec avec latence <50ms
 * 
 * @version 1.0.0
 * @date 2026-05-02
 */

#ifndef LUM_PIPELINE_H
#define LUM_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LUM_PIPELINE_BUFFER_SIZE 10000

typedef enum {
    STAGE_FETCH,
    STAGE_SIG_VERIFY,
    STAGE_BANKING,
    STAGE_WRITE
} lum_pipeline_stage_t;

typedef struct {
    uint8_t signature[64];
    uint8_t* data;
    uint32_t size;
    bool sig_verified;
    bool executed;
    bool written;
} lum_pipeline_tx_t;

typedef struct {
    lum_pipeline_tx_t** buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} lum_pipeline_queue_t;

typedef struct {
    // Queues entre étapes
    lum_pipeline_queue_t* fetch_queue;
    lum_pipeline_queue_t* verify_queue;
    lum_pipeline_queue_t* banking_queue;
    lum_pipeline_queue_t* write_queue;
    
    // Threads par étape
    pthread_t fetch_thread;
    pthread_t verify_thread;
    pthread_t banking_thread;
    pthread_t write_thread;
    
    bool running;
    
    // Métriques
    uint64_t total_fetched;
    uint64_t total_verified;
    uint64_t total_executed;
    uint64_t total_written;
    double avg_latency_ms;
} lum_pipeline_t;

lum_pipeline_t* lum_pipeline_init(void);
void lum_pipeline_free(lum_pipeline_t* pipeline);
bool lum_pipeline_start(lum_pipeline_t* pipeline);
void lum_pipeline_stop(lum_pipeline_t* pipeline);

bool lum_pipeline_submit(lum_pipeline_t* pipeline, const uint8_t* signature,
                         const uint8_t* data, uint32_t size);

void lum_pipeline_get_metrics(lum_pipeline_t* pipeline, uint64_t* fetched,
                              uint64_t* verified, uint64_t* executed,
                              uint64_t* written, double* avg_latency_ms);

#ifdef __cplusplus
}
#endif

#endif /* LUM_PIPELINE_H */

// Made with Bob
