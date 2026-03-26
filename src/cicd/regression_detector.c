#include "regression_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

regression_detector_t* regression_detector_create(void) {
    regression_detector_t* detector = (regression_detector_t*)calloc(1, sizeof(regression_detector_t));
    if (!detector) return NULL;
    
    detector->history_capacity = REGRESSION_MAX_HISTORY;
    detector->history = (regression_snapshot_t*)calloc(detector->history_capacity, sizeof(regression_snapshot_t));
    if (!detector->history) {
        free(detector);
        return NULL;
    }
    
    detector->result_capacity = 64;
    detector->results = (regression_result_t*)calloc(detector->result_capacity, sizeof(regression_result_t));
    if (!detector->results) {
        free(detector->history);
        free(detector);
        return NULL;
    }
    
    detector->minor_threshold = REGRESSION_DEFAULT_THRESHOLD;
    detector->major_threshold = REGRESSION_DEFAULT_THRESHOLD * 2;
    detector->critical_threshold = REGRESSION_DEFAULT_THRESHOLD * 5;
    
    return detector;
}

void regression_detector_destroy(regression_detector_t* detector) {
    if (!detector) return;
    
    for (size_t i = 0; i < detector->history_count; i++) {
        free(detector->history[i].metrics);
    }
    
    free(detector->history);
    free(detector->results);
    free(detector);
}

int regression_detector_set_thresholds(regression_detector_t* detector,
                                        double minor, double major, double critical) {
    if (!detector) return -1;
    
    detector->minor_threshold = minor;
    detector->major_threshold = major;
    detector->critical_threshold = critical;
    
    return 0;
}

int regression_detector_add_snapshot(regression_detector_t* detector,
                                      const char* version, const char* commit,
                                      benchmark_stats_t* metrics, size_t metric_count) {
    if (!detector || !version || !metrics) return -1;
    
    if (detector->history_count >= detector->history_capacity) {
        memmove(detector->history, detector->history + 1,
                (detector->history_capacity - 1) * sizeof(regression_snapshot_t));
        detector->history_count--;
        free(detector->history[0].metrics);
    }
    
    regression_snapshot_t* snap = &detector->history[detector->history_count];
    
    snap->timestamp = get_timestamp_ms();
    strncpy(snap->version, version, sizeof(snap->version) - 1);
    if (commit) {
        strncpy(snap->commit_hash, commit, sizeof(snap->commit_hash) - 1);
    }
    
    snap->metrics = (benchmark_stats_t*)malloc(metric_count * sizeof(benchmark_stats_t));
    if (!snap->metrics) return -2;
    
    memcpy(snap->metrics, metrics, metric_count * sizeof(benchmark_stats_t));
    snap->metric_count = metric_count;
    
    detector->history_count++;
    
    return 0;
}

static regression_severity_t classify_regression(regression_detector_t* detector, double change_percent) {
    double abs_change = fabs(change_percent);
    
    if (abs_change >= detector->critical_threshold) {
        return REGRESSION_CRITICAL;
    } else if (abs_change >= detector->major_threshold) {
        return REGRESSION_MAJOR;
    } else if (abs_change >= detector->minor_threshold) {
        return REGRESSION_MINOR;
    }
    
    return REGRESSION_NONE;
}

int regression_detector_analyze(regression_detector_t* detector,
                                 const benchmark_stats_t* current,
                                 const char* metric_name) {
    if (!detector || !current || !metric_name) return -1;
    
    if (detector->history_count == 0) return 0;
    
    regression_snapshot_t* baseline = &detector->history[detector->history_count - 1];
    
    benchmark_stats_t* baseline_metric = NULL;
    for (size_t i = 0; i < baseline->metric_count; i++) {
        baseline_metric = &baseline->metrics[i];
        break;
    }
    
    if (!baseline_metric) return 0;
    
    double change_percent = 0.0;
    if (baseline_metric->mean_ns > 0) {
        change_percent = ((double)current->mean_ns - (double)baseline_metric->mean_ns) 
                         / (double)baseline_metric->mean_ns * 100.0;
    }
    
    regression_severity_t severity = classify_regression(detector, change_percent);
    
    if (severity != REGRESSION_NONE || detector->strict_mode) {
        if (detector->result_count >= detector->result_capacity) {
            size_t new_cap = detector->result_capacity * 2;
            regression_result_t* new_results = (regression_result_t*)realloc(
                detector->results, new_cap * sizeof(regression_result_t));
            if (!new_results) return -2;
            detector->results = new_results;
            detector->result_capacity = new_cap;
        }
        
        regression_result_t* result = &detector->results[detector->result_count];
        strncpy(result->metric_name, metric_name, sizeof(result->metric_name) - 1);
        result->baseline_value = (double)baseline_metric->mean_ns;
        result->current_value = (double)current->mean_ns;
        result->change_percent = change_percent;
        result->severity = severity;
        
        detector->result_count++;
    }
    
    return 0;
}

int regression_detector_analyze_all(regression_detector_t* detector,
                                     benchmark_suite_t* suite) {
    if (!detector || !suite) return -1;
    
    regression_detector_clear_results(detector);
    
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        if (suite->benchmarks[i].completed) {
            regression_detector_analyze(detector, 
                                         &suite->benchmarks[i].stats,
                                         suite->benchmarks[i].name);
        }
    }
    
    return 0;
}

regression_severity_t regression_detector_get_max_severity(regression_detector_t* detector) {
    if (!detector) return REGRESSION_NONE;
    
    regression_severity_t max_severity = REGRESSION_NONE;
    
    for (size_t i = 0; i < detector->result_count; i++) {
        if (detector->results[i].severity > max_severity) {
            max_severity = detector->results[i].severity;
        }
    }
    
    return max_severity;
}

int regression_detector_get_regressions(regression_detector_t* detector,
                                         regression_result_t** results,
                                         size_t* count) {
    if (!detector || !results || !count) return -1;
    
    *results = detector->results;
    *count = detector->result_count;
    
    return 0;
}

static const char* severity_to_string(regression_severity_t severity) {
    switch (severity) {
        case REGRESSION_NONE: return "NONE";
        case REGRESSION_MINOR: return "MINOR";
        case REGRESSION_MAJOR: return "MAJOR";
        case REGRESSION_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

int regression_detector_export_report(regression_detector_t* detector,
                                       char* buffer, size_t buffer_size) {
    if (!detector || !buffer || buffer_size == 0) return -1;
    
    char* ptr = buffer;
    size_t remaining = buffer_size;
    int written;
    
    written = snprintf(ptr, remaining, "=== REGRESSION DETECTION REPORT ===\n\n");
    if (written < 0 || (size_t)written >= remaining) return -2;
    ptr += written;
    remaining -= written;
    
    written = snprintf(ptr, remaining, "Thresholds: Minor=%.1f%%, Major=%.1f%%, Critical=%.1f%%\n",
                       detector->minor_threshold, detector->major_threshold, detector->critical_threshold);
    if (written < 0 || (size_t)written >= remaining) return -3;
    ptr += written;
    remaining -= written;
    
    written = snprintf(ptr, remaining, "Results: %zu regression(s) detected\n\n",
                       detector->result_count);
    if (written < 0 || (size_t)written >= remaining) return -4;
    ptr += written;
    remaining -= written;
    
    for (size_t i = 0; i < detector->result_count; i++) {
        regression_result_t* r = &detector->results[i];
        
        written = snprintf(ptr, remaining, "[%s] %s: %.2f -> %.2f (%+.2f%%)\n",
                           severity_to_string(r->severity),
                           r->metric_name,
                           r->baseline_value,
                           r->current_value,
                           r->change_percent);
        if (written < 0 || (size_t)written >= remaining) return -5;
        ptr += written;
        remaining -= written;
    }
    
    regression_severity_t max = regression_detector_get_max_severity(detector);
    written = snprintf(ptr, remaining, "\nMax Severity: %s\n", severity_to_string(max));
    if (written < 0 || (size_t)written >= remaining) return -6;
    ptr += written;
    remaining -= written;
    
    written = snprintf(ptr, remaining, "Status: %s\n",
                       regression_detector_is_acceptable(detector) ? "ACCEPTABLE" : "FAILED");
    if (written < 0 || (size_t)written >= remaining) return -7;
    
    return (int)(buffer_size - remaining + written);
}

bool regression_detector_is_acceptable(regression_detector_t* detector) {
    if (!detector) return true;
    
    regression_severity_t max = regression_detector_get_max_severity(detector);
    
    if (detector->strict_mode) {
        return max == REGRESSION_NONE;
    }
    
    return max < REGRESSION_CRITICAL;
}

void regression_detector_clear_results(regression_detector_t* detector) {
    if (detector) {
        detector->result_count = 0;
    }
}
