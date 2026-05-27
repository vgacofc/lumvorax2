#ifndef REGRESSION_DETECTOR_H
#define REGRESSION_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "benchmark_runner.h"

#define REGRESSION_VERSION "1.0.0"
#define REGRESSION_MAX_HISTORY 100
#define REGRESSION_DEFAULT_THRESHOLD 5.0

typedef enum {
    REGRESSION_NONE = 0,
    REGRESSION_MINOR,
    REGRESSION_MAJOR,
    REGRESSION_CRITICAL
} regression_severity_t;

typedef struct {
    char metric_name[128];
    double baseline_value;
    double current_value;
    double change_percent;
    regression_severity_t severity;
    const char* description;
} regression_result_t;

typedef struct {
    uint64_t timestamp;
    char version[32];
    char commit_hash[64];
    benchmark_stats_t* metrics;
    size_t metric_count;
} regression_snapshot_t;

typedef struct {
    regression_snapshot_t* history;
    size_t history_count;
    size_t history_capacity;
    regression_result_t* results;
    size_t result_count;
    size_t result_capacity;
    double minor_threshold;
    double major_threshold;
    double critical_threshold;
    bool strict_mode;
} regression_detector_t;

regression_detector_t* regression_detector_create(void);
void regression_detector_destroy(regression_detector_t* detector);
int regression_detector_set_thresholds(regression_detector_t* detector,
                                        double minor, double major, double critical);
int regression_detector_add_snapshot(regression_detector_t* detector,
                                      const char* version, const char* commit,
                                      benchmark_stats_t* metrics, size_t metric_count);
int regression_detector_analyze(regression_detector_t* detector,
                                 const benchmark_stats_t* current,
                                 const char* metric_name);
int regression_detector_analyze_all(regression_detector_t* detector,
                                     benchmark_suite_t* suite);
regression_severity_t regression_detector_get_max_severity(regression_detector_t* detector);
int regression_detector_get_regressions(regression_detector_t* detector,
                                         regression_result_t** results,
                                         size_t* count);
int regression_detector_export_report(regression_detector_t* detector,
                                       char* buffer, size_t buffer_size);
bool regression_detector_is_acceptable(regression_detector_t* detector);
void regression_detector_clear_results(regression_detector_t* detector);

#endif
