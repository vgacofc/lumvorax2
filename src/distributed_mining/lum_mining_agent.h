/**
 * @file lum_mining_agent.h
 * @brief LUM Mining Agent — Agent mineur local
 * @author LumVorax Team
 * @date 2026-04-30
 * @version 1.0.0
 * 
 * Agent local exécuté sur machine utilisateur :
 * - Détection automatique ressources (CPU/GPU/RAM)
 * - Réception tâches depuis orchestrateur
 * - Exécution calculs SHA-256
 * - Envoi résultats + preuve forensique LUM
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 */

#ifndef LUM_MINING_AGENT_H
#define LUM_MINING_AGENT_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * CONSTANTES
 * ======================================================================== */

#define AGENT_VERSION_MAJOR 1
#define AGENT_VERSION_MINOR 0
#define AGENT_VERSION_PATCH 0

#define AGENT_MAX_THREADS 256           // Max threads CPU
#define AGENT_MAX_GPU_DEVICES 8         // Max devices GPU
#define AGENT_WALLET_ADDR_LEN 64        // Longueur adresse wallet
#define AGENT_API_KEY_LEN 64            // Longueur clé API
#define AGENT_SERVER_ADDR_LEN 256       // Longueur adresse serveur

/* ========================================================================
 * TYPES ET STRUCTURES
 * ======================================================================== */

/**
 * @brief Type ressource
 */
typedef enum {
    RESOURCE_CPU = 0,
    RESOURCE_GPU = 1,
    RESOURCE_RAM = 2
} resource_type_t;

/**
 * @brief Informations CPU
 */
typedef struct {
    uint32_t cores;                     // Nombre cœurs physiques
    uint32_t threads;                   // Nombre threads logiques
    uint32_t frequency_mhz;             // Fréquence (MHz)
    char model[128];                    // Modèle CPU
    
    uint8_t avx2_support;               // 1 si AVX2 supporté
    uint8_t sse42_support;              // 1 si SSE4.2 supporté
    
    uint32_t hashrate_estimate;         // Hashrate estimé (H/s)
} agent_cpu_info_t;

/**
 * @brief Informations GPU
 */
typedef struct {
    uint32_t device_id;                 // ID device
    char name[128];                     // Nom GPU
    uint64_t memory_mb;                 // Mémoire (MB)
    uint32_t compute_units;             // Unités calcul
    
    uint8_t opencl_support;             // 1 si OpenCL supporté
    uint8_t cuda_support;               // 1 si CUDA supporté
    
    uint32_t hashrate_estimate;         // Hashrate estimé (H/s)
} agent_gpu_info_t;

/**
 * @brief Informations système
 */
typedef struct {
    agent_cpu_info_t cpu;               // Informations CPU
    agent_gpu_info_t gpus[AGENT_MAX_GPU_DEVICES];  // Informations GPUs
    uint32_t gpu_count;                 // Nombre GPUs détectés
    
    uint64_t ram_total_mb;              // RAM totale (MB)
    uint64_t ram_available_mb;          // RAM disponible (MB)
    
    char os_name[64];                   // Nom OS
    char os_version[32];                // Version OS
} agent_system_info_t;

/**
 * @brief Tâche minage
 */
typedef struct {
    uint64_t task_id;                   // ID unique tâche
    
    uint8_t block_header[80];           // Header bloc Bitcoin
    uint32_t target_bits;               // Target difficulté (compact)
    
    uint64_t nonce_start;               // Début plage nonces
    uint64_t nonce_end;                 // Fin plage nonces
    
    uint64_t assigned_at;               // Timestamp assignation
    uint64_t deadline;                  // Deadline (timestamp)
} mining_task_t;

/**
 * @brief Résultat minage
 */
typedef struct {
    uint64_t task_id;                   // ID tâche
    
    uint8_t found_solution;             // 1 si solution trouvée
    uint32_t nonce_solution;            // Nonce solution (si trouvé)
    uint8_t block_hash[32];             // Hash bloc (si trouvé)
    
    uint64_t hashes_computed;           // Nombre hashes calculés
    uint64_t elapsed_time_ms;           // Temps écoulé (ms)
    uint32_t hashrate;                  // Hashrate (H/s)
    
    /* Preuve forensique LUM */
    uint8_t sample_hashes[10][32];      // Échantillons SHA-256
    uint64_t sample_nonces[10];         // Nonces échantillons
    uint64_t sample_timestamps[10];     // Timestamps échantillons
    
    uint32_t crc32c;                    // CRC32C intégrité
    uint8_t signature[64];              // Signature Ed25519
} mining_result_t;

/**
 * @brief Configuration agent
 */
typedef struct {
    char wallet_address[AGENT_WALLET_ADDR_LEN];    // Adresse wallet BTC
    char api_key[AGENT_API_KEY_LEN];               // Clé API
    char server_address[AGENT_SERVER_ADDR_LEN];    // Adresse serveur
    uint16_t server_port;                          // Port serveur
    
    uint8_t use_cpu;                    // 1 si utiliser CPU
    uint8_t use_gpu;                    // 1 si utiliser GPU
    uint32_t cpu_threads;               // Nombre threads CPU (0 = auto)
    uint32_t gpu_device_id;             // ID GPU à utiliser (0 = premier)
    
    uint32_t report_interval_sec;       // Intervalle rapports (s)
    uint8_t auto_reconnect;             // 1 si reconnexion auto
    uint32_t reconnect_delay_sec;       // Délai reconnexion (s)
} agent_config_t;

/**
 * @brief État agent
 */
typedef enum {
    AGENT_IDLE = 0,                     // Inactif
    AGENT_CONNECTING = 1,               // Connexion en cours
    AGENT_CONNECTED = 2,                // Connecté
    AGENT_MINING = 3,                   // Minage en cours
    AGENT_DISCONNECTED = 4,             // Déconnecté
    AGENT_ERROR = 5                     // Erreur
} agent_state_t;

/**
 * @brief Contexte agent
 */
typedef struct {
    agent_config_t config;              // Configuration
    agent_system_info_t system_info;    // Informations système
    
    agent_state_t state;                // État actuel
    mining_task_t current_task;         // Tâche en cours
    
    uint64_t total_hashes;              // Total hashes calculés
    uint64_t total_tasks;               // Total tâches traitées
    uint32_t current_hashrate;          // Hashrate actuel (H/s)
    
    int socket_fd;                      // Socket serveur
    pthread_t mining_thread;            // Thread minage
    pthread_t report_thread;            // Thread rapports
    
    uint8_t running;                    // 1 si agent actif
    pthread_mutex_t mutex;              // Mutex thread-safe
} agent_context_t;

/**
 * @brief Statistiques agent
 */
typedef struct {
    agent_state_t state;                // État actuel
    uint64_t uptime_sec;                // Temps activité (s)
    
    uint64_t total_hashes;              // Total hashes calculés
    uint64_t total_tasks;               // Total tâches traitées
    uint32_t current_hashrate;          // Hashrate actuel (H/s)
    uint32_t avg_hashrate;              // Hashrate moyen (H/s)
    
    uint32_t solutions_found;           // Solutions trouvées
    double earnings_btc;                // Gains BTC
} agent_stats_t;

/* ========================================================================
 * API PUBLIQUE
 * ======================================================================== */

/**
 * @brief Initialise l'agent mineur
 * @param config Configuration (NULL = config par défaut)
 * @return 0 si succès, -1 si erreur
 */
int agent_init(const agent_config_t* config);

/**
 * @brief Détecte les ressources système
 * @param info Structure informations système (sortie)
 * @return 0 si succès, -1 si erreur
 */
int agent_detect_resources(agent_system_info_t* info);

/**
 * @brief Démarre l'agent (connexion + minage)
 * @return 0 si succès, -1 si erreur
 */
int agent_start(void);

/**
 * @brief Arrête l'agent
 * @return 0 si succès, -1 si erreur
 */
int agent_stop(void);

/**
 * @brief Connecte au serveur orchestrateur
 * @return 0 si succès, -1 si erreur
 */
int agent_connect(void);

/**
 * @brief Déconnecte du serveur
 * @return 0 si succès, -1 si erreur
 */
int agent_disconnect(void);

/**
 * @brief Reçoit une tâche depuis le serveur
 * @param task Structure tâche (sortie)
 * @return 0 si succès, -1 si erreur
 */
int agent_receive_task(mining_task_t* task);

/**
 * @brief Exécute une tâche minage
 * @param task Tâche à exécuter
 * @param result Structure résultat (sortie)
 * @return 0 si succès, -1 si erreur
 */
int agent_execute_task(const mining_task_t* task, mining_result_t* result);

/**
 * @brief Envoie un résultat au serveur
 * @param result Résultat à envoyer
 * @return 0 si succès, -1 si erreur
 */
int agent_send_result(const mining_result_t* result);

/**
 * @brief Obtient les statistiques
 * @param stats Structure statistiques (sortie)
 * @return 0 si succès, -1 si erreur
 */
int agent_get_stats(agent_stats_t* stats);

/**
 * @brief Nettoie et libère les ressources
 * @return 0 si succès, -1 si erreur
 */
int agent_cleanup(void);

/* ========================================================================
 * API MINAGE
 * ======================================================================== */

/**
 * @brief Mine avec CPU
 * @param task Tâche minage
 * @param result Structure résultat (sortie)
 * @return 0 si succès, -1 si erreur
 */
int agent_mine_cpu(const mining_task_t* task, mining_result_t* result);

/**
 * @brief Mine avec GPU (OpenCL)
 * @param task Tâche minage
 * @param result Structure résultat (sortie)
 * @return 0 si succès, -1 si erreur
 */
int agent_mine_gpu(const mining_task_t* task, mining_result_t* result);

#ifdef __cplusplus
}
#endif

#endif /* LUM_MINING_AGENT_H */

// Made with Bob
