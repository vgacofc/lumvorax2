/* **************************************************************************
** fu002_clock_qual.c — Qualification métrologique CLOCK_REALTIME/MONOTONIC
**
** Projet : LUMVORAX (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : src/tests / FORENSIC-UNIF-002 S181-D
** Auteur : LUMVORAX Project <contact@artcb.me>
**
** Objectif : mesurer résolution effective, monotonicité, distribution
**   des deltas, coût de clock_gettime(), comportement sous concurrence.
**
** Métriques produites :
**   - min/max/mean/stddev des deltas en ns (REALTIME et MONOTONIC)
**   - nb_zeros : combien de deltas == 0 (timestamps identiques)
**   - nb_nonmono : violations de monotonicité MONOTONIC (doit = 0)
**   - nb_nonmono_rt : violations REALTIME (peut > 0 : NTP, leap second)
**   - résolution estimée = plus petit delta non nul
**   - coût moyen clock_gettime() en ns
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <math.h>

#define N_SAMPLES     100000
#define N_THREADS     4
#define SAMPLES_EACH  25000   /* 4 × 25000 = 100000 total */

/* ── Helper timestamp ─────────────────────────────────────────────────────── */

static uint64_t now_ns(clockid_t clk)
{
    struct timespec ts;
    clock_gettime(clk, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Mesure mono-thread ───────────────────────────────────────────────────── */

typedef struct {
    uint64_t min_delta;
    uint64_t max_delta;
    double   mean_delta;
    double   stddev_delta;
    uint64_t nb_zeros;         /* deltas == 0 */
    uint64_t nb_nonmono;       /* violations de monotonicité (MONOTONIC doit=0) */
    uint64_t resolution_est;   /* plus petit delta non nul */
    double   cost_ns;          /* coût moyen clock_gettime() */
} clock_stats_t;

static clock_stats_t measure_clock(clockid_t clk, const char* name,
                                   int n_samples)
{
    clock_stats_t s;
    memset(&s, 0, sizeof(s));

    uint64_t* deltas = malloc((size_t)n_samples * sizeof(uint64_t));
    if (!deltas) { fprintf(stderr, "OOM\n"); return s; }

    /* Mesure des deltas */
    uint64_t prev = now_ns(clk);
    uint64_t sum = 0;
    s.min_delta = UINT64_MAX;
    s.max_delta = 0;
    s.resolution_est = UINT64_MAX;
    s.nb_zeros = 0;
    s.nb_nonmono = 0;

    for (int i = 0; i < n_samples; i++) {
        uint64_t cur = now_ns(clk);
        if (cur < prev) {
            s.nb_nonmono++;
            deltas[i] = 0;
        } else {
            deltas[i] = cur - prev;
        }
        if (deltas[i] == 0)
            s.nb_zeros++;
        else if (deltas[i] < s.resolution_est)
            s.resolution_est = deltas[i];
        if (deltas[i] < s.min_delta) s.min_delta = deltas[i];
        if (deltas[i] > s.max_delta) s.max_delta = deltas[i];
        sum += deltas[i];
        prev = cur;
    }
    s.mean_delta = (double)sum / n_samples;

    /* Stddev */
    double var = 0.0;
    for (int i = 0; i < n_samples; i++) {
        double d = (double)deltas[i] - s.mean_delta;
        var += d * d;
    }
    s.stddev_delta = sqrt(var / n_samples);

    /* Coût de clock_gettime() : mesure avec boucle vide */
    uint64_t t0 = now_ns(CLOCK_MONOTONIC);
    for (int i = 0; i < 10000; i++) {
        uint64_t dummy;
        struct timespec ts2;
        clock_gettime(clk, &ts2);
        dummy = (uint64_t)ts2.tv_nsec;
        (void)dummy;
    }
    uint64_t t1 = now_ns(CLOCK_MONOTONIC);
    s.cost_ns = (double)(t1 - t0) / 10000.0;

    if (s.resolution_est == UINT64_MAX) s.resolution_est = 0;

    fprintf(stderr,
        "[CLOCK_QUAL][%s] min_delta=%"PRIu64"ns max=%"PRIu64"ns "
        "mean=%.1f stddev=%.1f zeros=%"PRIu64" nonmono=%"PRIu64
        " resolution_est=%"PRIu64"ns cost=%.1fns\n",
        name, s.min_delta, s.max_delta,
        s.mean_delta, s.stddev_delta,
        s.nb_zeros, s.nb_nonmono,
        s.resolution_est, s.cost_ns);

    free(deltas);
    return s;
}

/* ── Test monotonicité sous concurrence ──────────────────────────────────── */

typedef struct {
    int      tid;
    uint64_t nb_nonmono;
    uint64_t nb_zeros;
    uint64_t nb_samples;
} thread_mono_result_t;

static void* thread_mono(void* arg)
{
    thread_mono_result_t* r = (thread_mono_result_t*)arg;
    uint64_t nb_nonmono = 0, nb_zeros = 0;
    uint64_t prev = now_ns(CLOCK_MONOTONIC);

    for (int i = 0; i < SAMPLES_EACH; i++) {
        uint64_t cur = now_ns(CLOCK_MONOTONIC);
        uint64_t delta = (cur >= prev) ? (cur - prev) : 0;
        if (cur < prev) nb_nonmono++;
        if (delta == 0) nb_zeros++;
        prev = cur;
    }
    r->nb_nonmono = nb_nonmono;
    r->nb_zeros   = nb_zeros;
    r->nb_samples = SAMPLES_EACH;
    return NULL;
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    fprintf(stderr, "[CLOCK_QUAL] === Qualification métrologique horloges S181-D ===\n");
    fprintf(stderr, "[CLOCK_QUAL] N_SAMPLES=%d N_THREADS=%d\n", N_SAMPLES, N_THREADS);

    /* --- Mesure mono-thread --- */
    clock_stats_t rt  = measure_clock(CLOCK_REALTIME,  "REALTIME",  N_SAMPLES);
    clock_stats_t mono= measure_clock(CLOCK_MONOTONIC, "MONOTONIC", N_SAMPLES);

    /* --- Test monotonicité concurrent --- */
    pthread_t threads[N_THREADS];
    thread_mono_result_t results[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        results[i].tid = i;
        pthread_create(&threads[i], NULL, thread_mono, &results[i]);
    }
    uint64_t total_nonmono = 0, total_zeros = 0, total_samples = 0;
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_nonmono += results[i].nb_nonmono;
        total_zeros   += results[i].nb_zeros;
        total_samples += results[i].nb_samples;
    }
    fprintf(stderr,
        "[CLOCK_QUAL][MONOTONIC_CONCURRENT] threads=%d total_samples=%"PRIu64
        " nonmono=%"PRIu64" zeros=%"PRIu64"\n",
        N_THREADS, total_samples, total_nonmono, total_zeros);

    /* --- Décision --- */
    /* MONOTONIC mono-thread : 0 violation obligatoire */
    int pass_mono_st  = (mono.nb_nonmono == 0);
    /* MONOTONIC concurrent : 0 violation obligatoire
     * (CLOCK_MONOTONIC est par définition monotone par thread — les
     * violations concurrent seraient un bug OS/hardware) */
    int pass_mono_mt  = (total_nonmono == 0);
    /* Résolution : au moins 1 ns (tout système moderne) */
    int pass_reso     = (mono.resolution_est >= 1);
    /* Coût raisonnable : < 1000 ns (1 µs) */
    int pass_cost     = (mono.cost_ns < 1000.0);

    fprintf(stderr,
        "[CLOCK_QUAL] MONOTONIC mono-thread violations=%"PRIu64" : %s\n",
        mono.nb_nonmono, pass_mono_st ? "PASS" : "FAIL");
    fprintf(stderr,
        "[CLOCK_QUAL] MONOTONIC concurrent violations=%"PRIu64" : %s\n",
        total_nonmono, pass_mono_mt ? "PASS" : "FAIL");
    fprintf(stderr,
        "[CLOCK_QUAL] Résolution estimée=%"PRIu64"ns : %s\n",
        mono.resolution_est, pass_reso ? "PASS (>=1ns)" : "WARN");
    fprintf(stderr,
        "[CLOCK_QUAL] Coût clock_gettime=%.1fns : %s\n",
        mono.cost_ns, pass_cost ? "PASS (<1µs)" : "WARN (>1µs)");
    fprintf(stderr,
        "[CLOCK_QUAL] REALTIME violations_mono=%"PRIu64" (NTP OK si >0)\n",
        rt.nb_nonmono);

    int overall = pass_mono_st && pass_mono_mt && pass_reso;
    fprintf(stderr, "[CLOCK_QUAL] RÉSULTAT GLOBAL : %s\n",
            overall ? "PASS" : "FAIL");

    /* Sortie JSON pour log v34 */
    printf("{\n");
    printf("  \"clock_realtime\": {\n");
    printf("    \"min_delta_ns\": %"PRIu64",\n", rt.min_delta);
    printf("    \"max_delta_ns\": %"PRIu64",\n", rt.max_delta);
    printf("    \"mean_delta_ns\": %.2f,\n", rt.mean_delta);
    printf("    \"stddev_delta_ns\": %.2f,\n", rt.stddev_delta);
    printf("    \"zeros\": %"PRIu64",\n", rt.nb_zeros);
    printf("    \"nonmono_violations\": %"PRIu64",\n", rt.nb_nonmono);
    printf("    \"resolution_est_ns\": %"PRIu64",\n", rt.resolution_est);
    printf("    \"cost_ns\": %.2f\n", rt.cost_ns);
    printf("  },\n");
    printf("  \"clock_monotonic\": {\n");
    printf("    \"min_delta_ns\": %"PRIu64",\n", mono.min_delta);
    printf("    \"max_delta_ns\": %"PRIu64",\n", mono.max_delta);
    printf("    \"mean_delta_ns\": %.2f,\n", mono.mean_delta);
    printf("    \"stddev_delta_ns\": %.2f,\n", mono.stddev_delta);
    printf("    \"zeros\": %"PRIu64",\n", mono.nb_zeros);
    printf("    \"nonmono_violations\": %"PRIu64",\n", mono.nb_nonmono);
    printf("    \"resolution_est_ns\": %"PRIu64",\n", mono.resolution_est);
    printf("    \"cost_ns\": %.2f\n", mono.cost_ns);
    printf("  },\n");
    printf("  \"concurrent_monotonic\": {\n");
    printf("    \"threads\": %d,\n", N_THREADS);
    printf("    \"total_samples\": %"PRIu64",\n", total_samples);
    printf("    \"nonmono_violations\": %"PRIu64",\n", total_nonmono);
    printf("    \"zeros\": %"PRIu64"\n", total_zeros);
    printf("  },\n");
    printf("  \"pass_mono_single_thread\": %s,\n",
           pass_mono_st ? "true" : "false");
    printf("  \"pass_mono_concurrent\": %s,\n",
           pass_mono_mt ? "true" : "false");
    printf("  \"pass_resolution\": %s,\n",
           pass_reso ? "true" : "false");
    printf("  \"pass_cost\": %s,\n",
           pass_cost ? "true" : "false");
    printf("  \"overall\": %s\n", overall ? "true" : "false");
    printf("}\n");

    return overall ? 0 : 1;
}
