/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_native_runner.h — Header Implémentation NATIVE Gen9
 *
 * CYCLE C198 Phase 15C — Solution Native i915 DRM Direct
 * STANDARD_NAMES.md v4.6 §M-BTC-GEN9-NATIVE-C198
 *
 * OBJECTIF : CONTRÔLE GPU 100% NATIF
 * - 0% dépendance OpenCL
 * - 0% dépendance Level Zero
 * - 100% contrôle direct i915 DRM
 */

#ifndef BTC_GEN9_NATIVE_RUNNER_H
#define BTC_GEN9_NATIVE_RUNNER_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════════════════
 * TYPES OPAQUES
 * ══════════════════════════════════════════════════════════════════════ */

typedef struct btc_gen9_context_t btc_gen9_context_t;

/* ══════════════════════════════════════════════════════════════════════
 * CONFIGURATION
 * ══════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint32_t batch_size;           /* Nonces par batch (262144) */
    uint32_t work_group_size;      /* Threads par work-group (256) */
    const char* kernel_path;       /* Chemin kernel Gen9 ISA */
    const char* log_path;          /* Chemin log forensique */
    bool enable_profiling;
} btc_gen9_config_t;

/* Configuration par défaut — OPTIMISATION #6 C240 */
#define BTC_GEN9_DEFAULT_CONFIG { \
    .batch_size = 268435456, /* 268M nonces (limite GPU Gen9 validée C239) */ \
    .work_group_size = 256, \
    .kernel_path = "kernels/btc_sha256_gen9.bin", \
    .log_path = "logs/forensic/gen9_native.log", \
    .enable_profiling = true \
}

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Initialise contexte Gen9 natif
 * 
 * @param ctx_out Pointeur vers contexte (alloué par fonction)
 * @param config Configuration
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_init(btc_gen9_context_t** ctx_out, const btc_gen9_config_t* config);

/**
 * Exécute batch buffer sur GPU (version simple - données déjà dans buffers)
 *
 * @param ctx Contexte Gen9
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_execute(btc_gen9_context_t* ctx);

/**
 * Exécute mining Bitcoin sur GPU avec données spécifiques
 * VERSION ÉTENDUE C223 - Support mining Bitcoin complet
 *
 * @param ctx Contexte Gen9
 * @param block_header Header Bitcoin (80 bytes)
 * @param start_nonce Nonce de départ
 * @param nonce_count Nombre de nonces à tester
 * @param best_nonce_out [OUT] Meilleur nonce trouvé
 * @param leading_zeros_out [OUT] Nombre de zéros leading du meilleur hash
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_execute_mining(btc_gen9_context_t* ctx,
                            const uint8_t* block_header,
                            uint32_t start_nonce,
                            uint32_t nonce_count,
                            uint32_t* best_nonce_out,
                            uint32_t* leading_zeros_out);
/**
 * OPTIMISATION #1 : Exécute batch de dispatches avec synchronisation GPU
 * Contourne la limite Gen9 de 9 dispatches consécutifs
 * 
 * @param ctx Contexte Gen9
 * @param batch_count Nombre de dispatches dans le batch (recommandé: 9)
 * @return Nombre de dispatches réussis, -1 si erreur
 */
int btc_gen9_execute_batch(btc_gen9_context_t* ctx, int batch_count);
/**
 * C249 OPTIMISATION #3 : Parallélisation Multi-Contextes
 * Exécute 3 dispatches simultanés sur les 3 contextes GPU
 * 
 * ARCHITECTURE :
 * - Thread 1 : Dispatch sur ctx_pool[0]
 * - Thread 2 : Dispatch sur ctx_pool[1]
 * - Thread 3 : Dispatch sur ctx_pool[2]
 * 
 * GAIN ESTIMÉ : 2-3x throughput
 * 
 * @param ctx Contexte Gen9
 * @param total_dispatches Nombre total de dispatches à exécuter
 * @return Nombre de dispatches réussis, -1 si erreur
 */
int btc_gen9_execute_parallel_batch(btc_gen9_context_t* ctx, int total_dispatches);

/**
 * C255 OPTIMISATION MULTI-DISPATCH : Parallélisme 3 contextes × 3 cycles
 * Exécute 9 dispatches simultanés avec optimisations cache miss
 *
 * ARCHITECTURE :
 * - Phase 1 : Soumission asynchrone 9 dispatches (3 threads × 3 cycles)
 * - Phase 2 : Synchronisation finale
 * - Phase 3 : Reopen DRM + Dummy dispatch (warmup cache)
 *
 * OPTIMISATIONS :
 * - Dummy dispatch post-reopen (résout cache miss C252)
 * - Soumission asynchrone (pas d'attente intermédiaire)
 * - Monitoring cache miss automatique
 *
 * GAIN ESTIMÉ : ×12-15 vs C251 (5-6 GH/s)
 * OBJECTIF FINAL : 410 GH/s (nécessite optimisations supplémentaires)
 *
 * @param ctx Contexte Gen9
 * @param num_batches Nombre de batches (1 batch = 9 dispatches)
 * @return Nombre de dispatches réussis, -1 si erreur
 */
int c255_execute_multi_dispatch_batch(btc_gen9_context_t* ctx, int num_batches);

/**
 * Structure résultat parallèle GPU (1 résultat par work-item)
 */
typedef struct {
    uint32_t nonce;        /* Nonce testé */
    uint8_t hash[32];      /* Hash SHA256 résultant */
    uint32_t found;        /* 1 si hash valide trouvé, 0 sinon */
} gpu_parallel_result_t;

/**
 * Exécuter dispatch parallèle GPU (168 work-items)
 *
 * @param ctx Contexte Gen9
 * @param nonce_start Nonce de départ
 * @param results Buffer résultats (168 entrées)
 * @param num_found Nombre de hashes valides trouvés (sortie)
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_execute_parallel_gpu(btc_gen9_context_t* ctx,
                                   uint32_t nonce_start,
                                   gpu_parallel_result_t* results,
                                   int* num_found);

/* ══════════════════════════════════════════════════════════════════════
 * SECTION : FONCTIONS D'ACCÈS CONTEXTE (C250 Phase 1)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Obtenir file descriptor DRM
 */
int btc_gen9_get_drm_fd(const btc_gen9_context_t* ctx);

/**
 * Obtenir handle batch buffer
 */
uint32_t btc_gen9_get_batch_bo(const btc_gen9_context_t* ctx);

/**
 * Obtenir handle output buffer
 */
uint32_t btc_gen9_get_output_bo(const btc_gen9_context_t* ctx);

/**
 * Obtenir handle kernel ISA
 */
uint32_t btc_gen9_get_kernel_bo(const btc_gen9_context_t* ctx);

/**
 * Obtenir context ID i915 actuel
 */
uint32_t btc_gen9_get_ctx_id(const btc_gen9_context_t* ctx);

/**
 * Obtenir fichier log forensique
 */
FILE* btc_gen9_get_log_file(const btc_gen9_context_t* ctx);

/**
 * Obtenir mapping batch buffer
 */
void* btc_gen9_get_batch_map(const btc_gen9_context_t* ctx);

/**
 * Obtenir taille batch buffer
 */
size_t btc_gen9_get_batch_size(const btc_gen9_context_t* ctx);

/**
 * Obtenir mapping output buffer
 */
void* btc_gen9_get_output_map(const btc_gen9_context_t* ctx);

/**
 * Obtenir taille output buffer
 */
size_t btc_gen9_get_output_size(const btc_gen9_context_t* ctx);

/**
 * Obtenir ID contexte du pool à l'index spécifié
 */
uint32_t btc_gen9_get_ctx_pool_id(const btc_gen9_context_t* ctx, int index);

/**
 * Définir ID contexte actuel
 */
void btc_gen9_set_ctx_id(btc_gen9_context_t* ctx, uint32_t ctx_id);

/**
 * Obtenir taille input buffer
 */
size_t btc_gen9_get_input_size(const btc_gen9_context_t* ctx);

/**
 * Obtenir mapping kernel
 */
void* btc_gen9_get_kernel_map(const btc_gen9_context_t* ctx);

/**
 * Obtenir taille kernel
 */
size_t btc_gen9_get_kernel_size(const btc_gen9_context_t* ctx);

/* ══════════════════════════════════════════════════════════════════════
 * SECTION : FONCTIONS UTILITAIRES (C250 Phase 1)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Obtenir timestamp nanoseconde
 */
uint64_t get_timestamp_ns(void);

/**
 * Convertir nanosecondes en secondes
 */
double ns_to_sec(uint64_t ns);

/**
 * Réouvrir connexion DRM (gestion limite Gen9)
 */
int btc_gen9_reopen_drm(btc_gen9_context_t* ctx);

/* ══════════════════════════════════════════════════════════════════════
 * SECTION : GESTION CONTEXTE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Libère contexte Gen9
 * 
 * @param ctx Contexte à libérer
 */
void btc_gen9_cleanup(btc_gen9_context_t* ctx);

/**
 * Affiche métriques
 * 
 * @param ctx Contexte Gen9
 */
void btc_gen9_print_metrics(const btc_gen9_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* BTC_GEN9_NATIVE_RUNNER_H */

// Made with Bob
