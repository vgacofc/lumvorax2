/**
 * @file btc_gpu_backend.c
 * @brief Dispatcher unifié GPU backend - Route vers Level Zero UNIQUEMENT
 * 
 * OBJECTIF : 0% DÉPENDANCE OPENCL
 * Ce dispatcher route TOUS les appels vers Level Zero natif.
 * Le wrapper OpenCL est SUPPRIMÉ pour éliminer toute dépendance.
 * 
 * @author Bob (LumVorax AI Agent)
 * @date 2026-05-12
 * @version 1.0
 * @cycle C221
 */

#include "btc_gpu_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Forward declarations des implémentations Level Zero */
extern int btc_gpu_levelzero_init(const btc_gpu_config_t *config, 
                                  btc_gpu_backend_context_t **ctx_out);
extern int btc_gpu_levelzero_destroy(btc_gpu_backend_context_t *ctx);
extern int btc_gpu_levelzero_buffer_alloc(btc_gpu_backend_context_t *ctx,
                                          size_t size_bytes,
                                          btc_gpu_buffer_t **buffer_out);
extern int btc_gpu_levelzero_buffer_free(btc_gpu_buffer_t *buffer);
extern int btc_gpu_levelzero_buffer_write(btc_gpu_buffer_t *buffer,
                                          const void *data,
                                          size_t size_bytes,
                                          bool blocking);
extern int btc_gpu_levelzero_buffer_read(btc_gpu_buffer_t *buffer,
                                         void *data,
                                         size_t size_bytes,
                                         bool blocking);
extern int btc_gpu_levelzero_kernel_create(btc_gpu_backend_context_t *ctx,
                                           const char *kernel_name,
                                           btc_gpu_kernel_t **kernel_out);
extern int btc_gpu_levelzero_kernel_destroy(btc_gpu_kernel_t *kernel);
extern int btc_gpu_levelzero_kernel_set_arg_buffer(btc_gpu_kernel_t *kernel,
                                                    uint32_t arg_index,
                                                    btc_gpu_buffer_t *buffer);
extern int btc_gpu_levelzero_kernel_set_arg_value(btc_gpu_kernel_t *kernel,
                                                   uint32_t arg_index,
                                                   const void *value,
                                                   size_t size_bytes);
extern int btc_gpu_levelzero_kernel_launch(btc_gpu_kernel_t *kernel,
                                           size_t global_work_size,
                                           size_t local_work_size,
                                           btc_gpu_event_t **event_out);
extern int btc_gpu_levelzero_event_wait(btc_gpu_event_t *event);
extern int btc_gpu_levelzero_event_get_profiling(btc_gpu_event_t *event,
                                                  uint64_t *queued_ns,
                                                  uint64_t *submit_ns,
                                                  uint64_t *start_ns,
                                                  uint64_t *end_ns);
extern int btc_gpu_levelzero_event_release(btc_gpu_event_t *event);
extern int btc_gpu_levelzero_get_stats(btc_gpu_backend_context_t *ctx,
                                       btc_gpu_stats_t *stats);
extern int btc_gpu_levelzero_reset_stats(btc_gpu_backend_context_t *ctx);

/* ========================================================================
 * DÉTECTION BACKEND
 * ======================================================================== */

/**
 * @brief Détecte le meilleur backend disponible
 * 
 * IMPORTANT : Retourne TOUJOURS Level Zero (objectif 0% dépendance OpenCL)
 */
btc_gpu_backend_type_t btc_gpu_backend_detect(void) {
    fprintf(stderr, "[C221-BACKEND] Détection backend : Level Zero FORCÉ (0%% OpenCL)\n");
    return BTC_GPU_BACKEND_LEVELZERO;
}

/**
 * @brief Convertit type backend en string
 */
const char* btc_gpu_backend_type_to_string(btc_gpu_backend_type_t type) {
    switch (type) {
        case BTC_GPU_BACKEND_OPENCL:
            return "OpenCL (DEPRECATED - 0% dépendance objectif)";
        case BTC_GPU_BACKEND_LEVELZERO:
            return "Level Zero (NATIF)";
        case BTC_GPU_BACKEND_AUTO:
            return "Auto (→ Level Zero)";
        default:
            return "Unknown";
    }
}

/**
 * @brief Parse variable d'environnement pour type backend
 */
btc_gpu_backend_type_t btc_gpu_backend_parse_env(const char *env_value) {
    if (!env_value || env_value[0] == '\0') {
        return BTC_GPU_BACKEND_AUTO;  /* → Level Zero */
    }
    
    /* Case-insensitive comparison */
    if (strcasecmp(env_value, "levelzero") == 0 ||
        strcasecmp(env_value, "l0") == 0 ||
        strcasecmp(env_value, "ze") == 0) {
        return BTC_GPU_BACKEND_LEVELZERO;
    }
    
    if (strcasecmp(env_value, "opencl") == 0 ||
        strcasecmp(env_value, "cl") == 0) {
        fprintf(stderr, "[C221-BACKEND] ⚠️  OpenCL demandé mais IGNORÉ (objectif 0%% dépendance)\n");
        fprintf(stderr, "[C221-BACKEND] → Utilisation Level Zero à la place\n");
        return BTC_GPU_BACKEND_LEVELZERO;
    }
    
    if (strcasecmp(env_value, "auto") == 0) {
        return BTC_GPU_BACKEND_AUTO;  /* → Level Zero */
    }
    
    fprintf(stderr, "[C221-BACKEND] ⚠️  Backend inconnu '%s', utilisation Level Zero\n", env_value);
    return BTC_GPU_BACKEND_LEVELZERO;
}

/* ========================================================================
 * API PUBLIQUE - DISPATCH VERS LEVEL ZERO UNIQUEMENT
 * ======================================================================== */

/**
 * @brief Initialise backend GPU (Level Zero UNIQUEMENT)
 */
int btc_gpu_backend_init(const btc_gpu_config_t *config, 
                         btc_gpu_backend_context_t **ctx_out) {
    if (!config || !ctx_out) {
        fprintf(stderr, "[C221-BACKEND] ERREUR : Paramètres invalides\n");
        return -1;
    }
    
    /* Détection backend (toujours Level Zero) */
    btc_gpu_backend_type_t type = config->backend_type;
    if (type == BTC_GPU_BACKEND_AUTO) {
        type = btc_gpu_backend_detect();
    }
    
    /* Vérification environnement */
    const char *env_backend = getenv("BTC_GPU_BACKEND");
    if (env_backend) {
        btc_gpu_backend_type_t env_type = btc_gpu_backend_parse_env(env_backend);
        fprintf(stderr, "[C221-BACKEND] Variable BTC_GPU_BACKEND=%s → %s\n",
                env_backend, btc_gpu_backend_type_to_string(env_type));
        type = env_type;
    }
    
    /* FORCE Level Zero (objectif 0% OpenCL) */
    if (type != BTC_GPU_BACKEND_LEVELZERO) {
        fprintf(stderr, "[C221-BACKEND] ⚠️  Backend %s demandé mais FORCÉ vers Level Zero\n",
                btc_gpu_backend_type_to_string(type));
        fprintf(stderr, "[C221-BACKEND] OBJECTIF : 0%% dépendance OpenCL\n");
        type = BTC_GPU_BACKEND_LEVELZERO;
    }
    
    fprintf(stderr, "[C221-BACKEND] Initialisation backend : %s\n",
            btc_gpu_backend_type_to_string(type));
    
    /* Dispatch vers Level Zero */
    return btc_gpu_levelzero_init(config, ctx_out);
}

/**
 * @brief Détruit contexte backend
 */
int btc_gpu_backend_destroy(btc_gpu_backend_context_t *ctx) {
    if (!ctx) return -1;
    return btc_gpu_levelzero_destroy(ctx);
}

/**
 * @brief Alloue buffer GPU
 */
int btc_gpu_buffer_alloc(btc_gpu_backend_context_t *ctx,
                         size_t size_bytes,
                         btc_gpu_buffer_t **buffer_out) {
    if (!ctx || !buffer_out) return -1;
    return btc_gpu_levelzero_buffer_alloc(ctx, size_bytes, buffer_out);
}

/**
 * @brief Libère buffer GPU
 */
int btc_gpu_buffer_free(btc_gpu_buffer_t *buffer) {
    if (!buffer) return -1;
    return btc_gpu_levelzero_buffer_free(buffer);
}

/**
 * @brief Écrit données dans buffer GPU
 */
int btc_gpu_buffer_write(btc_gpu_buffer_t *buffer,
                         const void *data,
                         size_t size_bytes,
                         bool blocking) {
    if (!buffer || !data) return -1;
    return btc_gpu_levelzero_buffer_write(buffer, data, size_bytes, blocking);
}

/**
 * @brief Lit données depuis buffer GPU
 */
int btc_gpu_buffer_read(btc_gpu_buffer_t *buffer,
                        void *data,
                        size_t size_bytes,
                        bool blocking) {
    if (!buffer || !data) return -1;
    return btc_gpu_levelzero_buffer_read(buffer, data, size_bytes, blocking);
}

/**
 * @brief Crée kernel GPU
 */
int btc_gpu_kernel_create(btc_gpu_backend_context_t *ctx,
                          const char *kernel_name,
                          btc_gpu_kernel_t **kernel_out) {
    if (!ctx || !kernel_name || !kernel_out) return -1;
    return btc_gpu_levelzero_kernel_create(ctx, kernel_name, kernel_out);
}

/**
 * @brief Détruit kernel GPU
 */
int btc_gpu_kernel_destroy(btc_gpu_kernel_t *kernel) {
    if (!kernel) return -1;
    return btc_gpu_levelzero_kernel_destroy(kernel);
}

/**
 * @brief Définit argument kernel (buffer)
 */
int btc_gpu_kernel_set_arg_buffer(btc_gpu_kernel_t *kernel,
                                  uint32_t arg_index,
                                  btc_gpu_buffer_t *buffer) {
    if (!kernel || !buffer) return -1;
    return btc_gpu_levelzero_kernel_set_arg_buffer(kernel, arg_index, buffer);
}

/**
 * @brief Définit argument kernel (valeur)
 */
int btc_gpu_kernel_set_arg_value(btc_gpu_kernel_t *kernel,
                                 uint32_t arg_index,
                                 const void *value,
                                 size_t size_bytes) {
    if (!kernel || !value) return -1;
    return btc_gpu_levelzero_kernel_set_arg_value(kernel, arg_index, value, size_bytes);
}

/**
 * @brief Lance kernel GPU
 */
int btc_gpu_kernel_launch(btc_gpu_kernel_t *kernel,
                          size_t global_work_size,
                          size_t local_work_size,
                          btc_gpu_event_t **event_out) {
    if (!kernel) return -1;
    return btc_gpu_levelzero_kernel_launch(kernel, global_work_size, local_work_size, event_out);
}

/**
 * @brief Attend événement GPU
 */
int btc_gpu_event_wait(btc_gpu_event_t *event) {
    if (!event) return -1;
    return btc_gpu_levelzero_event_wait(event);
}

/**
 * @brief Obtient timestamps profiling événement
 */
int btc_gpu_event_get_profiling(btc_gpu_event_t *event,
                                uint64_t *queued_ns,
                                uint64_t *submit_ns,
                                uint64_t *start_ns,
                                uint64_t *end_ns) {
    if (!event) return -1;
    return btc_gpu_levelzero_event_get_profiling(event, queued_ns, submit_ns, start_ns, end_ns);
}

/**
 * @brief Libère événement GPU
 */
int btc_gpu_event_release(btc_gpu_event_t *event) {
    if (!event) return -1;
    return btc_gpu_levelzero_event_release(event);
}

/**
 * @brief Obtient statistiques GPU
 */
int btc_gpu_backend_get_stats(btc_gpu_backend_context_t *ctx,
                              btc_gpu_stats_t *stats) {
    if (!ctx || !stats) return -1;
    return btc_gpu_levelzero_get_stats(ctx, stats);
}

/**
 * @brief Réinitialise statistiques GPU
 */
int btc_gpu_backend_reset_stats(btc_gpu_backend_context_t *ctx) {
    if (!ctx) return -1;
    return btc_gpu_levelzero_reset_stats(ctx);
}

/**
 * @brief Obtient nom backend actif
 */
const char* btc_gpu_backend_get_name(btc_gpu_backend_context_t *ctx) {
    (void)ctx;  /* Unused - toujours Level Zero */
    return "Level Zero Native";
}

/**
 * @brief Obtient type backend actif
 */
btc_gpu_backend_type_t btc_gpu_backend_get_type(btc_gpu_backend_context_t *ctx) {
    (void)ctx;  /* Unused - toujours Level Zero */
    return BTC_GPU_BACKEND_LEVELZERO;
}

/* Made with Bob - LumVorax C221 - 0% Dépendance OpenCL */

// Made with Bob
