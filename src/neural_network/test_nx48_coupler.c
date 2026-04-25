/* test_nx48_coupler.c — Tests unitaires couplage NX48 ↔ Izhikevich+STDP.
 *
 * Cycle C98. Objectifs :
 *   T1 — Création/destruction sans fuite
 *   T2 — Mode OFF retourne 0.0
 *   T3 — Mode ON avec features hautes → mean_rate_hz augmente
 *   T4 — Near-miss event boost les spikes
 *   T5 — STDP modifie les poids après ~1000 steps
 *   T6 — Modulation bornée dans [-1, +1]
 *   T7 — Sérialisation JSONL produit ligne valide
 *
 * Compilation :
 *   gcc -O2 -Wall -Wextra -Iinclude \
 *     src/neural_network/{izhikevich_spiking,stdp_plasticity,nx48_neuro_coupler}.c \
 *     src/neural_network/test_nx48_coupler.c -lm -o /tmp/test_coupler
 */
#include "nx48_neuro_coupler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, msg); \
                   return 1; } \
    else { fprintf(stdout, "  OK   %s\n", msg); } \
} while (0)

static int test_t1_create_destroy(void) {
    fprintf(stdout, "[T1] Création/destruction\n");
    nx48_coupler_t *c = nx48_coupler_create("test_t1", 0.1);
    ASSERT(c != NULL, "T1 nx48_coupler_create");
    ASSERT(c->net != NULL, "T1 net alloué");
    ASSERT(c->net->n_neurons == 8, "T1 n_neurons == 8");
    ASSERT(c->net->n_synapses == 56, "T1 n_synapses == 56");
    ASSERT(c->coupler_active == 1, "T1 actif par défaut");
    nx48_coupler_destroy(c);
    return 0;
}

static int test_t2_off_returns_zero(void) {
    fprintf(stdout, "[T2] Mode OFF retourne 0.0\n");
    nx48_coupler_t *c = nx48_coupler_create("test_t2", 0.1);
    ASSERT(c != NULL, "T2 alloc");
    nx48_coupler_set_active(c, 0);
    double feats[8] = {0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9};
    double mod = nx48_coupler_step(c, feats, 0);
    ASSERT(mod == 0.0, "T2 OFF → 0.0");
    ASSERT(c->step_count == 0, "T2 step_count inchangé");
    nx48_coupler_destroy(c);
    return 0;
}

static int test_t3_high_features_increase_rate(void) {
    fprintf(stdout, "[T3] Features hautes → mean_rate_hz > 0\n");
    nx48_coupler_t *c = nx48_coupler_create("test_t3", 0.1);
    ASSERT(c != NULL, "T3 alloc");
    double feats_high[8] = {0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95};
    /* Run 200 steps (100 ms simulé) */
    for (int t = 0; t < 200; ++t) {
        nx48_coupler_step(c, feats_high, 0);
    }
    fprintf(stdout, "  → mean_rate_hz=%.3f spikes_total=%llu\n",
            c->mean_rate_hz, (unsigned long long)c->spike_count);
    ASSERT(c->mean_rate_hz > 5.0, "T3 mean_rate_hz > 5 Hz");
    ASSERT(c->spike_count > 0, "T3 au moins 1 spike");
    nx48_coupler_destroy(c);
    return 0;
}

static int test_t4_near_miss_boost(void) {
    fprintf(stdout, "[T4] Near-miss event injecte récompense\n");
    nx48_coupler_t *c1 = nx48_coupler_create("test_t4_no_reward", 0.1);
    nx48_coupler_t *c2 = nx48_coupler_create("test_t4_reward",    0.1);
    double feats[8] = {0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3};
    for (int t = 0; t < 100; ++t) {
        nx48_coupler_step(c1, feats, 0);
        /* Near-miss tous les 10 pas */
        nx48_coupler_step(c2, feats, (t % 10 == 0) ? 1 : 0);
    }
    fprintf(stdout, "  → c1 spikes=%llu  c2 spikes=%llu\n",
            (unsigned long long)c1->spike_count,
            (unsigned long long)c2->spike_count);
    ASSERT(c2->spike_count >= c1->spike_count,
           "T4 récompense ≥ pas-de-récompense");
    nx48_coupler_destroy(c1);
    nx48_coupler_destroy(c2);
    return 0;
}

static int test_t5_stdp_modifies_weights(void) {
    fprintf(stdout, "[T5] STDP modifie les poids\n");
    nx48_coupler_t *c = nx48_coupler_create("test_t5", 0.5);
    /* Toutes les synapses initialisées à 0.5 */
    /* Pattern corrélé : neurones 0..3 actifs, 4..7 silencieux */
    double feats[8] = {0.95, 0.95, 0.95, 0.95, 0.0, 0.0, 0.0, 0.0};
    for (int t = 0; t < 1000; ++t) {
        nx48_coupler_step(c, feats, 0);
    }
    double w_min = 1e30, w_max = -1e30;
    for (int s = 0; s < c->net->n_synapses; ++s) {
        if (c->net->weight[s] < w_min) w_min = c->net->weight[s];
        if (c->net->weight[s] > w_max) w_max = c->net->weight[s];
    }
    fprintf(stdout, "  → w_min=%.4f w_max=%.4f spread=%.4f\n",
            w_min, w_max, w_max - w_min);
    /* STDP doit avoir créé une asymétrie poids-corrélé vs poids-non-corrélé */
    ASSERT(fabs(w_max - w_min) > 1e-6, "T5 STDP a différencié les poids");
    nx48_coupler_destroy(c);
    return 0;
}

static int test_t6_modulation_bounded(void) {
    fprintf(stdout, "[T6] Modulation bornée dans [-1, +1]\n");
    nx48_coupler_t *c = nx48_coupler_create("test_t6", 0.1);
    double feats[8];
    for (int t = 0; t < 500; ++t) {
        /* Features aléatoires extrêmes */
        for (int i = 0; i < 8; ++i) {
            feats[i] = (t % 2 == 0) ? 1.0 : 0.0;
        }
        double mod = nx48_coupler_step(c, feats, t % 5 == 0 ? 1 : 0);
        ASSERT(mod >= -1.0 && mod <= +1.0, "T6 mod borné");
    }
    nx48_coupler_destroy(c);
    return 0;
}

static int test_t7_serialize_jsonl(void) {
    fprintf(stdout, "[T7] Sérialisation JSONL\n");
    const char *path = "/tmp/test_coupler_t7.jsonl";
    /* Nettoyer fichier précédent */
    remove(path);
    nx48_coupler_t *c = nx48_coupler_create("test_t7", 0.1);
    double feats[8] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    for (int t = 0; t < 50; ++t) {
        nx48_coupler_step(c, feats, 0);
    }
    int rc = nx48_coupler_serialize_jsonl(c, path);
    ASSERT(rc == 0, "T7 serialize rc==0");
    /* Vérifier ligne contient "checksum" */
    FILE *fp = fopen(path, "r");
    ASSERT(fp != NULL, "T7 fichier existe");
    char buf[1024] = {0};
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
    ASSERT(strstr(buf, "checksum") != NULL, "T7 contient checksum");
    ASSERT(strstr(buf, "test_t7") != NULL, "T7 contient run_id");
    fprintf(stdout, "  → ligne JSONL : %.120s...\n", buf);
    nx48_coupler_destroy(c);
    return 0;
}

int main(void) {
    fprintf(stdout, "═══════════════════════════════════════════════════\n");
    fprintf(stdout, " TEST NX48 ↔ IZHIKEVICH+STDP COUPLER (C98)\n");
    fprintf(stdout, " Couplage (NON substitution) — STANDARD_NAMES §M-BTC18\n");
    fprintf(stdout, "═══════════════════════════════════════════════════\n\n");
    int total_failed = 0;
    total_failed += test_t1_create_destroy();
    total_failed += test_t2_off_returns_zero();
    total_failed += test_t3_high_features_increase_rate();
    total_failed += test_t4_near_miss_boost();
    total_failed += test_t5_stdp_modifies_weights();
    total_failed += test_t6_modulation_bounded();
    total_failed += test_t7_serialize_jsonl();
    fprintf(stdout, "\n═══════════════════════════════════════════════════\n");
    if (total_failed == 0) {
        fprintf(stdout, " ✅ TOUS TESTS OK (7/7) — Couplage opérationnel\n");
    } else {
        fprintf(stdout, " ❌ %d tests échoués\n", total_failed);
    }
    fprintf(stdout, "═══════════════════════════════════════════════════\n");
    return total_failed;
}
