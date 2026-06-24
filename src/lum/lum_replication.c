/* _GNU_SOURCE déjà défini par CFLAGS dans Makefile */
#include "lum_replication.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
 * @file lum_replication.c
 * @brief Implémentation du système de réplication LUM DBMS
 */

// ============================================================================
// CONSTANTES INTERNES
// ============================================================================

#define LUM_REPL_MAGIC 0x4C554D52  // "LUMR"
#define LUM_REPL_VERSION 1
#define LUM_REPL_MAX_RETRIES 3
#define LUM_REPL_BUFFER_SIZE 65536

// ============================================================================
// FONCTIONS UTILITAIRES INTERNES
// ============================================================================

/**
 * @brief Calcule CRC32 pour intégrité
 */
static uint32_t calculate_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

/**
 * @brief Obtient timestamp en nanosecondes
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Thread heartbeat (master)
 */
static void* heartbeat_thread_func(void* arg) {
    lum_replication_context_t* ctx = (lum_replication_context_t*)arg;
    
    while (ctx->running) {
        pthread_mutex_lock(&ctx->slaves_lock);
        
        time_t now = time(NULL);
        for (uint32_t i = 0; i < ctx->slave_count; i++) {
            lum_slave_info_t* slave = &ctx->slaves[i];
            
            // Vérifier timeout heartbeat
            if (slave->state == LUM_SLAVE_CONNECTED || 
                slave->state == LUM_SLAVE_SYNCING) {
                
                time_t elapsed = now - slave->last_heartbeat;
                if (elapsed > (time_t)(ctx->config.heartbeat_timeout_ms / 1000)) {
                    slave->state = LUM_SLAVE_DISCONNECTED;
                    fprintf(stderr, "[REPL] Slave %u disconnected (timeout)\n", 
                            slave->node_id);
                }
            }
            
            // Vérifier lag
            if (slave->state == LUM_SLAVE_CONNECTED) {
                slave->lag_bytes = ctx->current_lsn - slave->last_applied_lsn;
                if (slave->lag_bytes > ctx->config.max_lag_bytes) {
                    slave->state = LUM_SLAVE_LAGGING;
                    fprintf(stderr, "[REPL] Slave %u lagging (%lu bytes)\n",
                            slave->node_id, slave->lag_bytes);
                }
            }
        }
        
        pthread_mutex_unlock(&ctx->slaves_lock);
        
        // Attendre intervalle heartbeat
        usleep(ctx->config.heartbeat_interval_ms * 1000);
    }
    
    return NULL;
}

/**
 * @brief Thread réplication (slave)
 */
static void* replication_thread_func(void* arg) {
    lum_replication_context_t* ctx = (lum_replication_context_t*)arg;
    
    while (ctx->running) {
        // TODO: Implémenter connexion au master et réception événements
        // Pour l'instant, simple sleep
        usleep(100000); // 100ms
    }
    
    return NULL;
}

// ============================================================================
// IMPLÉMENTATION API PUBLIQUE
// ============================================================================

lum_replication_context_t* lum_replication_init(const lum_replication_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    lum_replication_context_t* ctx = calloc(1, sizeof(lum_replication_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Copier configuration
    memcpy(&ctx->config, config, sizeof(lum_replication_config_t));
    
    // Allouer tableau slaves
    ctx->slaves = calloc(config->max_slaves, sizeof(lum_slave_info_t));
    if (!ctx->slaves) {
        free(ctx);
        return NULL;
    }
    
    // Initialiser mutex
    if (pthread_mutex_init(&ctx->slaves_lock, NULL) != 0) {
        free(ctx->slaves);
        free(ctx);
        return NULL;
    }
    
    ctx->slave_count = 0;
    ctx->current_lsn = 0;
    ctx->running = false;
    ctx->node_type = LUM_NODE_OBSERVER;
    
    return ctx;
}

void lum_replication_free(lum_replication_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Arrêter réplication si active
    if (ctx->running) {
        lum_replication_stop(ctx);
    }
    
    // Libérer ressources
    pthread_mutex_destroy(&ctx->slaves_lock);
    free(ctx->slaves);
    free(ctx);
}

bool lum_replication_start(lum_replication_context_t* ctx, lum_node_type_t node_type) {
    if (!ctx || ctx->running) {
        return false;
    }
    
    ctx->node_type = node_type;
    ctx->running = true;
    
    if (node_type == LUM_NODE_MASTER) {
        // Démarrer thread heartbeat
        if (pthread_create(&ctx->heartbeat_thread, NULL, 
                          heartbeat_thread_func, ctx) != 0) {
            ctx->running = false;
            return false;
        }
    } else if (node_type == LUM_NODE_SLAVE) {
        // Démarrer thread réplication
        if (pthread_create(&ctx->replication_thread, NULL,
                          replication_thread_func, ctx) != 0) {
            ctx->running = false;
            return false;
        }
    }
    
    return true;
}

void lum_replication_stop(lum_replication_context_t* ctx) {
    if (!ctx || !ctx->running) {
        return;
    }
    
    ctx->running = false;
    
    // Attendre fin des threads
    if (ctx->node_type == LUM_NODE_MASTER) {
        pthread_join(ctx->heartbeat_thread, NULL);
    } else if (ctx->node_type == LUM_NODE_SLAVE) {
        pthread_join(ctx->replication_thread, NULL);
    }
}

uint32_t lum_replication_add_slave(lum_replication_context_t* ctx,
                                    const char* hostname,
                                    uint16_t port) {
    if (!ctx || !hostname || ctx->node_type != LUM_NODE_MASTER) {
        return 0;
    }
    
    pthread_mutex_lock(&ctx->slaves_lock);
    
    // Vérifier capacité
    if (ctx->slave_count >= ctx->config.max_slaves) {
        pthread_mutex_unlock(&ctx->slaves_lock);
        return 0;
    }
    
    // Générer ID unique
    uint32_t slave_id = ctx->slave_count + 1;
    
    // Initialiser slave
    lum_slave_info_t* slave = &ctx->slaves[ctx->slave_count];
    slave->node_id = slave_id;
    strncpy(slave->hostname, hostname, sizeof(slave->hostname) - 1);
    slave->port = port;
    slave->state = LUM_SLAVE_SYNCING;
    slave->last_applied_lsn = 0;
    slave->lag_bytes = ctx->current_lsn;
    slave->last_heartbeat = time(NULL);
    slave->replication_delay_ms = 0;
    
    ctx->slave_count++;
    
    pthread_mutex_unlock(&ctx->slaves_lock);
    
    return slave_id;
}

bool lum_replication_remove_slave(lum_replication_context_t* ctx, uint32_t slave_id) {
    if (!ctx || ctx->node_type != LUM_NODE_MASTER) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->slaves_lock);
    
    // Trouver slave
    bool found = false;
    uint32_t index = 0;
    for (uint32_t i = 0; i < ctx->slave_count; i++) {
        if (ctx->slaves[i].node_id == slave_id) {
            found = true;
            index = i;
            break;
        }
    }
    
    if (!found) {
        pthread_mutex_unlock(&ctx->slaves_lock);
        return false;
    }
    
    // Décaler les slaves suivants
    for (uint32_t i = index; i < ctx->slave_count - 1; i++) {
        ctx->slaves[i] = ctx->slaves[i + 1];
    }
    
    ctx->slave_count--;
    
    pthread_mutex_unlock(&ctx->slaves_lock);
    
    return true;
}

bool lum_replication_replicate(lum_replication_context_t* ctx,
                                const lum_replication_event_t* event) {
    if (!ctx || !event || ctx->node_type != LUM_NODE_MASTER) {
        return false;
    }
    
    // Vérifier intégrité
    uint32_t crc = calculate_crc32(event->data, event->data_size);
    if (crc != event->crc32) {
        return false;
    }
    
    // Incrémenter LSN
    ctx->current_lsn = event->lsn;
    ctx->total_transactions++;
    ctx->total_bytes_replicated += event->data_size;
    
    pthread_mutex_lock(&ctx->slaves_lock);
    
    // Compter slaves synchrones disponibles
    uint32_t sync_slaves = 0;
    for (uint32_t i = 0; i < ctx->slave_count; i++) {
        if (ctx->slaves[i].state == LUM_SLAVE_CONNECTED) {
            sync_slaves++;
            // TODO: Envoyer événement au slave
        }
    }
    
    pthread_mutex_unlock(&ctx->slaves_lock);
    
    // Vérifier mode synchrone
    if (ctx->config.mode == LUM_REPL_SYNC ||
        ctx->config.mode == LUM_REPL_SEMI_SYNC) {
        
        if (sync_slaves < ctx->config.sync_slaves_required) {
            fprintf(stderr, "[REPL] Not enough sync slaves (%u < %u)\n",
                    sync_slaves, ctx->config.sync_slaves_required);
            return false;
        }
    }
    
    return true;
}

bool lum_replication_wait_sync(lum_replication_context_t* ctx,
                                uint64_t lsn,
                                uint32_t timeout_ms) {
    if (!ctx || ctx->node_type != LUM_NODE_MASTER) {
        return false;
    }
    
    uint64_t start = get_timestamp_ns();
    uint64_t timeout_ns = (uint64_t)timeout_ms * 1000000ULL;
    
    while (true) {
        pthread_mutex_lock(&ctx->slaves_lock);
        
        // Compter slaves à jour
        uint32_t synced = 0;
        for (uint32_t i = 0; i < ctx->slave_count; i++) {
            if (ctx->slaves[i].state == LUM_SLAVE_CONNECTED &&
                ctx->slaves[i].last_applied_lsn >= lsn) {
                synced++;
            }
        }
        
        pthread_mutex_unlock(&ctx->slaves_lock);
        
        // Vérifier si assez de slaves synchronisés
        if (synced >= ctx->config.sync_slaves_required) {
            return true;
        }
        
        // Vérifier timeout
        uint64_t elapsed = get_timestamp_ns() - start;
        if (elapsed >= timeout_ns) {
            return false;
        }
        
        // Attendre un peu
        usleep(1000); // 1ms
    }
}

bool lum_replication_get_slave_info(lum_replication_context_t* ctx,
                                     uint32_t slave_id,
                                     lum_slave_info_t* info) {
    if (!ctx || !info) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->slaves_lock);
    
    bool found = false;
    for (uint32_t i = 0; i < ctx->slave_count; i++) {
        if (ctx->slaves[i].node_id == slave_id) {
            memcpy(info, &ctx->slaves[i], sizeof(lum_slave_info_t));
            found = true;
            break;
        }
    }
    
    pthread_mutex_unlock(&ctx->slaves_lock);
    
    return found;
}

uint32_t lum_replication_list_slaves(lum_replication_context_t* ctx,
                                      lum_slave_info_t* slaves,
                                      uint32_t max_slaves) {
    if (!ctx || !slaves) {
        return 0;
    }
    
    pthread_mutex_lock(&ctx->slaves_lock);
    
    uint32_t count = ctx->slave_count < max_slaves ? ctx->slave_count : max_slaves;
    memcpy(slaves, ctx->slaves, count * sizeof(lum_slave_info_t));
    
    pthread_mutex_unlock(&ctx->slaves_lock);
    
    return count;
}

bool lum_replication_failover(lum_replication_context_t* ctx, uint32_t new_master_id) {
    if (!ctx || !ctx->config.auto_failover) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->slaves_lock);
    
    // Trouver meilleur candidat si auto
    if (new_master_id == 0) {
        uint64_t max_lsn = 0;
        for (uint32_t i = 0; i < ctx->slave_count; i++) {
            if (ctx->slaves[i].state == LUM_SLAVE_CONNECTED &&
                ctx->slaves[i].last_applied_lsn > max_lsn) {
                max_lsn = ctx->slaves[i].last_applied_lsn;
                new_master_id = ctx->slaves[i].node_id;
            }
        }
    }
    
    if (new_master_id == 0) {
        pthread_mutex_unlock(&ctx->slaves_lock);
        return false;
    }
    
    // Promouvoir nouveau master
    ctx->node_type = LUM_NODE_SLAVE;
    ctx->master_node_id = new_master_id;
    ctx->failover_count++;
    ctx->last_failover = time(NULL);
    
    pthread_mutex_unlock(&ctx->slaves_lock);
    
    return true;
}

bool lum_replication_is_master(const lum_replication_context_t* ctx) {
    return ctx && ctx->node_type == LUM_NODE_MASTER;
}

uint64_t lum_replication_get_current_lsn(const lum_replication_context_t* ctx) {
    return ctx ? ctx->current_lsn : 0;
}

void lum_replication_get_stats(const lum_replication_context_t* ctx,
                                uint64_t* total_bytes,
                                uint64_t* total_txns,
                                uint32_t* failover_count) {
    if (!ctx) {
        return;
    }
    
    if (total_bytes) {
        *total_bytes = ctx->total_bytes_replicated;
    }
    if (total_txns) {
        *total_txns = ctx->total_transactions;
    }
    if (failover_count) {
        *failover_count = ctx->failover_count;
    }
}

bool lum_replication_generate_report(const lum_replication_context_t* ctx,
                                      const char* output_path) {
    if (!ctx || !output_path) {
        return false;
    }
    
    FILE* f = fopen(output_path, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "# LUM DBMS Replication Report\n\n");
    fprintf(f, "**Generated**: %s\n", ctime(&(time_t){time(NULL)}));
    fprintf(f, "**Node Type**: %s\n", 
            ctx->node_type == LUM_NODE_MASTER ? "MASTER" :
            ctx->node_type == LUM_NODE_SLAVE ? "SLAVE" : "OBSERVER");
    fprintf(f, "**Node ID**: %u\n\n", ctx->node_id);
    
    fprintf(f, "## Configuration\n\n");
    fprintf(f, "| Parameter | Value |\n");
    fprintf(f, "|-----------|-------|\n");
    fprintf(f, "| Mode | %s |\n",
            ctx->config.mode == LUM_REPL_SYNC ? "SYNC" :
            ctx->config.mode == LUM_REPL_ASYNC ? "ASYNC" : "SEMI-SYNC");
    fprintf(f, "| Max Slaves | %u |\n", ctx->config.max_slaves);
    fprintf(f, "| Sync Slaves Required | %u |\n", ctx->config.sync_slaves_required);
    fprintf(f, "| Heartbeat Interval | %u ms |\n", ctx->config.heartbeat_interval_ms);
    fprintf(f, "| Auto Failover | %s |\n", ctx->config.auto_failover ? "YES" : "NO");
    
    fprintf(f, "\n## Statistics\n\n");
    fprintf(f, "| Metric | Value |\n");
    fprintf(f, "|--------|-------|\n");
    fprintf(f, "| Current LSN | %lu |\n", ctx->current_lsn);
    fprintf(f, "| Total Bytes Replicated | %lu |\n", ctx->total_bytes_replicated);
    fprintf(f, "| Total Transactions | %lu |\n", ctx->total_transactions);
    fprintf(f, "| Failover Count | %u |\n", ctx->failover_count);
    fprintf(f, "| Active Slaves | %u |\n", ctx->slave_count);
    
    if (ctx->node_type == LUM_NODE_MASTER && ctx->slave_count > 0) {
        fprintf(f, "\n## Slaves Status\n\n");
        fprintf(f, "| ID | Hostname | State | LSN | Lag (bytes) |\n");
        fprintf(f, "|----|----------|-------|-----|-------------|\n");
        
        for (uint32_t i = 0; i < ctx->slave_count; i++) {
            const lum_slave_info_t* slave = &ctx->slaves[i];
            const char* state_str = 
                slave->state == LUM_SLAVE_CONNECTED ? "CONNECTED" :
                slave->state == LUM_SLAVE_SYNCING ? "SYNCING" :
                slave->state == LUM_SLAVE_LAGGING ? "LAGGING" :
                slave->state == LUM_SLAVE_DISCONNECTED ? "DISCONNECTED" : "FAILED";
            
            fprintf(f, "| %u | %s:%u | %s | %lu | %lu |\n",
                    slave->node_id, slave->hostname, slave->port,
                    state_str, slave->last_applied_lsn, slave->lag_bytes);
        }
    }
    
    fclose(f);
    return true;
}

// Made with Bob
