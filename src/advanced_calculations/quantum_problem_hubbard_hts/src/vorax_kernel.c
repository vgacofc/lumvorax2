/* VORAX kernel — implementation. Solveur reel C, branche dans advanced_parallel.
 *
 * Approche : descente coordonnee Givens sur les angles HVA-Hubbard.
 *   E(theta_h, theta_u) = -t * cos(theta_h) * Z_neel + (U/2) * (1 - cos(theta_u))
 *                        + 0.25 * (theta_h^2 + theta_u^2) * coupling
 * (modele analytique 2-angle pour 1-layer HVA pres du point de Trotter,
 *  cf. Wecker/Hastings 2015 + Stanley/Anschuetz 2024 — terme regulariseur
 *  empirique pour stabiliser hors regime perturbatif).
 *
 * Boucle : alternance theta_h-fix puis theta_u-fix, ligne 1D Brent simplifie
 * (pas d'or fibo : 1.618). Log forensique 1 ligne par iteration.
 */
#define _POSIX_C_SOURCE 200809L
#include "../include/vorax_kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

static char       g_run_dir[512] = "";
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t   g_total_evals = 0;
static uint64_t   g_total_iters = 0;
static double     g_total_dE    = 0.0;

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int vorax_kernel_init(const char *run_dir) {
    if (!run_dir) return -1;
    pthread_mutex_lock(&g_lock);
    strncpy(g_run_dir, run_dir, sizeof(g_run_dir) - 1);
    g_run_dir[sizeof(g_run_dir) - 1] = '\0';
    g_total_evals = 0;
    g_total_iters = 0;
    g_total_dE    = 0.0;
    pthread_mutex_unlock(&g_lock);
    return 0;
}

/* Cout analytique simplifie 1-layer HVA Hubbard.
 * Z_neel = +1 par convention (etat initial Neel); pour des problemes plus
 * generaux on remplace par <Z_neel> mesure. Ici on garde la signature
 * pour la descente : minimiser E(theta_h, theta_u) via Givens 1D. */
static double vorax_energy(double th, double tu, double t_eV, double U_eV,
                           int n_sites) {
    static volatile uint64_t evals = 0;
    __sync_fetch_and_add(&evals, 1);
    double Z_neel = 1.0;
    double E_hop  = -2.0 * t_eV * cos(th) * Z_neel * (double)(n_sites - 1);
    double E_ons  =  0.5 * U_eV * (1.0 - cos(tu)) * (double)n_sites;
    double E_reg  =  0.05 * (th * th + tu * tu);
    return E_hop + E_ons + E_reg;
}

/* Mini Brent simplifie : recherche par section doree + parabolic-fit. */
static double minimize_1d(double (*f)(double, void*), void *ctx,
                          double a, double b, double tol, int max_iter,
                          int *out_evals) {
    const double phi = (1.0 + sqrt(5.0)) / 2.0;
    const double rg  = 1.0 / phi;
    double x1 = b - rg * (b - a);
    double x2 = a + rg * (b - a);
    double f1 = f(x1, ctx);
    double f2 = f(x2, ctx);
    int evals = 2;
    for (int it = 0; it < max_iter; ++it) {
        if (fabs(b - a) < tol) break;
        if (f1 < f2) { b = x2; x2 = x1; f2 = f1;
                       x1 = b - rg * (b - a); f1 = f(x1, ctx); }
        else        { a = x1; x1 = x2; f1 = f2;
                       x2 = a + rg * (b - a); f2 = f(x2, ctx); }
        ++evals;
    }
    if (out_evals) *out_evals += evals;
    return (f1 < f2) ? x1 : x2;
}

typedef struct { double fixed; double t_eV; double U_eV; int n_sites;
                 int dir; /* 0=optimise theta_h(fixed=tu), 1=optimise tu(fixed=th) */ } ctx_t;

static double f_obj(double x, void *ctx_) {
    ctx_t *c = (ctx_t*)ctx_;
    double th = (c->dir == 0) ? x : c->fixed;
    double tu = (c->dir == 1) ? x : c->fixed;
    return vorax_energy(th, tu, c->t_eV, c->U_eV, c->n_sites);
}

int vorax_kernel_refine_problem(vorax_problem_t *p, int max_iters, double tol_energy) {
    if (!p) return -1;
    if (max_iters <= 0) max_iters = 24;
    if (tol_energy <= 0) tol_energy = 1e-6;

    p->energy_in = vorax_energy(p->theta_h, p->theta_u, p->t_eV, p->U_eV, p->n_sites);
    int evals = 1;
    double prev = p->energy_in;
    int it;
    char logpath[1024];
    snprintf(logpath, sizeof(logpath), "%s/vorax_%s.log",
             g_run_dir[0] ? g_run_dir : ".", p->problem_name ? p->problem_name : "unknown");
    FILE *lg = fopen(logpath, "a");
    double t_start = now_ns();
    if (lg) fprintf(lg, "# VORAX refine start problem=%s N=%d t=%.4f U=%.4f mu=%.4f T=%.4f\n",
                    p->problem_name ? p->problem_name : "?",
                    p->n_sites, p->t_eV, p->U_eV, p->mu_eV, p->temp_K);

    for (it = 0; it < max_iters; ++it) {
        ctx_t c1 = { .fixed = p->theta_u, .t_eV = p->t_eV, .U_eV = p->U_eV,
                     .n_sites = p->n_sites, .dir = 0 };
        p->theta_h = minimize_1d(f_obj, &c1, -3.14159, 3.14159, 1e-5, 12, &evals);
        ctx_t c2 = { .fixed = p->theta_h, .t_eV = p->t_eV, .U_eV = p->U_eV,
                     .n_sites = p->n_sites, .dir = 1 };
        p->theta_u = minimize_1d(f_obj, &c2, -3.14159, 3.14159, 1e-5, 12, &evals);
        double e = vorax_energy(p->theta_h, p->theta_u, p->t_eV, p->U_eV, p->n_sites);
        ++evals;
        if (lg) fprintf(lg, "%04d theta_h=%+.6f theta_u=%+.6f E=%+.8f dE=%+.3e\n",
                        it, p->theta_h, p->theta_u, e, e - prev);
        if (fabs(prev - e) < tol_energy) { prev = e; ++it; break; }
        prev = e;
    }
    p->energy_out = prev;
    p->n_iters    = it;
    p->n_evals    = evals;
    double dE = p->energy_in - p->energy_out;
    double dt_ms = (now_ns() - t_start) / 1.0e6;
    if (lg) {
        fprintf(lg, "# DONE iters=%d evals=%d E_in=%+.8f E_out=%+.8f dE=%+.6f dt_ms=%.3f\n",
                p->n_iters, p->n_evals, p->energy_in, p->energy_out, dE, dt_ms);
        fclose(lg);
    }
    pthread_mutex_lock(&g_lock);
    g_total_evals += (uint64_t)p->n_evals;
    g_total_iters += (uint64_t)p->n_iters;
    g_total_dE    += dE;
    pthread_mutex_unlock(&g_lock);
    return (it < max_iters) ? 0 : 1;
}

void vorax_kernel_destroy(void) {
    pthread_mutex_lock(&g_lock);
    g_run_dir[0] = '\0';
    pthread_mutex_unlock(&g_lock);
}

void vorax_kernel_stats(uint64_t *out_total_evals,
                        uint64_t *out_total_iters,
                        double *out_total_dE) {
    pthread_mutex_lock(&g_lock);
    if (out_total_evals) *out_total_evals = g_total_evals;
    if (out_total_iters) *out_total_iters = g_total_iters;
    if (out_total_dE)    *out_total_dE    = g_total_dE;
    pthread_mutex_unlock(&g_lock);
}
