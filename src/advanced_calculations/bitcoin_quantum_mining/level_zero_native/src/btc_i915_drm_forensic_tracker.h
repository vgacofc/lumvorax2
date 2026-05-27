/**
 * LumVorax C255v3 — i915 DRM Forensic Tracker BIT-LEVEL
 * Header file
 */

#ifndef BTC_I915_DRM_FORENSIC_TRACKER_H
#define BTC_I915_DRM_FORENSIC_TRACKER_H

#include <stdint.h>
#include <stddef.h>

/**
 * Initialiser tracker forensique
 * 
 * @param log_dir Répertoire logs (ex: "logs/forensic")
 * @return 0 si succès, -1 si erreur
 */
int i915_forensic_tracker_init(const char* log_dir);

/**
 * Cleanup tracker
 */
void i915_forensic_tracker_cleanup(void);

/**
 * Wrapper ioctl forensique (remplace ioctl() standard)
 *
 * Trace AVANT et APRÈS chaque appel ioctl:
 * - Timestamp nanoseconde
 * - Thread ID
 * - Paramètres ioctl (structure complète)
 * - Code retour + errno
 * - Temps exécution
 *
 * @param fd File descriptor DRM
 * @param request Numéro requête ioctl
 * @param ... Arguments variables (structure ioctl)
 * @return Code retour ioctl
 */
int i915_forensic_ioctl_impl(int fd, unsigned long request, ...);

/**
 * Dump buffer object complet (hexadécimal)
 *
 * @param bo_handle Handle buffer object
 * @param addr Adresse mapping CPU
 * @param size Taille buffer
 */
void i915_forensic_dump_buffer(uint32_t bo_handle, void* addr, size_t size);

/**
 * Macro pour remplacer ioctl() par version forensique
 *
 * Usage automatique: tous les appels ioctl() sont interceptés
 */
#define ENABLE_I915_FORENSIC_TRACKING

#ifdef ENABLE_I915_FORENSIC_TRACKING
#define ioctl i915_forensic_ioctl_impl
#endif

#endif /* BTC_I915_DRM_FORENSIC_TRACKER_H */

// Made with Bob