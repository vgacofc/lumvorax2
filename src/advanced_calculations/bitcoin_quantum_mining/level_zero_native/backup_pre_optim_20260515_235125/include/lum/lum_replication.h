#ifndef LUM_REPLICATION_H
#define LUM_REPLICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/**
 * @file lum_replication.h
 * @brief LUM DBMS Replication System
 * 
 * Système de réplication Master-Slave avec failover automatique.
 * Supporte réplication synchrone et asynchrone.
 * Compatible avec consensus Raft pour haute disponibilité.
 */

// ============================================================================
// TYPES ET STRUCTURES
// ============================================================================

/**
 * @brief Type de nœud dans le cluster
 */
typedef enum {
    LUM_NODE_MASTER,      // Nœud maître (lecture/écriture)
    LUM_NODE_SLAVE,       // Nœud esclave (lecture seule)
    LUM_NODE_CANDIDATE,   // Candidat pour élection
    LUM_NODE_OBSERVER     // Observateur (pas de vote)
} lum_node_type_t;

/**
 * @brief Mode de réplication
 */
typedef enum {
    LUM_REPL_SYNC,        // Synchrone (attente confirmation slaves)
    LUM_REPL_ASYNC,       // Asynchrone (pas d'attente)
    LUM_REPL_SEMI_SYNC    // Semi-synchrone (attente N slaves)
} lum_replication_mode_t;

/**
 * @brief État de réplication d'un slave
 */
typedef enum {
    LUM_SLAVE_CONNECTED,      // Connecté et à jour
    LUM_SLAVE_SYNCING,        // En cours de synchronisation
    LUM_SLAVE_LAGGING,        // En retard (lag > seuil)
    LUM_SLAVE_DISCONNECTED,   // Déconnecté
    LUM_SLAVE_FAILED          // Échec permanent
} lum_slave_state_t;

/**
 * @brief Information sur un nœud slave
 */
typedef struct {
    uint32_t node_id;                 // ID unique du slave
    char hostname[256];               // Nom d'hôte
    uint16_t port;                    // Port de connexion
    lum_slave_state_t state;          // État actuel
    uint64_t last_applied_lsn;        // Dernier LSN appliqué
    uint64_t lag_bytes;               // Retard en octets
    time_t last_heartbeat;            // Dernier heartbeat reçu
    uint32_t replication_delay_ms;    // Délai réplication (ms)
} lum_slave_info_t;

/**
 * @brief Configuration de réplication
 */
typedef struct {
    lum_replication_mode_t mode;      // Mode réplication
    uint32_t max_slaves;              // Nombre max de slaves
    uint32_t sync_slaves_required;    // Slaves requis pour sync
    uint32_t heartbeat_interval_ms;   // Intervalle heartbeat (ms)
    uint32_t heartbeat_timeout_ms;    // Timeout heartbeat (ms)
    uint32_t max_lag_bytes;           // Lag max autorisé (bytes)
    uint32_t reconnect_interval_ms;   // Intervalle reconnexion (ms)
    bool auto_failover;               // Failover automatique activé
    uint32_t failover_timeout_ms;     // Timeout failover (ms)
} lum_replication_config_t;

/**
 * @brief Contexte de réplication
 */
typedef struct {
    lum_node_type_t node_type;        // Type de ce nœud
    uint32_t node_id;                 // ID de ce nœud
    lum_replication_config_t config;  // Configuration
    
    // Master state
    lum_slave_info_t* slaves;         // Liste des slaves
    uint32_t slave_count;             // Nombre de slaves
    uint64_t current_lsn;             // LSN courant
    pthread_mutex_t slaves_lock;      // Lock pour slaves
    
    // Slave state
    uint32_t master_node_id;          // ID du master
    char master_hostname[256];        // Hostname du master
    uint16_t master_port;             // Port du master
    uint64_t last_received_lsn;       // Dernier LSN reçu
    
    // Threads
    pthread_t heartbeat_thread;       // Thread heartbeat
    pthread_t replication_thread;     // Thread réplication
    bool running;                     // Threads actifs
    
    // Statistiques
    uint64_t total_bytes_replicated;  // Total octets répliqués
    uint64_t total_transactions;      // Total transactions
    uint32_t failover_count;          // Nombre de failovers
    time_t last_failover;             // Dernier failover
} lum_replication_context_t;

/**
 * @brief Événement de réplication
 */
typedef struct {
    uint64_t lsn;                     // Log Sequence Number
    uint32_t transaction_id;          // ID transaction
    uint64_t timestamp_ns;            // Timestamp nanoseconde
    uint32_t data_size;               // Taille données
    uint8_t* data;                    // Données à répliquer
    uint32_t crc32;                   // CRC32 intégrité
} lum_replication_event_t;

// ============================================================================
// API PUBLIQUE
// ============================================================================

/**
 * @brief Initialise le système de réplication
 * @param config Configuration de réplication
 * @return Contexte de réplication ou NULL si erreur
 */
lum_replication_context_t* lum_replication_init(const lum_replication_config_t* config);

/**
 * @brief Libère le contexte de réplication
 * @param ctx Contexte à libérer
 */
void lum_replication_free(lum_replication_context_t* ctx);

/**
 * @brief Démarre la réplication (master ou slave)
 * @param ctx Contexte de réplication
 * @param node_type Type de nœud (MASTER ou SLAVE)
 * @return true si succès, false sinon
 */
bool lum_replication_start(lum_replication_context_t* ctx, lum_node_type_t node_type);

/**
 * @brief Arrête la réplication
 * @param ctx Contexte de réplication
 */
void lum_replication_stop(lum_replication_context_t* ctx);

/**
 * @brief Ajoute un slave au cluster (master uniquement)
 * @param ctx Contexte de réplication
 * @param hostname Nom d'hôte du slave
 * @param port Port du slave
 * @return ID du slave ou 0 si erreur
 */
uint32_t lum_replication_add_slave(lum_replication_context_t* ctx, 
                                    const char* hostname, 
                                    uint16_t port);

/**
 * @brief Retire un slave du cluster (master uniquement)
 * @param ctx Contexte de réplication
 * @param slave_id ID du slave à retirer
 * @return true si succès, false sinon
 */
bool lum_replication_remove_slave(lum_replication_context_t* ctx, uint32_t slave_id);

/**
 * @brief Réplique un événement vers les slaves (master uniquement)
 * @param ctx Contexte de réplication
 * @param event Événement à répliquer
 * @return true si succès, false sinon
 */
bool lum_replication_replicate(lum_replication_context_t* ctx, 
                                const lum_replication_event_t* event);

/**
 * @brief Attend la confirmation de réplication (mode sync)
 * @param ctx Contexte de réplication
 * @param lsn LSN à attendre
 * @param timeout_ms Timeout en millisecondes
 * @return true si confirmé, false si timeout
 */
bool lum_replication_wait_sync(lum_replication_context_t* ctx, 
                                uint64_t lsn, 
                                uint32_t timeout_ms);

/**
 * @brief Obtient l'état d'un slave
 * @param ctx Contexte de réplication
 * @param slave_id ID du slave
 * @param info Structure à remplir avec les infos
 * @return true si succès, false sinon
 */
bool lum_replication_get_slave_info(lum_replication_context_t* ctx, 
                                     uint32_t slave_id, 
                                     lum_slave_info_t* info);

/**
 * @brief Obtient la liste de tous les slaves
 * @param ctx Contexte de réplication
 * @param slaves Tableau à remplir (alloué par l'appelant)
 * @param max_slaves Taille max du tableau
 * @return Nombre de slaves retournés
 */
uint32_t lum_replication_list_slaves(lum_replication_context_t* ctx, 
                                      lum_slave_info_t* slaves, 
                                      uint32_t max_slaves);

/**
 * @brief Déclenche un failover manuel
 * @param ctx Contexte de réplication
 * @param new_master_id ID du nouveau master (0 = auto)
 * @return true si succès, false sinon
 */
bool lum_replication_failover(lum_replication_context_t* ctx, uint32_t new_master_id);

/**
 * @brief Vérifie si le nœud est le master actuel
 * @param ctx Contexte de réplication
 * @return true si master, false sinon
 */
bool lum_replication_is_master(const lum_replication_context_t* ctx);

/**
 * @brief Obtient le LSN courant
 * @param ctx Contexte de réplication
 * @return LSN courant
 */
uint64_t lum_replication_get_current_lsn(const lum_replication_context_t* ctx);

/**
 * @brief Obtient les statistiques de réplication
 * @param ctx Contexte de réplication
 * @param total_bytes Pointeur pour total octets répliqués
 * @param total_txns Pointeur pour total transactions
 * @param failover_count Pointeur pour nombre de failovers
 */
void lum_replication_get_stats(const lum_replication_context_t* ctx,
                                uint64_t* total_bytes,
                                uint64_t* total_txns,
                                uint32_t* failover_count);

/**
 * @brief Génère un rapport de réplication au format Markdown
 * @param ctx Contexte de réplication
 * @param output_path Chemin du fichier de sortie
 * @return true si succès, false sinon
 */
bool lum_replication_generate_report(const lum_replication_context_t* ctx, 
                                      const char* output_path);

#endif // LUM_REPLICATION_H

// Made with Bob
