/* LumVorax C241 — Thermal Monitoring Native Module
 * 0% OpenCL, 100% Native Linux Sensors
 * Device: Intel UHD Graphics 620 (Gen9)
 */

#ifndef LUM_THERMAL_MONITOR_C241_H
#define LUM_THERMAL_MONITOR_C241_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Thermal snapshot structure */
typedef struct {
    /* Timestamps */
    uint64_t timestamp_ns;           /* Nanosecond timestamp */
    struct timespec timestamp_real;  /* Real time */
    
    /* CPU Thermal */
    double cpu_temp_celsius;         /* CPU temperature (°C) */
    uint32_t cpu_freq_mhz;           /* CPU frequency (MHz) */
    uint32_t cpu_freq_max_mhz;       /* CPU max frequency (MHz) */
    
    /* GPU Thermal */
    double gpu_temp_celsius;         /* GPU temperature (°C) */
    uint32_t gpu_freq_mhz;           /* GPU current frequency (MHz) */
    uint32_t gpu_freq_max_mhz;       /* GPU max frequency (MHz) */
    uint32_t gpu_freq_min_mhz;       /* GPU min frequency (MHz) */
    
    /* GPU Power States */
    uint32_t gpu_rc6_percent;        /* RC6 residency (power saving %) */
    uint64_t gpu_rc6_residency_ms;   /* RC6 residency (milliseconds) */
    
    /* Power Draw */
    double power_draw_watts;         /* Power draw (Watts) */
    
    /* Validity flags */
    bool cpu_temp_valid;
    bool cpu_freq_valid;
    bool gpu_temp_valid;
    bool gpu_freq_valid;
    bool gpu_rc6_valid;
    bool power_draw_valid;
} lum_thermal_snapshot_t;

/* Thermal monitor context */
typedef struct lum_thermal_monitor_s lum_thermal_monitor_t;

/* Thermal statistics */
typedef struct {
    /* CPU stats */
    double cpu_temp_min;
    double cpu_temp_max;
    double cpu_temp_avg;
    uint32_t cpu_freq_min;
    uint32_t cpu_freq_max;
    uint32_t cpu_freq_avg;
    
    /* GPU stats */
    double gpu_temp_min;
    double gpu_temp_max;
    double gpu_temp_avg;
    uint32_t gpu_freq_min;
    uint32_t gpu_freq_max;
    uint32_t gpu_freq_avg;
    
    /* RC6 stats */
    uint32_t gpu_rc6_min;
    uint32_t gpu_rc6_max;
    uint32_t gpu_rc6_avg;
    
    /* Power stats */
    double power_draw_min;
    double power_draw_max;
    double power_draw_avg;
    
    /* Sample count */
    uint64_t sample_count;
} lum_thermal_stats_t;

/* Configuration */
typedef struct {
    const char* log_path;            /* Log file path (NULL = no logging) */
    uint32_t sample_interval_ms;     /* Sampling interval (milliseconds) */
    bool enable_cpu_temp;            /* Enable CPU temperature monitoring */
    bool enable_cpu_freq;            /* Enable CPU frequency monitoring */
    bool enable_gpu_temp;            /* Enable GPU temperature monitoring */
    bool enable_gpu_freq;            /* Enable GPU frequency monitoring */
    bool enable_gpu_rc6;             /* Enable GPU RC6 monitoring */
    bool enable_power_draw;          /* Enable power draw monitoring */
    bool enable_auto_logging;        /* Enable automatic logging */
} lum_thermal_config_t;

/* Default configuration */
#define LUM_THERMAL_DEFAULT_CONFIG { \
    .log_path = "logs/forensic/thermal_c241.csv", \
    .sample_interval_ms = 100, \
    .enable_cpu_temp = true, \
    .enable_cpu_freq = true, \
    .enable_gpu_temp = true, \
    .enable_gpu_freq = true, \
    .enable_gpu_rc6 = true, \
    .enable_power_draw = true, \
    .enable_auto_logging = true \
}

/* API Functions */

/**
 * Initialize thermal monitor
 * @param monitor_out Output monitor context
 * @param config Configuration (NULL = default)
 * @return 0 on success, -1 on error
 */
int lum_thermal_init(lum_thermal_monitor_t** monitor_out, const lum_thermal_config_t* config);

/**
 * Take thermal snapshot
 * @param monitor Monitor context
 * @param snapshot Output snapshot
 * @return 0 on success, -1 on error
 */
int lum_thermal_snapshot(lum_thermal_monitor_t* monitor, lum_thermal_snapshot_t* snapshot);

/**
 * Get thermal statistics
 * @param monitor Monitor context
 * @param stats Output statistics
 * @return 0 on success, -1 on error
 */
int lum_thermal_get_stats(lum_thermal_monitor_t* monitor, lum_thermal_stats_t* stats);

/**
 * Reset thermal statistics
 * @param monitor Monitor context
 * @return 0 on success, -1 on error
 */
int lum_thermal_reset_stats(lum_thermal_monitor_t* monitor);

/**
 * Log thermal snapshot to file
 * @param monitor Monitor context
 * @param snapshot Snapshot to log
 * @return 0 on success, -1 on error
 */
int lum_thermal_log_snapshot(lum_thermal_monitor_t* monitor, const lum_thermal_snapshot_t* snapshot);

/**
 * Flush log file
 * @param monitor Monitor context
 * @return 0 on success, -1 on error
 */
int lum_thermal_flush_log(lum_thermal_monitor_t* monitor);

/**
 * Cleanup thermal monitor
 * @param monitor Monitor context
 */
void lum_thermal_cleanup(lum_thermal_monitor_t* monitor);

/**
 * Print thermal snapshot (human-readable)
 * @param snapshot Snapshot to print
 */
void lum_thermal_print_snapshot(const lum_thermal_snapshot_t* snapshot);

/**
 * Print thermal statistics (human-readable)
 * @param stats Statistics to print
 */
void lum_thermal_print_stats(const lum_thermal_stats_t* stats);

/**
 * Detect thermal throttling
 * @param monitor Monitor context
 * @param threshold_temp Temperature threshold (°C)
 * @param threshold_freq_drop Frequency drop threshold (%)
 * @return true if throttling detected, false otherwise
 */
bool lum_thermal_detect_throttling(lum_thermal_monitor_t* monitor, 
                                   double threshold_temp,
                                   double threshold_freq_drop);

/**
 * Correlate dispatch time with thermal data
 * @param monitor Monitor context
 * @param dispatch_time_ms Dispatch time (milliseconds)
 * @param correlation_temp Output temperature correlation (-1.0 to 1.0)
 * @param correlation_freq Output frequency correlation (-1.0 to 1.0)
 * @return 0 on success, -1 on error
 */
int lum_thermal_correlate_dispatch(lum_thermal_monitor_t* monitor,
                                   double dispatch_time_ms,
                                   double* correlation_temp,
                                   double* correlation_freq);

#ifdef __cplusplus
}
#endif

#endif /* LUM_THERMAL_MONITOR_C241_H */

// Made with Bob
