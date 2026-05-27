/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_mining_adapter.h — Couche Adaptation OpenCL→Gen9 Native
 *
 * CYCLE C255v8n Phase 2 — Migration Moteur Complet PT-MC
 * STANDARD_NAMES.md v4.6 §M-BTC-GEN9-ADAPTER-C255V8N
 *
 * OBJECTIF : COMPATIBILITÉ API OpenCL → Gen9 Native
 * - Wrappers fonctions OpenCL → Gen9
 * - Structures stats compatibles
 * - Constantes batch size
 * - 0% modification btc_mining_engine.c (sauf includes)
 */

#ifndef BTC_GEN9_MINING_ADAPTER_H
#define BTC_GEN9_MINING_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "btc_gen9_native_runner.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════════════════
 * CONSTANTES COMPATIBILITÉ OpenCL
 * ══════════════════════════════════════════════════════════════════════ */

/* C286: Batch size 10M nonces (C283 validé: 100/100 dispatches OK, 40 GH/s pic) */
#define BTC_OCL_BATCH_DEFAULT 10000000  /* 10M nonces/dispatch (C283: 2.506 GH/s moyen, 42.19 GH/s pic) */

/* Codes retour compatibles OpenCL */
#define BTC_OCL_OK 0
#define BTC_OCL_ERROR -1

/* ══════════════════════════════════════════════════════════════════════
 * STRUCTURES COMPATIBILITÉ OpenCL
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Structure stats GPU (compatible lum_gpu_stats_t OpenCL)
 */
typedef struct {
    double current_mhs;           /* Hashrate actuel (MH/s) */
    double gpu_utilization_pct;   /* Utilisation GPU (%) */
    double cpu_free_pct;          /* CPU libre (%) */
    uint64_t kernel_exec_ns;      /* Temps exécution kernel (ns) */
    uint64_t total_hashes;        /* Total hashes calculés */
    uint32_t best_bits;           /* Meilleur résultat (bits zéro) */
} lum_gpu_stats_t;

/* ══════════════════════════════════════════════════════════════════════
 * CONTEXTE GLOBAL GEN9 (Thread-Safe)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Contexte global Gen9 pour adaptation OpenCL
 * Initialisé par btc_gen9_adapter_init()
 */
typedef struct {
    btc_gen9_context_t* ctx;      /* Contexte Gen9 natif */
    
    /* C255v8n PHASE 3 OPTIMISATION #3: Triple buffering (vs double)
     * OBJECTIF: Éliminer GPU idle entre dispatches
     * PIPELINE: Buffer 0 (Dispatch) → Buffer 1 (Wait) → Buffer 2 (Prepare)
     * GAIN ATTENDU: +3% hashrate (réduction idle 5-10% → 0%)
     */
    #define GEN9_BUFFER_COUNT 3
    struct {
        uint32_t midstate[8];     /* Midstate SHA-256 */
        uint32_t tail[4];         /* Tail block */
        uint32_t nonce_start;     /* Nonce départ */
        uint32_t batch_size;      /* Taille batch */
        uint32_t target_bits;     /* Bits zéro requis */
        uint32_t out_nonce;       /* Nonce trouvé */
        uint32_t out_hash[8];     /* Hash résultant */
        uint32_t out_best;        /* Meilleur bits */
        bool in_flight;           /* Batch en cours */
        uint64_t submit_ns;       /* Timestamp soumission */
        uint64_t complete_ns;     /* Timestamp complétion */
    } buffers[GEN9_BUFFER_COUNT];
    
    /* Stats cumulées */
    lum_gpu_stats_t stats;
    uint64_t total_batches;
    uint64_t total_hashes;
    uint32_t best_bits_global;
    
    /* Timestamps */
    uint64_t init_ns;
    uint64_t last_stats_ns;
    
} btc_gen9_adapter_context_t;

/* ══════════════════════════════════════════════════════════════════════
 * API ADAPTATION OpenCL → Gen9
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Initialise adaptateur Gen9 (remplace init OpenCL)
 * 
 * @param config Configuration Gen9
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_adapter_init(const btc_gen9_config_t* config);

/**
 * Lance batch mining asynchrone (compatible lum_gpu_mine_batch_async)
 * 
 * @param buffer_idx Index buffer (0 ou 1 pour double-buffering)
 * @param midstate Midstate SHA-256 (8 uint32_t)
 * @param tail Tail block (4 uint32_t)
 * @param nonce_start Nonce de départ
 * @param batch_size Nombre de nonces à tester
 * @param target_bits Bits zéro requis
 * @param out_nonce [OUT] Nonce trouvé (si succès)
 * @param out_hash [OUT] Hash résultant (32 bytes)
 * @param out_best [OUT] Meilleur bits trouvé
 * @return BTC_OCL_OK si succès, BTC_OCL_ERROR si erreur
 */
int lum_gpu_mine_batch_async(
    int buffer_idx,
    const uint32_t midstate[8],
    const uint32_t tail[4],
    uint32_t nonce_start,
    uint32_t batch_size,
    uint32_t target_bits,
    uint32_t* out_nonce,
    uint32_t out_hash[8],
    uint32_t* out_best
);

/**
 * Attend complétion batch (compatible lum_gpu_wait_batch)
 * 
 * @param buffer_idx Index buffer à attendre
 * @param out_best [OUT] Meilleur bits trouvé
 * @return BTC_OCL_OK si succès, BTC_OCL_ERROR si erreur
 */
int lum_gpu_wait_batch(int buffer_idx, uint32_t* out_best);

/**
 * Obtient stats GPU (compatible lum_gpu_get_stats)
 * 
 * @param stats [OUT] Structure stats à remplir
 * @return 0 si succès, -1 si erreur
 */
int lum_gpu_get_stats(lum_gpu_stats_t* stats);

/**
 * C260: Obtient batch size dynamique actuel (progression adaptative)
 *
 * @return Batch size optimal calculé par algorithme adaptatif C258
 */
uint32_t btc_gen9_get_current_batch_size(void);

/**
 * Cleanup adaptateur (remplace btc_opencl_cleanup)
 */
void btc_opencl_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* BTC_GEN9_MINING_ADAPTER_H */

// Made with Bob
