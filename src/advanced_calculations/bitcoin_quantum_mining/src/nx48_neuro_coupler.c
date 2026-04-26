/* nx48_neuro_coupler.c — Couplage neural Izhikevich + STDP (C99 P0.1) */
#include "nx48_neuro_coupler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Paramètres Izhikevich Regular Spiking (Izhikevich 2003) */
#define IZH_A   0.02
#define IZH_B   0.2
#define IZH_C   -65.0
#define IZH_D   8.0
#define IZH_VTHR 30.0

/* Paramètres STDP Song-Miller-Abbott 2000 */
#define STDP_TAU_PLUS_MS  20.0
#define STDP_TAU_MINUS_MS 20.0
#define STDP_A_PLUS       0.005
#define STDP_A_MINUS      (STDP_A_PLUS / 1.05)  /* asymétrie 1.05× pour stabilité */
#define STDP_W_MIN        0.0
#define STDP_W_MAX        1.0

static uint64_t fnv1a64(const void* data, size_t n) {
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 0x100000001b3ULL; }
    return h;
}

static double clamp_d(double x, double lo, double hi) {
    if (x < lo) return lo; if (x > hi) return hi; return x;
}

nx48_coupler_t* nx48_coupler_create(const char* run_id, double log_freq) {
    nx48_coupler_t* c = (nx48_coupler_t*)calloc(1, sizeof(nx48_coupler_t));
    if (!c) return NULL;
    (void)log_freq;
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) {
        c->v[i] = IZH_C;
        c->u[i] = IZH_B * IZH_C;
        c->spike_count[i] = 0;
        c->last_spike_ms[i] = -1.0;
        for (int j = 0; j < NX48_COUPLER_N_NEURONS; j++) {
            c->w[i][j] = (i == j) ? 0.0 : 0.5;  /* init 0.5 hors diagonale */
        }
    }
    c->lz_history_idx = 0;
    c->lz_history_filled = 0;
    c->t_ms = 0.0;
    c->step_count = 0;
    c->reward_count = 0;
    if (run_id && *run_id) {
        snprintf(c->run_id, sizeof(c->run_id), "%s", run_id);
    } else {
        snprintf(c->run_id, sizeof(c->run_id), "coupler_%ld", (long)time(NULL));
    }
    snprintf(c->jsonl_path, sizeof(c->jsonl_path),
             "logs/coupler_%s.jsonl", c->run_id);
    return c;
}

void nx48_coupler_destroy(nx48_coupler_t* c) {
    if (c) free(c);
}

int nx48_coupler_check_near_miss(nx48_coupler_t* c, int lz_current) {
    if (!c) return 0;
    /* Calcule percentile 90 sur historique courant */
    int near_miss = 0;
    if (c->lz_history_filled > 10) {
        int sorted[NX48_COUPLER_HISTORY];
        int n = c->lz_history_filled;
        memcpy(sorted, c->lz_history, n * sizeof(int));
        /* Tri insertion */
        for (int i = 1; i < n; i++) {
            int v = sorted[i], j = i - 1;
            while (j >= 0 && sorted[j] > v) { sorted[j+1] = sorted[j]; j--; }
            sorted[j+1] = v;
        }
        int idx_p90 = (int)(0.9 * n);
        if (idx_p90 >= n) idx_p90 = n - 1;
        int p90 = sorted[idx_p90];
        if (lz_current >= p90 && lz_current >= 18) near_miss = 1;  /* min 18 LZ */
    } else if (lz_current >= 20) {
        near_miss = 1;  /* fallback statique avant warmup */
    }
    /* Push dans historique circulaire */
    c->lz_history[c->lz_history_idx] = lz_current;
    c->lz_history_idx = (c->lz_history_idx + 1) % NX48_COUPLER_HISTORY;
    if (c->lz_history_filled < NX48_COUPLER_HISTORY) c->lz_history_filled++;
    return near_miss;
}

double nx48_coupler_step(nx48_coupler_t* c,
                         const double features[NX48_COUPLER_N_NEURONS],
                         int near_miss) {
    if (!c || !features) return 0.0;
    /* 1) Calcul du courant entrant par neurone */
    double I[NX48_COUPLER_N_NEURONS];
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) {
        double f = clamp_d(features[i], 0.0, 1.0);
        I[i] = f * NX48_COUPLER_GAIN_PA;
        /* Couplage synaptique : somme pondérée des spikes récents des autres neurones */
        for (int j = 0; j < NX48_COUPLER_N_NEURONS; j++) {
            if (i == j) continue;
            double dt_spike = c->t_ms - c->last_spike_ms[j];
            if (c->last_spike_ms[j] >= 0.0 && dt_spike < 5.0) {
                I[i] += c->w[i][j] * 5.0;  /* 5 pA par spike récent */
            }
        }
        if (near_miss) I[i] += NX48_COUPLER_REWARD_PA;
    }
    /* 2) Intégration Izhikevich (Euler 1 ms) */
    int spiked[NX48_COUPLER_N_NEURONS] = {0};
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) {
        double dv = (0.04 * c->v[i] * c->v[i] + 5.0 * c->v[i] + 140.0 - c->u[i] + I[i]);
        double du = IZH_A * (IZH_B * c->v[i] - c->u[i]);
        c->v[i] += dv * NX48_COUPLER_DT_MS;
        c->u[i] += du * NX48_COUPLER_DT_MS;
        if (c->v[i] >= IZH_VTHR) {
            spiked[i] = 1;
            c->v[i] = IZH_C;
            c->u[i] += IZH_D;
            c->spike_count[i]++;
            c->spike_total[i]++;
            c->last_spike_ms[i] = c->t_ms;
        }
    }
    /* 3) STDP pour chaque paire (i, j) où l'un des deux a spiké ce step */
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) {
        if (!spiked[i]) continue;
        for (int j = 0; j < NX48_COUPLER_N_NEURONS; j++) {
            if (i == j || c->last_spike_ms[j] < 0.0) continue;
            double dt = c->t_ms - c->last_spike_ms[j];  /* >= 0 car j a spiké avant i */
            if (dt > 0.0 && dt < 5.0 * STDP_TAU_PLUS_MS) {
                /* Pré (j) → Post (i) : LTP */
                double dw = STDP_A_PLUS * exp(-dt / STDP_TAU_PLUS_MS);
                c->w[i][j] = clamp_d(c->w[i][j] + dw, STDP_W_MIN, STDP_W_MAX);
            } else if (dt < 0.0 && -dt < 5.0 * STDP_TAU_MINUS_MS) {
                /* Post (j) → Pré (i) : LTD (cas où j spike après i) */
                double dw = STDP_A_MINUS * exp(dt / STDP_TAU_MINUS_MS);
                c->w[i][j] = clamp_d(c->w[i][j] - dw, STDP_W_MIN, STDP_W_MAX);
            }
        }
    }
    c->t_ms += NX48_COUPLER_DT_MS;
    c->step_count++;
    if (near_miss) c->reward_count++;
    /* 4) Calcul signal de modulation après fenêtre WINDOW_MS */
    if (fmod(c->t_ms, NX48_COUPLER_WINDOW_MS) < NX48_COUPLER_DT_MS / 2.0) {
        double mean_rate = nx48_coupler_mean_rate_hz(c);
        /* Reset compteurs pour prochaine fenêtre */
        for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) c->spike_count[i] = 0;
        /* Normalisation : 20 Hz = neutre (0), 0 Hz = -1 (exploration), 80 Hz = +1 (exploitation) */
        double signal = (mean_rate - 20.0) / 60.0;
        return clamp_d(signal, -1.0, 1.0);
    }
    /* Hors fenêtre, retourner signal partiel basé sur compteurs courants */
    double partial_rate = 0.0;
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) partial_rate += (double)c->spike_count[i];
    partial_rate = (partial_rate / NX48_COUPLER_N_NEURONS) * (1000.0 / fmax(c->t_ms, 1.0));
    double signal = (partial_rate - 20.0) / 60.0;
    return clamp_d(signal, -1.0, 1.0);
}

double nx48_coupler_mean_rate_hz(const nx48_coupler_t* c) {
    if (!c) return 0.0;
    uint64_t total = 0;
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) total += c->spike_count[i];
    double mean_per_neuron = (double)total / NX48_COUPLER_N_NEURONS;
    /* Fenêtre WINDOW_MS → freq Hz */
    return mean_per_neuron * (1000.0 / NX48_COUPLER_WINDOW_MS);
}

double nx48_coupler_weight_spread(const nx48_coupler_t* c) {
    if (!c) return 0.0;
    double w_min = 1e9, w_max = -1e9;
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; i++) {
        for (int j = 0; j < NX48_COUPLER_N_NEURONS; j++) {
            if (i == j) continue;
            if (c->w[i][j] < w_min) w_min = c->w[i][j];
            if (c->w[i][j] > w_max) w_max = c->w[i][j];
        }
    }
    return w_max - w_min;
}

int nx48_coupler_log_jsonl(const nx48_coupler_t* c) {
    if (!c) return -1;
    FILE* f = fopen(c->jsonl_path, "a");
    if (!f) return -2;
    char line[2048];
    int n = snprintf(line, sizeof(line),
        "{\"ts\":%ld,\"run_id\":\"%s\",\"step\":%llu,\"t_ms\":%.1f,"
        "\"mean_rate_hz\":%.3f,\"w_spread\":%.4f,\"reward_count\":%llu",
        (long)time(NULL), c->run_id,
        (unsigned long long)c->step_count, c->t_ms,
        nx48_coupler_mean_rate_hz(c),
        nx48_coupler_weight_spread(c),
        (unsigned long long)c->reward_count);
    /* Vecteur poids compact */
    n += snprintf(line + n, sizeof(line) - n, ",\"w\":[");
    int first = 1;
    for (int i = 0; i < NX48_COUPLER_N_NEURONS && n < (int)sizeof(line) - 32; i++) {
        for (int j = 0; j < NX48_COUPLER_N_NEURONS && n < (int)sizeof(line) - 32; j++) {
            if (i == j) continue;
            n += snprintf(line + n, sizeof(line) - n, "%s%.4f", first ? "" : ",", c->w[i][j]);
            first = 0;
        }
    }
    n += snprintf(line + n, sizeof(line) - n, "]");
    uint64_t cs = fnv1a64(line, n);
    fprintf(f, "%s,\"checksum\":\"%016llx\"}\n", line, (unsigned long long)cs);
    fclose(f);
    return 0;
}
