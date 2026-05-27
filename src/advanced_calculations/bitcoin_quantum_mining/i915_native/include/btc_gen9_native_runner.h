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

/* Configuration par défaut */
#define BTC_GEN9_DEFAULT_CONFIG { \
    .batch_size = 262144, \
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
 * Exécute batch buffer sur GPU
 * 
 * @param ctx Contexte Gen9
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_execute(btc_gen9_context_t* ctx);
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
