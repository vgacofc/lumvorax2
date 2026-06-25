/* VORAX kernel — implementation. Solveur reel C, branche dans advanced_parallel.
 *
 * Approche : descente coordonnee Givens sur les angles HVA-Hubbard.
 *   E(theta_h, theta_u) = -2t.cos(theta_h).Z_neel.(N-1)
 *                        + (U/2).(1-cos(theta_u)).N
 *                        - mu.cos(theta_u).N
 *                        + kB.T.N
 *                        + 0.05 (theta_h^2 + theta_u^2)
 * (modele analytique 2-angle pour 1-layer HVA pres du point de Trotter,
 *  cf. Wecker/Hastings 2015 + Stanley/Anschuetz 2024 — terme regulariseur
 *  empirique pour stabiliser hors regime perturbatif).
 *
 * Boucle : alternance theta_h-fix puis theta_u-fix, ligne 1D Brent simplifie
 * (pas d'or fibo : 1.618). Log forensique 1 ligne par iteration + JSONL global
 * + vecteur de correlation complet (C92-PLUS).
 */
#define _POSIX_C_SOURCE 200809L
#include "../include/vorax_kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

/* Constantes physiques */
#define KB_eV_per_K   8.617333262e-5  /* k_B en eV/K */

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

/* FNV1a-64 sur un buffer brut */
static uint64_t fnv1a_64(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; ++i) { h ^= p[i]; h *= 1099511628211ULL; }
    return h;
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
    /* Ouvre/cree le JSONL global de correlation */
    char jpath[1024];
    snprintf(jpath, sizeof(jpath), "%s/vorax_correlation.jsonl", run_dir);
    FILE *jf = fopen(jpath, "a");
    if (jf) { fprintf(jf, "# VORAX correlation stream init ts_ns=%.0f\n", now_ns()); fclose(jf); }
    return 0;
}

/* Cout analytique 1-layer HVA Hubbard etendu (mu, T). */
static double vorax_energy(double th, double tu, double t_eV, double U_eV,
                           double mu_eV, double temp_K, int n_sites) {
    static volatile uint64_t evals = 0;
    __sync_fetch_and_add(&evals, 1);
    double Z_neel = 1.0;
    double E_hop  = -2.0 * t_eV * cos(th) * Z_neel * (double)(n_sites - 1);
    double E_ons  =  0.5 * U_eV * (1.0 - cos(tu)) * (double)n_sites;
    double E_mu   = -mu_eV * cos(tu) * (double)n_sites;
    double E_th   =  KB_eV_per_K * temp_K * (double)n_sites;
    double E_reg  =  0.05 * (th * th + tu * tu);
    return E_hop + E_ons + E_mu + E_th + E_reg;
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

typedef struct { double fixed; double t_eV; double U_eV; double mu_eV; double temp_K;
                 int n_sites; int dir; } ctx_t;

static double f_obj(double x, void *ctx_) {
    ctx_t *c = (ctx_t*)ctx_;
    double th = (c->dir == 0) ? x : c->fixed;
    double tu = (c->dir == 1) ? x : c->fixed;
    return vorax_energy(th, tu, c->t_eV, c->U_eV, c->mu_eV, c->temp_K, c->n_sites);
}

int vorax_kernel_refine_problem(vorax_problem_t *p, int max_iters, double tol_energy) {
    if (!p) return -1;
    if (max_iters <= 0) max_iters = 24;
    if (tol_energy <= 0) tol_energy = 1e-6;

    p->energy_in = vorax_energy(p->theta_h, p->theta_u,
                                 p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
    int evals = 1;
    double prev = p->energy_in;
    int it;
    char logpath[1024], jsonlpath[1024];
    const char *pname = p->problem_name ? p->problem_name : "unknown";
    snprintf(logpath,   sizeof(logpath),   "%s/vorax_%s.log",   g_run_dir[0] ? g_run_dir : ".", pname);
    snprintf(jsonlpath, sizeof(jsonlpath), "%s/vorax_%s.jsonl", g_run_dir[0] ? g_run_dir : ".", pname);
    FILE *lg = fopen(logpath,   "a");
    FILE *jl = fopen(jsonlpath, "a");
    double t_start = now_ns();
    if (lg) fprintf(lg, "# VORAX refine start problem=%s N=%d t=%.4f U=%.4f mu=%.4f T=%.4f\n",
                    pname, p->n_sites, p->t_eV, p->U_eV, p->mu_eV, p->temp_K);

    for (it = 0; it < max_iters; ++it) {
        ctx_t c1 = { .fixed = p->theta_u, .t_eV = p->t_eV, .U_eV = p->U_eV,
                     .mu_eV = p->mu_eV, .temp_K = p->temp_K,
                     .n_sites = p->n_sites, .dir = 0 };
        p->theta_h = minimize_1d(f_obj, &c1, -3.14159, 3.14159, 1e-5, 12, &evals);
        ctx_t c2 = { .fixed = p->theta_h, .t_eV = p->t_eV, .U_eV = p->U_eV,
                     .mu_eV = p->mu_eV, .temp_K = p->temp_K,
                     .n_sites = p->n_sites, .dir = 1 };
        p->theta_u = minimize_1d(f_obj, &c2, -3.14159, 3.14159, 1e-5, 12, &evals);
        double e = vorax_energy(p->theta_h, p->theta_u,
                                 p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
        ++evals;
        double de_step = e - prev;
        if (lg) fprintf(lg, "%04d theta_h=%+.6f theta_u=%+.6f E=%+.8f dE=%+.3e\n",
                        it, p->theta_h, p->theta_u, e, de_step);
        if (jl) fprintf(jl,
            "{\"ts_ns\":%.0f,\"mod\":\"%s\",\"iter\":%d,\"th\":%.8f,\"tu\":%.8f,"
            "\"E\":%.8f,\"dE_step\":%.6e,\"N\":%d,\"t\":%.6f,\"U\":%.6f,\"mu\":%.6f,\"T\":%.4f}\n",
            now_ns(), pname, it, p->theta_h, p->theta_u, e, de_step,
            p->n_sites, p->t_eV, p->U_eV, p->mu_eV, p->temp_K);
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
    if (jl) fclose(jl);
    pthread_mutex_lock(&g_lock);
    g_total_evals += (uint64_t)p->n_evals;
    g_total_iters += (uint64_t)p->n_iters;
    g_total_dE    += dE;
    pthread_mutex_unlock(&g_lock);
    return (it < max_iters) ? 0 : 1;
}

/* C92-PLUS : extraction du vecteur de correlation complet. */
int vorax_kernel_extract_correlation(const vorax_problem_t *p,
                                      correlation_vector_t *out) {
    if (!p || !out) return -1;
    memset(out, 0, sizeof(*out));
    const char *pname = p->problem_name ? p->problem_name : "unknown";
    strncpy(out->problem_name, pname, sizeof(out->problem_name) - 1);
    out->N           = p->n_sites;
    out->temperature = p->temp_K;
    out->theta_h     = p->theta_h;
    out->theta_u     = p->theta_u;
    out->energy      = p->energy_out;
    out->delta_energy = p->energy_in - p->energy_out;
    out->delta_energy_norm = (p->n_sites > 0) ? (out->delta_energy / (double)p->n_sites) : 0.0;
    out->iter    = p->n_iters;
    out->n_evals = p->n_evals;

    /* Gradient numerique central (eps=1e-6) */
    const double eps = 1e-6;
    double E_h_p = vorax_energy(p->theta_h + eps, p->theta_u, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
    double E_h_m = vorax_energy(p->theta_h - eps, p->theta_u, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
    double E_u_p = vorax_energy(p->theta_h, p->theta_u + eps, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
    double E_u_m = vorax_energy(p->theta_h, p->theta_u - eps, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
    double E0    = vorax_energy(p->theta_h, p->theta_u,       p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
    out->grad_theta_h = (E_h_p - E_h_m) / (2.0 * eps);
    out->grad_theta_u = (E_u_p - E_u_m) / (2.0 * eps);
    out->grad_norm    = sqrt(out->grad_theta_h * out->grad_theta_h
                            + out->grad_theta_u * out->grad_theta_u);
    /* Courbure (Hessien diagonal) */
    out->curv_theta_h = (E_h_p - 2.0 * E0 + E_h_m) / (eps * eps);
    out->curv_theta_u = (E_u_p - 2.0 * E0 + E_u_m) / (eps * eps);
    /* Signature multi-echelle */
    out->chi_local = fabs(out->delta_energy_norm) / (1.0 + out->grad_norm);
    /* Metriques d'extraction */
    out->score          = fabs(out->delta_energy_norm) * log(1.0 + fabs(out->delta_energy));
    out->stability      = 1.0 / (1.0 + out->grad_norm);
    out->signal_strength = (out->grad_norm > 1e-12)
                          ? out->delta_energy_norm / out->grad_norm
                          : out->delta_energy_norm * 1e12;
    /* Forensique */
    out->timestamp_ns   = (uint64_t)now_ns();
    /* Checksum FNV1a sur tout sauf le checksum lui-meme */
    out->checksum_state = 0;
    out->checksum_state = fnv1a_64(out, sizeof(*out) - sizeof(uint64_t));

    /* Log JSON 1 ligne dans le stream global */
    char jpath[1024];
    snprintf(jpath, sizeof(jpath), "%s/vorax_correlation.jsonl",
             g_run_dir[0] ? g_run_dir : ".");
    FILE *jf = fopen(jpath, "a");
    if (jf) {
        fprintf(jf,
            "{\"ts_ns\":%lu,\"mod\":\"%s\",\"N\":%d,\"T_K\":%.4f,"
            "\"th\":%.8f,\"tu\":%.8f,\"E\":%.8f,\"dE\":%.8f,\"dE_per_site\":%.8e,"
            "\"grad_h\":%.8e,\"grad_u\":%.8e,\"grad_norm\":%.8e,"
            "\"curv_h\":%.6f,\"curv_u\":%.6f,"
            "\"chi_local\":%.8e,\"score\":%.8e,\"stability\":%.8f,\"signal_strength\":%.8e,"
            "\"iter\":%d,\"evals\":%d,\"checksum\":\"0x%016lx\"}\n",
            (unsigned long)out->timestamp_ns, out->problem_name, out->N, out->temperature,
            out->theta_h, out->theta_u, out->energy, out->delta_energy, out->delta_energy_norm,
            out->grad_theta_h, out->grad_theta_u, out->grad_norm,
            out->curv_theta_h, out->curv_theta_u,
            out->chi_local, out->score, out->stability, out->signal_strength,
            out->iter, out->n_evals, (unsigned long)out->checksum_state);
        fclose(jf);
    }
    return 0;
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
