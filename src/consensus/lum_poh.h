/**
 * @file lum_poh.h
 * @brief LumVorax Proof of History (PoH) - Horloge Cryptographique Décentralisée
 * 
 * Implémentation inspirée de Solana mais optimisée pour dépasser 100K TPS
 * - SHA-256 séquentiel pour ordre global sans consensus
 * - Précision nanoseconde pour latence <50ms
 * - Parallélisation des vérifications
 * 
 * @version 1.0.0
 * @date 2026-05-02
 * @author Bob (LumVorax Core Team)
 */

#ifndef LUM_POH_H
#define LUM_POH_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <openssl/sha.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define LUM_POH_HASH_SIZE 32              // SHA-256 = 32 bytes
#define LUM_POH_TICKS_PER_SECOND 1000000  // 1M ticks/sec (1 tick = 1µs)
#define LUM_POH_SLOT_DURATION_MS 400      // 400ms par slot (vs Solana)
#define LUM_POH_TICKS_PER_SLOT 400000     // 400K ticks par slot
#define LUM_POH_MAX_ENTRIES 1000000       // Buffer circulaire 1M entrées

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

/**
 * @brief Entrée PoH individuelle
 */
typedef struct {
    uint8_t hash[LUM_POH_HASH_SIZE];  // Hash SHA-256 courant
    uint64_t sequence;                 // Numéro séquence global
    uint64_t timestamp_ns;             // Timestamp nanoseconde
    uint64_t slot;                     // Numéro slot
    uint32_t tick_in_slot;             // Tick dans le slot
} lum_poh_entry_t;

/**
 * @brief Générateur PoH (thread dédié)
 */
typedef struct {
    // État courant
    uint8_t current_hash[LUM_POH_HASH_SIZE];
    uint64_t sequence;
    uint64_t slot;
    uint32_t tick_in_slot;
    
    // Buffer circulaire pour historique
    lum_poh_entry_t* entries;
    uint64_t entries_capacity;
    uint64_t entries_head;
    uint64_t entries_tail;
    
    // Thread et synchronisation
    pthread_t thread;
    pthread_mutex_t mutex;
    bool running;
    
    // Métriques performance
    uint64_t total_ticks;
    uint64_t start_time_ns;
    double ticks_per_second;
    
    // Configuration
    uint32_t target_ticks_per_second;
    bool enable_recording;  // Enregistrer historique ?
} lum_poh_t;

/**
 * @brief Preuve PoH pour transaction
 */
typedef struct {
    uint8_t tx_hash[LUM_POH_HASH_SIZE];     // Hash transaction
    uint8_t poh_hash[LUM_POH_HASH_SIZE];    // Hash PoH au moment insertion
    uint64_t sequence;                       // Séquence PoH
    uint64_t timestamp_ns;                   // Timestamp insertion
    uint64_t slot;                           // Slot insertion
} lum_poh_proof_t;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

/**
 * @brief Initialiser générateur PoH
 * 
 * @param target_tps Ticks par seconde cible (défaut: 1M)
 * @param enable_recording Enregistrer historique ?
 * @return Générateur PoH ou NULL si erreur
 */
lum_poh_t* lum_poh_init(uint32_t target_tps, bool enable_recording);

/**
 * @brief Démarrer générateur PoH (thread dédié)
 * 
 * @param poh Générateur PoH
 * @return true si succès, false sinon
 */
bool lum_poh_start(lum_poh_t* poh);

/**
 * @brief Arrêter générateur PoH
 * 
 * @param poh Générateur PoH
 */
void lum_poh_stop(lum_poh_t* poh);

/**
 * @brief Libérer générateur PoH
 * 
 * @param poh Générateur PoH
 */
void lum_poh_free(lum_poh_t* poh);

/**
 * @brief Obtenir état PoH courant (thread-safe)
 * 
 * @param poh Générateur PoH
 * @param entry Entrée à remplir
 * @return true si succès, false sinon
 */
bool lum_poh_get_current(lum_poh_t* poh, lum_poh_entry_t* entry);

/**
 * @brief Mixer transaction dans PoH (pour ordre global)
 * 
 * @param poh Générateur PoH
 * @param tx_hash Hash transaction (32 bytes)
 * @param proof Preuve PoH générée
 * @return true si succès, false sinon
 */
bool lum_poh_mix_transaction(lum_poh_t* poh, const uint8_t* tx_hash, lum_poh_proof_t* proof);

/**
 * @brief Vérifier preuve PoH
 * 
 * @param poh Générateur PoH
 * @param proof Preuve à vérifier
 * @return true si valide, false sinon
 */
bool lum_poh_verify_proof(lum_poh_t* poh, const lum_poh_proof_t* proof);

/**
 * @brief Obtenir entrée PoH par séquence
 * 
 * @param poh Générateur PoH
 * @param sequence Numéro séquence
 * @param entry Entrée à remplir
 * @return true si trouvée, false sinon
 */
bool lum_poh_get_entry(lum_poh_t* poh, uint64_t sequence, lum_poh_entry_t* entry);

/**
 * @brief Obtenir métriques performance
 * 
 * @param poh Générateur PoH
 * @param ticks_per_second Ticks/sec actuels
 * @param total_ticks Total ticks générés
 * @param uptime_seconds Temps fonctionnement (secondes)
 */
void lum_poh_get_metrics(lum_poh_t* poh, double* ticks_per_second, 
                         uint64_t* total_ticks, double* uptime_seconds);

/**
 * @brief Calculer slot depuis séquence
 * 
 * @param sequence Numéro séquence
 * @return Numéro slot
 */
static inline uint64_t lum_poh_sequence_to_slot(uint64_t sequence) {
    return sequence / LUM_POH_TICKS_PER_SLOT;
}

/**
 * @brief Calculer tick dans slot depuis séquence
 * 
 * @param sequence Numéro séquence
 * @return Tick dans slot (0-399999)
 */
static inline uint32_t lum_poh_sequence_to_tick(uint64_t sequence) {
    return (uint32_t)(sequence % LUM_POH_TICKS_PER_SLOT);
}

/* ============================================================================
 * FONCTIONS INTERNES (EXPOSÉES POUR TESTS)
 * ============================================================================ */

/**
 * @brief Générer tick PoH (SHA-256 séquentiel)
 * 
 * @param poh Générateur PoH
 */
void lum_poh_tick_internal(lum_poh_t* poh);

/**
 * @brief Thread générateur PoH
 * 
 * @param arg Pointeur vers lum_poh_t
 * @return NULL
 */
void* lum_poh_generator_thread(void* arg);

/**
 * @brief Obtenir timestamp nanoseconde (CLOCK_MONOTONIC_RAW)
 * 
 * @return Timestamp en nanosecondes
 */
uint64_t lum_poh_get_timestamp_ns(void);

#ifdef __cplusplus
}
#endif

#endif /* LUM_POH_H */

// Made with Bob
