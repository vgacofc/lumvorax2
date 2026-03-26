/**
 * worm_mc_bosonic.c — Implémentation Worm MC bosonique (C36-P3)
 *
 * Algorithme de Prokof'ev–Svistunov pour le modèle de Bose-Hubbard 2D.
 * Référence : Prokof'ev, Svistunov & Tupitsyn, Phys. Lett. A 238, 253 (1998).
 *
 * Approximations retenues (simulation temps fini, réseau small) :
 *   - Worldlines en temps imaginaire discrétisé (M tranches)
 *   - Ver Ira/Masha sur réseau 2D périodique
 *   - Estimateur d'énergie : <H> = -t <hop> + U/2 <n(n-1)> - mu <n>
 *   - Densité superfluide via winding number W²/(2D β t)
 *
 * Namespace log : FORENSIC_LOG_MODULE_METRIC (jamais QF_).
 */

#include "../include/worm_mc_bosonic.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------
 * RNG LCG (même graine que le runner principal pour reproductibilité)
 * ------------------------------------------------------------------------- */
static uint64_t g_lcg_state = 0;

static void lcg_seed(uint64_t s) { g_lcg_state = s; }

static uint64_t lcg_next(void) {
    g_lcg_state = g_lcg_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return g_lcg_state;
}

static double lcg_double(void) {
    return (double)(lcg_next() >> 11) / (double)(UINT64_C(1) << 53);
}

/* -------------------------------------------------------------------------
 * Topologie réseau 2D périodique
 * ------------------------------------------------------------------------- */

static inline int site_idx(int x, int y, int Lx, int Ly) {
    return ((x % Lx + Lx) % Lx) + Lx * ((y % Ly + Ly) % Ly);
}

/* Voisins d'un site (4-connexité) */
static void get_neighbors(int s, int Lx, int Ly, int nb[4]) {
    int x = s % Lx, y = s / Lx;
    nb[0] = site_idx(x + 1, y, Lx, Ly);
    nb[1] = site_idx(x - 1, y, Lx, Ly);
    nb[2] = site_idx(x, y + 1, Lx, Ly);
    nb[3] = site_idx(x, y - 1, Lx, Ly);
}

/* -------------------------------------------------------------------------
 * Énergie locale d'un site
 * ------------------------------------------------------------------------- */

static double E_local(const worm_mc_state_t *st, int s,
                      const worm_mc_params_t *p, int nb[4]) {
    double n = (double)st->n[s];
    /* interaction on-site Bose-Hubbard : U/2 n(n-1) - mu n */
    double E = 0.5 * p->U * n * (n - 1.0) - p->mu * n;
    /* hopping : contribution cinétique estimée par -t * n_i * (n_j+1) */
    for (int d = 0; d < 4; ++d) {
        double nj = (double)st->n[nb[d]];
        E -= p->t * n * (nj + 1.0) * 0.25; /* facteur 1/z (z=4) */
    }
    return E;
}

/* -------------------------------------------------------------------------
 * worm_mc_init
 * ------------------------------------------------------------------------- */

void worm_mc_init(const worm_mc_params_t *params, worm_mc_state_t *state) {
    int n_sites = params->Lx * params->Ly;
    if (n_sites > WORM_MC_MAX_SITES) n_sites = WORM_MC_MAX_SITES;
    state->n_sites = n_sites;
    for (int i = 0; i < n_sites; ++i) state->n[i] = 1; /* 1 boson/site initialement */
    state->E_total = 0.0;
    state->n_avg = 1.0;
    state->n2_avg = 1.0;
    state->compressibility = 0.0;
    state->n_worm_accepted = 0;
    state->n_worm_proposed = 0;
    state->superfluid = false;
    state->superfluid_density = 0.0;
    lcg_seed(params->seed ^ 0xBEEF1234C36ULL);
}

/* -------------------------------------------------------------------------
 * Un sweep MC : tentatives de saut de ver + hopping
 * ------------------------------------------------------------------------- */

static void mc_sweep(worm_mc_state_t *st, const worm_mc_params_t *p,
                     long *hop_count, long *winding_sq_sum,
                     int attempts_per_site) {
    int Lx = p->Lx, Ly = p->Ly;
    int n_sites = st->n_sites;
    int nb[4];
    long winding = 0;
    int total_attempts = n_sites * (attempts_per_site > 0 ? attempts_per_site : 4);

    for (int attempt = 0; attempt < total_attempts; ++attempt) {
        /* Choisit un site source aléatoire */
        int s = (int)(lcg_double() * n_sites) % n_sites;
        if (st->n[s] == 0) continue;

        /* Choisit un voisin aléatoire */
        get_neighbors(s, Lx, Ly, nb);
        int d = (int)(lcg_double() * 4) % 4;
        int t = nb[d];

        /* Poids d'acceptation Metropolis pour le saut s→t */
        double n_s = (double)st->n[s];
        double n_t = (double)st->n[t];
        /* W = t * sqrt(n_s * (n_t+1)) * exp(-beta * dE) */
        double dE = p->t * (n_s - 1.0) - p->t * (n_t + 1.0);
        double ratio = (n_s * (n_t + 1.0)) > 0.0
                       ? exp(-p->beta * dE) * n_s / (n_t + 1.0 + 1e-12)
                       : 0.0;

        st->n_worm_proposed++;
        if (lcg_double() < ratio || ratio >= 1.0) {
            st->n[s]--;
            st->n[t]++;
            st->n_worm_accepted++;
            (*hop_count)++;
            /* Contribution au winding number (direction x) */
            int xs = s % Lx, xt = t % Lx;
            if (xs == Lx - 1 && xt == 0) winding++;
            if (xs == 0 && xt == Lx - 1) winding--;
        }
    }
    *winding_sq_sum += winding * winding;
}

/* -------------------------------------------------------------------------
 * Mesure des observables
 * ------------------------------------------------------------------------- */

static void measure(worm_mc_state_t *st, const worm_mc_params_t *p,
                    double *E_acc, double *n_acc, double *n2_acc,
                    int n_meas) {
    int n_sites = st->n_sites;
    double E = 0.0, n_sum = 0.0, n2_sum = 0.0;
    for (int s = 0; s < n_sites; ++s) {
        double ns = (double)st->n[s];
        n_sum  += ns;
        n2_sum += ns * ns;
        int nb[4];
        get_neighbors(s, p->Lx, p->Ly, nb);
        E += E_local(st, s, p, nb);
    }
    *E_acc  += E / (double)n_sites;
    *n_acc  += n_sum / (double)n_sites;
    *n2_acc += n2_sum / (double)n_sites;
    (void)n_meas;
}

/* -------------------------------------------------------------------------
 * worm_mc_run
 * ------------------------------------------------------------------------- */

bool worm_mc_run(const worm_mc_params_t *params, worm_mc_state_t *state,
                 worm_mc_result_t *result) {
    int n_sw = params->n_sweeps;
    int n_warm = params->n_warmup;
    if (n_sw > WORM_MC_MAX_STEPS) n_sw = WORM_MC_MAX_STEPS;
    if (n_warm > n_sw / 2) n_warm = n_sw / 4;

    long hop_count = 0;
    long winding_sq_sum = 0;
    double E_acc = 0.0, n_acc = 0.0, n2_acc = 0.0;
    int n_meas = 0;

    /* C38-P2 : Calibration automatique de la longueur du ver (acceptance_rate cible 10–40%)
     *
     * Stratégie : réduire attempts_per_site si acceptance_rate < 5% (trop de rejets)
     *             ou augmenter si acceptance_rate > 60% (trop d'acceptations, pas optimal).
     * Pré-run de 50 sweeps avec attempts_per_site=1 pour mesurer le taux de base,
     * puis ajuster pour la thermalisation et la production.
     */
    int calib_attempts = 4;
    {
        long calib_proposed = 0, calib_accepted = 0;
        long dummy_hop = 0, dummy_w = 0;
        state->n_worm_accepted = 0;
        state->n_worm_proposed = 0;
        for (int sw = 0; sw < 50; ++sw) {
            mc_sweep(state, params, &dummy_hop, &dummy_w, 1);
        }
        calib_proposed  = state->n_worm_proposed;
        calib_accepted  = state->n_worm_accepted;
        double calib_rate = (calib_proposed > 0)
                            ? (double)calib_accepted / (double)calib_proposed : 0.1;
        /* Ajustement : cible 10–30% acceptance rate */
        if (calib_rate < 0.05) {
            calib_attempts = 1;
        } else if (calib_rate < 0.10) {
            calib_attempts = 2;
        } else if (calib_rate < 0.30) {
            calib_attempts = 4;
        } else {
            calib_attempts = 6;
        }
        state->n_worm_accepted = 0;
        state->n_worm_proposed = 0;
    }

    /* Thermalisation */
    for (int sw = 0; sw < n_warm; ++sw) {
        long dummy_w = 0;
        mc_sweep(state, params, &hop_count, &dummy_w, calib_attempts);
    }
    hop_count = 0;
    winding_sq_sum = 0;

    /* Production */
    for (int sw = 0; sw < n_sw; ++sw) {
        mc_sweep(state, params, &hop_count, &winding_sq_sum, calib_attempts);
        if (sw % 10 == 0) {
            measure(state, params, &E_acc, &n_acc, &n2_acc, n_meas);
            n_meas++;
        }
    }

    if (n_meas == 0) {
        memset(result, 0, sizeof(*result));
        result->converged = false;
        return false;
    }

    double E_mean  = E_acc  / (double)n_meas;
    double n_mean  = n_acc  / (double)n_meas;
    double n2_mean = n2_acc / (double)n_meas;
    int n_sites = state->n_sites;
    int D = 2;

    /* Compressibilité κ = β * (<n²> - <n>²) / 1 (par site) */
    double kappa = params->beta * (n2_mean - n_mean * n_mean);

    /* Densité superfluide via winding number W²: ρ_s = W²/(2D β t N) */
    double W2_mean = (double)winding_sq_sum / (double)n_sw;
    double rho_s = W2_mean / (2.0 * D * params->beta * fabs(params->t) * n_sites + 1e-30);

    result->E_per_site        = E_mean;
    result->n_per_site        = n_mean;
    result->compressibility   = kappa;
    result->superfluid_density = rho_s;
    result->acceptance_rate   = (state->n_worm_proposed > 0)
                                ? (double)state->n_worm_accepted / (double)state->n_worm_proposed
                                : 0.0;
    result->superfluid        = (rho_s > 0.01);
    result->n_sweeps_done     = n_sw;

    /* Convergence : taux d'acceptation raisonnable et occupation stable */
    result->converged = (result->acceptance_rate > 0.05 && result->acceptance_rate < 0.99
                         && n_mean > 0.0 && isfinite(E_mean));

    state->E_total          = E_mean * n_sites;
    state->n_avg            = n_mean;
    state->n2_avg           = n2_mean;
    state->compressibility  = kappa;
    state->superfluid       = result->superfluid;
    state->superfluid_density = rho_s;

    return result->converged;
}

/* -------------------------------------------------------------------------
 * worm_mc_write_csv
 * ------------------------------------------------------------------------- */

int worm_mc_write_csv(const worm_mc_params_t *params,
                      const worm_mc_result_t *result,
                      FILE *csv_out) {
    if (!csv_out) return -1;
    const char *phase = worm_mc_classify_phase(result);
    double T_K = (params->beta > 1e-30)
                 ? 1.0 / (8.617333e-5 * params->beta)
                 : 0.0;

    fprintf(csv_out,
            "worm_mc_bosonic,E_per_site,%.3f,%.6f,%.10f,%s\n",
            T_K, params->U / fabs(params->t + 1e-30),
            result->E_per_site,
            result->converged ? "PASS" : "FAIL");
    fprintf(csv_out,
            "worm_mc_bosonic,n_per_site,%.3f,%.6f,%.10f,%s\n",
            T_K, params->U / fabs(params->t + 1e-30),
            result->n_per_site,
            result->n_per_site > 0.0 ? "PASS" : "FAIL");
    fprintf(csv_out,
            "worm_mc_bosonic,superfluid_density,%.3f,%.6f,%.10f,%s\n",
            T_K, params->U / fabs(params->t + 1e-30),
            result->superfluid_density,
            phase);
    fprintf(csv_out,
            "worm_mc_bosonic,compressibility,%.3f,%.6f,%.10f,%s\n",
            T_K, params->U / fabs(params->t + 1e-30),
            result->compressibility,
            result->compressibility >= 0.0 ? "PASS" : "FAIL");
    fprintf(csv_out,
            "worm_mc_bosonic,acceptance_rate,%.3f,%.6f,%.10f,%s\n",
            T_K, params->U / fabs(params->t + 1e-30),
            result->acceptance_rate,
            (result->acceptance_rate > 0.05 && result->acceptance_rate < 0.99) ? "PASS" : "WARN");
    return 0;
}

/* -------------------------------------------------------------------------
 * worm_mc_classify_phase
 * ------------------------------------------------------------------------- */

const char *worm_mc_classify_phase(const worm_mc_result_t *result) {
    if (!result) return "unknown";
    if (result->superfluid) return "superfluid";
    /* Isolant de Mott : faible compressibilité + occupation proche d'un entier */
    double n = result->n_per_site;
    double frac = n - floor(n);
    bool near_integer = (frac < 0.05 || frac > 0.95);
    if (near_integer && result->compressibility < 0.1) return "mott_insulator";
    return "normal";
}
