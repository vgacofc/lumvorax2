/**
 * @file btc_gpu_monitor_enhanced.h
 * @brief Header pour monitoring GPU amélioré avec méthodes alternatives
 * 
 * Cycle: C242
 * Objectif: Résoudre le problème des capteurs GPU non accessibles détecté en C241
 */

#ifndef BTC_GPU_MONITOR_ENHANCED_H
#define BTC_GPU_MONITOR_ENHANCED_H

#include <stdint.h>
#include <time.h>

/**
 * Méthodes de monitoring GPU disponibles
 */
enum gpu_monitor_method {
    GPU_MONITOR_METHOD_NONE = 0,    // Aucune méthode disponible
    GPU_MONITOR_METHOD_HWMON,       // Sysfs hwmon (standard)
    GPU_MONITOR_METHOD_SYSFS,       // Sysfs DRM direct
    GPU_MONITOR_METHOD_DEBUGFS,     // Debugfs i915_frequency_info
    GPU_MONITOR_METHOD_QUERY,       // DRM i915_query ioctl
    GPU_MONITOR_METHOD_PERF,        // Perf events (avancé)
};

/**
 * Structure des métriques GPU améliorées
 */
struct gpu_metrics_enhanced {
    // Température
    double gpu_temp_c;              // Température GPU en °C (-1.0 si non disponible)
    
    // Fréquences
    int gpu_freq_mhz;               // Fréquence actuelle en MHz
    int gpu_freq_max_mhz;           // Fréquence maximale en MHz
    int gpu_freq_min_mhz;           // Fréquence minimale en MHz
    
    // État d'économie d'énergie
    int gpu_rc6_percent;            // Pourcentage de temps en RC6 (idle)
    int gpu_rc6_residency_ms;       // Temps total en RC6 en millisecondes
    
    // Puissance
    double gpu_power_w;             // Consommation en Watts (-1.0 si non disponible)
    
    // Timestamps
    struct timespec timestamp;      // Timestamp de la collecte
};

/**
 * Structure du moniteur GPU amélioré
 */
struct btc_gpu_monitor_enhanced {
    int drm_fd;                     // File descriptor DRM
    
    // Méthodes disponibles
    int hwmon_available;            // hwmon disponible
    int sysfs_freq_available;       // sysfs fréquences disponible
    int debugfs_available;          // debugfs disponible
    int query_available;            // i915_query disponible
    int perf_available;             // perf events disponible
    
    // Méthode préférée
    enum gpu_monitor_method preferred_method;
    
    // Métriques actuelles
    struct gpu_metrics_enhanced metrics;
};

/**
 * Initialise le moniteur GPU amélioré
 * 
 * @param monitor Pointeur vers la structure du moniteur
 * @param drm_fd File descriptor DRM ouvert
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int btc_gpu_monitor_enhanced_init(struct btc_gpu_monitor_enhanced *monitor, int drm_fd);

/**
 * Collecte les métriques GPU avec la meilleure méthode disponible
 * 
 * @param monitor Pointeur vers la structure du moniteur
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int btc_gpu_monitor_enhanced_collect(struct btc_gpu_monitor_enhanced *monitor);

/**
 * Affiche les métriques GPU
 * 
 * @param metrics Pointeur vers les métriques à afficher
 */
void btc_gpu_monitor_enhanced_print(const struct gpu_metrics_enhanced *metrics);

/**
 * Libère les ressources du moniteur
 * 
 * @param monitor Pointeur vers la structure du moniteur
 */
void btc_gpu_monitor_enhanced_cleanup(struct btc_gpu_monitor_enhanced *monitor);

#endif // BTC_GPU_MONITOR_ENHANCED_H

// Made with Bob
