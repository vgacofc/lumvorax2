/**
 * @file lum_turbine.c
 * @brief Implémentation Turbine - Propagation de Blocs en Arbre
 * 
 * Turbine utilise un arbre de propagation avec Reed-Solomon pour
 * propager les blocs en O(log N) au lieu de O(N).
 * 
 * Architecture :
 * - Arbre N-aire (fanout configurable)
 * - Fragmentation avec FEC (Forward Error Correction)
 * - Reconstruction parallèle
 * 
 * @version 1.0.0
 * @date 2026-05-02
 */

#include "lum_turbine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ============================================================================
 * GESTION NŒUDS
 * ============================================================================ */

static lum_turbine_node_t* create_node(const uint8_t* node_id,
                                        const char* ip_address,
                                        uint16_t port,
                                        uint32_t max_children) {
    lum_turbine_node_t* node = (lum_turbine_node_t*)calloc(1, sizeof(lum_turbine_node_t));
    if (!node) return NULL;
    
    memcpy(node->node_id, node_id, 32);
    strncpy(node->ip_address, ip_address, sizeof(node->ip_address) - 1);
    node->port = port;
    node->parent = NULL;
    node->num_children = 0;
    node->max_children = max_children;
    node->active = true;
    node->last_seen_ns = get_timestamp_ns();
    
    if (max_children > 0) {
        node->children = (lum_turbine_node_t**)calloc(max_children, sizeof(lum_turbine_node_t*));
        if (!node->children) {
            free(node);
            return NULL;
        }
    }
    
    return node;
}

static void free_node(lum_turbine_node_t* node) {
    if (!node) return;
    free(node->children);
    free(node);
}

/* ============================================================================
 * INITIALISATION / DESTRUCTION
 * ============================================================================ */

lum_turbine_t* lum_turbine_init(const uint8_t* node_id,
                                 const char* ip_address,
                                 uint16_t port,
                                 uint32_t fanout) {
    if (!node_id || !ip_address) return NULL;
    
    lum_turbine_t* turbine = (lum_turbine_t*)calloc(1, sizeof(lum_turbine_t));
    if (!turbine) return NULL;
    
    // Configuration
    turbine->fanout = (fanout > 0 && fanout <= LUM_TURBINE_MAX_FANOUT) ? 
                      fanout : LUM_TURBINE_DEFAULT_FANOUT;
    turbine->fec_rate = LUM_TURBINE_FEC_RATE;
    
    // Nœud local
    turbine->local_node = create_node(node_id, ip_address, port, turbine->fanout);
    if (!turbine->local_node) {
        free(turbine);
        return NULL;
    }
    
    // Allouer tableaux
    turbine->max_nodes = 10000;
    turbine->all_nodes = (lum_turbine_node_t**)calloc(turbine->max_nodes, 
                                                       sizeof(lum_turbine_node_t*));
    if (!turbine->all_nodes) {
        free_node(turbine->local_node);
        free(turbine);
        return NULL;
    }
    
    turbine->all_nodes[0] = turbine->local_node;
    turbine->num_nodes = 1;
    
    turbine->max_pending = 1000;
    turbine->pending_blocks = (lum_turbine_block_t**)calloc(turbine->max_pending,
                                                             sizeof(lum_turbine_block_t*));
    if (!turbine->pending_blocks) {
        free(turbine->all_nodes);
        free_node(turbine->local_node);
        free(turbine);
        return NULL;
    }
    
    // Mutex
    pthread_mutex_init(&turbine->nodes_mutex, NULL);
    pthread_mutex_init(&turbine->blocks_mutex, NULL);
    
    turbine->running = false;
    
    return turbine;
}

void lum_turbine_free(lum_turbine_t* turbine) {
    if (!turbine) return;
    
    if (turbine->running) {
        lum_turbine_stop(turbine);
    }
    
    // Libérer nœuds
    for (uint32_t i = 0; i < turbine->num_nodes; i++) {
        free_node(turbine->all_nodes[i]);
    }
    free(turbine->all_nodes);
    
    // Libérer blocs en cours
    for (uint32_t i = 0; i < turbine->num_pending; i++) {
        if (turbine->pending_blocks[i]) {
            free(turbine->pending_blocks[i]->data);
            free(turbine->pending_blocks[i]->shreds_received);
            pthread_mutex_destroy(&turbine->pending_blocks[i]->mutex);
            free(turbine->pending_blocks[i]);
        }
    }
    free(turbine->pending_blocks);
    
    pthread_mutex_destroy(&turbine->nodes_mutex);
    pthread_mutex_destroy(&turbine->blocks_mutex);
    
    free(turbine);
}

/* ============================================================================
 * GESTION ARBRE
 * ============================================================================ */

bool lum_turbine_add_node(lum_turbine_t* turbine,
                          const uint8_t* node_id,
                          const char* ip_address,
                          uint16_t port) {
    if (!turbine || !node_id || !ip_address) return false;
    
    pthread_mutex_lock(&turbine->nodes_mutex);
    
    if (turbine->num_nodes >= turbine->max_nodes) {
        pthread_mutex_unlock(&turbine->nodes_mutex);
        return false;
    }
    
    // Créer nœud
    lum_turbine_node_t* node = create_node(node_id, ip_address, port, turbine->fanout);
    if (!node) {
        pthread_mutex_unlock(&turbine->nodes_mutex);
        return false;
    }
    
    turbine->all_nodes[turbine->num_nodes++] = node;
    
    pthread_mutex_unlock(&turbine->nodes_mutex);
    
    // Reconstruire arbre
    return lum_turbine_rebuild_tree(turbine);
}

bool lum_turbine_remove_node(lum_turbine_t* turbine, const uint8_t* node_id) {
    if (!turbine || !node_id) return false;
    
    pthread_mutex_lock(&turbine->nodes_mutex);
    
    bool found = false;
    for (uint32_t i = 0; i < turbine->num_nodes; i++) {
        if (memcmp(turbine->all_nodes[i]->node_id, node_id, 32) == 0) {
            free_node(turbine->all_nodes[i]);
            
            // Décaler nœuds
            memmove(&turbine->all_nodes[i], &turbine->all_nodes[i + 1],
                    (turbine->num_nodes - i - 1) * sizeof(lum_turbine_node_t*));
            turbine->num_nodes--;
            found = true;
            break;
        }
    }
    
    pthread_mutex_unlock(&turbine->nodes_mutex);
    
    if (found) {
        return lum_turbine_rebuild_tree(turbine);
    }
    
    return false;
}

bool lum_turbine_rebuild_tree(lum_turbine_t* turbine) {
    if (!turbine) return false;
    
    pthread_mutex_lock(&turbine->nodes_mutex);
    
    // Réinitialiser relations parent-enfant
    for (uint32_t i = 0; i < turbine->num_nodes; i++) {
        turbine->all_nodes[i]->parent = NULL;
        turbine->all_nodes[i]->num_children = 0;
    }
    
    // Construire arbre niveau par niveau
    for (uint32_t i = 0; i < turbine->num_nodes; i++) {
        if (i == 0) continue;  // Skip racine (local_node)
        
        // Trouver parent (nœud avec moins d'enfants)
        lum_turbine_node_t* parent = NULL;
        for (uint32_t j = 0; j < i; j++) {
            if (turbine->all_nodes[j]->num_children < turbine->fanout) {
                parent = turbine->all_nodes[j];
                break;
            }
        }
        
        if (parent) {
            turbine->all_nodes[i]->parent = parent;
            parent->children[parent->num_children++] = turbine->all_nodes[i];
        }
    }
    
    pthread_mutex_unlock(&turbine->nodes_mutex);
    return true;
}

/* ============================================================================
 * ENCODAGE / DÉCODAGE REED-SOLOMON (SIMPLIFIÉ)
 * ============================================================================ */

bool lum_turbine_encode_block(const uint8_t* data,
                               uint32_t size,
                               lum_turbine_shred_t** shreds,
                               uint32_t* num_shreds,
                               uint32_t* data_shreds,
                               uint32_t* fec_shreds) {
    if (!data || !shreds || !num_shreds || !data_shreds || !fec_shreds) {
        return false;
    }
    
    // Calculer nombre de fragments
    uint32_t n_data = (size + LUM_TURBINE_SHRED_SIZE - 1) / LUM_TURBINE_SHRED_SIZE;
    uint32_t n_fec = (uint32_t)(n_data * LUM_TURBINE_FEC_RATE);
    uint32_t n_total = n_data + n_fec;
    
    if (n_total > LUM_TURBINE_MAX_SHREDS) {
        return false;
    }
    
    // Allouer fragments
    *shreds = (lum_turbine_shred_t*)calloc(n_total, sizeof(lum_turbine_shred_t));
    if (!*shreds) {
        return false;
    }
    
    // Fragmenter données
    for (uint32_t i = 0; i < n_data; i++) {
        lum_turbine_shred_t* shred = &(*shreds)[i];
        shred->shred_index = i;
        shred->total_shreds = n_total;
        shred->data_shreds = n_data;
        shred->fec_shreds = n_fec;
        
        uint32_t offset = i * LUM_TURBINE_SHRED_SIZE;
        uint32_t chunk_size = (offset + LUM_TURBINE_SHRED_SIZE <= size) ?
                              LUM_TURBINE_SHRED_SIZE : (size - offset);
        
        memcpy(shred->data, data + offset, chunk_size);
        shred->data_size = chunk_size;
    }
    
    // Générer fragments FEC (XOR simplifié pour démo)
    for (uint32_t i = 0; i < n_fec; i++) {
        lum_turbine_shred_t* shred = &(*shreds)[n_data + i];
        shred->shred_index = n_data + i;
        shred->total_shreds = n_total;
        shred->data_shreds = n_data;
        shred->fec_shreds = n_fec;
        shred->data_size = LUM_TURBINE_SHRED_SIZE;
        
        // XOR de plusieurs fragments données
        memset(shred->data, 0, LUM_TURBINE_SHRED_SIZE);
        for (uint32_t j = 0; j < n_data; j++) {
            if ((j % n_fec) == i) {
                for (uint32_t k = 0; k < LUM_TURBINE_SHRED_SIZE; k++) {
                    shred->data[k] ^= (*shreds)[j].data[k];
                }
            }
        }
    }
    
    *num_shreds = n_total;
    *data_shreds = n_data;
    *fec_shreds = n_fec;
    
    return true;
}

bool lum_turbine_decode_block(lum_turbine_shred_t** shreds,
                               uint32_t num_shreds,
                               uint8_t* data,
                               uint32_t* size) {
    if (!shreds || num_shreds == 0 || !data || !size) {
        return false;
    }
    
    // Vérifier qu'on a assez de fragments
    uint32_t data_shreds = shreds[0]->data_shreds;
    if (num_shreds < data_shreds) {
        return false;  // Pas assez de fragments
    }
    
    // Reconstruire données
    uint32_t total_size = 0;
    for (uint32_t i = 0; i < data_shreds; i++) {
        if (i >= num_shreds) break;
        
        memcpy(data + total_size, shreds[i]->data, shreds[i]->data_size);
        total_size += shreds[i]->data_size;
    }
    
    *size = total_size;
    return true;
}

/* ============================================================================
 * PROPAGATION
 * ============================================================================ */

bool lum_turbine_broadcast_block(lum_turbine_t* turbine,
                                  uint64_t block_id,
                                  const uint8_t* data,
                                  uint32_t size) {
    if (!turbine || !data || size == 0 || size > LUM_TURBINE_MAX_BLOCK_SIZE) {
        return false;
    }
    
    // Encoder bloc
    lum_turbine_shred_t* shreds = NULL;
    uint32_t num_shreds, data_shreds, fec_shreds;
    
    if (!lum_turbine_encode_block(data, size, &shreds, &num_shreds, 
                                   &data_shreds, &fec_shreds)) {
        return false;
    }
    
    // Définir block_id pour tous les fragments
    for (uint32_t i = 0; i < num_shreds; i++) {
        shreds[i].block_id = block_id;
    }
    
    // Envoyer fragments aux enfants
    pthread_mutex_lock(&turbine->nodes_mutex);
    
    for (uint32_t i = 0; i < turbine->local_node->num_children; i++) {
        lum_turbine_node_t* child = turbine->local_node->children[i];
        
        // Envoyer subset de fragments à chaque enfant
        for (uint32_t j = i; j < num_shreds; j += turbine->local_node->num_children) {
            lum_turbine_send_shred(turbine, child, &shreds[j]);
        }
    }
    
    pthread_mutex_unlock(&turbine->nodes_mutex);
    
    // Métriques
    turbine->total_blocks_sent++;
    turbine->total_bytes_sent += size;
    
    free(shreds);
    return true;
}

bool lum_turbine_send_shred(lum_turbine_t* turbine,
                             lum_turbine_node_t* node,
                             const lum_turbine_shred_t* shred) {
    if (!turbine || !node || !shred) return false;
    
    // TODO: Implémenter envoi UDP/TCP réel
    // Pour l'instant, simulation
    node->shreds_sent++;
    node->bytes_sent += sizeof(lum_turbine_shred_t);
    
    return true;
}

bool lum_turbine_receive_shred(lum_turbine_t* turbine,
                                const lum_turbine_shred_t* shred) {
    if (!turbine || !shred) return false;
    
    pthread_mutex_lock(&turbine->blocks_mutex);
    
    // Trouver ou créer bloc en cours
    lum_turbine_block_t* block = NULL;
    for (uint32_t i = 0; i < turbine->num_pending; i++) {
        if (turbine->pending_blocks[i] && 
            turbine->pending_blocks[i]->block_id == shred->block_id) {
            block = turbine->pending_blocks[i];
            break;
        }
    }
    
    if (!block) {
        // Créer nouveau bloc
        if (turbine->num_pending >= turbine->max_pending) {
            pthread_mutex_unlock(&turbine->blocks_mutex);
            return false;
        }
        
        block = (lum_turbine_block_t*)calloc(1, sizeof(lum_turbine_block_t));
        if (!block) {
            pthread_mutex_unlock(&turbine->blocks_mutex);
            return false;
        }
        
        block->block_id = shred->block_id;
        block->total_shreds = shred->total_shreds;
        block->data_shreds = shred->data_shreds;
        block->fec_shreds = shred->fec_shreds;
        block->shreds_received = (bool*)calloc(shred->total_shreds, sizeof(bool));
        block->data = (uint8_t*)malloc(LUM_TURBINE_MAX_BLOCK_SIZE);
        block->start_time_ns = get_timestamp_ns();
        pthread_mutex_init(&block->mutex, NULL);
        
        turbine->pending_blocks[turbine->num_pending++] = block;
    }
    
    pthread_mutex_lock(&block->mutex);
    
    // Marquer fragment reçu
    if (!block->shreds_received[shred->shred_index]) {
        block->shreds_received[shred->shred_index] = true;
        block->num_shreds_received++;
    }
    
    // Vérifier si bloc complet
    bool complete = (block->num_shreds_received >= block->data_shreds);
    
    if (complete && !block->reconstructed) {
        block->reconstructed = true;
        block->end_time_ns = get_timestamp_ns();
        
        // Métriques
        turbine->total_blocks_received++;
        double propagation_ms = (block->end_time_ns - block->start_time_ns) / 1000000.0;
        turbine->avg_propagation_time_ms = 
            (turbine->avg_propagation_time_ms * (turbine->total_blocks_received - 1) + propagation_ms) /
            turbine->total_blocks_received;
    }
    
    pthread_mutex_unlock(&block->mutex);
    pthread_mutex_unlock(&turbine->blocks_mutex);
    
    return complete;
}

/* ============================================================================
 * THREAD RÉCEPTION
 * ============================================================================ */

void* lum_turbine_receiver_thread(void* arg) {
    lum_turbine_t* turbine = (lum_turbine_t*)arg;
    
    // TODO: Implémenter socket UDP/TCP réel
    // Pour l'instant, thread vide
    
    while (turbine->running) {
        sleep(1);
    }
    
    return NULL;
}

bool lum_turbine_start(lum_turbine_t* turbine) {
    if (!turbine || turbine->running) return false;
    
    turbine->running = true;
    
    if (pthread_create(&turbine->receiver_thread, NULL, 
                       lum_turbine_receiver_thread, turbine) != 0) {
        turbine->running = false;
        return false;
    }
    
    return true;
}

void lum_turbine_stop(lum_turbine_t* turbine) {
    if (!turbine || !turbine->running) return;
    
    turbine->running = false;
    pthread_join(turbine->receiver_thread, NULL);
}

/* ============================================================================
 * MÉTRIQUES
 * ============================================================================ */

void lum_turbine_get_metrics(lum_turbine_t* turbine,
                              uint64_t* blocks_sent,
                              uint64_t* blocks_received,
                              uint64_t* bytes_sent,
                              uint64_t* bytes_received,
                              double* avg_propagation_ms) {
    if (!turbine) return;
    
    if (blocks_sent) *blocks_sent = turbine->total_blocks_sent;
    if (blocks_received) *blocks_received = turbine->total_blocks_received;
    if (bytes_sent) *bytes_sent = turbine->total_bytes_sent;
    if (bytes_received) *bytes_received = turbine->total_bytes_received;
    if (avg_propagation_ms) *avg_propagation_ms = turbine->avg_propagation_time_ms;
}

bool lum_turbine_get_block(lum_turbine_t* turbine,
                            uint64_t block_id,
                            uint8_t* data,
                            uint32_t* size) {
    if (!turbine || !data || !size) return false;
    
    pthread_mutex_lock(&turbine->blocks_mutex);
    
    bool found = false;
    for (uint32_t i = 0; i < turbine->num_pending; i++) {
        lum_turbine_block_t* block = turbine->pending_blocks[i];
        if (block && block->block_id == block_id && block->reconstructed) {
            memcpy(data, block->data, block->data_size);
            *size = block->data_size;
            found = true;
            break;
        }
    }
    
    pthread_mutex_unlock(&turbine->blocks_mutex);
    return found;
}

// Made with Bob
