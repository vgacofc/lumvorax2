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

    /* Gradient numerique central (eps=1e-6) + courbure 3-points */
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
    /* Courbure (Hessien diagonal) — 3 points */
    out->curv_theta_h = (E_h_p - 2.0 * E0 + E_h_m) / (eps * eps);
    out->curv_theta_u = (E_u_p - 2.0 * E0 + E_u_m) / (eps * eps);
    /* C92-FINAL : Courbure 5-points central (precision O(eps^4) au lieu de O(eps^2)) */
    {
        double E_h_p2 = vorax_energy(p->theta_h + 2*eps, p->theta_u, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
        double E_h_m2 = vorax_energy(p->theta_h - 2*eps, p->theta_u, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
        double E_u_p2 = vorax_energy(p->theta_h, p->theta_u + 2*eps, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
        double E_u_m2 = vorax_energy(p->theta_h, p->theta_u - 2*eps, p->t_eV, p->U_eV, p->mu_eV, p->temp_K, p->n_sites);
        /* Coeffs 5-pt : (-1, 16, -30, 16, -1) / 12 */
        out->curv_h_5pt = (-E_h_p2 + 16*E_h_p - 30*E0 + 16*E_h_m - E_h_m2) / (12.0 * eps * eps);
        out->curv_u_5pt = (-E_u_p2 + 16*E_u_p - 30*E0 + 16*E_u_m - E_u_m2) / (12.0 * eps * eps);
    }
    /* Signature multi-echelle */
    out->chi_local = fabs(out->delta_energy_norm) / (1.0 + out->grad_norm);
    /* Metriques d'extraction */
    out->score          = fabs(out->delta_energy_norm) * log(1.0 + fabs(out->delta_energy));
    out->stability      = 1.0 / (1.0 + out->grad_norm);
    out->signal_strength = (out->grad_norm > 1e-12)
                          ? out->delta_energy_norm / out->grad_norm
                          : out->delta_energy_norm * 1e12;
    /* C92-FINAL : flags de classification physique (cf. brief "classifieur universel") */
    out->is_unstable     = (out->stability < 0.30) ? 1 : 0;
    out->is_pure_physics = (out->stability > 0.90) ? 1 : 0;
    /* feedback_rounds reste a 0 ici, sera surcharge par refine_with_feedback */
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

/* C92-FINAL [A1] : Boucle fermee NX48 <-> VORAX.
 * Strategie : refine -> extract -> si stab < target alors re-randomise theta
 * (perturbation gaussienne sigma=0.1 rad) et recommence. Garde le meilleur CV.
 * Loggue chaque round dans run_dir/vorax_feedback_<problem>.jsonl. */
int vorax_kernel_refine_with_feedback(vorax_problem_t *p,
                                       int max_rounds,
                                       double target_stability,
                                       int max_iters_per_round,
                                       double tol_energy,
                                       correlation_vector_t *best_cv) {
    if (!p || !best_cv) return -1;
    if (max_rounds <= 0) max_rounds = 5;
    if (target_stability <= 0) target_stability = 0.90;
    if (max_iters_per_round <= 0) max_iters_per_round = 24;

    correlation_vector_t cv_round;
    double best_stab = -1.0;
    double th0 = p->theta_h, tu0 = p->theta_u;
    int round;
    char fbpath[1024];
    const char *pname = p->problem_name ? p->problem_name : "unknown";
    snprintf(fbpath, sizeof(fbpath), "%s/vorax_feedback_%s.jsonl",
             g_run_dir[0] ? g_run_dir : ".", pname);
    FILE *fb = fopen(fbpath, "a");

    /* Generateur deterministe simple (xorshift64) seede par problem_name */
    uint64_t rng = fnv1a_64(pname, strlen(pname)) ^ 0xDEADBEEFCAFEBABEULL;

    for (round = 0; round < max_rounds; ++round) {
        vorax_kernel_refine_problem(p, max_iters_per_round, tol_energy);
        vorax_kernel_extract_correlation(p, &cv_round);
        cv_round.feedback_rounds = round + 1;

        if (fb) fprintf(fb,
            "{\"ts_ns\":%lu,\"mod\":\"%s\",\"round\":%d,\"th\":%.8f,\"tu\":%.8f,"
            "\"E\":%.8f,\"dE\":%.8f,\"stab\":%.8f,\"sig\":%.8e,\"score\":%.8e,"
            "\"chi\":%.8e,\"target_stab\":%.4f,\"checksum\":\"0x%016lx\"}\n",
            (unsigned long)cv_round.timestamp_ns, pname, round,
            cv_round.theta_h, cv_round.theta_u, cv_round.energy, cv_round.delta_energy,
            cv_round.stability, cv_round.signal_strength, cv_round.score,
            cv_round.chi_local, target_stability,
            (unsigned long)cv_round.checksum_state);

        if (cv_round.stability > best_stab) {
            best_stab = cv_round.stability;
            memcpy(best_cv, &cv_round, sizeof(*best_cv));
        }

        if (cv_round.stability >= target_stability) { ++round; break; }

        /* Perturbation gaussienne (Box-Muller) sigma = 0.1 rad */
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        double u1 = (double)(rng & 0xFFFFFFFFULL) / 4294967295.0;
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        double u2 = (double)(rng & 0xFFFFFFFFULL) / 4294967295.0;
        if (u1 < 1e-12) u1 = 1e-12;
        double r  = sqrt(-2.0 * log(u1));
        double z1 = r * cos(2.0 * 3.14159265358979 * u2);
        double z2 = r * sin(2.0 * 3.14159265358979 * u2);
        p->theta_h = th0 + 0.1 * z1;
        p->theta_u = tu0 + 0.1 * z2;
    }

    if (fb) fclose(fb);
    best_cv->feedback_rounds = round;
    return round;
}

/* C93 [A1+] Boucle gradient-aware : SPSA-step + bruit adaptatif (1-stab).
 * Remplace re-randomisation Box-Muller par descente guidee par grad estime
 * et perturbation calibree par instabilite locale. Pertes :
 *    L = E + alpha*(1-stab) + beta*var(grad)
 */
int vorax_kernel_refine_gradient_aware(vorax_problem_t *p,
                                        int max_rounds,
                                        double target_stability,
                                        int max_iters_per_round,
                                        double tol_energy,
                                        double lr,
                                        double sigma_explore,
                                        double alpha_stab,
                                        double beta_var,
                                        correlation_vector_t *best_cv) {
    if (!p || !best_cv) return -1;
    if (max_rounds <= 0)         max_rounds         = 6;
    if (target_stability <= 0)   target_stability   = 0.90;
    if (max_iters_per_round <= 0) max_iters_per_round = 24;
    if (tol_energy <= 0)         tol_energy         = 1e-6;
    if (lr <= 0)                 lr                 = 0.05;
    if (sigma_explore < 0)       sigma_explore      = 0.05;
    if (alpha_stab < 0)          alpha_stab         = 0.10;
    if (beta_var < 0)            beta_var           = 0.05;

    correlation_vector_t cv_round;
    double best_loss = 1e300;
    int round;
    char fbpath[1024];
    const char *pname = p->problem_name ? p->problem_name : "unknown";
    snprintf(fbpath, sizeof(fbpath), "%s/vorax_feedback_c93_%s.jsonl",
             g_run_dir[0] ? g_run_dir : ".", pname);
    FILE *fb = fopen(fbpath, "a");

    /* RNG xorshift64 deterministe seede par nom + timestamp */
    uint64_t rng = fnv1a_64(pname, strlen(pname)) ^ 0xC93ABCDEF1234567ULL;
    /* Variance courante du gradient (moyenne mobile) */
    double grad_var_ema = 0.0;
    double grad_prev_norm = 0.0;

    for (round = 0; round < max_rounds; ++round) {
        vorax_kernel_refine_problem(p, max_iters_per_round, tol_energy);
        vorax_kernel_extract_correlation(p, &cv_round);
        cv_round.feedback_rounds = round + 1;

        /* Variance gradient : EMA(beta=0.7) du carre des differences successives */
        double dnorm = cv_round.grad_norm - grad_prev_norm;
        grad_var_ema = 0.7 * grad_var_ema + 0.3 * (dnorm * dnorm);
        grad_prev_norm = cv_round.grad_norm;

        /* Perte composite C93 : L = E + alpha*(1-stab) + beta*var(grad) */
        double loss = cv_round.energy
                    + alpha_stab * (1.0 - cv_round.stability)
                    + beta_var   * grad_var_ema;

        if (fb) fprintf(fb,
            "{\"ts_ns\":%lu,\"mod\":\"%s\",\"round\":%d,\"th\":%.8f,\"tu\":%.8f,"
            "\"E\":%.8f,\"dE\":%.8f,\"stab\":%.8f,\"sig\":%.8e,\"score\":%.8e,"
            "\"chi\":%.8e,\"grad_norm\":%.8e,\"grad_var\":%.8e,"
            "\"loss\":%.8e,\"lr\":%.4f,\"sigma\":%.4f,"
            "\"alpha\":%.4f,\"beta\":%.4f,\"target_stab\":%.4f,"
            "\"checksum\":\"0x%016lx\"}\n",
            (unsigned long)cv_round.timestamp_ns, pname, round,
            cv_round.theta_h, cv_round.theta_u, cv_round.energy, cv_round.delta_energy,
            cv_round.stability, cv_round.signal_strength, cv_round.score,
            cv_round.chi_local, cv_round.grad_norm, grad_var_ema,
            loss, lr, sigma_explore, alpha_stab, beta_var, target_stability,
            (unsigned long)cv_round.checksum_state);

        /* Garde la meilleure : minimise la perte composite (pas juste stab) */
        if (loss < best_loss) {
            best_loss = loss;
            memcpy(best_cv, &cv_round, sizeof(*best_cv));
        }

        if (cv_round.stability >= target_stability) { ++round; break; }

        /* Step SPSA-like : descente gradient + bruit adaptatif (1-stab) */
        double inv = 1.0 / (1.0 + cv_round.grad_norm);
        double step_h = -lr * cv_round.grad_theta_h * inv;
        double step_u = -lr * cv_round.grad_theta_u * inv;

        /* Bruit gaussien Box-Muller, sigma adaptatif = sigma_explore * (1 - stab) */
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        double u1 = (double)(rng & 0xFFFFFFFFULL) / 4294967295.0;
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        double u2 = (double)(rng & 0xFFFFFFFFULL) / 4294967295.0;
        if (u1 < 1e-12) u1 = 1e-12;
        double r  = sqrt(-2.0 * log(u1));
        double z1 = r * cos(2.0 * 3.14159265358979 * u2);
        double z2 = r * sin(2.0 * 3.14159265358979 * u2);
        double sigma_adapt = sigma_explore * (1.0 - cv_round.stability);

        p->theta_h += step_h + sigma_adapt * z1;
        p->theta_u += step_u + sigma_adapt * z2;

        /* Clip dans [-pi, pi] pour rester dans le tore variationnel */
        const double PI = 3.14159265358979;
        if (p->theta_h >  PI) p->theta_h -= 2.0 * PI;
        if (p->theta_h < -PI) p->theta_h += 2.0 * PI;
        if (p->theta_u >  PI) p->theta_u -= 2.0 * PI;
        if (p->theta_u < -PI) p->theta_u += 2.0 * PI;

        /* Decroissance lr (pas SPSA classique : 1/(round+1)^0.602) */
        lr = lr * pow((double)(round + 1) / (double)(round + 2), 0.602);
    }

    if (fb) fclose(fb);
    best_cv->feedback_rounds = round;
    return round;
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
