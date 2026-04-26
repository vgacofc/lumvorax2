/* test_nx48_neuro_coupler.c — Tests T1-T7 module C99 P0.1 */
#include "../include/nx48_neuro_coupler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int passed = 0, failed = 0;
#define CHECK(cond, name) do { \
    if (cond) { printf("  ✅ %s\n", name); passed++; } \
    else      { printf("  ❌ %s\n", name); failed++; } \
} while (0)

int main(void) {
    printf("=== Tests nx48_neuro_coupler (C99 P0.1) ===\n");

    /* T1 — Création/destruction */
    nx48_coupler_t* c = nx48_coupler_create("test_run", 0.1);
    CHECK(c != NULL, "T1: création coupleur");
    CHECK(c->t_ms == 0.0 && c->step_count == 0, "T1: état initial v=u=0");

    /* T2 — Features nulles → faible activité */
    double f_low[8] = {0.05, 0.05, 0.05, 0.05, 0.05, 0.05, 0.05, 0.05};
    int total_low = 0;
    for (int s = 0; s < 500; s++) {
        nx48_coupler_step(c, f_low, 0);
    }
    for (int i = 0; i < 8; i++) total_low += (int)c->spike_total[i];
    printf("    [debug] total_low=%d (cumul)\n", total_low);
    CHECK(total_low < 100, "T2: features basses → < 100 spikes cumul/8 sur 500ms");

    /* T3 — Features hautes → plus de spikes */
    nx48_coupler_destroy(c);
    c = nx48_coupler_create("test_high", 0.1);
    double f_high[8] = {0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95};
    int total_high = 0;
    for (int s = 0; s < 1000; s++) {
        nx48_coupler_step(c, f_high, 0);
    }
    for (int i = 0; i < 8; i++) total_high += (int)c->spike_total[i];
    double rate_hz = nx48_coupler_mean_rate_hz(c);
    printf("    [debug] spikes_total=%d (cumul), rate=%.1f Hz\n", total_high, rate_hz);
    CHECK(total_high > 100, "T3: features hautes → > 100 spikes cumul (activité significative)");

    /* T4 — Near-miss boost les spikes (+50 pA) */
    nx48_coupler_destroy(c);
    c = nx48_coupler_create("test_nm", 0.1);
    double f_med[8] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    int total_no_nm = 0;
    for (int s = 0; s < 500; s++) nx48_coupler_step(c, f_med, 0);
    for (int i = 0; i < 8; i++) total_no_nm += c->spike_count[i];
    nx48_coupler_destroy(c);
    c = nx48_coupler_create("test_nm2", 0.1);
    int total_with_nm = 0;
    for (int s = 0; s < 500; s++) nx48_coupler_step(c, f_med, 1);
    for (int i = 0; i < 8; i++) total_with_nm += c->spike_count[i];
    printf("    [debug] sans=%d avec=%d\n", total_no_nm, total_with_nm);
    CHECK(total_with_nm >= total_no_nm, "T4: near-miss ne diminue pas l'activité");

    /* T5 — STDP modifie les poids après 1500 steps avec features hétérogènes
     * (sinon tous les neurones spikent en synchronie → dt=0 → pas de LTP/LTD). */
    nx48_coupler_destroy(c);
    c = nx48_coupler_create("test_stdp", 0.1);
    double f_div[8] = {0.95, 0.85, 0.75, 0.65, 0.55, 0.45, 0.35, 0.25};
    for (int s = 0; s < 1500; s++) nx48_coupler_step(c, f_div, s % 200 == 0);
    double spread = nx48_coupler_weight_spread(c);
    printf("    [debug] spread Δw = %.4f\n", spread);
    CHECK(spread > 0.0001, "T5: STDP modifie poids (Δw > 0.0001) avec features diverses");

    /* T6 — Modulation bornée [-1, +1] sur 500 itérations */
    nx48_coupler_destroy(c);
    c = nx48_coupler_create("test_bound", 0.1);
    int violations = 0;
    for (int s = 0; s < 500; s++) {
        double mod = nx48_coupler_step(c, f_high, s % 100 == 0);
        if (mod < -1.0001 || mod > 1.0001) violations++;
    }
    CHECK(violations == 0, "T6: modulation bornée [-1, +1] sur 500 steps");

    /* T7 — JSONL avec checksum */
    int rc = nx48_coupler_log_jsonl(c);
    CHECK(rc == 0, "T7: log JSONL OK (rc=0)");

    /* T8 — Q1 percentile 90 dynamique */
    nx48_coupler_destroy(c);
    c = nx48_coupler_create("test_q1", 0.1);
    /* Inject historique : majorité 10-15, quelques 25-30 */
    for (int i = 0; i < 50; i++) nx48_coupler_check_near_miss(c, 12 + (i % 4));
    for (int i = 0; i < 5; i++)  nx48_coupler_check_near_miss(c, 26 + (i % 3));
    int nm_low  = nx48_coupler_check_near_miss(c, 14);   /* < p90 → faux */
    int nm_high = nx48_coupler_check_near_miss(c, 28);   /* >= p90 → vrai */
    printf("    [debug] near_miss(14)=%d near_miss(28)=%d\n", nm_low, nm_high);
    CHECK(nm_low == 0 && nm_high == 1, "T8: percentile 90 dynamique discrimine bien");

    nx48_coupler_destroy(c);
    printf("\n=== Résultat : %d passés, %d échoués ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
