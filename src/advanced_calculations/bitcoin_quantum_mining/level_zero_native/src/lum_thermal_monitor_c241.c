/* LumVorax C241 — Thermal Monitoring Native Implementation
 * 0% OpenCL, 100% Native Linux Sensors
 * Device: Intel UHD Graphics 620 (Gen9)
 */

#define _POSIX_C_SOURCE 199309L

#include "lum_thermal_monitor_c241.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>

/* Internal context structure */
struct lum_thermal_monitor_s {
    lum_thermal_config_t config;
    FILE* log_file;
    
    /* Sensor paths */
    char cpu_temp_path[256];
    char cpu_freq_path[256];
    char cpu_freq_max_path[256];
    char gpu_temp_path[256];
    char gpu_freq_cur_path[256];
    char gpu_freq_max_path[256];
    char gpu_freq_min_path[256];
    char gpu_rc6_path[256];
    char power_draw_path[256];
    
    /* Statistics */
    lum_thermal_stats_t stats;
    
    /* Correlation data */
    double* dispatch_times;
    double* temps;
    double* freqs;
    size_t correlation_count;
    size_t correlation_capacity;
};

/* Helper: Get nanosecond timestamp */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Helper: Read integer from sysfs */
static int read_sysfs_int(const char* path, uint32_t* value) {
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    
    int ret = fscanf(f, "%u", value);
    fclose(f);
    
    return (ret == 1) ? 0 : -1;
}

/* Helper: Read double from sysfs */
static int read_sysfs_double(const char* path, double* value) {
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    
    int ret = fscanf(f, "%lf", value);
    fclose(f);
    
    return (ret == 1) ? 0 : -1;
}

/* Helper: Find thermal zone for CPU */
static int find_cpu_thermal_zone(char* path_out, size_t path_size) {
    for (int i = 0; i < 20; i++) {
        char type_path[256];
        snprintf(type_path, sizeof(type_path), "/sys/class/thermal/thermal_zone%d/type", i);
        
        FILE* f = fopen(type_path, "r");
        if (!f) continue;
        
        char type[64];
        if (fgets(type, sizeof(type), f)) {
            fclose(f);
            
            /* Look for x86_pkg_temp or coretemp */
            if (strstr(type, "x86_pkg_temp") || strstr(type, "coretemp")) {
                snprintf(path_out, path_size, "/sys/class/thermal/thermal_zone%d/temp", i);
                return 0;
            }
        } else {
            fclose(f);
        }
    }
    
    return -1;
}

/* Helper: Find GPU temperature sensor */
static int find_gpu_temp_sensor(char* path_out, size_t path_size) {
    /* Try DRM card0 hwmon */
    DIR* dir = opendir("/sys/class/drm/card0/device/hwmon");
    if (!dir) return -1;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) == 0) {
            /* Found hwmon directory */
            char temp_path[256];
            snprintf(temp_path, sizeof(temp_path), 
                    "/sys/class/drm/card0/device/hwmon/%s/temp1_input", entry->d_name);
            
            /* Check if file exists */
            if (access(temp_path, R_OK) == 0) {
                snprintf(path_out, path_size, "%s", temp_path);
                closedir(dir);
                return 0;
            }
        }
    }
    
    closedir(dir);
    return -1;
}

/* Initialize thermal monitor */
int lum_thermal_init(lum_thermal_monitor_t** monitor_out, const lum_thermal_config_t* config) {
    if (!monitor_out) return -1;
    
    lum_thermal_monitor_t* monitor = calloc(1, sizeof(lum_thermal_monitor_t));
    if (!monitor) return -1;
    
    /* Copy configuration */
    if (config) {
        monitor->config = *config;
    } else {
        lum_thermal_config_t default_config = LUM_THERMAL_DEFAULT_CONFIG;
        monitor->config = default_config;
    }
    
    /* Find CPU thermal zone */
    if (monitor->config.enable_cpu_temp) {
        if (find_cpu_thermal_zone(monitor->cpu_temp_path, sizeof(monitor->cpu_temp_path)) != 0) {
            fprintf(stderr, "[THERMAL] Warning: CPU thermal zone not found\n");
        }
    }
    
    /* CPU frequency paths */
    if (monitor->config.enable_cpu_freq) {
        snprintf(monitor->cpu_freq_path, sizeof(monitor->cpu_freq_path),
                "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        snprintf(monitor->cpu_freq_max_path, sizeof(monitor->cpu_freq_max_path),
                "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    }
    
    /* Find GPU temperature sensor */
    if (monitor->config.enable_gpu_temp) {
        if (find_gpu_temp_sensor(monitor->gpu_temp_path, sizeof(monitor->gpu_temp_path)) != 0) {
            fprintf(stderr, "[THERMAL] Warning: GPU temperature sensor not found\n");
        }
    }
    
    /* GPU frequency paths */
    if (monitor->config.enable_gpu_freq) {
        snprintf(monitor->gpu_freq_cur_path, sizeof(monitor->gpu_freq_cur_path),
                "/sys/class/drm/card0/gt_cur_freq_mhz");
        snprintf(monitor->gpu_freq_max_path, sizeof(monitor->gpu_freq_max_path),
                "/sys/class/drm/card0/gt_max_freq_mhz");
        snprintf(monitor->gpu_freq_min_path, sizeof(monitor->gpu_freq_min_path),
                "/sys/class/drm/card0/gt_min_freq_mhz");
    }
    
    /* GPU RC6 path */
    if (monitor->config.enable_gpu_rc6) {
        snprintf(monitor->gpu_rc6_path, sizeof(monitor->gpu_rc6_path),
                "/sys/class/drm/card0/power/rc6_residency_ms");
    }
    
    /* Power draw path (if available) */
    if (monitor->config.enable_power_draw) {
        snprintf(monitor->power_draw_path, sizeof(monitor->power_draw_path),
                "/sys/class/hwmon/hwmon0/power1_average");
    }
    
    /* Open log file */
    if (monitor->config.log_path && monitor->config.enable_auto_logging) {
        monitor->log_file = fopen(monitor->config.log_path, "w");
        if (monitor->log_file) {
            /* Write CSV header */
            fprintf(monitor->log_file, "timestamp_ns,timestamp_real,");
            fprintf(monitor->log_file, "cpu_temp_c,cpu_freq_mhz,cpu_freq_max_mhz,");
            fprintf(monitor->log_file, "gpu_temp_c,gpu_freq_mhz,gpu_freq_max_mhz,gpu_freq_min_mhz,");
            fprintf(monitor->log_file, "gpu_rc6_percent,gpu_rc6_residency_ms,");
            fprintf(monitor->log_file, "power_draw_w\n");
            fflush(monitor->log_file);
        }
    }
    
    /* Initialize statistics */
    monitor->stats.cpu_temp_min = 1000.0;
    monitor->stats.cpu_temp_max = -1000.0;
    monitor->stats.gpu_temp_min = 1000.0;
    monitor->stats.gpu_temp_max = -1000.0;
    monitor->stats.cpu_freq_min = UINT32_MAX;
    monitor->stats.cpu_freq_max = 0;
    monitor->stats.gpu_freq_min = UINT32_MAX;
    monitor->stats.gpu_freq_max = 0;
    monitor->stats.gpu_rc6_min = UINT32_MAX;
    monitor->stats.gpu_rc6_max = 0;
    monitor->stats.power_draw_min = 1000.0;
    monitor->stats.power_draw_max = 0.0;
    
    /* Initialize correlation arrays */
    monitor->correlation_capacity = 10000;
    monitor->dispatch_times = calloc(monitor->correlation_capacity, sizeof(double));
    monitor->temps = calloc(monitor->correlation_capacity, sizeof(double));
    monitor->freqs = calloc(monitor->correlation_capacity, sizeof(double));
    
    *monitor_out = monitor;
    return 0;
}

/* Take thermal snapshot */
int lum_thermal_snapshot(lum_thermal_monitor_t* monitor, lum_thermal_snapshot_t* snapshot) {
    if (!monitor || !snapshot) return -1;
    
    memset(snapshot, 0, sizeof(lum_thermal_snapshot_t));
    
    /* Timestamp */
    snapshot->timestamp_ns = get_timestamp_ns();
    clock_gettime(CLOCK_REALTIME, &snapshot->timestamp_real);
    
    /* CPU temperature */
    if (monitor->config.enable_cpu_temp && monitor->cpu_temp_path[0]) {
        uint32_t temp_millidegrees;
        if (read_sysfs_int(monitor->cpu_temp_path, &temp_millidegrees) == 0) {
            snapshot->cpu_temp_celsius = temp_millidegrees / 1000.0;
            snapshot->cpu_temp_valid = true;
        }
    }
    
    /* CPU frequency */
    if (monitor->config.enable_cpu_freq && monitor->cpu_freq_path[0]) {
        uint32_t freq_khz;
        if (read_sysfs_int(monitor->cpu_freq_path, &freq_khz) == 0) {
            snapshot->cpu_freq_mhz = freq_khz / 1000;
            snapshot->cpu_freq_valid = true;
        }
        
        uint32_t freq_max_khz;
        if (read_sysfs_int(monitor->cpu_freq_max_path, &freq_max_khz) == 0) {
            snapshot->cpu_freq_max_mhz = freq_max_khz / 1000;
        }
    }
    
    /* GPU temperature */
    if (monitor->config.enable_gpu_temp && monitor->gpu_temp_path[0]) {
        uint32_t temp_millidegrees;
        if (read_sysfs_int(monitor->gpu_temp_path, &temp_millidegrees) == 0) {
            snapshot->gpu_temp_celsius = temp_millidegrees / 1000.0;
            snapshot->gpu_temp_valid = true;
        }
    }
    
    /* GPU frequency */
    if (monitor->config.enable_gpu_freq && monitor->gpu_freq_cur_path[0]) {
        if (read_sysfs_int(monitor->gpu_freq_cur_path, &snapshot->gpu_freq_mhz) == 0) {
            snapshot->gpu_freq_valid = true;
        }
        read_sysfs_int(monitor->gpu_freq_max_path, &snapshot->gpu_freq_max_mhz);
        read_sysfs_int(monitor->gpu_freq_min_path, &snapshot->gpu_freq_min_mhz);
    }
    
    /* GPU RC6 */
    if (monitor->config.enable_gpu_rc6 && monitor->gpu_rc6_path[0]) {
        uint32_t rc6_ms;
        if (read_sysfs_int(monitor->gpu_rc6_path, &rc6_ms) == 0) {
            snapshot->gpu_rc6_residency_ms = rc6_ms;
            
            /* Calculate RC6 percentage (approximation) */
            static uint64_t last_timestamp_ns = 0;
            static uint64_t last_rc6_ms = 0;
            
            if (last_timestamp_ns > 0) {
                uint64_t time_delta_ms = (snapshot->timestamp_ns - last_timestamp_ns) / 1000000;
                uint64_t rc6_delta_ms = rc6_ms - last_rc6_ms;
                
                if (time_delta_ms > 0) {
                    snapshot->gpu_rc6_percent = (uint32_t)((rc6_delta_ms * 100) / time_delta_ms);
                    if (snapshot->gpu_rc6_percent > 100) snapshot->gpu_rc6_percent = 100;
                }
            }
            
            last_timestamp_ns = snapshot->timestamp_ns;
            last_rc6_ms = rc6_ms;
            snapshot->gpu_rc6_valid = true;
        }
    }
    
    /* Power draw */
    if (monitor->config.enable_power_draw && monitor->power_draw_path[0]) {
        uint32_t power_microwatts;
        if (read_sysfs_int(monitor->power_draw_path, &power_microwatts) == 0) {
            snapshot->power_draw_watts = power_microwatts / 1000000.0;
            snapshot->power_draw_valid = true;
        }
    }
    
    /* Update statistics */
    monitor->stats.sample_count++;
    
    if (snapshot->cpu_temp_valid) {
        if (snapshot->cpu_temp_celsius < monitor->stats.cpu_temp_min)
            monitor->stats.cpu_temp_min = snapshot->cpu_temp_celsius;
        if (snapshot->cpu_temp_celsius > monitor->stats.cpu_temp_max)
            monitor->stats.cpu_temp_max = snapshot->cpu_temp_celsius;
        monitor->stats.cpu_temp_avg += (snapshot->cpu_temp_celsius - monitor->stats.cpu_temp_avg) / monitor->stats.sample_count;
    }
    
    if (snapshot->cpu_freq_valid) {
        if (snapshot->cpu_freq_mhz < monitor->stats.cpu_freq_min)
            monitor->stats.cpu_freq_min = snapshot->cpu_freq_mhz;
        if (snapshot->cpu_freq_mhz > monitor->stats.cpu_freq_max)
            monitor->stats.cpu_freq_max = snapshot->cpu_freq_mhz;
        monitor->stats.cpu_freq_avg += (snapshot->cpu_freq_mhz - monitor->stats.cpu_freq_avg) / monitor->stats.sample_count;
    }
    
    if (snapshot->gpu_temp_valid) {
        if (snapshot->gpu_temp_celsius < monitor->stats.gpu_temp_min)
            monitor->stats.gpu_temp_min = snapshot->gpu_temp_celsius;
        if (snapshot->gpu_temp_celsius > monitor->stats.gpu_temp_max)
            monitor->stats.gpu_temp_max = snapshot->gpu_temp_celsius;
        monitor->stats.gpu_temp_avg += (snapshot->gpu_temp_celsius - monitor->stats.gpu_temp_avg) / monitor->stats.sample_count;
    }
    
    if (snapshot->gpu_freq_valid) {
        if (snapshot->gpu_freq_mhz < monitor->stats.gpu_freq_min)
            monitor->stats.gpu_freq_min = snapshot->gpu_freq_mhz;
        if (snapshot->gpu_freq_mhz > monitor->stats.gpu_freq_max)
            monitor->stats.gpu_freq_max = snapshot->gpu_freq_mhz;
        monitor->stats.gpu_freq_avg += (snapshot->gpu_freq_mhz - monitor->stats.gpu_freq_avg) / monitor->stats.sample_count;
    }
    
    if (snapshot->gpu_rc6_valid) {
        if (snapshot->gpu_rc6_percent < monitor->stats.gpu_rc6_min)
            monitor->stats.gpu_rc6_min = snapshot->gpu_rc6_percent;
        if (snapshot->gpu_rc6_percent > monitor->stats.gpu_rc6_max)
            monitor->stats.gpu_rc6_max = snapshot->gpu_rc6_percent;
        monitor->stats.gpu_rc6_avg += (snapshot->gpu_rc6_percent - monitor->stats.gpu_rc6_avg) / monitor->stats.sample_count;
    }
    
    if (snapshot->power_draw_valid) {
        if (snapshot->power_draw_watts < monitor->stats.power_draw_min)
            monitor->stats.power_draw_min = snapshot->power_draw_watts;
        if (snapshot->power_draw_watts > monitor->stats.power_draw_max)
            monitor->stats.power_draw_max = snapshot->power_draw_watts;
        monitor->stats.power_draw_avg += (snapshot->power_draw_watts - monitor->stats.power_draw_avg) / monitor->stats.sample_count;
    }
    
    /* Auto-log if enabled */
    if (monitor->config.enable_auto_logging) {
        lum_thermal_log_snapshot(monitor, snapshot);
    }
    
    return 0;
}

/* Get thermal statistics */
int lum_thermal_get_stats(lum_thermal_monitor_t* monitor, lum_thermal_stats_t* stats) {
    if (!monitor || !stats) return -1;
    *stats = monitor->stats;
    return 0;
}

/* Reset thermal statistics */
int lum_thermal_reset_stats(lum_thermal_monitor_t* monitor) {
    if (!monitor) return -1;
    
    memset(&monitor->stats, 0, sizeof(lum_thermal_stats_t));
    monitor->stats.cpu_temp_min = 1000.0;
    monitor->stats.cpu_temp_max = -1000.0;
    monitor->stats.gpu_temp_min = 1000.0;
    monitor->stats.gpu_temp_max = -1000.0;
    monitor->stats.cpu_freq_min = UINT32_MAX;
    monitor->stats.cpu_freq_max = 0;
    monitor->stats.gpu_freq_min = UINT32_MAX;
    monitor->stats.gpu_freq_max = 0;
    monitor->stats.gpu_rc6_min = UINT32_MAX;
    monitor->stats.gpu_rc6_max = 0;
    monitor->stats.power_draw_min = 1000.0;
    monitor->stats.power_draw_max = 0.0;
    
    monitor->correlation_count = 0;
    
    return 0;
}

/* Log thermal snapshot to file */
int lum_thermal_log_snapshot(lum_thermal_monitor_t* monitor, const lum_thermal_snapshot_t* snapshot) {
    if (!monitor || !snapshot || !monitor->log_file) return -1;
    
    fprintf(monitor->log_file, "%lu,%ld.%09ld,",
            snapshot->timestamp_ns,
            snapshot->timestamp_real.tv_sec,
            snapshot->timestamp_real.tv_nsec);
    
    fprintf(monitor->log_file, "%.2f,%u,%u,",
            snapshot->cpu_temp_valid ? snapshot->cpu_temp_celsius : -1.0,
            snapshot->cpu_freq_valid ? snapshot->cpu_freq_mhz : 0,
            snapshot->cpu_freq_max_mhz);
    
    fprintf(monitor->log_file, "%.2f,%u,%u,%u,",
            snapshot->gpu_temp_valid ? snapshot->gpu_temp_celsius : -1.0,
            snapshot->gpu_freq_valid ? snapshot->gpu_freq_mhz : 0,
            snapshot->gpu_freq_max_mhz,
            snapshot->gpu_freq_min_mhz);
    
    fprintf(monitor->log_file, "%u,%lu,",
            snapshot->gpu_rc6_valid ? snapshot->gpu_rc6_percent : 0,
            snapshot->gpu_rc6_residency_ms);
    
    fprintf(monitor->log_file, "%.2f\n",
            snapshot->power_draw_valid ? snapshot->power_draw_watts : -1.0);
    
    return 0;
}

/* Flush log file */
int lum_thermal_flush_log(lum_thermal_monitor_t* monitor) {
    if (!monitor || !monitor->log_file) return -1;
    fflush(monitor->log_file);
    return 0;
}

/* Cleanup thermal monitor */
void lum_thermal_cleanup(lum_thermal_monitor_t* monitor) {
    if (!monitor) return;
    
    if (monitor->log_file) {
        fflush(monitor->log_file);
        fclose(monitor->log_file);
    }
    
    free(monitor->dispatch_times);
    free(monitor->temps);
    free(monitor->freqs);
    free(monitor);
}

/* Print thermal snapshot */
void lum_thermal_print_snapshot(const lum_thermal_snapshot_t* snapshot) {
    if (!snapshot) return;
    
    printf("[THERMAL SNAPSHOT]\n");
    printf("  Timestamp: %lu ns\n", snapshot->timestamp_ns);
    
    if (snapshot->cpu_temp_valid)
        printf("  CPU Temp: %.2f °C\n", snapshot->cpu_temp_celsius);
    if (snapshot->cpu_freq_valid)
        printf("  CPU Freq: %u MHz (max: %u MHz)\n", snapshot->cpu_freq_mhz, snapshot->cpu_freq_max_mhz);
    
    if (snapshot->gpu_temp_valid)
        printf("  GPU Temp: %.2f °C\n", snapshot->gpu_temp_celsius);
    if (snapshot->gpu_freq_valid)
        printf("  GPU Freq: %u MHz (min: %u, max: %u MHz)\n", 
               snapshot->gpu_freq_mhz, snapshot->gpu_freq_min_mhz, snapshot->gpu_freq_max_mhz);
    
    if (snapshot->gpu_rc6_valid)
        printf("  GPU RC6: %u%% (residency: %lu ms)\n", snapshot->gpu_rc6_percent, snapshot->gpu_rc6_residency_ms);
    
    if (snapshot->power_draw_valid)
        printf("  Power Draw: %.2f W\n", snapshot->power_draw_watts);
}

/* Print thermal statistics */
void lum_thermal_print_stats(const lum_thermal_stats_t* stats) {
    if (!stats) return;
    
    printf("[THERMAL STATISTICS] (samples: %lu)\n", stats->sample_count);
    printf("  CPU Temp: min=%.2f avg=%.2f max=%.2f °C\n",
           stats->cpu_temp_min, stats->cpu_temp_avg, stats->cpu_temp_max);
    printf("  CPU Freq: min=%u avg=%u max=%u MHz\n",
           stats->cpu_freq_min, stats->cpu_freq_avg, stats->cpu_freq_max);
    printf("  GPU Temp: min=%.2f avg=%.2f max=%.2f °C\n",
           stats->gpu_temp_min, stats->gpu_temp_avg, stats->gpu_temp_max);
    printf("  GPU Freq: min=%u avg=%u max=%u MHz\n",
           stats->gpu_freq_min, stats->gpu_freq_avg, stats->gpu_freq_max);
    printf("  GPU RC6: min=%u avg=%u max=%u %%\n",
           stats->gpu_rc6_min, stats->gpu_rc6_avg, stats->gpu_rc6_max);
    printf("  Power Draw: min=%.2f avg=%.2f max=%.2f W\n",
           stats->power_draw_min, stats->power_draw_avg, stats->power_draw_max);
}

/* Detect thermal throttling */
bool lum_thermal_detect_throttling(lum_thermal_monitor_t* monitor, 
                                   double threshold_temp,
                                   double threshold_freq_drop) {
    if (!monitor) return false;
    
    lum_thermal_snapshot_t snapshot;
    if (lum_thermal_snapshot(monitor, &snapshot) != 0) return false;
    
    /* Check GPU temperature */
    if (snapshot.gpu_temp_valid && snapshot.gpu_temp_celsius > threshold_temp) {
        /* Check GPU frequency drop */
        if (snapshot.gpu_freq_valid && snapshot.gpu_freq_max_mhz > 0) {
            double freq_ratio = (double)snapshot.gpu_freq_mhz / snapshot.gpu_freq_max_mhz;
            double freq_drop = (1.0 - freq_ratio) * 100.0;
            
            if (freq_drop > threshold_freq_drop) {
                return true;
            }
        }
    }
    
    return false;
}

/* Correlate dispatch time with thermal data */
int lum_thermal_correlate_dispatch(lum_thermal_monitor_t* monitor,
                                   double dispatch_time_ms,
                                   double* correlation_temp,
                                   double* correlation_freq) {
    if (!monitor || !correlation_temp || !correlation_freq) return -1;
    
    /* Take snapshot */
    lum_thermal_snapshot_t snapshot;
    if (lum_thermal_snapshot(monitor, &snapshot) != 0) return -1;
    
    /* Store data for correlation */
    if (monitor->correlation_count < monitor->correlation_capacity) {
        monitor->dispatch_times[monitor->correlation_count] = dispatch_time_ms;
        monitor->temps[monitor->correlation_count] = snapshot.gpu_temp_valid ? snapshot.gpu_temp_celsius : 0.0;
        monitor->freqs[monitor->correlation_count] = snapshot.gpu_freq_valid ? (double)snapshot.gpu_freq_mhz : 0.0;
        monitor->correlation_count++;
    }
    
    /* Calculate Pearson correlation if enough samples */
    if (monitor->correlation_count < 10) {
        *correlation_temp = 0.0;
        *correlation_freq = 0.0;
        return 0;
    }
    
    /* Calculate means */
    double mean_dispatch = 0.0, mean_temp = 0.0, mean_freq = 0.0;
    for (size_t i = 0; i < monitor->correlation_count; i++) {
        mean_dispatch += monitor->dispatch_times[i];
        mean_temp += monitor->temps[i];
        mean_freq += monitor->freqs[i];
    }
    mean_dispatch /= monitor->correlation_count;
    mean_temp /= monitor->correlation_count;
    mean_freq /= monitor->correlation_count;
    
    /* Calculate Pearson correlation */
    double sum_dispatch_temp = 0.0, sum_dispatch_freq = 0.0;
    double sum_dispatch_sq = 0.0, sum_temp_sq = 0.0, sum_freq_sq = 0.0;
    
    for (size_t i = 0; i < monitor->correlation_count; i++) {
        double d_dispatch = monitor->dispatch_times[i] - mean_dispatch;
        double d_temp = monitor->temps[i] - mean_temp;
        double d_freq = monitor->freqs[i] - mean_freq;
        
        sum_dispatch_temp += d_dispatch * d_temp;
        sum_dispatch_freq += d_dispatch * d_freq;
        sum_dispatch_sq += d_dispatch * d_dispatch;
        sum_temp_sq += d_temp * d_temp;
        sum_freq_sq += d_freq * d_freq;
    }
    
    double denom_temp = sqrt(sum_dispatch_sq * sum_temp_sq);
    double denom_freq = sqrt(sum_dispatch_sq * sum_freq_sq);
    
    *correlation_temp = (denom_temp > 0.0) ? (sum_dispatch_temp / denom_temp) : 0.0;
    *correlation_freq = (denom_freq > 0.0) ? (sum_dispatch_freq / denom_freq) : 0.0;
    
    return 0;
}

// Made with Bob
