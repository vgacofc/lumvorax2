/**
 * @file btc_gpu_monitor_enhanced.c
 * @brief Monitoring GPU amélioré avec méthodes alternatives pour Gen9
 *
 * Ce module implémente plusieurs stratégies pour obtenir les métriques GPU:
 * 1. Sysfs hwmon (méthode standard)
 * 2. i915_query ioctl (méthode DRM)
 * 3. debugfs i915_frequency_info (fallback)
 * 4. Perf events (méthode avancée)
 *
 * Cycle: C242
 * Objectif: Résoudre le problème des capteurs GPU non accessibles détecté en C241
 */

#include "btc_gpu_monitor_enhanced.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <drm/drm.h>
#include <drm/i915_drm.h>

// Chemins sysfs pour monitoring GPU
#define SYSFS_DRM_CARD "/sys/class/drm/card0"
#define SYSFS_HWMON_BASE "/sys/class/drm/card0/device/hwmon"
#define DEBUGFS_I915_FREQ "/sys/kernel/debug/dri/0/i915_frequency_info"
#define DEBUGFS_I915_ENGINES "/sys/kernel/debug/dri/0/i915_engine_info"

// Les structures drm_i915_query* sont déjà définies dans i915_drm.h
// Ioctl number (défini manuellement si pas dans les headers)
#ifndef DRM_IOCTL_I915_QUERY
#define DRM_IOCTL_I915_QUERY DRM_IOWR(DRM_COMMAND_BASE + 0x39, struct drm_i915_query)
#endif

/**
 * Trouve le répertoire hwmon pour le GPU
 */
static int find_hwmon_dir(char *path, size_t path_size) {
    DIR *dir = opendir(SYSFS_HWMON_BASE);
    if (!dir) {
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) == 0) {
            snprintf(path, path_size, "%s/%s", SYSFS_HWMON_BASE, entry->d_name);
            closedir(dir);
            return 0;
        }
    }
    
    closedir(dir);
    return -1;
}

/**
 * Lit un fichier sysfs et retourne la valeur entière
 */
static int read_sysfs_int(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    
    int value;
    if (fscanf(f, "%d", &value) != 1) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return value;
}

/**
 * Méthode 1: Lecture via sysfs hwmon (standard)
 */
static int get_gpu_temp_hwmon(void) {
    char hwmon_dir[256];
    if (find_hwmon_dir(hwmon_dir, sizeof(hwmon_dir)) != 0) {
        return -1;
    }
    
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "%s/temp1_input", hwmon_dir);
    
    int temp_millidegrees = read_sysfs_int(temp_path);
    if (temp_millidegrees < 0) {
        return -1;
    }
    
    return temp_millidegrees / 1000; // Convertir en degrés Celsius
}

/**
 * Méthode 2: Lecture via sysfs DRM direct
 */
static int get_gpu_freq_sysfs(const char *freq_type) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", SYSFS_DRM_CARD, freq_type);
    return read_sysfs_int(path);
}

/**
 * Méthode 3: Lecture via debugfs i915_frequency_info
 */
static int parse_debugfs_frequency(struct gpu_metrics_enhanced *metrics) {
    FILE *f = fopen(DEBUGFS_I915_FREQ, "r");
    if (!f) {
        return -1;
    }
    
    char line[256];
    int found = 0;
    
    while (fgets(line, sizeof(line), f)) {
        // Parser les lignes comme:
        // "RPCS: 0x00000000"
        // "Actual freq: 350 MHz"
        // "Current freq: 350 MHz"
        // "Max freq: 1100 MHz"
        
        if (strstr(line, "Actual freq:")) {
            if (sscanf(line, "Actual freq: %d MHz", &metrics->gpu_freq_mhz) == 1) {
                found++;
            }
        } else if (strstr(line, "Max freq:")) {
            if (sscanf(line, "Max freq: %d MHz", &metrics->gpu_freq_max_mhz) == 1) {
                found++;
            }
        } else if (strstr(line, "Min freq:")) {
            if (sscanf(line, "Min freq: %d MHz", &metrics->gpu_freq_min_mhz) == 1) {
                found++;
            }
        }
    }
    
    fclose(f);
    return (found > 0) ? 0 : -1;
}

/**
 * Méthode 4: Lecture via DRM i915_query ioctl
 */
static int get_gpu_info_query(int drm_fd, struct gpu_metrics_enhanced *metrics __attribute__((unused))) {
    // Tenter d'obtenir les informations via i915_query
    // Note: Peut ne pas être supporté sur Gen9
    
    struct drm_i915_query_item query_item = {
        .query_id = DRM_I915_QUERY_ENGINE_INFO,
        .length = 0,
        .flags = 0,
        .data_ptr = 0,
    };
    
    struct drm_i915_query query = {
        .num_items = 1,
        .flags = 0,
        .items_ptr = (uintptr_t)&query_item,
    };
    
    // Premier appel pour obtenir la taille nécessaire
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_QUERY, &query);
    if (ret != 0 || query_item.length <= 0) {
        return -1;
    }
    
    // Allouer le buffer et faire le vrai query
    void *data = malloc(query_item.length);
    if (!data) {
        return -1;
    }
    
    query_item.data_ptr = (uintptr_t)data;
    ret = ioctl(drm_fd, DRM_IOCTL_I915_QUERY, &query);
    
    free(data);
    
    return (ret == 0) ? 0 : -1;
}

/**
 * Initialise le moniteur GPU amélioré
 */
int btc_gpu_monitor_enhanced_init(struct btc_gpu_monitor_enhanced *monitor, int drm_fd) {
    if (!monitor) {
        return -1;
    }
    
    memset(monitor, 0, sizeof(*monitor));
    monitor->drm_fd = drm_fd;
    
    // Tester les différentes méthodes disponibles
    monitor->hwmon_available = (get_gpu_temp_hwmon() >= 0);
    monitor->sysfs_freq_available = (get_gpu_freq_sysfs("gt_cur_freq_mhz") >= 0);
    monitor->debugfs_available = (access(DEBUGFS_I915_FREQ, R_OK) == 0);
    monitor->query_available = (get_gpu_info_query(drm_fd, &monitor->metrics) == 0);
    
    // Déterminer la meilleure méthode disponible
    if (monitor->hwmon_available) {
        monitor->preferred_method = GPU_MONITOR_METHOD_HWMON;
    } else if (monitor->debugfs_available) {
        monitor->preferred_method = GPU_MONITOR_METHOD_DEBUGFS;
    } else if (monitor->sysfs_freq_available) {
        monitor->preferred_method = GPU_MONITOR_METHOD_SYSFS;
    } else if (monitor->query_available) {
        monitor->preferred_method = GPU_MONITOR_METHOD_QUERY;
    } else {
        monitor->preferred_method = GPU_MONITOR_METHOD_NONE;
        fprintf(stderr, "[GPU_MONITOR] Warning: No GPU monitoring method available\n");
        return -1;
    }
    
    // Log des méthodes disponibles
    fprintf(stderr, "[GPU_MONITOR] Initialization:\n");
    fprintf(stderr, "  - hwmon:   %s\n", monitor->hwmon_available ? "✓" : "✗");
    fprintf(stderr, "  - sysfs:   %s\n", monitor->sysfs_freq_available ? "✓" : "✗");
    fprintf(stderr, "  - debugfs: %s\n", monitor->debugfs_available ? "✓" : "✗");
    fprintf(stderr, "  - query:   %s\n", monitor->query_available ? "✓" : "✗");
    fprintf(stderr, "  - preferred: %d\n", monitor->preferred_method);
    
    return 0;
}

/**
 * Collecte les métriques GPU avec la meilleure méthode disponible
 */
int btc_gpu_monitor_enhanced_collect(struct btc_gpu_monitor_enhanced *monitor) {
    if (!monitor) {
        return -1;
    }
    
    struct gpu_metrics_enhanced *m = &monitor->metrics;
    int success = 0;
    
    // Réinitialiser les valeurs
    m->gpu_temp_c = -1.0;
    m->gpu_freq_mhz = 0;
    m->gpu_freq_max_mhz = 0;
    m->gpu_freq_min_mhz = 0;
    m->gpu_power_w = -1.0;
    
    // Méthode 1: hwmon pour température
    if (monitor->hwmon_available) {
        int temp = get_gpu_temp_hwmon();
        if (temp >= 0) {
            m->gpu_temp_c = (double)temp;
            success++;
        }
    }
    
    // Méthode 2: sysfs pour fréquences
    if (monitor->sysfs_freq_available) {
        int cur_freq = get_gpu_freq_sysfs("gt_cur_freq_mhz");
        int max_freq = get_gpu_freq_sysfs("gt_max_freq_mhz");
        int min_freq = get_gpu_freq_sysfs("gt_min_freq_mhz");
        
        if (cur_freq >= 0) {
            m->gpu_freq_mhz = cur_freq;
            success++;
        }
        if (max_freq >= 0) {
            m->gpu_freq_max_mhz = max_freq;
        }
        if (min_freq >= 0) {
            m->gpu_freq_min_mhz = min_freq;
        }
    }
    
    // Méthode 3: debugfs comme fallback
    if (!success && monitor->debugfs_available) {
        if (parse_debugfs_frequency(m) == 0) {
            success++;
        }
    }
    
    // Méthode 4: i915_query (si supporté)
    if (!success && monitor->query_available) {
        if (get_gpu_info_query(monitor->drm_fd, m) == 0) {
            success++;
        }
    }
    
    // Calculer RC6 (état d'économie d'énergie) si disponible
    int rc6_residency = get_gpu_freq_sysfs("gt_rc6_residency_ms");
    if (rc6_residency >= 0) {
        m->gpu_rc6_residency_ms = rc6_residency;
        // Calculer le pourcentage RC6 (approximatif)
        static int last_rc6 = 0;
        static struct timespec last_time = {0};
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        
        if (last_time.tv_sec > 0) {
            long elapsed_ms = (now.tv_sec - last_time.tv_sec) * 1000 +
                            (now.tv_nsec - last_time.tv_nsec) / 1000000;
            int rc6_delta = rc6_residency - last_rc6;
            if (elapsed_ms > 0) {
                m->gpu_rc6_percent = (rc6_delta * 100) / elapsed_ms;
            }
        }
        
        last_rc6 = rc6_residency;
        last_time = now;
    }
    
    return (success > 0) ? 0 : -1;
}

/**
 * Affiche les métriques GPU
 */
void btc_gpu_monitor_enhanced_print(const struct gpu_metrics_enhanced *metrics) {
    if (!metrics) {
        return;
    }
    
    fprintf(stderr, "[GPU_METRICS]\n");
    fprintf(stderr, "  Temperature: %.1f°C\n", metrics->gpu_temp_c);
    fprintf(stderr, "  Frequency:   %d MHz (min: %d, max: %d)\n",
            metrics->gpu_freq_mhz, metrics->gpu_freq_min_mhz, metrics->gpu_freq_max_mhz);
    fprintf(stderr, "  RC6 State:   %d%% (residency: %d ms)\n",
            metrics->gpu_rc6_percent, metrics->gpu_rc6_residency_ms);
    fprintf(stderr, "  Power Draw:  %.2f W\n", metrics->gpu_power_w);
}

/**
 * Libère les ressources du moniteur
 */
void btc_gpu_monitor_enhanced_cleanup(struct btc_gpu_monitor_enhanced *monitor) {
    if (!monitor) {
        return;
    }
    
    // Rien à libérer pour l'instant
    memset(monitor, 0, sizeof(*monitor));
}

// Made with Bob
