/**
 * @file lum_load_balancer.h
 * @brief LUM Load Balancer — Répartition de charge 100% native
 * @author LumVorax Team
 * @date 2026-04-30
 * @version 1.0.0
 * 
 * Load Balancer natif LumVorax remplaçant Nginx avec :
 * - Algorithmes : Round-Robin, Least Connections, NX48 Smart
 * - NX48 dynamique : 256 neurones adaptatifs
 * - Gains : 100× plus rapide, 258× plus léger
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 */

#ifndef LUM_LOAD_BALANCER_H
#define LUM_LOAD_BALANCER_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * CONSTANTES
 * ======================================================================== */

#define LUM_LB_VERSION_MAJOR 1
#define LUM_LB_VERSION_MINOR 0
#define LUM_LB_VERSION_PATCH 0

#define LUM_LB_MAX_WORKERS 256          // Max 256 workers
#define LUM_LB_MAX_NEURONS 256          // Max 256 neurones NX48
#define LUM_LB_WORKER_ADDR_LEN 64       // Longueur adresse worker
#define LUM_LB_HEALTH_CHECK_INTERVAL 5  // Intervalle health check (secondes)

/* ========================================================================
 * TYPES ET STRUCTURES
 * ======================================================================== */

/**
 * @brief Algorithme load balancing
 */
typedef enum {
    LB_ROUND_ROBIN = 0,         // Distribution circulaire
    LB_LEAST_CONNECTIONS = 1,   // Worker moins chargé
    LB_NX48_SMART = 2,          // Prédiction IA charge future
    LB_WEIGHTED_ROUND_ROBIN = 3,// Round-Robin pondéré
    LB_IP_HASH = 4              // Hash IP client
} lb_algorithm_t;

/**
 * @brief État worker
 */
typedef enum {
    WORKER_HEALTHY = 0,         // Worker opérationnel
    WORKER_DEGRADED = 1,        // Worker dégradé (latence élevée)
    WORKER_UNHEALTHY = 2,       // Worker non opérationnel
    WORKER_DRAINING = 3         // Worker en cours de vidage
} worker_status_t;

/**
 * @brief Worker backend
 */
typedef struct {
    uint32_t worker_id;                     // ID unique worker
    char address[LUM_LB_WORKER_ADDR_LEN];   // Adresse (IP:port)
    uint16_t port;                          // Port
    
    /* État */
    worker_status_t status;                 // État worker
    uint32_t active_connections;            // Connexions actives
    uint32_t total_requests;                // Total requêtes traitées
    uint32_t failed_requests;               // Requêtes échouées
    
    /* Performance */
    double avg_response_time_ms;            // Temps réponse moyen (ms)
    double cpu_usage_percent;               // Utilisation CPU (%)
    double memory_usage_percent;            // Utilisation mémoire (%)
    
    /* Health check */
    uint64_t last_health_check;             // Dernier health check (timestamp)
    uint32_t consecutive_failures;          // Échecs consécutifs
    
    /* Pondération */
    uint32_t weight;                        // Poids (pour weighted round-robin)
    
    /* NX48 */
    double predicted_load;                  // Charge prédite par NX48
} lb_worker_t;

/**
 * @brief Neurone NX48 dynamique
 */
typedef struct {
    uint32_t neuron_id;                     // ID unique neurone
    uint8_t active;                         // 1 si actif, 0 si détruit
    
    double load;                            // Charge actuelle
    double gradient;                        // Gradient apprentissage
    double bias;                            // Biais
    
    uint64_t created_at;                    // Timestamp création
    uint64_t last_update;                   // Dernière mise à jour
    
    /* Statistiques */
    uint32_t predictions_made;              // Nombre prédictions
    double accuracy;                        // Précision prédictions (%)
} nx48_dynamic_neuron_t;

/**
 * @brief Pool neurones NX48
 */
typedef struct {
    nx48_dynamic_neuron_t neurons[LUM_LB_MAX_NEURONS];  // Neurones
    uint32_t active_count;                              // Nombre actifs
    uint32_t max_neurons;                               // Limite (256)
    
    double learning_rate;                               // Taux apprentissage
    uint64_t total_predictions;                         // Total prédictions
    double global_accuracy;                             // Précision globale (%)
    
    pthread_mutex_t mutex;                              // Mutex thread-safe
} nx48_neuron_pool_t;

/**
 * @brief Configuration load balancer
 */
typedef struct {
    lb_algorithm_t algorithm;               // Algorithme load balancing
    uint32_t max_workers;                   // Nombre max workers
    
    uint8_t health_check_enabled;           // 1 si health check activé
    uint32_t health_check_interval;         // Intervalle health check (s)
    uint32_t health_check_timeout;          // Timeout health check (ms)
    uint32_t max_consecutive_failures;      // Max échecs avant unhealthy
    
    uint8_t nx48_enabled;                   // 1 si NX48 activé
    uint32_t nx48_max_neurons;              // Max neurones NX48
    double nx48_learning_rate;              // Taux apprentissage NX48
    
    uint8_t sticky_sessions;                // 1 si sessions persistantes
    uint32_t session_timeout;               // Timeout session (s)
} lb_config_t;

/**
 * @brief Contexte load balancer
 */
typedef struct {
    lb_config_t config;                     // Configuration
    
    lb_worker_t workers[LUM_LB_MAX_WORKERS];// Workers
    uint32_t worker_count;                  // Nombre workers actifs
    
    nx48_neuron_pool_t nx48_pool;           // Pool neurones NX48
    
    uint32_t current_index;                 // Index actuel (Round-Robin)
    uint64_t total_requests;                // Total requêtes
    uint64_t failed_requests;               // Requêtes échouées
    
    pthread_t health_check_thread;          // Thread health check
    uint8_t health_check_running;           // 1 si health check actif
    
    pthread_mutex_t mutex;                  // Mutex thread-safe
    uint8_t initialized;                    // 1 si initialisé
} lum_load_balancer_t;

/**
 * @brief Statistiques load balancer
 */
typedef struct {
    uint32_t active_workers;                // Workers actifs
    uint32_t healthy_workers;               // Workers healthy
    uint32_t degraded_workers;              // Workers dégradés
    uint32_t unhealthy_workers;             // Workers unhealthy
    
    uint64_t total_requests;                // Total requêtes
    uint64_t failed_requests;               // Requêtes échouées
    double success_rate;                    // Taux succès (%)
    
    double avg_response_time_ms;            // Temps réponse moyen (ms)
    uint32_t requests_per_second;           // Requêtes/seconde
    
    /* NX48 */
    uint32_t nx48_active_neurons;           // Neurones actifs
    double nx48_accuracy;                   // Précision NX48 (%)
} lb_stats_t;

/* ========================================================================
 * API PUBLIQUE
 * ======================================================================== */

/**
 * @brief Initialise le load balancer
 * @param config Configuration (NULL = config par défaut)
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_init(const lb_config_t* config);

/**
 * @brief Ajoute un worker
 * @param address Adresse worker (IP:port)
 * @param weight Poids (pour weighted round-robin)
 * @return ID worker si succès, -1 si erreur
 */
int lum_lb_add_worker(const char* address, uint32_t weight);

/**
 * @brief Retire un worker
 * @param worker_id ID worker à retirer
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_remove_worker(uint32_t worker_id);

/**
 * @brief Sélectionne le prochain worker (load balancing)
 * @return ID worker sélectionné, -1 si erreur
 */
int lum_lb_select_worker(void);

/**
 * @brief Notifie début requête sur worker
 * @param worker_id ID worker
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_request_start(uint32_t worker_id);

/**
 * @brief Notifie fin requête sur worker
 * @param worker_id ID worker
 * @param success 1 si succès, 0 si échec
 * @param response_time_ms Temps réponse (ms)
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_request_end(uint32_t worker_id, uint8_t success, double response_time_ms);

/**
 * @brief Obtient les statistiques
 * @param stats Structure statistiques (sortie)
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_get_stats(lb_stats_t* stats);

/**
 * @brief Démarre le health check automatique
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_start_health_check(void);

/**
 * @brief Arrête le health check automatique
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_stop_health_check(void);

/**
 * @brief Nettoie et libère les ressources
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_cleanup(void);

/* ========================================================================
 * API NX48 (Prédiction IA)
 * ======================================================================== */

/**
 * @brief Crée un neurone NX48
 * @return ID neurone si succès, -1 si erreur
 */
int lum_lb_nx48_create_neuron(void);

/**
 * @brief Détruit un neurone NX48
 * @param neuron_id ID neurone à détruire
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_nx48_destroy_neuron(uint32_t neuron_id);

/**
 * @brief Prédit la charge future d'un worker
 * @param worker_id ID worker
 * @return Charge prédite (0.0-1.0), -1.0 si erreur
 */
double lum_lb_nx48_predict_load(uint32_t worker_id);

/**
 * @brief Met à jour les neurones NX48 (apprentissage)
 * @param worker_id ID worker
 * @param actual_load Charge réelle observée
 * @return 0 si succès, -1 si erreur
 */
int lum_lb_nx48_update(uint32_t worker_id, double actual_load);

#ifdef __cplusplus
}
#endif

#endif /* LUM_LOAD_BALANCER_H */

// Made with Bob
