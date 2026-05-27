/**
 * @file lum_tower_bft.h
 * @brief LumVorax Tower BFT - Consensus Optimisé pour Proof of History
 * 
 * Tower BFT est un algorithme de consensus Byzantine Fault Tolerant optimisé
 * pour fonctionner avec PoH. Il utilise des lockouts exponentiels pour
 * garantir la finalité sans nécessiter de communication synchrone.
 * 
 * Caractéristiques :
 * - Lockouts exponentiels (2^n slots)
 * - Finalité en ~13 secondes (32 confirmations)
 * - Tolérance Byzantine (33% nœuds malveillants)
 * - Optimisé pour latence <50ms
 * 
 * @version 1.0.0
 * @date 2026-05-02
 * @author Bob (LumVorax Core Team)
 */

#ifndef LUM_TOWER_BFT_H
#define LUM_TOWER_BFT_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "lum_poh.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define LUM_TOWER_MAX_VOTES 32              // Max votes dans tour
#define LUM_TOWER_INITIAL_LOCKOUT 2         // Lockout initial (2 slots)
#define LUM_TOWER_FINALITY_DEPTH 32         // Profondeur finalité
#define LUM_TOWER_VOTE_THRESHOLD 0.67       // 67% votes requis
#define LUM_TOWER_MAX_VALIDATORS 10000      // Max validateurs

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

/**
 * @brief Vote Tower BFT
 */
typedef struct {
    uint64_t slot;                          // Slot PoH voté
    uint8_t vote_hash[32];                  // Hash du vote
    uint64_t lockout;                       // Durée lockout (slots)
    uint64_t timestamp_ns;                  // Timestamp vote
    bool confirmed;                         // Vote confirmé ?
} lum_tower_vote_t;

/**
 * @brief Tour de votes (stack de votes avec lockouts)
 */
typedef struct {
    lum_tower_vote_t votes[LUM_TOWER_MAX_VOTES];  // Stack votes
    uint32_t num_votes;                            // Nombre votes actifs
    uint64_t root_slot;                            // Slot racine (finalisé)
    uint64_t last_vote_slot;                       // Dernier slot voté
    pthread_mutex_t mutex;                         // Protection thread-safe
} lum_tower_t;

/**
 * @brief Validateur Tower BFT
 */
typedef struct {
    uint8_t pubkey[32];                     // Clé publique validateur
    uint64_t stake;                         // Stake (poids vote)
    lum_tower_t* tower;                     // Tour de votes
    bool active;                            // Validateur actif ?
} lum_tower_validator_t;

/**
 * @brief État consensus Tower BFT
 */
typedef struct {
    // Validateurs
    lum_tower_validator_t** validators;
    uint32_t num_validators;
    uint64_t total_stake;
    
    // PoH
    lum_poh_t* poh;
    
    // État consensus
    uint64_t current_slot;
    uint64_t finalized_slot;
    uint8_t finalized_hash[32];
    
    // Votes par slot
    struct {
        uint64_t slot;
        uint64_t vote_stake;                // Stake total ayant voté
        uint8_t hash[32];
    } slot_votes[1024];                     // Buffer circulaire
    uint32_t slot_votes_head;
    
    // Synchronisation
    pthread_mutex_t mutex;
    pthread_rwlock_t validators_lock;
    
    // Métriques
    uint64_t total_votes;
    uint64_t total_confirmations;
    double avg_confirmation_time_ms;
} lum_tower_consensus_t;

/* ============================================================================
 * API PUBLIQUE - TOWER (VALIDATEUR)
 * ============================================================================ */

/**
 * @brief Initialiser tour de votes
 * 
 * @return Tour initialisée ou NULL si erreur
 */
lum_tower_t* lum_tower_init(void);

/**
 * @brief Libérer tour de votes
 * 
 * @param tower Tour à libérer
 */
void lum_tower_free(lum_tower_t* tower);

/**
 * @brief Vérifier si peut voter pour un slot
 * 
 * @param tower Tour de votes
 * @param slot Slot à vérifier
 * @return true si peut voter, false si lockout actif
 */
bool lum_tower_can_vote(lum_tower_t* tower, uint64_t slot);

/**
 * @brief Ajouter vote à la tour
 * 
 * @param tower Tour de votes
 * @param slot Slot voté
 * @param vote_hash Hash du vote
 * @return true si succès, false sinon
 */
bool lum_tower_add_vote(lum_tower_t* tower, uint64_t slot, const uint8_t* vote_hash);

/**
 * @brief Confirmer vote (lockout expiré)
 * 
 * @param tower Tour de votes
 * @param slot Slot à confirmer
 * @return true si confirmé, false sinon
 */
bool lum_tower_confirm_vote(lum_tower_t* tower, uint64_t slot);

/**
 * @brief Obtenir lockout pour un slot
 * 
 * @param tower Tour de votes
 * @param slot Slot
 * @return Lockout en slots, 0 si pas de lockout
 */
uint64_t lum_tower_get_lockout(lum_tower_t* tower, uint64_t slot);

/**
 * @brief Calculer lockout exponentiel
 * 
 * @param vote_index Index du vote dans la tour (0 = plus récent)
 * @return Lockout en slots (2^(vote_index+1))
 */
static inline uint64_t lum_tower_calculate_lockout(uint32_t vote_index) {
    return 1ULL << (vote_index + 1);  // 2^(n+1)
}

/**
 * @brief Vérifier si slot est finalisé
 * 
 * @param tower Tour de votes
 * @param slot Slot à vérifier
 * @return true si finalisé, false sinon
 */
bool lum_tower_is_finalized(lum_tower_t* tower, uint64_t slot);

/* ============================================================================
 * API PUBLIQUE - CONSENSUS
 * ============================================================================ */

/**
 * @brief Initialiser consensus Tower BFT
 * 
 * @param poh Générateur PoH
 * @return Consensus initialisé ou NULL si erreur
 */
lum_tower_consensus_t* lum_tower_consensus_init(lum_poh_t* poh);

/**
 * @brief Libérer consensus Tower BFT
 * 
 * @param consensus Consensus à libérer
 */
void lum_tower_consensus_free(lum_tower_consensus_t* consensus);

/**
 * @brief Ajouter validateur
 * 
 * @param consensus Consensus
 * @param pubkey Clé publique validateur (32 bytes)
 * @param stake Stake du validateur
 * @return true si succès, false sinon
 */
bool lum_tower_consensus_add_validator(lum_tower_consensus_t* consensus,
                                        const uint8_t* pubkey, uint64_t stake);

/**
 * @brief Retirer validateur
 * 
 * @param consensus Consensus
 * @param pubkey Clé publique validateur
 * @return true si succès, false sinon
 */
bool lum_tower_consensus_remove_validator(lum_tower_consensus_t* consensus,
                                           const uint8_t* pubkey);

/**
 * @brief Soumettre vote pour un slot
 * 
 * @param consensus Consensus
 * @param validator_pubkey Clé publique validateur
 * @param slot Slot voté
 * @param vote_hash Hash du vote
 * @return true si vote accepté, false sinon
 */
bool lum_tower_consensus_submit_vote(lum_tower_consensus_t* consensus,
                                      const uint8_t* validator_pubkey,
                                      uint64_t slot,
                                      const uint8_t* vote_hash);

/**
 * @brief Vérifier si slot a atteint consensus (67% stake)
 * 
 * @param consensus Consensus
 * @param slot Slot à vérifier
 * @return true si consensus atteint, false sinon
 */
bool lum_tower_consensus_has_supermajority(lum_tower_consensus_t* consensus,
                                            uint64_t slot);

/**
 * @brief Obtenir slot finalisé (32 confirmations)
 * 
 * @param consensus Consensus
 * @return Slot finalisé
 */
uint64_t lum_tower_consensus_get_finalized_slot(lum_tower_consensus_t* consensus);

/**
 * @brief Mettre à jour état consensus (appelé périodiquement)
 * 
 * @param consensus Consensus
 */
void lum_tower_consensus_update(lum_tower_consensus_t* consensus);

/**
 * @brief Obtenir métriques consensus
 * 
 * @param consensus Consensus
 * @param total_votes Total votes soumis
 * @param total_confirmations Total confirmations
 * @param avg_confirmation_time_ms Temps moyen confirmation (ms)
 * @param finalized_slot Slot finalisé
 */
void lum_tower_consensus_get_metrics(lum_tower_consensus_t* consensus,
                                      uint64_t* total_votes,
                                      uint64_t* total_confirmations,
                                      double* avg_confirmation_time_ms,
                                      uint64_t* finalized_slot);

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

/**
 * @brief Calculer hash de vote
 * 
 * @param slot Slot
 * @param block_hash Hash du bloc
 * @param validator_pubkey Clé publique validateur
 * @param vote_hash Hash calculé (32 bytes)
 */
void lum_tower_calculate_vote_hash(uint64_t slot,
                                    const uint8_t* block_hash,
                                    const uint8_t* validator_pubkey,
                                    uint8_t* vote_hash);

/**
 * @brief Vérifier signature vote
 * 
 * @param vote_hash Hash du vote
 * @param signature Signature (64 bytes)
 * @param pubkey Clé publique (32 bytes)
 * @return true si signature valide, false sinon
 */
bool lum_tower_verify_vote_signature(const uint8_t* vote_hash,
                                      const uint8_t* signature,
                                      const uint8_t* pubkey);

#ifdef __cplusplus
}
#endif

#endif /* LUM_TOWER_BFT_H */

// Made with Bob
