#ifndef LUM_P2P_NODE_H
#define LUM_P2P_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/**
 * @file lum_p2p_node.h
 * @brief LUM P2P Node - Architecture décentralisée peer-to-peer
 * 
 * Nœud P2P pour réseau de minage distribué décentralisé.
 * Supporte DHT (Distributed Hash Table), Gossip protocol, et consensus Byzantine Fault Tolerant.
 */

// ============================================================================
// TYPES ET STRUCTURES
// ============================================================================

/**
 * @brief État d'un nœud P2P
 */
typedef enum {
    P2P_NODE_DISCONNECTED,    // Déconnecté
    P2P_NODE_CONNECTING,      // En cours de connexion
    P2P_NODE_CONNECTED,       // Connecté au réseau
    P2P_NODE_SYNCING,         // Synchronisation en cours
    P2P_NODE_ACTIVE           // Actif et opérationnel
} p2p_node_state_t;

/**
 * @brief Type de message P2P
 */
typedef enum {
    P2P_MSG_PING,             // Ping (keepalive)
    P2P_MSG_PONG,             // Pong (réponse ping)
    P2P_MSG_FIND_NODE,        // Recherche nœud
    P2P_MSG_FIND_NODE_REPLY,  // Réponse recherche
    P2P_MSG_STORE,            // Stockage DHT
    P2P_MSG_FIND_VALUE,       // Recherche valeur DHT
    P2P_MSG_GOSSIP,           // Message gossip
    P2P_MSG_TASK_REQUEST,     // Demande tâche minage
    P2P_MSG_TASK_RESULT,      // Résultat tâche
    P2P_MSG_BLOCK_FOUND,      // Bloc trouvé
    P2P_MSG_VOTE,             // Vote consensus
    P2P_MSG_SNAPSHOT          // Snapshot contributions
} p2p_message_type_t;

/**
 * @brief Information sur un pair P2P
 */
typedef struct {
    uint8_t node_id[32];      // ID unique (SHA-256)
    char ip_address[46];      // IPv4 ou IPv6
    uint16_t port;            // Port
    time_t last_seen;         // Dernier contact
    uint32_t latency_ms;      // Latence (ms)
    uint64_t reputation;      // Réputation (0-1000)
    bool is_bootstrap;        // Nœud bootstrap
} p2p_peer_info_t;

/**
 * @brief Message P2P
 */
typedef struct {
    p2p_message_type_t type;  // Type de message
    uint8_t sender_id[32];    // ID expéditeur
    uint64_t timestamp_ns;    // Timestamp nanoseconde
    uint32_t data_size;       // Taille données
    uint8_t* data;            // Données message
    uint8_t signature[64];    // Signature Ed25519
    uint32_t crc32;           // CRC32 intégrité
} p2p_message_t;

/**
 * @brief Configuration nœud P2P
 */
typedef struct {
    uint16_t listen_port;             // Port d'écoute
    uint32_t max_peers;               // Nombre max de pairs
    uint32_t target_peers;            // Nombre cible de pairs
    uint32_t ping_interval_ms;        // Intervalle ping (ms)
    uint32_t ping_timeout_ms;         // Timeout ping (ms)
    uint32_t gossip_fanout;           // Fanout gossip (nombre pairs)
    uint32_t gossip_interval_ms;      // Intervalle gossip (ms)
    uint32_t dht_replication_factor;  // Facteur réplication DHT
    uint32_t dht_bucket_size;         // Taille bucket Kademlia
    bool enable_nat_traversal;        // Activer NAT traversal
    bool enable_encryption;           // Activer chiffrement
} p2p_config_t;

/**
 * @brief Contexte nœud P2P
 */
typedef struct {
    // Configuration
    p2p_config_t config;
    uint8_t node_id[32];              // ID de ce nœud
    uint8_t private_key[32];          // Clé privée Ed25519
    uint8_t public_key[32];           // Clé publique Ed25519
    
    // État
    p2p_node_state_t state;           // État courant
    int listen_socket;                // Socket d'écoute
    
    // Pairs
    p2p_peer_info_t* peers;           // Liste des pairs
    uint32_t peer_count;              // Nombre de pairs
    pthread_mutex_t peers_lock;       // Lock pour pairs
    
    // DHT (Distributed Hash Table)
    void* dht_context;                // Contexte DHT (opaque)
    
    // Gossip
    void* gossip_context;             // Contexte Gossip (opaque)
    
    // Threads
    pthread_t listen_thread;          // Thread écoute
    pthread_t ping_thread;            // Thread ping
    pthread_t gossip_thread;          // Thread gossip
    bool running;                     // Threads actifs
    
    // Statistiques
    uint64_t total_messages_sent;     // Total messages envoyés
    uint64_t total_messages_received; // Total messages reçus
    uint64_t total_bytes_sent;        // Total octets envoyés
    uint64_t total_bytes_received;    // Total octets reçus
    uint32_t connection_count;        // Nombre de connexions
    time_t start_time;                // Heure démarrage
} p2p_node_context_t;

// ============================================================================
// API PUBLIQUE
// ============================================================================

/**
 * @brief Initialise un nœud P2P
 * @param config Configuration du nœud
 * @return Contexte du nœud ou NULL si erreur
 */
p2p_node_context_t* p2p_node_init(const p2p_config_t* config);

/**
 * @brief Libère le contexte du nœud
 * @param ctx Contexte à libérer
 */
void p2p_node_free(p2p_node_context_t* ctx);

/**
 * @brief Démarre le nœud P2P
 * @param ctx Contexte du nœud
 * @return true si succès, false sinon
 */
bool p2p_node_start(p2p_node_context_t* ctx);

/**
 * @brief Arrête le nœud P2P
 * @param ctx Contexte du nœud
 */
void p2p_node_stop(p2p_node_context_t* ctx);

/**
 * @brief Connecte à un pair bootstrap
 * @param ctx Contexte du nœud
 * @param ip_address Adresse IP du pair
 * @param port Port du pair
 * @return true si succès, false sinon
 */
bool p2p_node_connect_bootstrap(p2p_node_context_t* ctx,
                                 const char* ip_address,
                                 uint16_t port);

/**
 * @brief Ajoute un pair au réseau
 * @param ctx Contexte du nœud
 * @param peer_info Information sur le pair
 * @return true si succès, false sinon
 */
bool p2p_node_add_peer(p2p_node_context_t* ctx,
                       const p2p_peer_info_t* peer_info);

/**
 * @brief Retire un pair du réseau
 * @param ctx Contexte du nœud
 * @param node_id ID du pair à retirer
 * @return true si succès, false sinon
 */
bool p2p_node_remove_peer(p2p_node_context_t* ctx,
                          const uint8_t node_id[32]);

/**
 * @brief Envoie un message à un pair
 * @param ctx Contexte du nœud
 * @param peer_id ID du pair destinataire
 * @param message Message à envoyer
 * @return true si succès, false sinon
 */
bool p2p_node_send_message(p2p_node_context_t* ctx,
                           const uint8_t peer_id[32],
                           const p2p_message_t* message);

/**
 * @brief Broadcast un message à tous les pairs
 * @param ctx Contexte du nœud
 * @param message Message à broadcaster
 * @return Nombre de pairs ayant reçu le message
 */
uint32_t p2p_node_broadcast(p2p_node_context_t* ctx,
                            const p2p_message_t* message);

/**
 * @brief Recherche des nœuds proches d'un ID (Kademlia)
 * @param ctx Contexte du nœud
 * @param target_id ID cible
 * @param peers Tableau à remplir avec les pairs trouvés
 * @param max_peers Taille max du tableau
 * @return Nombre de pairs trouvés
 */
uint32_t p2p_node_find_nodes(p2p_node_context_t* ctx,
                             const uint8_t target_id[32],
                             p2p_peer_info_t* peers,
                             uint32_t max_peers);

/**
 * @brief Obtient la liste de tous les pairs connectés
 * @param ctx Contexte du nœud
 * @param peers Tableau à remplir
 * @param max_peers Taille max du tableau
 * @return Nombre de pairs retournés
 */
uint32_t p2p_node_list_peers(p2p_node_context_t* ctx,
                             p2p_peer_info_t* peers,
                             uint32_t max_peers);

/**
 * @brief Obtient l'état du nœud
 * @param ctx Contexte du nœud
 * @return État du nœud
 */
p2p_node_state_t p2p_node_get_state(const p2p_node_context_t* ctx);

/**
 * @brief Obtient l'ID du nœud
 * @param ctx Contexte du nœud
 * @param node_id Buffer pour stocker l'ID (32 bytes)
 */
void p2p_node_get_id(const p2p_node_context_t* ctx, uint8_t node_id[32]);

/**
 * @brief Obtient les statistiques du nœud
 * @param ctx Contexte du nœud
 * @param msgs_sent Pointeur pour messages envoyés
 * @param msgs_received Pointeur pour messages reçus
 * @param bytes_sent Pointeur pour octets envoyés
 * @param bytes_received Pointeur pour octets reçus
 */
void p2p_node_get_stats(const p2p_node_context_t* ctx,
                        uint64_t* msgs_sent,
                        uint64_t* msgs_received,
                        uint64_t* bytes_sent,
                        uint64_t* bytes_received);

/**
 * @brief Calcule la distance XOR entre deux IDs (Kademlia)
 * @param id1 Premier ID
 * @param id2 Deuxième ID
 * @param distance Buffer pour stocker la distance (32 bytes)
 */
void p2p_node_xor_distance(const uint8_t id1[32],
                           const uint8_t id2[32],
                           uint8_t distance[32]);

/**
 * @brief Génère un rapport P2P au format Markdown
 * @param ctx Contexte du nœud
 * @param output_path Chemin du fichier de sortie
 * @return true si succès, false sinon
 */
bool p2p_node_generate_report(const p2p_node_context_t* ctx,
                              const char* output_path);

#endif // LUM_P2P_NODE_H

// Made with Bob
