/**
 * @file contribution_tracker.h
 * @brief Contribution Tracker — Suivi contributions mineurs
 * @author LumVorax Team
 * @date 2026-04-30
 * @version 1.0.0
 * 
 * Système de tracking des contributions pour distribution équitable :
 * - Enregistrement temps réel des hashes calculés
 * - Calcul proportionnel des parts
 * - Preuve forensique LUM obligatoire
 * - Anti-fraude (vérification échantillons)
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 */

#ifndef CONTRIBUTION_TRACKER_H
#define CONTRIBUTION_TRACKER_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * CONSTANTES
 * ======================================================================== */

#define CONTRIB_TRACKER_VERSION_MAJOR 1
#define CONTRIB_TRACKER_VERSION_MINOR 0
#define CONTRIB_TRACKER_VERSION_PATCH 0

#define CONTRIB_MAX_MINERS 10000        // Max 10K mineurs simultanés
#define CONTRIB_MINER_ID_LEN 32         // Longueur ID mineur (SHA-256)
#define CONTRIB_WALLET_ADDR_LEN 64      // Longueur adresse wallet
#define CONTRIB_SIGNATURE_LEN 64        // Longueur signature Ed25519
#define CONTRIB_SAMPLE_HASHES 10        // Nombre échantillons preuve

/* ========================================================================
 * TYPES ET STRUCTURES
 * ======================================================================== */

/**
 * @brief Rapport contribution mineur
 */
typedef struct {
    uint8_t miner_id[CONTRIB_MINER_ID_LEN];     // ID unique mineur
    uint64_t nonce_start;                       // Début plage testée
    uint64_t nonce_end;                         // Fin plage testée
    uint64_t hashes_computed;                   // Nombre hashes calculés
    uint64_t timestamp;                         // Horodatage (ns)
    
    /* Preuve forensique LUM */
    uint8_t sample_hashes[CONTRIB_SAMPLE_HASHES][32];  // Échantillons SHA-256
    uint64_t sample_nonces[CONTRIB_SAMPLE_HASHES];     // Nonces échantillons
    uint64_t sample_timestamps[CONTRIB_SAMPLE_HASHES]; // Timestamps échantillons
    
    /* Intégrité */
    uint32_t crc32c;                            // CRC32C intégrité
    uint8_t signature[CONTRIB_SIGNATURE_LEN];   // Signature Ed25519
} contribution_report_t;

/**
 * @brief Contribution mineur agrégée
 */
typedef struct {
    uint8_t miner_id[CONTRIB_MINER_ID_LEN];     // ID unique mineur
    char wallet_address[CONTRIB_WALLET_ADDR_LEN]; // Adresse wallet BTC
    
    uint64_t total_hashes;                      // Total hashes calculés
    uint64_t valid_reports;                     // Nombre rapports validés
    uint64_t invalid_reports;                   // Nombre rapports rejetés
    
    double contribution_percent;                // % du travail total
    double reward_btc;                          // Récompense BTC
    
    uint8_t reputation;                         // Réputation (0-100)
    uint64_t first_seen;                        // Premier rapport (timestamp)
    uint64_t last_seen;                         // Dernier rapport (timestamp)
} miner_contribution_t;

/**
 * @brief Pool contributions (snapshot)
 */
typedef struct {
    miner_contribution_t miners[CONTRIB_MAX_MINERS];  // Mineurs actifs
    uint32_t active_miners;                           // Nombre mineurs actifs
    
    uint64_t total_hashes_network;              // Total hashes réseau
    uint64_t block_height;                      // Hauteur bloc en cours
    uint8_t block_found;                        // 1 si bloc trouvé
    
    uint64_t snapshot_timestamp;                // Timestamp snapshot
    uint8_t snapshot_hash[32];                  // Hash snapshot (Merkle root)
    
    pthread_mutex_t mutex;                      // Mutex thread-safe
} contribution_pool_t;

/**
 * @brief Configuration tracker
 */
typedef struct {
    uint8_t anti_fraud_enabled;                 // 1 si anti-fraude activé
    double sampling_rate;                       // Taux échantillonnage (0.0-1.0)
    uint32_t max_hashrate_cpu;                  // Hashrate max CPU (H/s)
    uint32_t max_hashrate_gpu;                  // Hashrate max GPU (H/s)
    
    uint8_t reputation_enabled;                 // 1 si système réputation activé
    uint32_t min_reputation;                    // Réputation min pour paiement
    
    double platform_fee_percent;                // Frais plateforme (%)
} contrib_config_t;

/**
 * @brief Statistiques tracker
 */
typedef struct {
    uint32_t active_miners;                     // Mineurs actifs
    uint64_t total_reports;                     // Total rapports reçus
    uint64_t valid_reports;                     // Rapports valides
    uint64_t invalid_reports;                   // Rapports invalides
    
    uint64_t total_hashes_network;              // Total hashes réseau
    uint32_t network_hashrate;                  // Hashrate réseau (H/s)
    
    uint32_t fraud_attempts;                    // Tentatives fraude détectées
    double fraud_rate;                          // Taux fraude (%)
} contrib_stats_t;

/* ========================================================================
 * API PUBLIQUE
 * ======================================================================== */

/**
 * @brief Initialise le tracker contributions
 * @param config Configuration (NULL = config par défaut)
 * @return 0 si succès, -1 si erreur
 */
int contrib_tracker_init(const contrib_config_t* config);

/**
 * @brief Enregistre un rapport contribution
 * @param report Rapport contribution
 * @return 0 si succès, -1 si erreur (fraude détectée)
 */
int contrib_tracker_record(const contribution_report_t* report);

/**
 * @brief Vérifie la validité d'un rapport (anti-fraude)
 * @param report Rapport à vérifier
 * @return 1 si valide, 0 si invalide (fraude)
 */
int contrib_tracker_verify(const contribution_report_t* report);

/**
 * @brief Calcule les contributions proportionnelles
 * @return 0 si succès, -1 si erreur
 */
int contrib_tracker_calculate_shares(void);

/**
 * @brief Crée un snapshot des contributions (bloc trouvé)
 * @param block_height Hauteur bloc trouvé
 * @return 0 si succès, -1 si erreur
 */
int contrib_tracker_snapshot(uint64_t block_height);

/**
 * @brief Obtient la contribution d'un mineur
 * @param miner_id ID mineur
 * @param contrib Structure contribution (sortie)
 * @return 0 si succès, -1 si erreur
 */
int contrib_tracker_get_miner(const uint8_t* miner_id, miner_contribution_t* contrib);

/**
 * @brief Obtient les statistiques
 * @param stats Structure statistiques (sortie)
 * @return 0 si succès, -1 si erreur
 */
int contrib_tracker_get_stats(contrib_stats_t* stats);

/**
 * @brief Réinitialise le pool (nouveau bloc)
 * @return 0 si succès, -1 si erreur
 */
int contrib_tracker_reset(void);

/**
 * @brief Nettoie et libère les ressources
 * @return 0 si succès, -1 si erreur
 */
int contrib_tracker_cleanup(void);

/* ========================================================================
 * API ANTI-FRAUDE
 * ======================================================================== */

/**
 * @brief Vérifie les échantillons de hashes
 * @param report Rapport à vérifier
 * @return 1 si valide, 0 si invalide
 */
int contrib_verify_samples(const contribution_report_t* report);

/**
 * @brief Vérifie le hashrate (réaliste ?)
 * @param report Rapport à vérifier
 * @return 1 si valide, 0 si invalide
 */
int contrib_verify_hashrate(const contribution_report_t* report);

/**
 * @brief Vérifie la signature Ed25519
 * @param report Rapport à vérifier
 * @return 1 si valide, 0 si invalide
 */
int contrib_verify_signature(const contribution_report_t* report);

/**
 * @brief Met à jour la réputation d'un mineur
 * @param miner_id ID mineur
 * @param valid 1 si rapport valide, 0 si invalide
 * @return 0 si succès, -1 si erreur
 */
int contrib_update_reputation(const uint8_t* miner_id, uint8_t valid);

#ifdef __cplusplus
}
#endif

#endif /* CONTRIBUTION_TRACKER_H */

// Made with Bob
