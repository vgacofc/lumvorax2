/**
 * @file btc_warmup_post_reopen.h
 * @brief Header pour module de warm-up GPU après reopen DRM
 * 
 * Cycle: C242
 * Objectif: Éliminer la pénalité de performance post-reopen (1.5-3.5s → 0.7s)
 */

#ifndef BTC_WARMUP_POST_REOPEN_H
#define BTC_WARMUP_POST_REOPEN_H

#include <stdint.h>
#include <stddef.h>

/**
 * Contexte de warm-up GPU
 */
struct btc_warmup_context {
    int drm_fd;                     // File descriptor DRM
    uint32_t batch_handle;          // Handle du batch buffer
    void *batch_ptr;                // Pointeur vers le batch buffer mappé
    size_t batch_size;              // Taille du batch buffer
    uint32_t context_id;            // ID du contexte DRM
    
    int warmup_enabled;             // Warm-up activé (1) ou désactivé (0)
    int warmup_count;               // Nombre de warm-ups effectués
    double last_warmup_time_ms;     // Temps du dernier warm-up en ms
};

/**
 * Statistiques de warm-up
 */
struct btc_warmup_stats {
    int total_warmups;              // Nombre total de warm-ups
    double last_warmup_time_ms;     // Temps du dernier warm-up
    int warmup_enabled;             // État actuel (activé/désactivé)
};

/**
 * Initialise le contexte de warm-up
 * 
 * @param ctx Pointeur vers le contexte
 * @param drm_fd File descriptor DRM
 * @param batch_handle Handle du batch buffer
 * @param batch_ptr Pointeur vers le batch buffer mappé
 * @param batch_size Taille du batch buffer
 * @param context_id ID du contexte DRM
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int btc_warmup_init(struct btc_warmup_context *ctx, int drm_fd,
                    uint32_t batch_handle, void *batch_ptr, size_t batch_size,
                    uint32_t context_id);

/**
 * Exécute le warm-up après reopen
 * 
 * Cette fonction doit être appelée immédiatement après la réouverture
 * du file descriptor DRM et la recréation des buffers.
 * 
 * @param ctx Pointeur vers le contexte
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int btc_warmup_execute(struct btc_warmup_context *ctx);

/**
 * Désactive le warm-up (pour tests comparatifs)
 * 
 * @param ctx Pointeur vers le contexte
 */
void btc_warmup_disable(struct btc_warmup_context *ctx);

/**
 * Active le warm-up
 * 
 * @param ctx Pointeur vers le contexte
 */
void btc_warmup_enable(struct btc_warmup_context *ctx);

/**
 * Obtient les statistiques de warm-up
 * 
 * @param ctx Pointeur vers le contexte
 * @param stats Pointeur vers la structure de statistiques à remplir
 */
void btc_warmup_get_stats(const struct btc_warmup_context *ctx,
                          struct btc_warmup_stats *stats);

/**
 * Affiche les statistiques de warm-up
 * 
 * @param ctx Pointeur vers le contexte
 */
void btc_warmup_print_stats(const struct btc_warmup_context *ctx);

/**
 * Libère les ressources du contexte de warm-up
 * 
 * @param ctx Pointeur vers le contexte
 */
void btc_warmup_cleanup(struct btc_warmup_context *ctx);

#endif // BTC_WARMUP_POST_REOPEN_H

// Made with Bob
