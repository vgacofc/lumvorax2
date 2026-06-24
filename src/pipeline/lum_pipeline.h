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

// Buffers dynamiques OPTIMAUX : allocation adaptative selon charge
// Dimensionnement optimal : 5× TPS pic (47,456) × latence (0.05s) = 11,864 TX → arrondi 15,000
// LEÇON-317 : Capacités Maximales ≠ Performance Maximale (marge 5× > 20×)
#define LUM_PIPELINE_BUFFER_SIZE_INITIAL 15000    // Optimal validé (vs 50K excessif)
#define LUM_PIPELINE_BUFFER_SIZE_MAX 150000       // Optimal validé (vs 500K excessif)
#define LUM_PIPELINE_BUFFER_GROWTH_FACTOR 2       // Doublement rapide
#define LUM_PIPELINE_BUFFER_SHRINK_THRESHOLD 0.25 // Réduction si usage <25%

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
    uint32_t max_capacity;  // Capacité maximale pour croissance dynamique
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
    
    // Accélérateurs externes (pointeurs opaques)
    void* gpu_context;      // lum_poh_gpu_context_t*
    void* sealevel_vm;      // lum_sealevel_t*
    
    // Métriques
    uint64_t total_fetched;
    uint64_t total_verified;
    uint64_t total_executed;
    uint64_t total_written;
    double avg_latency_ms;
    
    // Métriques accélérateurs
    uint64_t gpu_verifications;
    uint64_t sealevel_executions;
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
