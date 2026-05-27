/**
 * @file lum_pipeline.c
 * @brief Implémentation Pipeline - Architecture 4 Étapes
 */

#include "lum_pipeline.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static lum_pipeline_queue_t* queue_init(uint32_t capacity) {
    lum_pipeline_queue_t* q = (lum_pipeline_queue_t*)calloc(1, sizeof(lum_pipeline_queue_t));
    if (!q) return NULL;
    
    q->buffer = (lum_pipeline_tx_t**)calloc(capacity, sizeof(lum_pipeline_tx_t*));
    q->capacity = capacity;
    q->head = q->tail = 0;
    
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    
    return q;
}

static void queue_free(lum_pipeline_queue_t* q) {
    if (!q) return;
    
    for (uint32_t i = 0; i < q->capacity; i++) {
        if (q->buffer[i]) {
            free(q->buffer[i]->data);
            free(q->buffer[i]);
        }
    }
    
    free(q->buffer);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q);
}

static bool queue_push(lum_pipeline_queue_t* q, lum_pipeline_tx_t* tx) {
    pthread_mutex_lock(&q->mutex);
    
    while ((q->tail + 1) % q->capacity == q->head) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    
    q->buffer[q->tail] = tx;
    q->tail = (q->tail + 1) % q->capacity;
    
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    
    return true;
}

static lum_pipeline_tx_t* queue_pop(lum_pipeline_queue_t* q) {
    pthread_mutex_lock(&q->mutex);
    
    while (q->head == q->tail) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    
    lum_pipeline_tx_t* tx = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    
    return tx;
}

static void* stage_fetch(void* arg) {
    lum_pipeline_t* pipeline = (lum_pipeline_t*)arg;
    
    while (pipeline->running) {
        // Fetch depuis fetch_queue et passer à verify_queue
        lum_pipeline_tx_t* tx = queue_pop(pipeline->fetch_queue);
        if (tx) {
            pipeline->total_fetched++;
            queue_push(pipeline->verify_queue, tx);
        }
    }
    
    return NULL;
}

static void* stage_sig_verify(void* arg) {
    lum_pipeline_t* pipeline = (lum_pipeline_t*)arg;
    
    while (pipeline->running) {
        lum_pipeline_tx_t* tx = queue_pop(pipeline->verify_queue);
        if (tx) {
            // Vérifier signature (simulation)
            tx->sig_verified = true;
            pipeline->total_verified++;
            queue_push(pipeline->banking_queue, tx);
        }
    }
    
    return NULL;
}

static void* stage_banking(void* arg) {
    lum_pipeline_t* pipeline = (lum_pipeline_t*)arg;
    
    while (pipeline->running) {
        lum_pipeline_tx_t* tx = queue_pop(pipeline->banking_queue);
        if (tx && tx->sig_verified) {
            // Exécuter transaction (simulation)
            tx->executed = true;
            pipeline->total_executed++;
            queue_push(pipeline->write_queue, tx);
        }
    }
    
    return NULL;
}

static void* stage_write(void* arg) {
    lum_pipeline_t* pipeline = (lum_pipeline_t*)arg;
    
    while (pipeline->running) {
        lum_pipeline_tx_t* tx = queue_pop(pipeline->write_queue);
        if (tx && tx->executed) {
            // Écrire état (simulation)
            tx->written = true;
            pipeline->total_written++;
            
            // Libérer transaction
            free(tx->data);
            free(tx);
        }
    }
    
    return NULL;
}

lum_pipeline_t* lum_pipeline_init(void) {
    lum_pipeline_t* pipeline = (lum_pipeline_t*)calloc(1, sizeof(lum_pipeline_t));
    if (!pipeline) return NULL;
    
    pipeline->fetch_queue = queue_init(LUM_PIPELINE_BUFFER_SIZE);
    pipeline->verify_queue = queue_init(LUM_PIPELINE_BUFFER_SIZE);
    pipeline->banking_queue = queue_init(LUM_PIPELINE_BUFFER_SIZE);
    pipeline->write_queue = queue_init(LUM_PIPELINE_BUFFER_SIZE);
    
    if (!pipeline->fetch_queue || !pipeline->verify_queue ||
        !pipeline->banking_queue || !pipeline->write_queue) {
        lum_pipeline_free(pipeline);
        return NULL;
    }
    
    pipeline->running = false;
    
    return pipeline;
}

void lum_pipeline_free(lum_pipeline_t* pipeline) {
    if (!pipeline) return;
    
    if (pipeline->running) {
        lum_pipeline_stop(pipeline);
    }
    
    queue_free(pipeline->fetch_queue);
    queue_free(pipeline->verify_queue);
    queue_free(pipeline->banking_queue);
    queue_free(pipeline->write_queue);
    
    free(pipeline);
}

bool lum_pipeline_start(lum_pipeline_t* pipeline) {
    if (!pipeline || pipeline->running) return false;
    
    pipeline->running = true;
    
    pthread_create(&pipeline->fetch_thread, NULL, stage_fetch, pipeline);
    pthread_create(&pipeline->verify_thread, NULL, stage_sig_verify, pipeline);
    pthread_create(&pipeline->banking_thread, NULL, stage_banking, pipeline);
    pthread_create(&pipeline->write_thread, NULL, stage_write, pipeline);
    
    return true;
}

void lum_pipeline_stop(lum_pipeline_t* pipeline) {
    if (!pipeline || !pipeline->running) return;
    
    pipeline->running = false;
    
    pthread_join(pipeline->fetch_thread, NULL);
    pthread_join(pipeline->verify_thread, NULL);
    pthread_join(pipeline->banking_thread, NULL);
    pthread_join(pipeline->write_thread, NULL);
}

bool lum_pipeline_submit(lum_pipeline_t* pipeline, const uint8_t* signature,
                         const uint8_t* data, uint32_t size) {
    if (!pipeline || !signature || !data) return false;
    
    lum_pipeline_tx_t* tx = (lum_pipeline_tx_t*)calloc(1, sizeof(lum_pipeline_tx_t));
    memcpy(tx->signature, signature, 64);
    tx->data = (uint8_t*)malloc(size);
    memcpy(tx->data, data, size);
    tx->size = size;
    tx->sig_verified = false;
    tx->executed = false;
    tx->written = false;
    
    return queue_push(pipeline->fetch_queue, tx);
}

void lum_pipeline_get_metrics(lum_pipeline_t* pipeline, uint64_t* fetched,
                              uint64_t* verified, uint64_t* executed,
                              uint64_t* written, double* avg_latency_ms) {
    if (!pipeline) return;
    
    if (fetched) *fetched = pipeline->total_fetched;
    if (verified) *verified = pipeline->total_verified;
    if (executed) *executed = pipeline->total_executed;
    if (written) *written = pipeline->total_written;
    if (avg_latency_ms) *avg_latency_ms = pipeline->avg_latency_ms;
}

// Made with Bob
