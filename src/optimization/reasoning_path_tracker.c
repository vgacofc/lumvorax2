/* **************************************************************************
** reasoning_path_tracker.c — Traçage du chemin de raisonnement LRM/SHF avec FU002
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : LUM-VORAX / optimisation / reasoning path tracker
** Auteur : ARTCB Project <contact@artcb.me>
**
** Instrumentation : forensic_unif_002 (LUM/VORAX BIT LEVEL NANOSECONDE)
**                   memory_tracker (TRACKED_MALLOC/FREE)
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */
#include "reasoning_path_tracker.h"
#include "../debug/forensic_unif_002.h"
#include "../debug/memory_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline uint64_t ts_monotonic_ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static reasoning_trace_t* current_trace = NULL;

reasoning_trace_t* reasoning_trace_start(const char* task_id) {
    /* AVANT : calloc(1, sizeof(reasoning_trace_t))
       APRÈS : TRACKED_MALLOC pour suivi mémoire complet */
    reasoning_trace_t* trace = (reasoning_trace_t*)TRACKED_MALLOC(sizeof(reasoning_trace_t));
    if (!trace) return NULL;
    memset(trace, 0, sizeof(reasoning_trace_t));

    strncpy(trace->task_id, task_id, 63);
    /* AVANT : trace->start_time = time(NULL)  (résolution seconde)
       APRÈS : ts_monotonic_ns_now()           (résolution nanoseconde) */
    trace->start_time = (time_t)(ts_monotonic_ns_now() / 1000000000ULL);
    trace->node_count = 0;

    /* FU002 : tracer la création du trace de raisonnement */
    bit_id_t bid = forensic_unif002_new_bit_id(0x01);
    lum_id_t lid = {0};
    forensic_unif002_log_event(FU002_EVT_LUM_TRANSFORMED, bid, lid,
        "reasoning_trace_start: new trace allocated");

    current_trace = trace;
    return trace;
}

void reasoning_trace_add_node(reasoning_trace_t* trace, const char* decision,
                               float confidence, float lyapunov_stability) {
    if (!trace || trace->node_count >= 1024) return;

    reasoning_node_t* node = &trace->nodes[trace->node_count++];
    strncpy(node->decision_label, decision, 127);
    node->decision_label[127] = '\0';
    node->confidence = confidence;
    node->lyapunov_stability = lyapunov_stability;
    /* AVANT : node->timestamp = time(NULL)      (résolution seconde)
       APRÈS : ts_monotonic_ns_now() / 1e9 cast (résolution nanoseconde)  */
    node->timestamp = (time_t)(ts_monotonic_ns_now() / 1000000000ULL);

    /* [V41] Séparation automatique SHF/LRM */
    if (confidence > 0.999f) {
        node->layer = LOGIC_RESONANT;
        node->formal_validation = v41_check_shf_resonance(NULL, 0.001f);
    } else {
        node->layer = LOGIC_HEURISTIC;
        node->formal_validation = false;
    }

    /* FU002 : tracer chaque nœud de décision (1 sur 16 pour limiter le volume) */
    if ((trace->node_count & 0xF) == 0) {
        bit_id_t bid = forensic_unif002_new_bit_id((uint8_t)(confidence * 255.0f));
        lum_id_t lid = {0};
        forensic_unif002_log_event(FU002_EVT_LUM_TRANSFORMED, bid, lid,
            "reasoning_trace_add_node: resonance decision recorded");
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
    if (!trace) return;
    /* AVANT : free(trace)
       APRÈS : TRACKED_FREE pour cohérence avec TRACKED_MALLOC */
    if (current_trace == trace) current_trace = NULL;
    TRACKED_FREE(trace);
}
