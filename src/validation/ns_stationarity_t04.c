/* **************************************************************************
** ns_stationarity_t04.c — T04-STRONG : analyse multi-points quasi-stationnarité
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / T04-STRONG
** Auteur : LumVorax Project
**
** Ferme le chantier T04 du registre 176 §5 :
**   « La fermeture forte nécessite une analyse de la fenêtre finale
**     et pas uniquement deux points. »
**
** Fermeture requise (registre 176 §5) :
**   ✓ analyse de tous les derniers échantillons  → fenêtre StationarityWindow
**   ✓ variation max-min                          → range_rel
**   ✓ moyenne                                    → mean
**   ✓ écart-type                                 → std / std_rel
**   ✓ pente                                      → slope_rel (OLS)
**   ✓ critère explicite de quasi-stationnarité   → QS_RANGE_REL / STD / SLOPE
**   ✓ exécution reproductible                    → log 029_ns_stationarity_t04.txt
**
** Tests :
**   T04S-1 — Fenêtre Linf contient >= QS_MIN_POINTS points
**   T04S-2 — range_rel < QS_RANGE_REL (variation max-min relative)
**   T04S-3 — std_rel   < QS_STD_REL   (écart-type relatif)
**   T04S-4 — slope_rel < QS_SLOPE_REL (pente OLS normalisée)
**   T04S-5 — quasi_stationary == 1 (verdict global)
**   T04S-6 — Linf_final < 0.05       (critère T04c original — non régressé)
**   T04S-7 — u_max <= 1.0 + 1e-6     (pas d'overshoot — T05c)
**   T04S-8 — u_min >= 0.0 - 1e-6     (pas d'undershoot — T06c)
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"
#include "../debug/forensic_logger.h"
#include "../common/time_ns.h"
#include "ns_stationarity_analysis.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

/* ── Paramètres simulation ───────────────────────────────────────────────── */

#define N_SIM           128       /* grille protocole C (même que 003c) */
#define RE_COUETTE      100.0
#define DT_BASE         1e-4
#define DX_BASE         (1.0 / 32)

/* Proto C : dt ∝ dx² */
static double get_dt_protoC(int n)
{
    double dx = 1.0 / (double)n;
    return DT_BASE * (dx / DX_BASE) * (dx / DX_BASE);
}

/* Stationnarité : fenêtre de collecte */
#define POLL_INTERVAL   50
#define WIN_CAPACITY    200      /* 200 × POLL_INTERVAL = 10 000 pas de fenêtre */
#define LIMIT_STEPS     500000

/* Log */
#define LOG_PATH "logs/forensic/ns_stationarity_t04.log"
#define RUN_LOG  "logs/029_ns_stationarity_t04.txt"

/* ── Accesseurs MAC (locaux) ─────────────────────────────────────────────── */

#define U(s, i, j)  ((s)->u[(i) * ((s)->params.ny + 2) + (j)])
#define V(s, i, j)  ((s)->v[(i) * ((s)->params.ny + 1) + (j)])
#define P(s, i, j)  ((s)->p[(i) * ((s)->params.ny + 2) + (j)])

/* ── Solution Couette + CL ───────────────────────────────────────────────── */

static double couette_exact(double y) { return y; }

static void set_couette_bc(NSSolver2D *s)
{
    int nx = s->params.nx, ny = s->params.ny;

    for (int i = 0; i <= nx; i++) {
        U(s, i, 0)      = -U(s, i, 1);
        U(s, i, ny + 1) =  2.0 - U(s, i, ny);
    }
    for (int j = 0; j <= ny + 1; j++) {
        U(s, 0,  j) = U(s, nx - 1, j);
        U(s, nx, j) = U(s, 1,      j);
    }
    for (int i = 0; i <= nx + 1; i++) {
        V(s, i, 0)  = 0.0;
        V(s, i, ny) = 0.0;
    }
    for (int j = 0; j <= ny; j++) {
        V(s, 0,      j) = 0.0;
        V(s, nx + 1, j) = 0.0;
    }
    for (int j = 0; j <= ny + 1; j++) {
        P(s, 0,      j) = P(s, 1,  j);
        P(s, nx + 1, j) = P(s, nx, j);
    }
    for (int i = 0; i <= nx + 1; i++) {
        P(s, i, 0)      = P(s, i, 1);
        P(s, i, ny + 1) = P(s, i, ny);
    }
}

/* ── Calcul Linf ─────────────────────────────────────────────────────────── */

static double compute_linf(const NSSolver2D *s)
{
    int    nx   = s->params.nx, ny = s->params.ny;
    double dy   = s->dy;
    double linf = 0.0;

    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double y_node = (j - 0.5) * dy;
            double err    = fabs(s->u[i * (ny + 2) + j] - couette_exact(y_node));
            if (err > linf) linf = err;
        }
    }
    return linf;
}

/* ── Calcul u_min / u_max ────────────────────────────────────────────────── */

static void compute_urange(const NSSolver2D *s, double *u_min_out, double *u_max_out)
{
    int    nx   = s->params.nx, ny = s->params.ny;
    double umin =  1e30, umax = -1e30;

    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double u = s->u[i * (ny + 2) + j];
            if (u < umin) umin = u;
            if (u > umax) umax = u;
        }
    }
    *u_min_out = umin;
    *u_max_out = umax;
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    /* Ouvrir le log brut */
    FILE *runlog = fopen(RUN_LOG, "w");
    if (!runlog) {
        fprintf(stderr, "[T04-STRONG][FATAL] Impossible d'ouvrir %s\n", RUN_LOG);
        return 1;
    }

#define LOG(...) do {                         \
    printf(__VA_ARGS__);                      \
    fprintf(runlog, __VA_ARGS__);             \
} while(0)

    LOG("=== T04-STRONG : ANALYSE MULTI-POINTS QUASI-STATIONNARITE ===\n");
    LOG("[SESSION] S177 | CERTIFIED_100=false | unique_human_proven=false\n");
    LOG("[MODE] DEBUG actif\n");
    LOG("[REF] Registre 176 §5 — T04 OPEN renforcé\n");
    LOG("[GRILLE] %d × %d | Proto C : dt prop dx^2\n\n", N_SIM, N_SIM);

    forensic_logger_init(LOG_PATH);

    double dt  = get_dt_protoC(N_SIM);
    double dx  = 1.0 / (double)N_SIM;

    LOG("[PARAM] Re=%.1f | N=%d | dx=%.4e | dt=%.4e\n", RE_COUETTE, N_SIM, dx, dt);
    LOG("[PARAM] POLL_INTERVAL=%d | WIN_CAPACITY=%d | LIMIT_STEPS=%d\n\n",
        POLL_INTERVAL, WIN_CAPACITY, LIMIT_STEPS);

    NSParams p = {
        .nx          = N_SIM,
        .ny          = N_SIM,
        .lx          = 1.0,
        .ly          = 1.0,
        .re          = RE_COUETTE,
        .dt          = dt,
        .max_iter    = 1,
        .tol         = 1e-6,
        .max_poisson = 100,
        .debug       = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        LOG("[FATAL] ns_solver_create echec — N=%d\n", N_SIM);
        fclose(runlog);
        forensic_logger_destroy();
        return 1;
    }

    /* Initialisation analytique u = y (comme 003c Proto C) */
    {
        double dy_i = s->dy;
        int    ny_i = s->params.ny, nx_i = s->params.nx;
        for (int ii = 0; ii <= nx_i; ii++)
            for (int jj = 0; jj <= ny_i + 1; jj++)
                U(s, ii, jj) = (jj - 0.5) * dy_i;
    }
    set_couette_bc(s);

    /* Fenêtre de stationnarité */
    StationarityWindow *sw = stationarity_window_create(WIN_CAPACITY);
    if (!sw) {
        LOG("[FATAL] stationarity_window_create(%d) echec\n", WIN_CAPACITY);
        ns_solver_destroy(s);
        fclose(runlog);
        forensic_logger_destroy();
        return 1;
    }

    /* Boucle de simulation avec collecte Linf à chaque POLL_INTERVAL pas */
    int    total_steps = 0;
    int    converged   = 0;
    double linf_final  = 0.0;

    LOG("=== BOUCLE SIMULATION (pas par pas) ===\n");
    LOG("  [step]   [Linf]       [stationnarite_window_n]\n");

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    while (total_steps < LIMIT_STEPS) {
        for (int k = 0; k < POLL_INTERVAL && total_steps < LIMIT_STEPS; k++) {
            (void)ns_solver_step_with_bc(s, set_couette_bc);
            total_steps++;
        }

        double linf = compute_linf(s);
        stationarity_window_push(sw, linf, total_steps);
        linf_final = linf;

        /* Affichage intermédiaire tous les 5000 pas */
        if (total_steps % 5000 == 0) {
            LOG("  step=%-7d  Linf=%.6e  win_n=%d\n",
                total_steps, linf, sw->count);
        }

        /* Vérification stationnarité forte dès que la fenêtre est pleine */
        if (sw->count >= WIN_CAPACITY) {
            StationarityResult r = stationarity_analyze(sw);
            if (r.quasi_stationary) {
                converged = 1;
                LOG("\n  [CONVERGENCE] step=%d  Linf=%.6e — critere QS satisfait\n\n",
                    total_steps, linf);
                break;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double wall_s = (t_end.tv_sec  - t_start.tv_sec)
                  + (t_end.tv_nsec - t_start.tv_nsec) * 1e-9;

    LOG("=== FIN SIMULATION ===\n");
    LOG("  steps=%d | t_phys=%.4f s | wall=%.2f s | converged=%d\n\n",
        total_steps, (double)total_steps * dt, wall_s, converged);

    /* ── Analyse finale multi-points ──────────────────────────────────────── */
    LOG("=== ANALYSE MULTI-POINTS FENETRE FINALE ===\n\n");

    StationarityResult final_r = stationarity_analyze(sw);

    LOG("  n_points  = %d  (seuil >= %d)\n", final_r.n_points, QS_MIN_POINTS);
    LOG("  min_val   = %.8e\n", final_r.min_val);
    LOG("  max_val   = %.8e\n", final_r.max_val);
    LOG("  range     = %.8e\n", final_r.range);
    LOG("  mean      = %.8e\n", final_r.mean);
    LOG("  std       = %.8e\n", final_r.std);
    LOG("  range_rel = %.6f  (seuil < %.4f)\n", final_r.range_rel, (double)QS_RANGE_REL);
    LOG("  std_rel   = %.6f  (seuil < %.4f)\n", final_r.std_rel,   (double)QS_STD_REL);
    LOG("  slope     = %.8e  (pente OLS brute)\n", final_r.slope);
    LOG("  intercept = %.8e\n", final_r.intercept);
    LOG("  slope_rel = %.6e  (seuil < %.4f)\n", final_r.slope_rel, (double)QS_SLOPE_REL);
    LOG("\n  DETAIL    : %s\n", final_r.reason);
    LOG("  VERDICT   : %s\n\n",
        final_r.quasi_stationary ? "QUASI_STATIONNAIRE" : "NON_STATIONNAIRE");

    /* u_min / u_max pour T04S-7 / T04S-8 */
    double u_min, u_max;
    compute_urange(s, &u_min, &u_max);

    /* ── Tests T04S-1 → T04S-8 ───────────────────────────────────────────── */
    LOG("=== TESTS T04S-1 → T04S-8 ===\n\n");

    int t04s1 = (final_r.n_points >= QS_MIN_POINTS);
    int t04s2 = (final_r.range_rel < QS_RANGE_REL);
    int t04s3 = (final_r.std_rel   < QS_STD_REL);
    int t04s4 = (final_r.slope_rel < QS_SLOPE_REL);
    int t04s5 = final_r.quasi_stationary;
    int t04s6 = (linf_final < 0.05);
    int t04s7 = (u_max <= 1.0 + 1e-6);
    int t04s8 = (u_min >= 0.0 - 1e-6);

    LOG("  T04S-1 — n_points >= %d              : %s  (n=%d)\n",
        QS_MIN_POINTS, t04s1 ? "PASS" : "FAIL", final_r.n_points);
    LOG("  T04S-2 — range_rel < %.4f             : %s  (range_rel=%.6f)\n",
        (double)QS_RANGE_REL, t04s2 ? "PASS" : "FAIL", final_r.range_rel);
    LOG("  T04S-3 — std_rel < %.4f               : %s  (std_rel=%.6f)\n",
        (double)QS_STD_REL, t04s3 ? "PASS" : "FAIL", final_r.std_rel);
    LOG("  T04S-4 — slope_rel < %.4f             : %s  (slope_rel=%.2e)\n",
        (double)QS_SLOPE_REL, t04s4 ? "PASS" : "FAIL", final_r.slope_rel);
    LOG("  T04S-5 — quasi_stationary == 1        : %s\n",
        t04s5 ? "PASS" : "FAIL");
    LOG("  T04S-6 — Linf_final < 0.05            : %s  (Linf=%.6e)\n",
        t04s6 ? "PASS" : "FAIL", linf_final);
    LOG("  T04S-7 — u_max <= 1.0 + 1e-6          : %s  (u_max=%.6f)\n",
        t04s7 ? "PASS" : "FAIL", u_max);
    LOG("  T04S-8 — u_min >= 0.0 - 1e-6          : %s  (u_min=%.6f)\n\n",
        t04s8 ? "PASS" : "FAIL", u_min);

    /* Forensic checkpoint */
    uint64_t ts_abs = time_ns_get_absolute();
    char op_buf[128];
    snprintf(op_buf, sizeof(op_buf),
             "T04-STRONG:n=%d:Linf=%.3e:range_rel=%.4f:std_rel=%.4f:slope_rel=%.2e:qs=%d",
             N_SIM, linf_final, final_r.range_rel, final_r.std_rel, final_r.slope_rel,
             final_r.quasi_stationary);
    uint64_t lum_id = ((uint64_t)(N_SIM & 0xFFFFU) << 48)
                    | ((uint64_t)(final_r.quasi_stationary & 0x1U) << 47);
    forensic_log_individual_lum(lum_id, op_buf, ts_abs);

    /* ── Limites honnêtes ─────────────────────────────────────────────────── */
    LOG("=== LIMITES HONNETES ===\n\n");
    LOG("  - Seuils QS : RANGE_REL=%.4f, STD_REL=%.4f, SLOPE_REL=%.4f — parametres, pas absolus.\n",
        (double)QS_RANGE_REL, (double)QS_STD_REL, (double)QS_SLOPE_REL);
    LOG("  - Grille %d×%d Proto C uniquement (dt prop dx^2).\n", N_SIM, N_SIM);
    LOG("  - Couette plan : advection inactive (u.grad_u = 0).\n");
    LOG("  - CERTIFIED_100=false | unique_human_proven=false\n\n");

    /* ── Verdict global ───────────────────────────────────────────────────── */
    int all_pass = t04s1 && t04s2 && t04s3 && t04s4 && t04s5
                && t04s6 && t04s7 && t04s8;

    LOG("[VERDICT] T04-STRONG : %s\n",
        all_pass
        ? "PASS — 8/8 tests quasi-stationnarite multi-points (registre 176 §5)"
        : "FAIL honnete — voir T04S-1..T04S-8 ci-dessus");
    LOG("[NOTE] T04S-1=%s T04S-2=%s T04S-3=%s T04S-4=%s "
               "T04S-5=%s T04S-6=%s T04S-7=%s T04S-8=%s\n",
        t04s1?"PASS":"FAIL", t04s2?"PASS":"FAIL",
        t04s3?"PASS":"FAIL", t04s4?"PASS":"FAIL",
        t04s5?"PASS":"FAIL", t04s6?"PASS":"FAIL",
        t04s7?"PASS":"FAIL", t04s8?"PASS":"FAIL");
    LOG("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    stationarity_window_destroy(sw);
    ns_solver_destroy(s);
    forensic_logger_destroy();
    fclose(runlog);

    return all_pass ? 0 : 1;

#undef LOG
}
