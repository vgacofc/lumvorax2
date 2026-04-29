/* ============================================================================
 * lum_aligned_alloc_safe.h — Cycle C134 (FIX-D2)
 * ----------------------------------------------------------------------------
 * Helper safe pour aligned_alloc(alignment, size) qui garantit :
 *
 *   1) size est arrondi au prochain multiple de alignment (POSIX/C11 § 7.22.3.1)
 *   2) alignment est une puissance de 2 (sinon NULL)
 *   3) En cas d'echec, fallback sur posix_memalign() qui retourne EINVAL
 *      au lieu d'avoir un comportement indefini.
 *
 * IMPORTANT : la memoire allouee se libere TOUJOURS avec free() — comportement
 * identique a aligned_alloc() pour faciliter la migration des sites existants.
 *
 * Sites cibles (cf. analysechatgpt133.1.md §2.1) :
 *   - src/vorax/vorax_operations.c:107
 *   - src/optimization/lockfree/lockfree_queue.c (3 sites)
 *   - src/tests/individual/test_simd_optimizer_individual.c:83
 *
 * Usage :
 *     void* p = lum_aligned_alloc_safe(64, 8 * parts);
 *     // au lieu de :
 *     void* p = aligned_alloc(64, 8 * parts);  // BUG si parts < 8
 * ============================================================================ */
#ifndef LUM_ALIGNED_ALLOC_SAFE_H
#define LUM_ALIGNED_ALLOC_SAFE_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Verifie que x est une puissance de 2 (>= 1). */
static inline int lum_is_power_of_two(size_t x) {
    return x > 0 && (x & (x - 1)) == 0;
}

/* Arrondit `size` au prochain multiple de `alignment` (alignment doit
 * etre une puissance de 2). Retourne 0 si overflow. */
static inline size_t lum_align_up(size_t size, size_t alignment) {
    if (!lum_is_power_of_two(alignment)) return 0;
    size_t mask = alignment - 1;
    /* Verification overflow (size + mask) */
    if (size > SIZE_MAX - mask) return 0;
    return (size + mask) & ~mask;
}

/* Allocation alignee SAFE. Garantit que :
 *   - alignment est une puissance de 2 (sinon NULL)
 *   - size est arrondi au prochain multiple de alignment AVANT appel
 *
 * Retourne NULL en cas d'erreur (au lieu d'UB).
 * La memoire DOIT etre liberee avec free(). */
static inline void* lum_aligned_alloc_safe(size_t alignment, size_t size) {
    if (!lum_is_power_of_two(alignment)) return NULL;
    if (size == 0) return NULL;

    size_t aligned_size = lum_align_up(size, alignment);
    if (aligned_size == 0) return NULL;  /* overflow */

    /* Tentative aligned_alloc (C11) avec size garanti multiple */
    void* p = aligned_alloc(alignment, aligned_size);
    if (p) return p;

    /* Fallback POSIX (errno-based, jamais d'UB) */
    void* q = NULL;
    int rc = posix_memalign(&q, alignment, aligned_size);
    if (rc == 0) return q;

    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* LUM_ALIGNED_ALLOC_SAFE_H */
