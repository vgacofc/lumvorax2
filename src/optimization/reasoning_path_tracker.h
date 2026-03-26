#ifndef REASONING_PATH_TRACKER_H
#define REASONING_PATH_TRACKER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "formal_kernel_v40.h"

typedef struct {
    char decision_label[128];
    float confidence;
    float lyapunov_stability;
    logic_layer_t layer; // [V40] Identification de la couche
    bool formal_validation; // [V40] Validation par le Kernel
    time_t timestamp;
} reasoning_node_t;

typedef struct {
    char task_id[64];
    time_t start_time;
    reasoning_node_t nodes[1024];
    size_t node_count;
} reasoning_trace_t;

reasoning_trace_t* reasoning_trace_start(const char* task_id);
void reasoning_trace_add_node(reasoning_trace_t* trace, const char* decision, float confidence, float lyapunov_stability);
void reasoning_trace_save(reasoning_trace_t* trace, const char* filepath);
void reasoning_trace_destroy(reasoning_trace_t* trace);

#endif
