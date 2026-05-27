/**
 * LumVorax C255v2 — Soumission Asynchrone i915 DRM (Header)
 */

#ifndef BTC_GEN9_ASYNC_SUBMIT_H
#define BTC_GEN9_ASYNC_SUBMIT_H

#include <stdint.h>
#include <pthread.h>

/* Forward declaration - doit correspondre à btc_gen9_native_runner.h */
typedef struct btc_gen9_context_t btc_gen9_context_t;

/**
 * Structure fence pour tracking dispatch asynchrone
 * 
 * Créée par btc_gen9_submit_async(), utilisée par btc_gen9_wait_fence()
 */
typedef struct {
    uint32_t batch_bo;      /* Handle batch buffer */
    int drm_fd;             /* File descriptor DRM */
    uint32_t ctx_id;        /* ID contexte i915 */
    uint64_t submit_ts;     /* Timestamp soumission (nanosec) */
} btc_gen9_fence_t;

/**
 * Mutex global pour DRM reopen thread-safe
 */
extern pthread_mutex_t g_drm_reopen_mutex;

/**
 * Soumettre dispatch asynchrone (retour immédiat, SANS GEM_WAIT)
 * 
 * DIFFÉRENCE vs btc_gen9_execute():
 * - btc_gen9_execute() = EXECBUFFER2 + GEM_WAIT (synchrone, bloque ~1 sec)
 * - btc_gen9_submit_async() = EXECBUFFER2 SEULEMENT (asynchrone, retour immédiat)
 * 
 * USAGE:
 *   btc_gen9_fence_t fence;
 *   btc_gen9_submit_async(ctx, &fence);  // Retour immédiat
 *   // ... soumettre autres dispatches ...
 *   btc_gen9_wait_fence(&fence);         // Attendre fin
 * 
 * @param ctx Contexte Gen9
 * @param fence_out Fence pour attente ultérieure (output)
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_submit_async(btc_gen9_context_t* ctx, btc_gen9_fence_t* fence_out);

/**
 * Attendre fin dispatch via fence
 * 
 * Bloque jusqu'à ce que le GPU ait terminé l'exécution du dispatch
 * correspondant à la fence.
 * 
 * @param fence Fence créée par btc_gen9_submit_async()
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_wait_fence(const btc_gen9_fence_t* fence);

/**
 * DRM Reopen thread-safe (avec mutex global)
 * 
 * CORRECTION BUG C255: Multiples threads appelaient btc_gen9_reopen_drm()
 * simultanément → race condition sur destruction contextes.
 * 
 * SOLUTION: Mutex global pour sérialiser les reopen.
 * 
 * @param ctx Contexte Gen9
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_reopen_drm_threadsafe(btc_gen9_context_t* ctx);

/**
 * Attendre multiples fences en parallèle
 * 
 * Attend la fin de tous les dispatches correspondant aux fences.
 * Les dispatches s'exécutent en parallèle sur le GPU.
 * 
 * @param fences Tableau de fences
 * @param count Nombre de fences
 * @return Nombre de fences réussies
 */
int btc_gen9_wait_fences_parallel(const btc_gen9_fence_t* fences, int count);

#endif /* BTC_GEN9_ASYNC_SUBMIT_H */

// Made with Bob
