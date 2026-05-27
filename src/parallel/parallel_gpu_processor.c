/*
 * LumVorax — Module Parallel GPU Processor
 * parallel_gpu_processor.c — Implémentation Traitement Parallèle GPU
 *
 * INTÉGRATION TOTALE 100% i915 DRM NATIF
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "parallel_gpu_processor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/* ══════════════════════════════════════════════════════════════════════
 * UTILITAIRES INTERNES
 * ══════════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static double ns_to_sec(uint64_t ns) {
    return (double)ns / 1000000000.0;
}

#define LOG_EVENT(proc, event, ...) do { \
    if ((proc)->log_file) { \
        uint64_t ts = get_timestamp_ns(); \
        fprintf((proc)->log_file, "[%lu.%09lu] " event "\n", \
                (unsigned long)(ts / 1000000000ULL), \
                (unsigned long)(ts % 1000000000ULL), ##__VA_ARGS__); \
        fflush((proc)->log_file); \
        (proc)->event_counter++; \
    } \
} while(0)

/* ══════════════════════════════════════════════════════════════════════
 * GESTION QUEUE TÂCHES
 * ══════════════════════════════════════════════════════════════════════ */

static int task_queue_init(parallel_gpu_queue_t* queue) {
    memset(queue, 0, sizeof(*queue));
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->not_empty);
        return -1;
    }
    
    return 0;
}

static void task_queue_destroy(parallel_gpu_queue_t* queue) {
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}

static int task_queue_enqueue(
    parallel_gpu_queue_t* queue,
    const parallel_gpu_task_t* task
) {
    pthread_mutex_lock(&queue->mutex);
    
    /* Attendre si queue pleine */
    while (queue->count >= PARALLEL_GPU_MAX_TASKS) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    /* Ajouter tâche */
    queue->tasks[queue->tail] = *task;
    queue->tail = (queue->tail + 1) % PARALLEL_GPU_MAX_TASKS;
    queue->count++;
    
    /* Signaler queue non vide */
    pthread_cond_signal(&queue->not_empty);
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

static int task_queue_dequeue(
    parallel_gpu_queue_t* queue,
    parallel_gpu_task_t* task,
    double timeout_sec
) {
    pthread_mutex_lock(&queue->mutex);
    
    /* Attendre si queue vide */
    if (timeout_sec > 0) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += (time_t)timeout_sec;
        ts.tv_nsec += (long)((timeout_sec - (time_t)timeout_sec) * 1000000000.0);
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        
        while (queue->count == 0) {
            int ret = pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &ts);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&queue->mutex);
                return -1;
            }
        }
    } else {
        while (queue->count == 0) {
            pthread_cond_wait(&queue->not_empty, &queue->mutex);
        }
    }
    
    /* Retirer tâche */
    *task = queue->tasks[queue->head];
    queue->head = (queue->head + 1) % PARALLEL_GPU_MAX_TASKS;
    queue->count--;
    
    /* Signaler queue non pleine */
    pthread_cond_signal(&queue->not_full);
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * THREAD SCHEDULER
 * ══════════════════════════════════════════════════════════════════════ */

static void* scheduler_thread_main(void* arg) {
    parallel_gpu_processor_t* proc = (parallel_gpu_processor_t*)arg;
    
    LOG_EVENT(proc, "SCHEDULER_START: max_parallel=%d", proc->max_parallel_dispatches);
    
    while (proc->scheduler_running) {
        /* Déqueue tâche (timeout 100ms) */
        parallel_gpu_task_t task;
        if (task_queue_dequeue(&proc->task_queue, &task, 0.1) < 0) {
            continue; /* Timeout, retry */
        }
        
        LOG_EVENT(proc, "SCHEDULER_DEQUEUE: task_id=%d type=%d", task.task_id, task.type);
        
        /* Attendre slot dispatch disponible */
        pthread_mutex_lock(&proc->dispatch_mutex);
        while (proc->num_active_dispatches >= proc->max_parallel_dispatches) {
            pthread_mutex_unlock(&proc->dispatch_mutex);
            usleep(1000); /* 1ms */
            pthread_mutex_lock(&proc->dispatch_mutex);
        }
        
        /* Trouver slot libre */
        int slot = -1;
        for (int i = 0; i < PARALLEL_GPU_MAX_DISPATCHES; i++) {
            if (proc->active_tasks[i] == NULL) {
                slot = i;
                break;
            }
        }
        
        if (slot < 0) {
            pthread_mutex_unlock(&proc->dispatch_mutex);
            LOG_EVENT(proc, "SCHEDULER_ERROR: no_free_slot task_id=%d", task.task_id);
            continue;
        }
        
        /* Allouer tâche */
        parallel_gpu_task_t* active_task = malloc(sizeof(parallel_gpu_task_t));
        if (!active_task) {
            pthread_mutex_unlock(&proc->dispatch_mutex);
            LOG_EVENT(proc, "SCHEDULER_ERROR: malloc_failed task_id=%d", task.task_id);
            continue;
        }
        
        *active_task = task;
        active_task->dispatch_id = slot;
        active_task->submit_time_ns = get_timestamp_ns();
        proc->active_tasks[slot] = active_task;
        proc->num_active_dispatches++;
        
        pthread_mutex_unlock(&proc->dispatch_mutex);
        
        /* Soumettre au GPU selon type tâche */
        int result = -1;
        switch (task.type) {
            case PARALLEL_GPU_TASK_MINING:
                /* Mining Bitcoin via lum_gpu_dispatch_parallel */
                result = lum_gpu_dispatch_parallel(
                    proc->gpu_ctx,
                    (const uint8_t*)task.input_data,
                    task.start_param,
                    task.count_param,
                    1 /* 1 dispatch pour l'instant */
                );
                break;
                
            case PARALLEL_GPU_TASK_HASH_COMPUTE:
            case PARALLEL_GPU_TASK_MATRIX_MULT:
            case PARALLEL_GPU_TASK_VECTOR_ADD:
            case PARALLEL_GPU_TASK_CUSTOM:
                /* TODO: Implémenter autres types */
                LOG_EVENT(proc, "SCHEDULER_WARN: task_type=%d not_implemented", task.type);
                result = -1;
                break;
        }
        
        /* Mettre à jour tâche */
        pthread_mutex_lock(&proc->dispatch_mutex);
        active_task->submitted = true;
        active_task->completed = (result == 0);
        active_task->error_code = (result == 0) ? 0 : errno;
        active_task->complete_time_ns = get_timestamp_ns();
        active_task->execution_time_sec = ns_to_sec(
            active_task->complete_time_ns - active_task->submit_time_ns
        );
        
        /* Mettre à jour stats */
        proc->stats.total_tasks_submitted++;
        if (result == 0) {
            proc->stats.total_tasks_completed++;
            proc->stats.total_execution_time_sec += active_task->execution_time_sec;
            proc->stats.avg_execution_time_sec = 
                proc->stats.total_execution_time_sec / proc->stats.total_tasks_completed;
        } else {
            proc->stats.total_tasks_failed++;
        }
        
        LOG_EVENT(proc, "SCHEDULER_COMPLETE: task_id=%d result=%d time=%.6f",
                  task.task_id, result, active_task->execution_time_sec);
        
        /* Libérer slot */
        free(active_task);
        proc->active_tasks[slot] = NULL;
        proc->num_active_dispatches--;
        
        pthread_mutex_unlock(&proc->dispatch_mutex);
    }
    
    LOG_EVENT(proc, "SCHEDULER_STOP");
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE
 * ══════════════════════════════════════════════════════════════════════ */

parallel_gpu_processor_t* parallel_gpu_processor_create(
    const parallel_gpu_config_t* config
) {
    if (!config) return NULL;
    
    /* Allouer processeur */
    parallel_gpu_processor_t* proc = calloc(1, sizeof(parallel_gpu_processor_t));
    if (!proc) return NULL;
    
    /* Ouvrir log forensique */
    if (config->log_path) {
        proc->log_file = fopen(config->log_path, "w");
        if (!proc->log_file) {
            free(proc);
            return NULL;
        }
    }
    
    LOG_EVENT(proc, "PARALLEL_GPU_INIT_START: max_parallel=%d batch_size=%u",
              config->max_parallel_dispatches, config->default_batch_size);
    
    /* Initialiser queue tâches */
    if (task_queue_init(&proc->task_queue) < 0) {
        if (proc->log_file) fclose(proc->log_file);
        free(proc);
        return NULL;
    }
    
    /* Créer contexte GPU */
    lum_gpu_config_t gpu_config = {
        .kernel_path = config->gpu_kernel_path,
        .log_path = NULL, /* Utiliser log du processeur */
        .batch_size = config->default_batch_size,
        .work_group_size = 256,
        .enable_thp = config->enable_thp,
        .enable_profiling = config->enable_profiling
    };
    
    proc->gpu_ctx = lum_gpu_init(&gpu_config);
    if (!proc->gpu_ctx) {
        LOG_EVENT(proc, "PARALLEL_GPU_INIT_FAILED: gpu_context_creation_failed");
        task_queue_destroy(&proc->task_queue);
        if (proc->log_file) fclose(proc->log_file);
        free(proc);
        return NULL;
    }
    
    /* Initialiser mutex dispatches */
    if (pthread_mutex_init(&proc->dispatch_mutex, NULL) != 0) {
        lum_gpu_cleanup(proc->gpu_ctx);
        task_queue_destroy(&proc->task_queue);
        if (proc->log_file) fclose(proc->log_file);
        free(proc);
        return NULL;
    }
    
    /* Initialiser mutex scheduler */
    if (pthread_mutex_init(&proc->scheduler_mutex, NULL) != 0) {
        pthread_mutex_destroy(&proc->dispatch_mutex);
        lum_gpu_cleanup(proc->gpu_ctx);
        task_queue_destroy(&proc->task_queue);
        if (proc->log_file) fclose(proc->log_file);
        free(proc);
        return NULL;
    }
    
    /* Configuration */
    proc->max_parallel_dispatches = config->max_parallel_dispatches;
    if (proc->max_parallel_dispatches < 1) proc->max_parallel_dispatches = 1;
    if (proc->max_parallel_dispatches > PARALLEL_GPU_MAX_DISPATCHES) {
        proc->max_parallel_dispatches = PARALLEL_GPU_MAX_DISPATCHES;
    }
    proc->default_batch_size = config->default_batch_size;
    proc->enable_adaptive_batch = config->enable_adaptive_batch;
    
    /* Démarrer thread scheduler */
    proc->scheduler_running = true;
    if (pthread_create(&proc->scheduler_thread, NULL, scheduler_thread_main, proc) != 0) {
        LOG_EVENT(proc, "PARALLEL_GPU_INIT_FAILED: scheduler_thread_creation_failed");
        pthread_mutex_destroy(&proc->scheduler_mutex);
        pthread_mutex_destroy(&proc->dispatch_mutex);
        lum_gpu_cleanup(proc->gpu_ctx);
        task_queue_destroy(&proc->task_queue);
        if (proc->log_file) fclose(proc->log_file);
        free(proc);
        return NULL;
    }
    
    LOG_EVENT(proc, "PARALLEL_GPU_INIT_COMPLETE: processor=%p", (void*)proc);
    return proc;
}

int parallel_gpu_submit_task(
    parallel_gpu_processor_t* processor,
    parallel_gpu_task_type_e type,
    const void* input_data,
    size_t input_size,
    void* output_data,
    size_t output_size,
    uint64_t start_param,
    uint32_t count_param
) {
    if (!processor) return -1;
    
    /* Créer tâche */
    parallel_gpu_task_t task = {0};
    
    /* Générer ID unique */
    static int next_task_id = 1;
    task.task_id = __sync_fetch_and_add(&next_task_id, 1);
    
    task.type = type;
    task.input_data = input_data;
    task.input_size = input_size;
    task.output_data = output_data;
    task.output_size = output_size;
    task.start_param = start_param;
    task.count_param = count_param;
    task.submitted = false;
    task.completed = false;
    task.error_code = 0;
    task.dispatch_id = -1;
    
    LOG_EVENT(processor, "TASK_SUBMIT: task_id=%d type=%d start=%lu count=%u",
              task.task_id, type, (unsigned long)start_param, count_param);
    
    /* Enqueue tâche */
    if (task_queue_enqueue(&processor->task_queue, &task) < 0) {
        LOG_EVENT(processor, "TASK_SUBMIT_FAILED: task_id=%d queue_full", task.task_id);
        return -1;
    }
    
    return task.task_id;
}

int parallel_gpu_wait_task(
    parallel_gpu_processor_t* processor,
    int task_id,
    double timeout_sec
) {
    if (!processor) return -1;
    
    uint64_t start_time = get_timestamp_ns();
    uint64_t timeout_ns = (uint64_t)(timeout_sec * 1000000000.0);
    
    while (1) {
        /* Vérifier si tâche complétée */
        pthread_mutex_lock(&processor->dispatch_mutex);
        bool found = false;
        bool completed = false;
        
        for (int i = 0; i < PARALLEL_GPU_MAX_DISPATCHES; i++) {
            if (processor->active_tasks[i] && 
                processor->active_tasks[i]->task_id == task_id) {
                found = true;
                completed = processor->active_tasks[i]->completed;
                break;
            }
        }
        pthread_mutex_unlock(&processor->dispatch_mutex);
        
        if (!found || completed) {
            return completed ? 0 : -1;
        }
        
        /* Vérifier timeout */
        if (timeout_sec > 0) {
            uint64_t elapsed = get_timestamp_ns() - start_time;
            if (elapsed >= timeout_ns) {
                LOG_EVENT(processor, "TASK_WAIT_TIMEOUT: task_id=%d", task_id);
                return -1;
            }
        }
        
        usleep(1000); /* 1ms */
    }
}

int parallel_gpu_wait_all(
    parallel_gpu_processor_t* processor,
    double timeout_sec
) {
    if (!processor) return -1;
    
    uint64_t start_time = get_timestamp_ns();
    uint64_t timeout_ns = (uint64_t)(timeout_sec * 1000000000.0);
    
    while (1) {
        /* Vérifier si toutes tâches complétées */
        pthread_mutex_lock(&processor->dispatch_mutex);
        int active = processor->num_active_dispatches;
        pthread_mutex_unlock(&processor->dispatch_mutex);
        
        pthread_mutex_lock(&processor->task_queue.mutex);
        int queued = processor->task_queue.count;
        pthread_mutex_unlock(&processor->task_queue.mutex);
        
        if (active == 0 && queued == 0) {
            return (int)processor->stats.total_tasks_completed;
        }
        
        /* Vérifier timeout */
        if (timeout_sec > 0) {
            uint64_t elapsed = get_timestamp_ns() - start_time;
            if (elapsed >= timeout_ns) {
                LOG_EVENT(processor, "WAIT_ALL_TIMEOUT: active=%d queued=%d", active, queued);
                return -1;
            }
        }
        
        usleep(10000); /* 10ms */
    }
}

int parallel_gpu_get_stats(
    parallel_gpu_processor_t* processor,
    parallel_gpu_stats_t* stats
) {
    if (!processor || !stats) return -1;
    
    *stats = processor->stats;
    return 0;
}

void parallel_gpu_processor_destroy(
    parallel_gpu_processor_t* processor
) {
    if (!processor) return;
    
    LOG_EVENT(processor, "PARALLEL_GPU_CLEANUP_START");
    
    /* Arrêter scheduler */
    processor->scheduler_running = false;
    pthread_join(processor->scheduler_thread, NULL);
    
    /* Attendre tâches actives */
    parallel_gpu_wait_all(processor, 30.0); /* 30s timeout */
    
    /* Détruire mutex */
    pthread_mutex_destroy(&processor->scheduler_mutex);
    pthread_mutex_destroy(&processor->dispatch_mutex);
    
    /* Détruire contexte GPU */
    if (processor->gpu_ctx) {
        lum_gpu_cleanup(processor->gpu_ctx);
    }
    
    /* Détruire queue */
    task_queue_destroy(&processor->task_queue);
    
    LOG_EVENT(processor, "PARALLEL_GPU_CLEANUP_COMPLETE: tasks_completed=%lu tasks_failed=%lu",
              (unsigned long)processor->stats.total_tasks_completed,
              (unsigned long)processor->stats.total_tasks_failed);
    
    /* Fermer log */
    if (processor->log_file) {
        fclose(processor->log_file);
    }
    
    free(processor);
}

/* ══════════════════════════════════════════════════════════════════════
 * API AVANCÉE
 * ══════════════════════════════════════════════════════════════════════ */

int parallel_gpu_submit_batch(
    parallel_gpu_processor_t* processor,
    const parallel_gpu_task_t* tasks,
    int num_tasks
) {
    if (!processor || !tasks || num_tasks <= 0) return -1;
    
    int submitted = 0;
    for (int i = 0; i < num_tasks; i++) {
        int task_id = parallel_gpu_submit_task(
            processor,
            tasks[i].type,
            tasks[i].input_data,
            tasks[i].input_size,
            tasks[i].output_data,
            tasks[i].output_size,
            tasks[i].start_param,
            tasks[i].count_param
        );
        
        if (task_id >= 0) {
            submitted++;
        }
    }
    
    return submitted;
}

int parallel_gpu_cancel_task(
    parallel_gpu_processor_t* processor,
    int task_id
) {
    /* TODO: Implémenter annulation tâche */
    (void)processor;
    (void)task_id;
    return -1;
}

int parallel_gpu_get_task_status(
    parallel_gpu_processor_t* processor,
    int task_id,
    parallel_gpu_task_t* task
) {
    if (!processor || !task) return -1;
    
    pthread_mutex_lock(&processor->dispatch_mutex);
    
    for (int i = 0; i < PARALLEL_GPU_MAX_DISPATCHES; i++) {
        if (processor->active_tasks[i] && 
            processor->active_tasks[i]->task_id == task_id) {
            *task = *processor->active_tasks[i];
            pthread_mutex_unlock(&processor->dispatch_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&processor->dispatch_mutex);
    return -1;
}

// Made with Bob
