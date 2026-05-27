#ifndef VORAX_SHA256_H_INCLUDED
#define VORAX_SHA256_H_INCLUDED

/*
 * VORAX SHA-256 — Module Calculs Cryptographiques
 * vorax_sha256.h — Interface SHA-256 intégrée VORAX
 *
 * CYCLE C266 — 2026-05-18
 * OBJECTIF : Intégrer calculs SHA-256 dans architecture VORAX
 *            pour remplacer calculs classiques btc_mining_engine.c
 *
 * ARCHITECTURE :
 *  - Manipulation LUM groups pour données SHA-256
 *  - Optimisations AVX-512 vectorisées (8-way SHA-256)
 *  - Zero-copy entre VORAX operations et GPU
 *  - Compatible pipeline i915 DRM natif
 *
 * AVANTAGES vs CLASSIQUE :
 *  - Réutilisation infrastructure VORAX (fuse, split, cycle)
 *  - Optimisations mémoire VORAX (THP, prefetch, alignment)
 *  - Traçabilité forensique unifiée
 *  - Architecture modulaire extensible
 */

#include "../lum/lum_core.h"
#include "vorax_operations.h"
#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// STRUCTURES SHA-256 VORAX
// ═══════════════════════════════════════════════════════════════════════════

// Structure block Bitcoin (80 bytes) encapsulé dans LUM
typedef struct {
    uint32_t version;           // 4 bytes
    uint8_t  prev_block[32];    // 32 bytes
    uint8_t  merkle_root[32];   // 32 bytes
    uint32_t timestamp;         // 4 bytes
    uint32_t bits;              // 4 bytes (difficulté)
    uint32_t nonce;             // 4 bytes
} __attribute__((packed, aligned(64))) vorax_btc_block_t;

// Structure résultat SHA-256 VORAX
typedef struct {
    uint8_t  hash[32];          // Hash SHA-256 résultat
    uint32_t nonce;             // Nonce utilisé
    uint8_t  leading_zeros;     // Nombre de zeros leading
    bool     valid;             // Hash < target ?
    uint64_t compute_time_ns;   // Temps calcul (ns)
} vorax_sha256_result_t;

// Structure batch SHA-256 VORAX (groupe LUM)
typedef struct {
    lum_group_t*           input_group;    // Groupe blocks input
    lum_group_t*           output_group;   // Groupe résultats
    uint32_t               start_nonce;    // Nonce début
    uint32_t               count;          // Nombre nonces
    uint8_t                target[32];     // Target difficulté
    vorax_sha256_result_t* results;        // Array résultats
    size_t                 results_count;  // Nombre résultats
    uint64_t               total_time_ns;  // Temps total
} vorax_sha256_batch_t;

// ═══════════════════════════════════════════════════════════════════════════
// FONCTIONS CORE SHA-256 VORAX
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Créer batch SHA-256 VORAX depuis block template
 * 
 * @param block_template Block Bitcoin 80 bytes
 * @param start_nonce Nonce début
 * @param count Nombre nonces à tester
 * @param target Target difficulté (32 bytes)
 * @return Batch SHA-256 VORAX ou NULL si erreur
 */
vorax_sha256_batch_t* vorax_sha256_batch_create(
    const vorax_btc_block_t* block_template,
    uint32_t start_nonce,
    uint32_t count,
    const uint8_t target[32]
);

/**
 * Calculer SHA-256 double (hash1 + hash2) sur batch VORAX
 * Utilise optimisations AVX-512 si disponible
 * 
 * @param batch Batch SHA-256 VORAX
 * @return 0 si succès, -1 si erreur
 */
int vorax_sha256_batch_compute(vorax_sha256_batch_t* batch);

/**
 * Calculer SHA-256 double sur GPU via i915 DRM natif
 * Utilise pipeline Gen9 Native Runner existant
 * 
 * @param batch Batch SHA-256 VORAX
 * @return 0 si succès, -1 si erreur
 */
int vorax_sha256_batch_compute_gpu(vorax_sha256_batch_t* batch);

/**
 * Trouver meilleur résultat (plus de leading zeros)
 * 
 * @param batch Batch SHA-256 VORAX
 * @return Pointeur vers meilleur résultat ou NULL
 */
const vorax_sha256_result_t* vorax_sha256_batch_best_result(
    const vorax_sha256_batch_t* batch
);

/**
 * Détruire batch SHA-256 VORAX
 * 
 * @param batch Batch à détruire
 */
void vorax_sha256_batch_destroy(vorax_sha256_batch_t* batch);

// ═══════════════════════════════════════════════════════════════════════════
// FONCTIONS OPTIMISATIONS VORAX
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Fusionner 2 batches SHA-256 VORAX
 * Utilise vorax_fuse() pour optimisation zero-copy
 * 
 * @param batch1 Premier batch
 * @param batch2 Deuxième batch
 * @return Batch fusionné ou NULL si erreur
 */
vorax_sha256_batch_t* vorax_sha256_batch_fuse(
    vorax_sha256_batch_t* batch1,
    vorax_sha256_batch_t* batch2
);

/**
 * Splitter batch SHA-256 VORAX en N parties
 * Utilise vorax_split() pour distribution parallèle
 * 
 * @param batch Batch à splitter
 * @param parts Nombre parties
 * @return Array batches splittés ou NULL si erreur
 */
vorax_sha256_batch_t** vorax_sha256_batch_split(
    vorax_sha256_batch_t* batch,
    size_t parts
);

/**
 * Compresser batch SHA-256 VORAX (Ω compression)
 * Utilise vorax_compress() pour réduction mémoire
 * 
 * @param batch Batch à compresser
 * @return Batch compressé ou NULL si erreur
 */
vorax_sha256_batch_t* vorax_sha256_batch_compress(
    vorax_sha256_batch_t* batch
);

// ═══════════════════════════════════════════════════════════════════════════
// FONCTIONS COMPARAISON CLASSIQUE vs VORAX
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Comparer performance VORAX vs classique
 * Exécute même batch avec 2 implémentations
 * 
 * @param block_template Block Bitcoin
 * @param start_nonce Nonce début
 * @param count Nombre nonces
 * @param target Target difficulté
 * @param use_gpu true = GPU, false = CPU
 * @return Rapport comparaison ou NULL si erreur
 */
typedef struct {
    double   vorax_time_ms;        // Temps VORAX (ms)
    double   classic_time_ms;      // Temps classique (ms)
    double   speedup;              // Ratio vorax/classic
    uint64_t vorax_hashes;         // Hashes VORAX
    uint64_t classic_hashes;       // Hashes classique
    bool     results_match;        // Résultats identiques ?
    char     analysis[512];        // Analyse détaillée
} vorax_sha256_comparison_t;

vorax_sha256_comparison_t* vorax_sha256_compare(
    const vorax_btc_block_t* block_template,
    uint32_t start_nonce,
    uint32_t count,
    const uint8_t target[32],
    bool use_gpu
);

/**
 * Détruire rapport comparaison
 */
void vorax_sha256_comparison_destroy(vorax_sha256_comparison_t* comp);

// ═══════════════════════════════════════════════════════════════════════════
// FONCTIONS FORENSIQUES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Logger événement SHA-256 VORAX (nano bit-level)
 * 
 * @param event Nom événement
 * @param batch Batch concerné
 * @param details Détails additionnels
 */
void vorax_sha256_log_event(
    const char* event,
    const vorax_sha256_batch_t* batch,
    const char* details
);

/**
 * Dump complet batch SHA-256 VORAX (forensique)
 * 
 * @param batch Batch à dumper
 * @param filepath Chemin fichier output
 * @return 0 si succès, -1 si erreur
 */
int vorax_sha256_batch_dump(
    const vorax_sha256_batch_t* batch,
    const char* filepath
);

#endif /* VORAX_SHA256_H_INCLUDED */

// Made with Bob
