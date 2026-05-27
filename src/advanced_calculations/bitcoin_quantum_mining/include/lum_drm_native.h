/* lum_drm_native.h - Accès DRM natif wrapper
 * Utilise les headers système DRM pour éviter les conflits
 * 100% compatible avec libdrm system headers
 */

#ifndef LUM_DRM_NATIVE_H
#define LUM_DRM_NATIVE_H

#include <stdint.h>
#include <stddef.h>

/* API LumVorax DRM Native */
typedef struct {
    int fd;                    /* File descriptor /dev/dri/renderD128 */
    int version_major;
    int version_minor;
    char driver_name[64];
    int initialized;
} lum_drm_context_t;

/* Initialisation */
int lum_drm_init(lum_drm_context_t *ctx);
void lum_drm_cleanup(lum_drm_context_t *ctx);

/* Pool de contextes DRM (OPTIMISATION C193) */
int lum_drm_pool_init(void);
lum_drm_context_t* lum_drm_pool_get(void);
void lum_drm_pool_cleanup(void);

/* GEM Buffer Object */
int lum_drm_gem_create(lum_drm_context_t *ctx, uint64_t size, uint32_t *handle_out);
int lum_drm_gem_mmap(lum_drm_context_t *ctx, uint32_t handle, uint64_t size, void **addr_out);
int lum_drm_gem_close(lum_drm_context_t *ctx, uint32_t handle);

/* GEM Batch Operations (OPTIMISATION C193) */
int lum_drm_gem_create_batch(lum_drm_context_t *ctx, uint64_t size, uint32_t count, uint32_t *handles_out);
int lum_drm_gem_mmap_batch(lum_drm_context_t *ctx, uint32_t *handles, uint64_t size, uint32_t count, void **addrs_out);
int lum_drm_gem_close_batch(lum_drm_context_t *ctx, uint32_t *handles, uint32_t count);

/* Execution */
int lum_drm_execbuffer(lum_drm_context_t *ctx, uint32_t *handles, uint32_t count);

/* Utilitaires */
const char* lum_drm_get_version(lum_drm_context_t *ctx);
int lum_drm_check_i915(lum_drm_context_t *ctx);

#endif /* LUM_DRM_NATIVE_H */

// Made with Bob
