#ifndef LUM_RAFT_H
#define LUM_RAFT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/**
 * @file lum_raft.h
 * @brief LUM DBMS Raft Consensus Algorithm
 * 
 * Implémentation du consensus Raft pour haute disponibilité.
 * Garantit cohérence forte dans un cluster distribué.
 * Résiste aux pannes byzantines (jusqu'à (N-1)/2 nœuds défaillants).
 */

// ============================================================================
// TYPES ET STRUCTURES
// ============================================================================

/**
 * @brief État d'un nœud Raft
 */
typedef enum {
    RAFT_STATE_FOLLOWER,    // Suiveur (état initial)
    RAFT_STATE_CANDIDATE,   // Candidat (pendant élection)
    RAFT_STATE_LEADER       // Leader (élu)
} raft_state_t;

/**
 * @brief Type de message Raft
 */
typedef enum {
    RAFT_MSG_REQUEST_VOTE,       // Demande de vote
    RAFT_MSG_REQUEST_VOTE_REPLY, // Réponse vote
    RAFT_MSG_APPEND_ENTRIES,     // Ajout entrées log
    RAFT_MSG_APPEND_ENTRIES_REPLY, // Réponse ajout
    RAFT_MSG_INSTALL_SNAPSHOT,   // Installation snapshot
    RAFT_MSG_HEARTBEAT           // Heartbeat leader
} raft_message_type_t;

/**
 * @brief Entrée du log Raft
 */
typedef struct {
    uint64_t index;           // Index dans le log
    uint64_t term;            // Terme de l'entrée
    uint32_t command_type;    // Type de commande
    uint32_t data_size;       // Taille données
    uint8_t* data;            // Données commande
    uint32_t crc32;           // CRC32 intégrité
} raft_log_entry_t;

/**
 * @brief Information sur un pair Raft
 */
typedef struct {
    uint32_t node_id;         // ID du nœud
    char hostname[256];       // Nom d'hôte
    uint16_t port;            // Port
    uint64_t next_index;      // Prochain index à envoyer
    uint64_t match_index;     // Index le plus haut répliqué
    time_t last_contact;      // Dernier contact
    bool vote_granted;        // Vote accordé pour terme courant
} raft_peer_t;

/**
 * @brief Message RequestVote
 */
typedef struct {
    uint64_t term;            // Terme du candidat
    uint32_t candidate_id;    // ID du candidat
    uint64_t last_log_index;  // Index dernière entrée log
    uint64_t last_log_term;   // Terme dernière entrée log
} raft_request_vote_t;

/**
 * @brief Message RequestVote Reply
 */
typedef struct {
    uint64_t term;            // Terme courant
    bool vote_granted;        // Vote accordé
} raft_request_vote_reply_t;

/**
 * @brief Message AppendEntries
 */
typedef struct {
    uint64_t term;            // Terme du leader
    uint32_t leader_id;       // ID du leader
    uint64_t prev_log_index;  // Index entrée précédente
    uint64_t prev_log_term;   // Terme entrée précédente
    uint64_t leader_commit;   // Index commit du leader
    uint32_t entry_count;     // Nombre d'entrées
    raft_log_entry_t* entries; // Entrées à ajouter
} raft_append_entries_t;

/**
 * @brief Message AppendEntries Reply
 */
typedef struct {
    uint64_t term;            // Terme courant
    bool success;             // Succès de l'ajout
    uint64_t match_index;     // Index correspondant
} raft_append_entries_reply_t;

/**
 * @brief Configuration Raft
 */
typedef struct {
    uint32_t election_timeout_min_ms;  // Timeout élection min (ms)
    uint32_t election_timeout_max_ms;  // Timeout élection max (ms)
    uint32_t heartbeat_interval_ms;    // Intervalle heartbeat (ms)
    uint32_t max_entries_per_append;   // Max entrées par AppendEntries
    uint32_t snapshot_interval;        // Intervalle snapshot (entrées)
    uint32_t max_log_size;             // Taille max log avant snapshot
    bool enable_pre_vote;              // Activer pré-vote (évite élections inutiles)
} raft_config_t;

/**
 * @brief Contexte Raft
 */
typedef struct {
    // Configuration
    raft_config_t config;
    uint32_t node_id;
    uint32_t cluster_size;
    
    // État persistant (doit survivre aux redémarrages)
    uint64_t current_term;        // Terme courant
    uint32_t voted_for;           // Vote pour ce terme (0 = aucun)
    raft_log_entry_t* log;        // Log des entrées
    uint64_t log_size;            // Taille du log
    uint64_t log_capacity;        // Capacité du log
    
    // État volatile
    raft_state_t state;           // État courant
    uint64_t commit_index;        // Index le plus haut commité
    uint64_t last_applied;        // Index le plus haut appliqué
    
    // État volatile (leader uniquement)
    raft_peer_t* peers;           // Liste des pairs
    uint32_t peer_count;          // Nombre de pairs
    
    // Élection
    time_t election_deadline;     // Deadline élection
    uint32_t votes_received;      // Votes reçus
    
    // Threads
    pthread_t election_thread;    // Thread élection
    pthread_t heartbeat_thread;   // Thread heartbeat
    pthread_mutex_t lock;         // Lock global
    bool running;                 // Threads actifs
    
    // Statistiques
    uint64_t total_elections;     // Total élections
    uint64_t total_heartbeats;    // Total heartbeats
    uint64_t total_log_entries;   // Total entrées log
    time_t last_election;         // Dernière élection
} raft_context_t;

// ============================================================================
// API PUBLIQUE
// ============================================================================

/**
 * @brief Initialise le contexte Raft
 * @param config Configuration Raft
 * @param node_id ID de ce nœud
 * @param cluster_size Taille du cluster
 * @return Contexte Raft ou NULL si erreur
 */
raft_context_t* raft_init(const raft_config_t* config, 
                          uint32_t node_id,
                          uint32_t cluster_size);

/**
 * @brief Libère le contexte Raft
 * @param ctx Contexte à libérer
 */
void raft_free(raft_context_t* ctx);

/**
 * @brief Démarre le nœud Raft
 * @param ctx Contexte Raft
 * @return true si succès, false sinon
 */
bool raft_start(raft_context_t* ctx);

/**
 * @brief Arrête le nœud Raft
 * @param ctx Contexte Raft
 */
void raft_stop(raft_context_t* ctx);

/**
 * @brief Ajoute un pair au cluster
 * @param ctx Contexte Raft
 * @param node_id ID du pair
 * @param hostname Nom d'hôte du pair
 * @param port Port du pair
 * @return true si succès, false sinon
 */
bool raft_add_peer(raft_context_t* ctx,
                   uint32_t node_id,
                   const char* hostname,
                   uint16_t port);

/**
 * @brief Retire un pair du cluster
 * @param ctx Contexte Raft
 * @param node_id ID du pair à retirer
 * @return true si succès, false sinon
 */
bool raft_remove_peer(raft_context_t* ctx, uint32_t node_id);

/**
 * @brief Soumet une commande au cluster (leader uniquement)
 * @param ctx Contexte Raft
 * @param command_type Type de commande
 * @param data Données de la commande
 * @param data_size Taille des données
 * @return Index de l'entrée ou 0 si erreur
 */
uint64_t raft_submit_command(raft_context_t* ctx,
                              uint32_t command_type,
                              const uint8_t* data,
                              uint32_t data_size);

/**
 * @brief Attend qu'une entrée soit commitée
 * @param ctx Contexte Raft
 * @param index Index de l'entrée
 * @param timeout_ms Timeout en millisecondes
 * @return true si commitée, false si timeout
 */
bool raft_wait_committed(raft_context_t* ctx,
                         uint64_t index,
                         uint32_t timeout_ms);

/**
 * @brief Traite un message RequestVote
 * @param ctx Contexte Raft
 * @param request Message RequestVote
 * @param reply Réponse à remplir
 * @return true si succès, false sinon
 */
bool raft_handle_request_vote(raft_context_t* ctx,
                               const raft_request_vote_t* request,
                               raft_request_vote_reply_t* reply);

/**
 * @brief Traite un message AppendEntries
 * @param ctx Contexte Raft
 * @param request Message AppendEntries
 * @param reply Réponse à remplir
 * @return true si succès, false sinon
 */
bool raft_handle_append_entries(raft_context_t* ctx,
                                 const raft_append_entries_t* request,
                                 raft_append_entries_reply_t* reply);

/**
 * @brief Vérifie si ce nœud est le leader
 * @param ctx Contexte Raft
 * @return true si leader, false sinon
 */
bool raft_is_leader(const raft_context_t* ctx);

/**
 * @brief Obtient l'ID du leader actuel
 * @param ctx Contexte Raft
 * @return ID du leader ou 0 si aucun
 */
uint32_t raft_get_leader_id(const raft_context_t* ctx);

/**
 * @brief Obtient le terme courant
 * @param ctx Contexte Raft
 * @return Terme courant
 */
uint64_t raft_get_current_term(const raft_context_t* ctx);

/**
 * @brief Obtient l'état du nœud
 * @param ctx Contexte Raft
 * @return État du nœud
 */
raft_state_t raft_get_state(const raft_context_t* ctx);

/**
 * @brief Obtient les statistiques Raft
 * @param ctx Contexte Raft
 * @param total_elections Pointeur pour total élections
 * @param total_heartbeats Pointeur pour total heartbeats
 * @param total_log_entries Pointeur pour total entrées log
 */
void raft_get_stats(const raft_context_t* ctx,
                    uint64_t* total_elections,
                    uint64_t* total_heartbeats,
                    uint64_t* total_log_entries);

/**
 * @brief Crée un snapshot du log
 * @param ctx Contexte Raft
 * @param last_included_index Dernier index inclus
 * @param last_included_term Dernier terme inclus
 * @return true si succès, false sinon
 */
bool raft_create_snapshot(raft_context_t* ctx,
                          uint64_t last_included_index,
                          uint64_t last_included_term);

/**
 * @brief Génère un rapport Raft au format Markdown
 * @param ctx Contexte Raft
 * @param output_path Chemin du fichier de sortie
 * @return true si succès, false sinon
 */
bool raft_generate_report(const raft_context_t* ctx,
                          const char* output_path);

#endif // LUM_RAFT_H

// Made with Bob
