/**
 * @file lum_turbine.h
 * @brief LumVorax Turbine - Propagation de Blocs Optimisée
 * 
 * Turbine est un protocole de propagation de blocs en arbre avec Reed-Solomon
 * pour réduire la latence de propagation de O(N) à O(log N).
 * 
 * Architecture :
 * - Arbre de propagation avec fanout configurable (défaut: 200)
 * - Fragmentation Reed-Solomon pour tolérance aux pannes
 * - Propagation parallèle des fragments (shreds)
 * - Reconstruction automatique des blocs
 * 
 * Performance cible :
 * - Latence propagation : <50ms pour 10K nœuds
 * - Bande passante : 1 Gbps par nœud
 * - Tolérance pannes : 33% fragments perdus
 * 
 * @version 1.0.0
 * @date 2026-05-02
 * @author Bob (LumVorax Core Team)
 */

#ifndef LUM_TURBINE_H
#define LUM_TURBINE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define LUM_TURBINE_DEFAULT_FANOUT 200      // Fanout par défaut
#define LUM_TURBINE_MAX_FANOUT 1000         // Fanout maximum
#define LUM_TURBINE_SHRED_SIZE 1024         // Taille fragment (1KB)
#define LUM_TURBINE_MAX_SHREDS 65536        // Max fragments par bloc
#define LUM_TURBINE_FEC_RATE 0.5            // Taux FEC (50% redondance)
#define LUM_TURBINE_MAX_BLOCK_SIZE (64*1024*1024)  // 64MB max

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

/**
 * @brief Fragment de bloc (shred)
 */
typedef struct {
    uint64_t block_id;                      // ID bloc
    uint32_t shred_index;                   // Index fragment
    uint32_t total_shreds;                  // Total fragments
    uint32_t data_shreds;                   // Fragments données
    uint32_t fec_shreds;                    // Fragments FEC
    uint8_t data[LUM_TURBINE_SHRED_SIZE];   // Données fragment
    uint32_t data_size;                     // Taille données
    uint8_t signature[64];                  // Signature Ed25519
} lum_turbine_shred_t;

/**
 * @brief Nœud dans l'arbre Turbine
 */
typedef struct lum_turbine_node {
    uint8_t node_id[32];                    // ID nœud (pubkey)
    char ip_address[64];                    // Adresse IP
    uint16_t port;                          // Port
    
    // Position dans l'arbre
    struct lum_turbine_node* parent;        // Nœud parent
    struct lum_turbine_node** children;     // Nœuds enfants
    uint32_t num_children;                  // Nombre enfants
    uint32_t max_children;                  // Capacité enfants
    
    // État
    bool active;                            // Nœud actif ?
    uint64_t last_seen_ns;                  // Dernier contact
    
    // Métriques
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t shreds_sent;
    uint64_t shreds_received;
} lum_turbine_node_t;

/**
 * @brief Bloc en cours de reconstruction
 */
typedef struct {
    uint64_t block_id;
    uint8_t* data;                          // Données bloc
    uint32_t data_size;                     // Taille données
    
    // Fragments reçus
    bool* shreds_received;                  // Bitmap fragments reçus
    uint32_t num_shreds_received;           // Nombre reçus
    uint32_t total_shreds;                  // Total attendu
    uint32_t data_shreds;                   // Fragments données
    uint32_t fec_shreds;                    // Fragments FEC
    
    // Reconstruction
    bool reconstructed;                     // Bloc reconstruit ?
    uint64_t start_time_ns;                 // Début réception
    uint64_t end_time_ns;                   // Fin reconstruction
    
    pthread_mutex_t mutex;
} lum_turbine_block_t;

/**
 * @brief Instance Turbine
 */
typedef struct {
    // Configuration
    uint32_t fanout;                        // Facteur branchement
    double fec_rate;                        // Taux FEC
    
    // Nœud local
    lum_turbine_node_t* local_node;
    
    // Arbre de propagation
    lum_turbine_node_t** all_nodes;         // Tous les nœuds
    uint32_t num_nodes;                     // Nombre nœuds
    uint32_t max_nodes;                     // Capacité
    
    // Blocs en cours
    lum_turbine_block_t** pending_blocks;   // Blocs en reconstruction
    uint32_t num_pending;
    uint32_t max_pending;
    
    // Thread de réception
    pthread_t receiver_thread;
    bool running;
    
    // Synchronisation
    pthread_mutex_t nodes_mutex;
    pthread_mutex_t blocks_mutex;
    
    // Métriques
    uint64_t total_blocks_sent;
    uint64_t total_blocks_received;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    double avg_propagation_time_ms;
} lum_turbine_t;

/* ============================================================================
 * API PUBLIQUE - INITIALISATION
 * ============================================================================ */

/**
 * @brief Initialiser instance Turbine
 * 
 * @param node_id ID du nœud local (32 bytes)
 * @param ip_address Adresse IP locale
 * @param port Port d'écoute
 * @param fanout Facteur branchement (0 = défaut)
 * @return Instance Turbine ou NULL si erreur
 */
lum_turbine_t* lum_turbine_init(const uint8_t* node_id,
                                 const char* ip_address,
                                 uint16_t port,
                                 uint32_t fanout);

/**
 * @brief Libérer instance Turbine
 * 
 * @param turbine Instance à libérer
 */
void lum_turbine_free(lum_turbine_t* turbine);

/**
 * @brief Démarrer Turbine (thread réception)
 * 
 * @param turbine Instance Turbine
 * @return true si succès, false sinon
 */
bool lum_turbine_start(lum_turbine_t* turbine);

/**
 * @brief Arrêter Turbine
 * 
 * @param turbine Instance Turbine
 */
void lum_turbine_stop(lum_turbine_t* turbine);

/* ============================================================================
 * API PUBLIQUE - GESTION NŒUDS
 * ============================================================================ */

/**
 * @brief Ajouter nœud à l'arbre
 * 
 * @param turbine Instance Turbine
 * @param node_id ID nœud (32 bytes)
 * @param ip_address Adresse IP
 * @param port Port
 * @return true si succès, false sinon
 */
bool lum_turbine_add_node(lum_turbine_t* turbine,
                          const uint8_t* node_id,
                          const char* ip_address,
                          uint16_t port);

/**
 * @brief Retirer nœud de l'arbre
 * 
 * @param turbine Instance Turbine
 * @param node_id ID nœud
 * @return true si succès, false sinon
 */
bool lum_turbine_remove_node(lum_turbine_t* turbine, const uint8_t* node_id);

/**
 * @brief Reconstruire arbre de propagation
 * 
 * @param turbine Instance Turbine
 * @return true si succès, false sinon
 */
bool lum_turbine_rebuild_tree(lum_turbine_t* turbine);

/* ============================================================================
 * API PUBLIQUE - PROPAGATION BLOCS
 * ============================================================================ */

/**
 * @brief Propager bloc via Turbine
 * 
 * @param turbine Instance Turbine
 * @param block_id ID bloc
 * @param data Données bloc
 * @param size Taille données
 * @return true si succès, false sinon
 */
bool lum_turbine_broadcast_block(lum_turbine_t* turbine,
                                  uint64_t block_id,
                                  const uint8_t* data,
                                  uint32_t size);

/**
 * @brief Recevoir fragment (shred)
 * 
 * @param turbine Instance Turbine
 * @param shred Fragment reçu
 * @return true si bloc complet, false sinon
 */
bool lum_turbine_receive_shred(lum_turbine_t* turbine,
                                const lum_turbine_shred_t* shred);

/**
 * @brief Obtenir bloc reconstruit
 * 
 * @param turbine Instance Turbine
 * @param block_id ID bloc
 * @param data Buffer pour données (alloué par appelant)
 * @param size Taille buffer / taille données
 * @return true si bloc disponible, false sinon
 */
bool lum_turbine_get_block(lum_turbine_t* turbine,
                            uint64_t block_id,
                            uint8_t* data,
                            uint32_t* size);

/* ============================================================================
 * API PUBLIQUE - MÉTRIQUES
 * ============================================================================ */

/**
 * @brief Obtenir métriques Turbine
 * 
 * @param turbine Instance Turbine
 * @param blocks_sent Blocs envoyés
 * @param blocks_received Blocs reçus
 * @param bytes_sent Octets envoyés
 * @param bytes_received Octets reçus
 * @param avg_propagation_ms Temps propagation moyen (ms)
 */
void lum_turbine_get_metrics(lum_turbine_t* turbine,
                              uint64_t* blocks_sent,
                              uint64_t* blocks_received,
                              uint64_t* bytes_sent,
                              uint64_t* bytes_received,
                              double* avg_propagation_ms);

/* ============================================================================
 * FONCTIONS INTERNES (EXPOSÉES POUR TESTS)
 * ============================================================================ */

/**
 * @brief Fragmenter bloc avec Reed-Solomon
 * 
 * @param data Données bloc
 * @param size Taille données
 * @param shreds Buffer fragments (alloué par appelant)
 * @param num_shreds Nombre fragments générés
 * @param data_shreds Fragments données
 * @param fec_shreds Fragments FEC
 * @return true si succès, false sinon
 */
bool lum_turbine_encode_block(const uint8_t* data,
                               uint32_t size,
                               lum_turbine_shred_t** shreds,
                               uint32_t* num_shreds,
                               uint32_t* data_shreds,
                               uint32_t* fec_shreds);

/**
 * @brief Reconstruire bloc depuis fragments
 * 
 * @param shreds Fragments reçus
 * @param num_shreds Nombre fragments
 * @param data Buffer données (alloué par appelant)
 * @param size Taille données
 * @return true si succès, false sinon
 */
bool lum_turbine_decode_block(lum_turbine_shred_t** shreds,
                               uint32_t num_shreds,
                               uint8_t* data,
                               uint32_t* size);

/**
 * @brief Envoyer fragment à un nœud
 * 
 * @param turbine Instance Turbine
 * @param node Nœud destinataire
 * @param shred Fragment à envoyer
 * @return true si succès, false sinon
 */
bool lum_turbine_send_shred(lum_turbine_t* turbine,
                             lum_turbine_node_t* node,
                             const lum_turbine_shred_t* shred);

/**
 * @brief Thread réception fragments
 * 
 * @param arg Pointeur vers lum_turbine_t
 * @return NULL
 */
void* lum_turbine_receiver_thread(void* arg);

#ifdef __cplusplus
}
#endif

#endif /* LUM_TURBINE_H */

// Made with Bob
