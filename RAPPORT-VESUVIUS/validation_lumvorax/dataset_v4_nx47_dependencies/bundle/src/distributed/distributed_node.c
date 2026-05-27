#include "distributed_node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static uint32_t generate_node_id(void) {
    static uint32_t counter = 0;
    return ++counter + (uint32_t)(get_timestamp_ms() & 0xFFFF);
}

static void* heartbeat_thread_func(void* arg) {
    dist_cluster_t* cluster = (dist_cluster_t*)arg;
    if (!cluster) return NULL;
    
    while (1) {
        pthread_mutex_lock(&cluster->lock);
        if (!cluster->running) {
            pthread_mutex_unlock(&cluster->lock);
            break;
        }
        
        uint64_t now = get_timestamp_ms();
        for (size_t i = 0; i < cluster->node_count; i++) {
            dist_node_info_t* node = cluster->nodes[i];
            if (!node || node->is_local) continue;
            
            if (now - node->last_heartbeat > DIST_TIMEOUT_MS) {
                node->state = DIST_NODE_FAILED;
            }
        }
        
        dist_node_info_t* local = NULL;
        for (size_t i = 0; i < cluster->node_count; i++) {
            if (cluster->nodes[i] && cluster->nodes[i]->node_id == cluster->local_node_id) {
                local = cluster->nodes[i];
                break;
            }
        }
        
        if (local) {
            local->last_heartbeat = now;
        }
        
        pthread_mutex_unlock(&cluster->lock);
        usleep(DIST_HEARTBEAT_INTERVAL_MS * 1000);
    }
    
    return NULL;
}

dist_cluster_t* dist_cluster_create(const char* local_name, uint16_t port) {
    dist_cluster_t* cluster = (dist_cluster_t*)calloc(1, sizeof(dist_cluster_t));
    if (!cluster) return NULL;
    
    pthread_mutex_init(&cluster->lock, NULL);
    
    dist_node_info_t* local = (dist_node_info_t*)calloc(1, sizeof(dist_node_info_t));
    if (!local) {
        pthread_mutex_destroy(&cluster->lock);
        free(cluster);
        return NULL;
    }
    
    local->node_id = generate_node_id();
    strncpy(local->name, local_name ? local_name : "local", DIST_MAX_NODE_NAME - 1);
    strncpy(local->address, "127.0.0.1", sizeof(local->address) - 1);
    local->port = port ? port : DIST_DEFAULT_PORT;
    local->state = DIST_NODE_READY;
    local->last_heartbeat = get_timestamp_ms();
    local->is_local = true;
    
    cluster->nodes[0] = local;
    cluster->node_count = 1;
    cluster->local_node_id = local->node_id;
    
    return cluster;
}

void dist_cluster_destroy(dist_cluster_t* cluster) {
    if (!cluster) return;
    
    dist_cluster_stop(cluster);
    
    for (size_t i = 0; i < cluster->node_count; i++) {
        free(cluster->nodes[i]);
    }
    
    pthread_mutex_destroy(&cluster->lock);
    free(cluster);
}

int dist_cluster_add_node(dist_cluster_t* cluster, const char* name, const char* address, uint16_t port) {
    if (!cluster || !name || !address) return -1;
    
    pthread_mutex_lock(&cluster->lock);
    
    if (cluster->node_count >= DIST_MAX_NODES) {
        pthread_mutex_unlock(&cluster->lock);
        return -2;
    }
    
    dist_node_info_t* node = (dist_node_info_t*)calloc(1, sizeof(dist_node_info_t));
    if (!node) {
        pthread_mutex_unlock(&cluster->lock);
        return -3;
    }
    
    node->node_id = generate_node_id();
    strncpy(node->name, name, DIST_MAX_NODE_NAME - 1);
    strncpy(node->address, address, sizeof(node->address) - 1);
    node->port = port ? port : DIST_DEFAULT_PORT;
    node->state = DIST_NODE_STARTING;
    node->last_heartbeat = get_timestamp_ms();
    node->is_local = false;
    
    cluster->nodes[cluster->node_count++] = node;
    
    pthread_mutex_unlock(&cluster->lock);
    
    return (int)node->node_id;
}

int dist_cluster_remove_node(dist_cluster_t* cluster, uint32_t node_id) {
    if (!cluster) return -1;
    
    pthread_mutex_lock(&cluster->lock);
    
    for (size_t i = 0; i < cluster->node_count; i++) {
        if (cluster->nodes[i] && cluster->nodes[i]->node_id == node_id) {
            if (cluster->nodes[i]->is_local) {
                pthread_mutex_unlock(&cluster->lock);
                return -2;
            }
            
            free(cluster->nodes[i]);
            
            for (size_t j = i; j < cluster->node_count - 1; j++) {
                cluster->nodes[j] = cluster->nodes[j + 1];
            }
            cluster->node_count--;
            
            pthread_mutex_unlock(&cluster->lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&cluster->lock);
    return -3;
}

dist_node_info_t* dist_cluster_get_node(dist_cluster_t* cluster, uint32_t node_id) {
    if (!cluster) return NULL;
    
    for (size_t i = 0; i < cluster->node_count; i++) {
        if (cluster->nodes[i] && cluster->nodes[i]->node_id == node_id) {
            return cluster->nodes[i];
        }
    }
    
    return NULL;
}

int dist_cluster_start(dist_cluster_t* cluster) {
    if (!cluster || cluster->running) return -1;
    
    pthread_mutex_lock(&cluster->lock);
    cluster->running = true;
    
    if (pthread_create(&cluster->heartbeat_thread, NULL, heartbeat_thread_func, cluster) != 0) {
        cluster->running = false;
        pthread_mutex_unlock(&cluster->lock);
        return -2;
    }
    
    dist_node_info_t* local = dist_cluster_get_node(cluster, cluster->local_node_id);
    if (local) {
        local->state = DIST_NODE_READY;
    }
    pthread_mutex_unlock(&cluster->lock);
    
    return 0;
}

int dist_cluster_stop(dist_cluster_t* cluster) {
    if (!cluster || !cluster->running) return -1;
    
    cluster->running = false;
    pthread_join(cluster->heartbeat_thread, NULL);
    
    pthread_mutex_lock(&cluster->lock);
    for (size_t i = 0; i < cluster->node_count; i++) {
        if (cluster->nodes[i]) {
            cluster->nodes[i]->state = DIST_NODE_STOPPED;
        }
    }
    pthread_mutex_unlock(&cluster->lock);
    
    return 0;
}

int dist_task_submit(dist_cluster_t* cluster, dist_task_t* task) {
    if (!cluster || !task) return -1;
    
    pthread_mutex_lock(&cluster->lock);
    
    dist_node_info_t* best_node = NULL;
    double min_load = 1000.0;
    
    for (size_t i = 0; i < cluster->node_count; i++) {
        dist_node_info_t* node = cluster->nodes[i];
        if (node && node->state == DIST_NODE_READY && node->load_factor < min_load) {
            best_node = node;
            min_load = node->load_factor;
        }
    }
    
    if (!best_node) {
        pthread_mutex_unlock(&cluster->lock);
        return -2;
    }
    
    task->assigned_node = best_node->node_id;
    task->start_time = get_timestamp_ms();
    task->completed = false;
    
    cluster->total_tasks++;
    best_node->load_factor += 0.1;
    
    pthread_mutex_unlock(&cluster->lock);
    
    return 0;
}

int dist_task_wait(dist_cluster_t* cluster, dist_task_t* task, uint64_t timeout_ms) {
    if (!cluster || !task) return -1;
    
    uint64_t start = get_timestamp_ms();
    
    while (!task->completed) {
        if (get_timestamp_ms() - start > timeout_ms) {
            cluster->failed_tasks++;
            return -2;
        }
        usleep(1000);
    }
    
    task->end_time = get_timestamp_ms();
    cluster->completed_tasks++;
    
    return 0;
}

int dist_broadcast(dist_cluster_t* cluster, dist_message_t* msg) {
    if (!cluster || !msg || !cluster->running) return -1;
    
    msg->timestamp = get_timestamp_ms();
    msg->sender_id = cluster->local_node_id;
    
    pthread_mutex_lock(&cluster->lock);
    
    for (size_t i = 0; i < cluster->node_count; i++) {
        if (cluster->nodes[i] && !cluster->nodes[i]->is_local) {
            msg->receiver_id = cluster->nodes[i]->node_id;
        }
    }
    
    pthread_mutex_unlock(&cluster->lock);
    
    return 0;
}

size_t dist_cluster_active_nodes(dist_cluster_t* cluster) {
    if (!cluster) return 0;
    
    size_t count = 0;
    
    pthread_mutex_lock(&cluster->lock);
    
    for (size_t i = 0; i < cluster->node_count; i++) {
        if (cluster->nodes[i] && cluster->nodes[i]->state == DIST_NODE_READY) {
            count++;
        }
    }
    
    pthread_mutex_unlock(&cluster->lock);
    
    return count;
}

void dist_cluster_stats(dist_cluster_t* cluster, uint64_t* total, uint64_t* completed, uint64_t* failed) {
    if (!cluster) return;
    
    pthread_mutex_lock(&cluster->lock);
    
    if (total) *total = cluster->total_tasks;
    if (completed) *completed = cluster->completed_tasks;
    if (failed) *failed = cluster->failed_tasks;
    
    pthread_mutex_unlock(&cluster->lock);
}
