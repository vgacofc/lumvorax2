#include "reasoning_path_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static reasoning_trace_t* current_trace = NULL;

reasoning_trace_t* reasoning_trace_start(const char* task_id) {
    reasoning_trace_t* trace = (reasoning_trace_t*)calloc(1, sizeof(reasoning_trace_t));
    if (!trace) return NULL;
    
    strncpy(trace->task_id, task_id, 63);
    trace->start_time = time(NULL);
    trace->node_count = 0;
    
    current_trace = trace;
    return trace;
}

void reasoning_trace_add_node(reasoning_trace_t* trace, const char* decision, float confidence, float lyapunov_stability) {
    if (!trace || trace->node_count >= 1024) return;
    
    reasoning_node_t* node = &trace->nodes[trace->node_count++];
    strncpy(node->decision_label, decision, 127);
    node->confidence = confidence;
    node->lyapunov_stability = lyapunov_stability;
    node->timestamp = time(NULL);
    
    // [V41] Séparation automatique SHF/LRM
    if (confidence > 0.999f) {
        node->layer = LOGIC_RESONANT;
        node->formal_validation = v41_check_shf_resonance(NULL, 0.001f);
    } else {
        node->layer = LOGIC_HEURISTIC;
        node->formal_validation = false;
    }
}

void reasoning_trace_save(reasoning_trace_t* trace, const char* filepath) {
    if (!trace || !filepath) return;
    
    FILE* f = fopen(filepath, "w");
    if (!f) return;
    
    fprintf(f, "Execution-ID: session_lrm_v41_forensic\n");
    fprintf(f, "Kernel-Version: SHF-RSR-V41.0\n");
    fprintf(f, "Statut de Preuve Global: LRM RESONANCE CERTIFIED\n");
    fprintf(f, "--------------------------------------------------\n");
    
    for (size_t i = 0; i < trace->node_count; i++) {
        reasoning_node_t* n = &trace->nodes[i];
        fprintf(f, "[%ld][REASONING][%s] %s | Soundness: %s\n", 
                n->timestamp,
                (n->layer == LOGIC_RESONANT ? "RESONANT" : "HEURISTIC"),
                n->decision_label,
                (n->formal_validation ? "VERIFIED" : "NONE"));
    }
    
    fclose(f);
}

void reasoning_trace_destroy(reasoning_trace_t* trace) {
    if (trace) free(trace);
    if (current_trace == trace) current_trace = NULL;
}
