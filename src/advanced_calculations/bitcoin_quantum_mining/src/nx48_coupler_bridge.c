/* nx48_coupler_bridge.c — Implémentation pont NX48↔coupleur (C99). */
#include "nx48_coupler_bridge.h"
#include "../../../neural_network/nx48_neuro_coupler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct nx48_bridge_s {
    nx48_coupler_t *coupler;          /* NULL si désactivé */
    int             enabled;           /* 1 = on, 0 = off */
    char            log_path[512];     /* "" = pas de log */
    int             log_every;         /* 1 = chaque step, N = 1/N */
};

nx48_bridge_t *nx48_bridge_create(const char *run_id,
                                   int use_neural,
                                   const char *log_path) {
    nx48_bridge_t *b = (nx48_bridge_t *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->enabled   = use_neural ? 1 : 0;
    b->log_every = 50;  /* log 1 ligne sur 50 (économie I/O) */
    if (log_path && log_path[0] != '\0') {
        strncpy(b->log_path, log_path, sizeof(b->log_path) - 1);
        b->log_path[sizeof(b->log_path) - 1] = '\0';
    }
    if (b->enabled) {
        b->coupler = nx48_coupler_create(run_id, 0.1);
        if (!b->coupler) { free(b); return NULL; }
    }
    /* Header JSONL (1ère ligne marqueur) si log demandé */
    if (b->log_path[0]) {
        FILE *fp = fopen(b->log_path, "a");
        if (fp) {
            fprintf(fp, "{\"_marker\":\"nx48_bridge_init\","
                        "\"run_id\":\"%s\",\"enabled\":%d}\n",
                    run_id ? run_id : "default", b->enabled);
            fclose(fp);
        }
    }
    return b;
}

void nx48_bridge_destroy(nx48_bridge_t *b) {
    if (!b) return;
    /* Flush final : sérialisation complète si log */
    if (b->coupler && b->log_path[0]) {
        nx48_coupler_serialize_jsonl(b->coupler, b->log_path);
    }
    if (b->coupler) nx48_coupler_destroy(b->coupler);
    free(b);
}

double nx48_bridge_modulate(nx48_bridge_t *b,
                            const double features[8],
                            int current_leading_zeros) {
    if (!b || !b->enabled || !b->coupler) return 0.0;
    double mod = nx48_coupler_step_auto(b->coupler, features, current_leading_zeros);
    /* Logging échantillonné */
    if (b->log_path[0] &&
        (b->coupler->step_count % (uint64_t)b->log_every) == 0) {
        int near_miss = (current_leading_zeros >= b->coupler->near_miss_threshold) ? 1 : 0;
        nx48_coupler_log_step_jsonl(b->coupler, b->log_path,
                                    near_miss, current_leading_zeros);
    }
    return mod;
}

double nx48_bridge_get_rate_hz(const nx48_bridge_t *b) {
    return (b && b->coupler) ? b->coupler->mean_rate_hz : 0.0;
}
double nx48_bridge_get_w_spread(const nx48_bridge_t *b) {
    return (b && b->coupler) ? b->coupler->w_spread_ema : 0.0;
}
int nx48_bridge_get_threshold(const nx48_bridge_t *b) {
    return (b && b->coupler) ? b->coupler->near_miss_threshold : 0;
}
unsigned long long nx48_bridge_get_step_count(const nx48_bridge_t *b) {
    return (b && b->coupler) ? (unsigned long long)b->coupler->step_count : 0ULL;
}
unsigned long long nx48_bridge_get_near_miss_total(const nx48_bridge_t *b) {
    return (b && b->coupler) ? (unsigned long long)b->coupler->near_miss_total : 0ULL;
}
