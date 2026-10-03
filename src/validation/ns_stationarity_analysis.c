/* **************************************************************************
** ns_stationarity_analysis.c — Analyse de stationnarité fenêtre finale (T04-STRONG)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / stationarity analysis
** Auteur : LumVorax Project
**
** Implémente :
**   - Fenêtre circulaire dynamique pour les observables NS
**   - Statistiques descriptives : min, max, mean, std, range
**   - Régression linéaire OLS sur la fenêtre (pente, intercept)
**   - Critère multi-points de quasi-stationnarité (registre 176 §5)
**
** Critère (tous obligatoires) :
**   C1 : n_points >= QS_MIN_POINTS
**   C2 : range_rel = (max-min)/mean < QS_RANGE_REL  (variation max-min)
**   C3 : std_rel   = std/mean        < QS_STD_REL   (écart-type)
**   C4 : slope_rel = |slope|/mean    < QS_SLOPE_REL (pente normalisée)
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "ns_stationarity_analysis.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <float.h>

/* ── Création / Destruction ─────────────────────────────────────────────── */

StationarityWindow *stationarity_window_create(int capacity)
{
    if (capacity <= 0)
        return NULL;

    StationarityWindow *sw = (StationarityWindow *)malloc(sizeof(StationarityWindow));
    if (!sw)
        return NULL;

    sw->values   = (double *)calloc((size_t)capacity, sizeof(double));
    sw->steps    = (int    *)calloc((size_t)capacity, sizeof(int));

    if (!sw->values || !sw->steps) {
        free(sw->values);
        free(sw->steps);
        free(sw);
        return NULL;
    }

    sw->capacity = capacity;
    sw->head     = 0;
    sw->count    = 0;
    return sw;
}

void stationarity_window_destroy(StationarityWindow *sw)
{
    if (!sw) return;
    free(sw->values);
    free(sw->steps);
    free(sw);
}

/* ── Ajout d'un point ───────────────────────────────────────────────────── */

void stationarity_window_push(StationarityWindow *sw, double value, int step)
{
    if (!sw) return;
    sw->values[sw->head] = value;
    sw->steps [sw->head] = step;
    sw->head = (sw->head + 1) % sw->capacity;
    if (sw->count < sw->capacity)
        sw->count++;
}

/* ── Analyse statistique ────────────────────────────────────────────────── */

StationarityResult stationarity_analyze(const StationarityWindow *sw)
{
    StationarityResult r;
    memset(&r, 0, sizeof(r));

    if (!sw || sw->count == 0) {
        snprintf(r.reason, sizeof(r.reason),
                 "FAIL : fenetre vide (n=0)");
        return r;
    }

    r.n_points = sw->count;

    /* ── Reconstruire les points dans l'ordre chronologique ── */
    /* Le plus ancien = (head - count + capacity) % capacity    */
    int n   = sw->count;
    int cap = sw->capacity;

    /* Allouer tableaux temporaires pour les statistiques */
    double *vals = (double *)malloc((size_t)n * sizeof(double));
    double *ts   = (double *)malloc((size_t)n * sizeof(double));
    if (!vals || !ts) {
        free(vals); free(ts);
        snprintf(r.reason, sizeof(r.reason),
                 "FAIL : allocation temporaire echouee");
        return r;
    }

    int oldest = (sw->head - n + cap) % cap;
    for (int i = 0; i < n; i++) {
        int idx  = (oldest + i) % cap;
        vals[i]  = sw->values[idx];
        ts[i]    = (double)sw->steps[idx];
    }

    /* ── Min / Max / Mean ── */
    double vmin = DBL_MAX;
    double vmax = -DBL_MAX;
    double sum  = 0.0;

    for (int i = 0; i < n; i++) {
        if (vals[i] < vmin) vmin = vals[i];
        if (vals[i] > vmax) vmax = vals[i];
        sum += vals[i];
    }

    double mean = sum / (double)n;
    r.min_val   = vmin;
    r.max_val   = vmax;
    r.mean      = mean;
    r.range     = vmax - vmin;

    /* ── Écart-type (population, n-1 pour n>1) ── */
    double var = 0.0;
    for (int i = 0; i < n; i++) {
        double d = vals[i] - mean;
        var += d * d;
    }
    double denom_std = (n > 1) ? (double)(n - 1) : 1.0;
    r.std = sqrt(var / denom_std);

    /* ── Ratios relatifs (protégés contre mean ~ 0) ── */
    double safe_mean = (mean > 1e-30) ? mean : 1e-30;
    r.range_rel = r.range / safe_mean;
    r.std_rel   = r.std   / safe_mean;

    /* ── Régression linéaire OLS : vals[i] = slope * ts[i] + intercept ──
     *
     * OLS normales :
     *   slope     = (n * Σ(t*v) - Σt * Σv) / (n * Σt² - (Σt)²)
     *   intercept = (Σv - slope * Σt) / n
     *
     * Si Σt² - (Σt/n)² ≈ 0 (tous ts identiques), pente = 0.
     */
    {
        double sum_t  = 0.0, sum_v  = 0.0;
        double sum_tt = 0.0, sum_tv = 0.0;

        for (int i = 0; i < n; i++) {
            sum_t  += ts[i];
            sum_v  += vals[i];
            sum_tt += ts[i] * ts[i];
            sum_tv += ts[i] * vals[i];
        }

        double denom_ols = (double)n * sum_tt - sum_t * sum_t;
        if (fabs(denom_ols) < 1e-30) {
            r.slope     = 0.0;
            r.intercept = mean;
        } else {
            r.slope     = ((double)n * sum_tv - sum_t * sum_v) / denom_ols;
            r.intercept = (sum_v - r.slope * sum_t) / (double)n;
        }

        /* slope_rel = |slope| / mean (par pas, indépendant de l'échelle) */
        r.slope_rel = fabs(r.slope) / safe_mean;
    }

    free(vals);
    free(ts);

    /* ── Verdict multi-critères ── */
    int c1 = (r.n_points >= QS_MIN_POINTS);
    int c2 = (r.range_rel < QS_RANGE_REL);
    int c3 = (r.std_rel   < QS_STD_REL);
    int c4 = (r.slope_rel < QS_SLOPE_REL);

    r.quasi_stationary = c1 && c2 && c3 && c4;

    snprintf(r.reason, sizeof(r.reason),
             "n=%d(>=%d:%s) range_rel=%.4f(<%.4f:%s) "
             "std_rel=%.4f(<%.4f:%s) slope_rel=%.2e(<%.4f:%s)",
             r.n_points, QS_MIN_POINTS, c1 ? "OK" : "FAIL",
             r.range_rel, QS_RANGE_REL,  c2 ? "OK" : "FAIL",
             r.std_rel,   QS_STD_REL,    c3 ? "OK" : "FAIL",
             r.slope_rel, QS_SLOPE_REL,  c4 ? "OK" : "FAIL");

    return r;
}

/* ── Impression DEBUG ───────────────────────────────────────────────────── */

void stationarity_print_result(const StationarityResult *r, const char *label)
{
    if (!r) return;
    const char *lbl = label ? label : "stationarity";

    printf("[%s] n_points=%d | min=%.6e max=%.6e mean=%.6e\n",
           lbl, r->n_points, r->min_val, r->max_val, r->mean);
    printf("[%s] std=%.6e range=%.6e\n",
           lbl, r->std, r->range);
    printf("[%s] range_rel=%.4f (seuil=%.4f) | std_rel=%.4f (seuil=%.4f)\n",
           lbl, r->range_rel, (double)QS_RANGE_REL,
                r->std_rel,   (double)QS_STD_REL);
    printf("[%s] slope=%.6e intercept=%.6e slope_rel=%.4e (seuil=%.4f)\n",
           lbl, r->slope, r->intercept, r->slope_rel, (double)QS_SLOPE_REL);
    printf("[%s] VERDICT : %s\n",
           lbl, r->quasi_stationary ? "QUASI_STATIONNAIRE" : "NON_STATIONNAIRE");
    printf("[%s] DETAIL  : %s\n", lbl, r->reason);
}
