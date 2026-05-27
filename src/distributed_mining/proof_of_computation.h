/**
 * @file proof_of_computation.h
 * @brief Proof of Computation — Preuve cryptographique calculs
 * @author LumVorax Team
 * @date 2026-04-30
 * @version 1.0.0
 * 
 * Système de preuve forensique LUM pour validation calculs :
 * - Échantillonnage aléatoire hashes
 * - Vérification timing (hashrate réaliste)
 * - Signature Ed25519
 * - Trace mémoire LUM
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 */

#ifndef PROOF_OF_COMPUTATION_H
#define PROOF_OF_COMPUTATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * CONSTANTES
 * ======================================================================== */

#define POC_VERSION_MAJOR 1
#define POC_VERSION_MINOR 0
#define POC_VERSION_PATCH 0

#define POC_SAMPLE_COUNT 10             // Nombre échantillons
#define POC_SIGNATURE_LEN 64            // Longueur signature Ed25519
#define POC_TRACE_SIZE 4096             // Taille trace LUM (bytes)
#define POC_MAX_HASHRATE_CPU 100000000  // 100 MH/s max CPU
#define POC_MAX_HASHRATE_GPU 1000000000 // 1 GH/s max GPU

/* ========================================================================
 * TYPES ET STRUCTURES
 * ======================================================================== */

/**
 * @brief Type preuve
 */
typedef enum {
    POC_TYPE_CPU = 0,
    POC_TYPE_GPU = 1,
    POC_TYPE_HYBRID = 2
} poc_type_t;

/**
 * @brief Échantillon hash
 */
typedef struct {
    uint64_t nonce;                     // Nonce testé
    uint8_t hash[32];                   // SHA-256 résultat
    uint64_t timestamp_ns;              // Timestamp (ns)
    uint32_t thread_id;                 // ID thread
} poc_sample_t;

/**
 * @brief Preuve de calcul
 */
typedef struct {
    /* Identification */
    uint8_t miner_id[32];               // ID mineur
    uint64_t task_id;                   // ID tâche
    poc_type_t type;                    // Type preuve (CPU/GPU)
    
    /* Calculs */
    uint64_t nonce_start;               // Début plage
    uint64_t nonce_end;                 // Fin plage
    uint64_t hashes_computed;           // Nombre hashes
    uint64_t elapsed_time_ns;           // Temps écoulé (ns)
    uint32_t hashrate;                  // Hashrate (H/s)
    
    /* Échantillons */
    poc_sample_t samples[POC_SAMPLE_COUNT];  // Échantillons
    uint32_t sample_count;              // Nombre échantillons
    
    /* Trace forensique LUM */
    uint8_t lum_trace[POC_TRACE_SIZE];  // Trace mémoire
    uint32_t trace_size;                // Taille trace réelle
    
    /* Intégrité */
    uint32_t crc32c;                    // CRC32C
    uint8_t signature[POC_SIGNATURE_LEN]; // Signature Ed25519
    uint8_t public_key[32];             // Clé publique Ed25519
} proof_of_computation_t;

/**
 * @brief Résultat vérification
 */
typedef struct {
    uint8_t valid;                      // 1 si valide, 0 si invalide
    uint8_t samples_valid;              // 1 si échantillons valides
    uint8_t hashrate_valid;             // 1 si hashrate réaliste
    uint8_t signature_valid;            // 1 si signature valide
    uint8_t trace_valid;                // 1 si trace LUM valide
    
    char error_message[256];            // Message erreur (si invalide)
} poc_verification_result_t;

/**
 * @brief Configuration POC
 */
typedef struct {
    uint32_t sample_count;              // Nombre échantillons
    double sampling_rate;               // Taux échantillonnage (0.0-1.0)
    
    uint32_t max_hashrate_cpu;          // Hashrate max CPU (H/s)
    uint32_t max_hashrate_gpu;          // Hashrate max GPU (H/s)
    
    uint8_t verify_samples;             // 1 si vérifier échantillons
    uint8_t verify_hashrate;            // 1 si vérifier hashrate
    uint8_t verify_signature;           // 1 si vérifier signature
    uint8_t verify_trace;               // 1 si vérifier trace LUM
} poc_config_t;

/* ========================================================================
 * API PUBLIQUE
 * ======================================================================== */

/**
 * @brief Initialise le système POC
 * @param config Configuration (NULL = config par défaut)
 * @return 0 si succès, -1 si erreur
 */
int poc_init(const poc_config_t* config);

/**
 * @brief Crée une preuve de calcul
 * @param miner_id ID mineur
 * @param task_id ID tâche
 * @param nonce_start Début plage
 * @param nonce_end Fin plage
 * @param hashes_computed Nombre hashes calculés
 * @param elapsed_time_ns Temps écoulé (ns)
 * @param proof Structure preuve (sortie)
 * @return 0 si succès, -1 si erreur
 */
int poc_create(
    const uint8_t* miner_id,
    uint64_t task_id,
    uint64_t nonce_start,
    uint64_t nonce_end,
    uint64_t hashes_computed,
    uint64_t elapsed_time_ns,
    proof_of_computation_t* proof
);

/**
 * @brief Ajoute un échantillon à la preuve
 * @param proof Preuve
 * @param nonce Nonce testé
 * @param hash Hash SHA-256 résultat
 * @param timestamp_ns Timestamp (ns)
 * @return 0 si succès, -1 si erreur
 */
int poc_add_sample(
    proof_of_computation_t* proof,
    uint64_t nonce,
    const uint8_t* hash,
    uint64_t timestamp_ns
);

/**
 * @brief Signe la preuve avec Ed25519
 * @param proof Preuve à signer
 * @param private_key Clé privée Ed25519 (32 bytes)
 * @return 0 si succès, -1 si erreur
 */
int poc_sign(proof_of_computation_t* proof, const uint8_t* private_key);

/**
 * @brief Vérifie une preuve de calcul
 * @param proof Preuve à vérifier
 * @param block_header Header bloc Bitcoin (80 bytes)
 * @param result Structure résultat vérification (sortie)
 * @return 0 si succès, -1 si erreur
 */
int poc_verify(
    const proof_of_computation_t* proof,
    const uint8_t* block_header,
    poc_verification_result_t* result
);

/**
 * @brief Vérifie les échantillons
 * @param proof Preuve
 * @param block_header Header bloc Bitcoin
 * @return 1 si valide, 0 si invalide
 */
int poc_verify_samples(const proof_of_computation_t* proof, const uint8_t* block_header);

/**
 * @brief Vérifie le hashrate (réaliste ?)
 * @param proof Preuve
 * @return 1 si valide, 0 si invalide
 */
int poc_verify_hashrate(const proof_of_computation_t* proof);

/**
 * @brief Vérifie la signature Ed25519
 * @param proof Preuve
 * @return 1 si valide, 0 si invalide
 */
int poc_verify_signature(const proof_of_computation_t* proof);

/**
 * @brief Vérifie la trace LUM
 * @param proof Preuve
 * @return 1 si valide, 0 si invalide
 */
int poc_verify_trace(const proof_of_computation_t* proof);

/**
 * @brief Sérialise une preuve (pour transmission)
 * @param proof Preuve
 * @param buffer Buffer sortie
 * @param buffer_size Taille buffer
 * @return Nombre bytes écrits, -1 si erreur
 */
int poc_serialize(const proof_of_computation_t* proof, uint8_t* buffer, size_t buffer_size);

/**
 * @brief Désérialise une preuve
 * @param buffer Buffer entrée
 * @param buffer_size Taille buffer
 * @param proof Structure preuve (sortie)
 * @return 0 si succès, -1 si erreur
 */
int poc_deserialize(const uint8_t* buffer, size_t buffer_size, proof_of_computation_t* proof);

/**
 * @brief Nettoie et libère les ressources
 * @return 0 si succès, -1 si erreur
 */
int poc_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* PROOF_OF_COMPUTATION_H */

// Made with Bob
