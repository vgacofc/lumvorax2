/**
 * @file lum_poh_simd.h
 * @brief Optimisations SIMD pour PoH avec détection runtime capacités CPU
 * 
 * PHASE 3 - Cycle C199.23
 * Objectif: Récupérer 15% TPS perdu via vectorisation AVX2/SSE4.2
 * 
 * Architecture:
 * - Détection runtime AVX2/SSE4.2 via /proc/cpuinfo
 * - Fallback automatique si instructions désactivées
 * - Vectorisation memcpy batches (4× hashes simultanés)
 * - Logging forensique détection capacités
 */

#ifndef LUM_POH_SIMD_H
#define LUM_POH_SIMD_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ============================================================================
 * DÉTECTION CAPACITÉS CPU
 * ============================================================================ */

/**
 * @brief Capacités SIMD détectées au runtime
 */
typedef struct {
    bool avx2_available;      // AVX2 (256-bit) disponible
    bool sse42_available;     // SSE4.2 (128-bit) disponible
    bool aes_ni_available;    // AES-NI (accélération crypto) disponible
    char cpu_model[256];      // Modèle CPU détecté
    int cpu_cores;            // Nombre cœurs physiques
    int cpu_threads;          // Nombre threads logiques
} lum_simd_capabilities_t;

/**
 * @brief Détecter capacités SIMD du CPU au runtime
 * 
 * Lit /proc/cpuinfo pour identifier:
 * - Flags AVX2, SSE4.2, AES-NI
 * - Modèle CPU
 * - Nombre cœurs/threads
 * 
 * @param caps Structure à remplir avec capacités détectées
 * @return true si détection réussie, false sinon
 */
bool lum_simd_detect_capabilities(lum_simd_capabilities_t* caps);

/**
 * @brief Afficher capacités SIMD détectées (forensique)
 * 
 * @param caps Capacités à afficher
 */
void lum_simd_print_capabilities(const lum_simd_capabilities_t* caps);

/* ============================================================================
 * OPTIMISATIONS MEMCPY VECTORISÉES
 * ============================================================================ */

/**
 * @brief Copier batch hashes avec optimisation SIMD
 * 
 * Stratégie:
 * - AVX2 disponible: Copier 4× hashes (128 bytes) par itération
 * - SSE4.2 disponible: Copier 2× hashes (64 bytes) par itération
 * - Fallback: memcpy() standard
 * 
 * @param dst Destination (batch.hashes[index])
 * @param src Source (poh->current_hash)
 * @param count Nombre hashes à copier
 * @param caps Capacités SIMD détectées
 */
void lum_simd_copy_hashes(uint8_t* dst, const uint8_t* src, int count,
                          const lum_simd_capabilities_t* caps);

/**
 * @brief Copier batch sequences avec optimisation SIMD
 * 
 * Stratégie:
 * - AVX2 disponible: Copier 4× sequences (32 bytes) par itération
 * - SSE4.2 disponible: Copier 2× sequences (16 bytes) par itération
 * - Fallback: memcpy() standard
 * 
 * @param dst Destination (batch.sequences[index])
 * @param src Source (poh->sequence)
 * @param count Nombre sequences à copier
 * @param caps Capacités SIMD détectées
 */
void lum_simd_copy_sequences(uint64_t* dst, const uint64_t* src, int count,
                             const lum_simd_capabilities_t* caps);

/* ============================================================================
 * MÉTRIQUES FORENSIQUES
 * ============================================================================ */

/**
 * @brief Statistiques utilisation SIMD
 */
typedef struct {
    uint64_t avx2_copies;     // Nombre copies AVX2
    uint64_t sse42_copies;    // Nombre copies SSE4.2
    uint64_t fallback_copies; // Nombre copies fallback
    uint64_t total_bytes;     // Total bytes copiés
    double avg_copy_time_ns;  // Temps moyen copie (ns)
} lum_simd_stats_t;

/**
 * @brief Obtenir statistiques SIMD
 * 
 * @return Statistiques cumulées depuis démarrage
 */
lum_simd_stats_t lum_simd_get_stats(void);

/**
 * @brief Réinitialiser statistiques SIMD
 */
void lum_simd_reset_stats(void);

#endif // LUM_POH_SIMD_H

// Made with Bob
