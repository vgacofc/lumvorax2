/* _GNU_SOURCE déjà défini par CFLAGS dans Makefile */
#include "lum_raft.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/**
 * @file lum_raft.c
 * @brief Implémentation du consensus Raft pour LUM DBMS
 */

// ============================================================================
// CONSTANTES INTERNES
// ============================================================================

#define RAFT_MAGIC 0x52414654  // "RAFT"
#define RAFT_VERSION 1
#define RAFT_INITIAL_LOG_CAPACITY 1024

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
 * @brief Génère timeout élection aléatoire
 */
static uint32_t generate_election_timeout(const raft_config_t* config) {
    uint32_t range = config->election_timeout_max_ms - config->election_timeout_min_ms;
    return config->election_timeout_min_ms + (rand() % range);
}

/**
 * @brief Obtient timestamp en secondes
 */
static time_t get_current_time(void) {
    return time(NULL);
}

/**
 * @brief Réinitialise le timeout d'élection
 */
static void reset_election_timeout(raft_context_t* ctx) {
    uint32_t timeout_ms = generate_election_timeout(&ctx->config);
    ctx->election_deadline = get_current_time() + (timeout_ms / 1000);
}

/**
 * @brief Vérifie si le timeout d'élection est expiré
 */
static bool is_election_timeout_expired(const raft_context_t* ctx) {
    return get_current_time() >= ctx->election_deadline;
}

/**
 * @brief Devient candidat et démarre élection
 */
static void become_candidate(raft_context_t* ctx) {
    pthread_mutex_lock(&ctx->lock);
    
    ctx->state = RAFT_STATE_CANDIDATE;
    ctx->current_term++;
    ctx->voted_for = ctx->node_id;
    ctx->votes_received = 1; // Vote pour soi-même
    ctx->total_elections++;
    ctx->last_election = get_current_time();
    
    reset_election_timeout(ctx);
    
    pthread_mutex_unlock(&ctx->lock);
    
    // TODO: Envoyer RequestVote à tous les pairs
}

/**
 * @brief Devient leader
 */
static void become_leader(raft_context_t* ctx) __attribute__((unused));
static void become_leader(raft_context_t* ctx) {
    pthread_mutex_lock(&ctx->lock);
    
    ctx->state = RAFT_STATE_LEADER;
    
    // Initialiser next_index et match_index pour chaque pair
    for (uint32_t i = 0; i < ctx->peer_count; i++) {
        ctx->peers[i].next_index = ctx->log_size + 1;
        ctx->peers[i].match_index = 0;
    }
    
    pthread_mutex_unlock(&ctx->lock);
    
    // Envoyer heartbeat immédiatement
}

/**
 * @brief Devient follower
 */
static void become_follower(raft_context_t* ctx, uint64_t term) __attribute__((unused));
static void become_follower(raft_context_t* ctx, uint64_t term) {
    pthread_mutex_lock(&ctx->lock);
    
    ctx->state = RAFT_STATE_FOLLOWER;
    ctx->current_term = term;
    ctx->voted_for = 0;
    
    reset_election_timeout(ctx);
    
    pthread_mutex_unlock(&ctx->lock);
}

/**
 * @brief Thread élection
 */
static void* election_thread_func(void* arg) {
    raft_context_t* ctx = (raft_context_t*)arg;
    
    while (ctx->running) {
        pthread_mutex_lock(&ctx->lock);
        raft_state_t state = ctx->state;
        pthread_mutex_unlock(&ctx->lock);
        
        if (state == RAFT_STATE_FOLLOWER || state == RAFT_STATE_CANDIDATE) {
            if (is_election_timeout_expired(ctx)) {
                become_candidate(ctx);
            }
        }
        
        usleep(10000); // 10ms
    }
    
    return NULL;
}

/**
 * @brief Thread heartbeat (leader uniquement)
 */
static void* heartbeat_thread_func(void* arg) {
    raft_context_t* ctx = (raft_context_t*)arg;
    
    while (ctx->running) {
        pthread_mutex_lock(&ctx->lock);
        
        if (ctx->state == RAFT_STATE_LEADER) {
            // Envoyer AppendEntries (heartbeat) à tous les pairs
            for (uint32_t i = 0; i < ctx->peer_count; i++) {
                // TODO: Envoyer AppendEntries au pair
                ctx->total_heartbeats++;
            }
        }
        
        pthread_mutex_unlock(&ctx->lock);
        
        usleep(ctx->config.heartbeat_interval_ms * 1000);
    }
    
    return NULL;
}

// ============================================================================
// IMPLÉMENTATION API PUBLIQUE
// ============================================================================

raft_context_t* raft_init(const raft_config_t* config,
                          uint32_t node_id,
                          uint32_t cluster_size) {
    if (!config || node_id == 0 || cluster_size == 0) {
        return NULL;
    }
    
    raft_context_t* ctx = calloc(1, sizeof(raft_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Copier configuration
    memcpy(&ctx->config, config, sizeof(raft_config_t));
    ctx->node_id = node_id;
    ctx->cluster_size = cluster_size;
    
    // Initialiser log
    ctx->log_capacity = RAFT_INITIAL_LOG_CAPACITY;
    ctx->log = calloc(ctx->log_capacity, sizeof(raft_log_entry_t));
    if (!ctx->log) {
        free(ctx);
        return NULL;
    }
    
    // Allouer tableau pairs
    ctx->peers = calloc(cluster_size - 1, sizeof(raft_peer_t));
    if (!ctx->peers) {
        free(ctx->log);
        free(ctx);
        return NULL;
    }
    
    // Initialiser mutex
    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        free(ctx->peers);
        free(ctx->log);
        free(ctx);
        return NULL;
    }
    
    // État initial
    ctx->state = RAFT_STATE_FOLLOWER;
    ctx->current_term = 0;
    ctx->voted_for = 0;
    ctx->log_size = 0;
    ctx->commit_index = 0;
    ctx->last_applied = 0;
    ctx->peer_count = 0;
    ctx->running = false;
    
    reset_election_timeout(ctx);
    
    return ctx;
}

void raft_free(raft_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Arrêter si actif
    if (ctx->running) {
        raft_stop(ctx);
    }
    
    // Libérer log
    for (uint64_t i = 0; i < ctx->log_size; i++) {
        free(ctx->log[i].data);
    }
    free(ctx->log);
    
    // Libérer ressources
    pthread_mutex_destroy(&ctx->lock);
    free(ctx->peers);
    free(ctx);
}

bool raft_start(raft_context_t* ctx) {
    if (!ctx || ctx->running) {
        return false;
    }
    
    ctx->running = true;
    
    // Démarrer thread élection
    if (pthread_create(&ctx->election_thread, NULL,
                      election_thread_func, ctx) != 0) {
        ctx->running = false;
        return false;
    }
    
    // Démarrer thread heartbeat
    if (pthread_create(&ctx->heartbeat_thread, NULL,
                      heartbeat_thread_func, ctx) != 0) {
        ctx->running = false;
        pthread_join(ctx->election_thread, NULL);
        return false;
    }
    
    return true;
}

void raft_stop(raft_context_t* ctx) {
    if (!ctx || !ctx->running) {
        return;
    }
    
    ctx->running = false;
    
    // Attendre fin des threads
    pthread_join(ctx->election_thread, NULL);
    pthread_join(ctx->heartbeat_thread, NULL);
}

bool raft_add_peer(raft_context_t* ctx,
                   uint32_t node_id,
                   const char* hostname,
                   uint16_t port) {
    if (!ctx || !hostname || node_id == ctx->node_id) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    // Vérifier capacité
    if (ctx->peer_count >= ctx->cluster_size - 1) {
        pthread_mutex_unlock(&ctx->lock);
        return false;
    }
    
    // Initialiser pair
    raft_peer_t* peer = &ctx->peers[ctx->peer_count];
    peer->node_id = node_id;
    strncpy(peer->hostname, hostname, sizeof(peer->hostname) - 1);
    peer->port = port;
    peer->next_index = ctx->log_size + 1;
    peer->match_index = 0;
    peer->last_contact = get_current_time();
    peer->vote_granted = false;
    
    ctx->peer_count++;
    
    pthread_mutex_unlock(&ctx->lock);
    
    return true;
}

bool raft_remove_peer(raft_context_t* ctx, uint32_t node_id) {
    if (!ctx) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    // Trouver pair
    bool found = false;
    uint32_t index = 0;
    for (uint32_t i = 0; i < ctx->peer_count; i++) {
        if (ctx->peers[i].node_id == node_id) {
            found = true;
            index = i;
            break;
        }
    }
    
    if (!found) {
        pthread_mutex_unlock(&ctx->lock);
        return false;
    }
    
    // Décaler pairs suivants
    for (uint32_t i = index; i < ctx->peer_count - 1; i++) {
        ctx->peers[i] = ctx->peers[i + 1];
    }
    
    ctx->peer_count--;
    
    pthread_mutex_unlock(&ctx->lock);
    
    return true;
}

uint64_t raft_submit_command(raft_context_t* ctx,
                              uint32_t command_type,
                              const uint8_t* data,
                              uint32_t data_size) {
    if (!ctx || !data || ctx->state != RAFT_STATE_LEADER) {
        return 0;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    // Vérifier capacité log
    if (ctx->log_size >= ctx->log_capacity) {
        // Doubler capacité
        uint64_t new_capacity = ctx->log_capacity * 2;
        raft_log_entry_t* new_log = realloc(ctx->log,
                                            new_capacity * sizeof(raft_log_entry_t));
        if (!new_log) {
            pthread_mutex_unlock(&ctx->lock);
            return 0;
        }
        ctx->log = new_log;
        ctx->log_capacity = new_capacity;
    }
    
    // Créer entrée
    raft_log_entry_t* entry = &ctx->log[ctx->log_size];
    entry->index = ctx->log_size + 1;
    entry->term = ctx->current_term;
    entry->command_type = command_type;
    entry->data_size = data_size;
    entry->data = malloc(data_size);
    if (!entry->data) {
        pthread_mutex_unlock(&ctx->lock);
        return 0;
    }
    memcpy(entry->data, data, data_size);
    entry->crc32 = calculate_crc32(data, data_size);
    
    uint64_t index = entry->index;
    ctx->log_size++;
    ctx->total_log_entries++;
    
    pthread_mutex_unlock(&ctx->lock);
    
    // TODO: Répliquer vers pairs
    
    return index;
}

bool raft_wait_committed(raft_context_t* ctx,
                         uint64_t index,
                         uint32_t timeout_ms) {
    if (!ctx || index == 0) {
        return false;
    }
    
    time_t start = get_current_time();
    time_t timeout_s = timeout_ms / 1000;
    
    while (true) {
        pthread_mutex_lock(&ctx->lock);
        uint64_t commit_index = ctx->commit_index;
        pthread_mutex_unlock(&ctx->lock);
        
        if (commit_index >= index) {
            return true;
        }
        
        if (get_current_time() - start >= timeout_s) {
            return false;
        }
        
        usleep(1000); // 1ms
    }
}

bool raft_handle_request_vote(raft_context_t* ctx,
                               const raft_request_vote_t* request,
                               raft_request_vote_reply_t* reply) {
    if (!ctx || !request || !reply) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    reply->term = ctx->current_term;
    reply->vote_granted = false;
    
    // Si terme du candidat est plus ancien, refuser
    if (request->term < ctx->current_term) {
        pthread_mutex_unlock(&ctx->lock);
        return true;
    }
    
    // Si terme du candidat est plus récent, devenir follower
    if (request->term > ctx->current_term) {
        ctx->current_term = request->term;
        ctx->voted_for = 0;
        ctx->state = RAFT_STATE_FOLLOWER;
    }
    
    // Vérifier si on peut voter
    bool can_vote = (ctx->voted_for == 0 || ctx->voted_for == request->candidate_id);
    
    // Vérifier si log du candidat est à jour
    uint64_t last_log_index = ctx->log_size;
    uint64_t last_log_term = (ctx->log_size > 0) ? ctx->log[ctx->log_size - 1].term : 0;
    
    bool log_up_to_date = (request->last_log_term > last_log_term) ||
                          (request->last_log_term == last_log_term &&
                           request->last_log_index >= last_log_index);
    
    if (can_vote && log_up_to_date) {
        ctx->voted_for = request->candidate_id;
        reply->vote_granted = true;
        reset_election_timeout(ctx);
    }
    
    pthread_mutex_unlock(&ctx->lock);
    
    return true;
}

bool raft_handle_append_entries(raft_context_t* ctx,
                                 const raft_append_entries_t* request,
                                 raft_append_entries_reply_t* reply) {
    if (!ctx || !request || !reply) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    reply->term = ctx->current_term;
    reply->success = false;
    reply->match_index = 0;
    
    // Si terme du leader est plus ancien, refuser
    if (request->term < ctx->current_term) {
        pthread_mutex_unlock(&ctx->lock);
        return true;
    }
    
    // Si terme du leader est plus récent, devenir follower
    if (request->term > ctx->current_term) {
        ctx->current_term = request->term;
        ctx->voted_for = 0;
        ctx->state = RAFT_STATE_FOLLOWER;
    }
    
    // Réinitialiser timeout élection
    reset_election_timeout(ctx);
    
    // Vérifier cohérence log
    if (request->prev_log_index > 0) {
        if (request->prev_log_index > ctx->log_size) {
            pthread_mutex_unlock(&ctx->lock);
            return true;
        }
        
        if (ctx->log[request->prev_log_index - 1].term != request->prev_log_term) {
            pthread_mutex_unlock(&ctx->lock);
            return true;
        }
    }
    
    // Ajouter nouvelles entrées
    for (uint32_t i = 0; i < request->entry_count; i++) {
        // TODO: Ajouter entrée au log
    }
    
    // Mettre à jour commit_index
    if (request->leader_commit > ctx->commit_index) {
        ctx->commit_index = (request->leader_commit < ctx->log_size) ?
                           request->leader_commit : ctx->log_size;
    }
    
    reply->success = true;
    reply->match_index = ctx->log_size;
    
    pthread_mutex_unlock(&ctx->lock);
    
    return true;
}

bool raft_is_leader(const raft_context_t* ctx) {
    return ctx && ctx->state == RAFT_STATE_LEADER;
}

uint32_t raft_get_leader_id(const raft_context_t* ctx) {
    if (!ctx) {
        return 0;
    }
    
    if (ctx->state == RAFT_STATE_LEADER) {
        return ctx->node_id;
    }
    
    // TODO: Retourner ID du leader connu
    return 0;
}

uint64_t raft_get_current_term(const raft_context_t* ctx) {
    return ctx ? ctx->current_term : 0;
}

raft_state_t raft_get_state(const raft_context_t* ctx) {
    return ctx ? ctx->state : RAFT_STATE_FOLLOWER;
}

void raft_get_stats(const raft_context_t* ctx,
                    uint64_t* total_elections,
                    uint64_t* total_heartbeats,
                    uint64_t* total_log_entries) {
    if (!ctx) {
        return;
    }
    
    if (total_elections) {
        *total_elections = ctx->total_elections;
    }
    if (total_heartbeats) {
        *total_heartbeats = ctx->total_heartbeats;
    }
    if (total_log_entries) {
        *total_log_entries = ctx->total_log_entries;
    }
}

bool raft_create_snapshot(raft_context_t* ctx,
                          uint64_t last_included_index,
                          uint64_t last_included_term) {
    (void)last_included_term; /* Reserved for future snapshot metadata */
    if (!ctx || last_included_index == 0) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    // TODO: Créer snapshot et compacter log
    
    pthread_mutex_unlock(&ctx->lock);
    
    return true;
}

bool raft_generate_report(const raft_context_t* ctx,
                          const char* output_path) {
    if (!ctx || !output_path) {
        return false;
    }
    
    FILE* f = fopen(output_path, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "# LUM DBMS Raft Consensus Report\n\n");
    fprintf(f, "**Generated**: %s\n", ctime(&(time_t){time(NULL)}));
    fprintf(f, "**Node ID**: %u\n", ctx->node_id);
    fprintf(f, "**State**: %s\n",
            ctx->state == RAFT_STATE_LEADER ? "LEADER" :
            ctx->state == RAFT_STATE_CANDIDATE ? "CANDIDATE" : "FOLLOWER");
    fprintf(f, "**Current Term**: %lu\n\n", ctx->current_term);
    
    fprintf(f, "## Configuration\n\n");
    fprintf(f, "| Parameter | Value |\n");
    fprintf(f, "|-----------|-------|\n");
    fprintf(f, "| Cluster Size | %u |\n", ctx->cluster_size);
    fprintf(f, "| Election Timeout | %u-%u ms |\n",
            ctx->config.election_timeout_min_ms,
            ctx->config.election_timeout_max_ms);
    fprintf(f, "| Heartbeat Interval | %u ms |\n", ctx->config.heartbeat_interval_ms);
    fprintf(f, "| Pre-Vote Enabled | %s |\n", ctx->config.enable_pre_vote ? "YES" : "NO");
    
    fprintf(f, "\n## Statistics\n\n");
    fprintf(f, "| Metric | Value |\n");
    fprintf(f, "|--------|-------|\n");
    fprintf(f, "| Log Size | %lu |\n", ctx->log_size);
    fprintf(f, "| Commit Index | %lu |\n", ctx->commit_index);
    fprintf(f, "| Last Applied | %lu |\n", ctx->last_applied);
    fprintf(f, "| Total Elections | %lu |\n", ctx->total_elections);
    fprintf(f, "| Total Heartbeats | %lu |\n", ctx->total_heartbeats);
    fprintf(f, "| Total Log Entries | %lu |\n", ctx->total_log_entries);
    fprintf(f, "| Active Peers | %u |\n", ctx->peer_count);
    
    if (ctx->peer_count > 0) {
        fprintf(f, "\n## Peers Status\n\n");
        fprintf(f, "| ID | Hostname | Next Index | Match Index |\n");
        fprintf(f, "|----|----------|------------|-------------|\n");
        
        for (uint32_t i = 0; i < ctx->peer_count; i++) {
            const raft_peer_t* peer = &ctx->peers[i];
            fprintf(f, "| %u | %s:%u | %lu | %lu |\n",
                    peer->node_id, peer->hostname, peer->port,
                    peer->next_index, peer->match_index);
        }
    }
    
    fclose(f);
    return true;
}

// Made with Bob
