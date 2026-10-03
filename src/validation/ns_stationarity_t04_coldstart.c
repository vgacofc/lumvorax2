/* **************************************************************************
** ns_stationarity_t04_coldstart.c — T04 hors-équilibre : convergence u=v=0
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / T04 cold-start S178-B
** Auteur : LumVorax Project
**
** Objet : compléter le chantier T04 (audit S177 §10-15) :
**   S177 prouvait la quasi-stationnarité d'une solution déjà exacte.
**   S178-B démontre la CONVERGENCE depuis un état hors-équilibre (u=v=0).
**
** Audit S177 §10 (verbatim) :
**   « Tu n'as pas démontré : la voiture sait revenir à cette position
**     après avoir été déplacée. »
**
** Ce programme :
**   1. Initialise le solveur avec u=v=0 (état hors-équilibre).
**   2. Applique les CL de Couette périodique.
**   3. Simule jusqu'à stationnarité (même fenêtre multi-points que S177).
**   4. Vérifie que Linf_final < 0.05 ET que la fenêtre finale est
**      quasi-stationnaire selon les 4 critères de ns_stationarity_analysis.h
**   5. Mesure et enregistre la relaxation (Linf au fil du temps).
**
** Différence fondamentale avec S177 :
**   S177  : u(t=0) = u_exact(y)  → Linf = 0 immédiat (PASS_MACHINE)
**   S178-B: u(t=0) = 0          → Linf élevé au départ, décroissance vers 0
**
** Tests :
**   T04C-1 — n_points fenêtre finale >= QS_MIN_POINTS
**   T04C-2 — range_rel fenêtre finale < QS_RANGE_REL
**   T04C-3 — std_rel   fenêtre finale < QS_STD_REL
**   T04C-4 — slope_rel fenêtre finale < QS_SLOPE_REL
**   T04C-5 — quasi_stationary == 1 (verdict global)
**   T04C-6 — Linf_final < 0.05
**   T04C-7 — u_max <= 1.0 + 1e-6
**   T04C-8 — u_min >= 0.0 - 1e-6
**   T04C-9 — Linf observé décroissant sur au moins 80% de la simulation
**            (preuve d'une vraie convergence, pas juste un plateau bas)
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

/* ── Paramètres ─────────────────────────────────────────────────────────── */

#define N_SIM           128
#define RE_COUETTE      100.0
#define DT_BASE         1e-4
#define DX_BASE         (1.0 / 32)

static double get_dt_protoC(int n)
{
    double dx = 1.0 / (double)n;
    return DT_BASE * (dx / DX_BASE) * (dx / DX_BASE);
}

#define POLL_INTERVAL   50
#define WIN_CAPACITY    300      /* plus grand que S177 : 300×50 = 15 000 pas */
#define LIMIT_STEPS     2000000  /* limite haute — Couette depuis 0 converge lentement */

/* Pour T04C-9 : enregistrement de la trajectoire Linf */
#define TRAJ_CAPACITY   2000    /* max 2000 points de trajectoire */

#define LOG_PATH "logs/forensic/ns_stationarity_t04_coldstart.log"
#define RUN_LOG  "logs/031_ns_stationarity_t04_coldstart.txt"

/* ── Accesseurs MAC ─────────────────────────────────────────────────────── */

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

/* ── Linf ───────────────────────────────────────────────────────────────── */

static double compute_linf(const NSSolver2D *s)
{
    int    nx = s->params.nx, ny = s->params.ny;
    double dy = s->dy;
    double linf = 0.0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++) {
            double err = fabs(s->u[i * (ny + 2) + j] - couette_exact((j - 0.5) * dy));
            if (err > linf) linf = err;
        }
    return linf;
}

static void compute_urange(const NSSolver2D *s, double *umin, double *umax)
{
    int nx = s->params.nx, ny = s->params.ny;
    *umin =  1e30;
    *umax = -1e30;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++) {
            double u = s->u[i * (ny + 2) + j];
            if (u < *umin) *umin = u;
            if (u > *umax) *umax = u;
        }
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    FILE *runlog = fopen(RUN_LOG, "w");
    if (!runlog) {
        fprintf(stderr, "[FATAL] Impossible d'ouvrir %s\n", RUN_LOG);
        return 1;
    }

#define LOG(...) do { printf(__VA_ARGS__); fprintf(runlog, __VA_ARGS__); } while(0)

    LOG("=== T04 COLD-START : CONVERGENCE DEPUIS u=v=0 ===\n");
    LOG("[SESSION] S178-B | CERTIFIED_100=false | unique_human_proven=false\n");
    LOG("[MODE] DEBUG actif\n");
    LOG("[REF] Audit S177 §10 — convergence hors-équilibre non démontrée par S177\n\n");
    LOG("[DIFFERENCE] S177: u(0)=u_exact → Linf=0 immédiat (PASS_MACHINE)\n");
    LOG("[DIFFERENCE] S178-B: u(0)=0 → convergence réelle depuis état nul\n\n");

    forensic_logger_init(LOG_PATH);

    double dt = get_dt_protoC(N_SIM);
    double dx = 1.0 / (double)N_SIM;

    LOG("[PARAM] Re=%.1f | N=%d | dx=%.4e | dt=%.4e\n", RE_COUETTE, N_SIM, dx, dt);
    LOG("[PARAM] POLL_INTERVAL=%d | WIN_CAPACITY=%d | LIMIT_STEPS=%d\n\n",
        POLL_INTERVAL, WIN_CAPACITY, LIMIT_STEPS);

    NSParams p = {
        .nx = N_SIM, .ny = N_SIM, .lx = 1.0, .ly = 1.0,
        .re = RE_COUETTE, .dt = dt,
        .max_iter = 1, .tol = 1e-6, .max_poisson = 100, .debug = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        LOG("[FATAL] ns_solver_create echec\n");
        fclose(runlog);
        forensic_logger_destroy();
        return 1;
    }

    /* ── Initialisation cold-start : u=v=0, p=0 (état nul) ────────────── */
    /* ns_solver_create initialise déjà à 0 via calloc — pas d'action supplémentaire */
    /* On impose les CL de Couette sur l'état nul */
    set_couette_bc(s);

    LOG("[INIT] u=v=0 partout — CL Couette imposées\n");
    LOG("[INIT] Linf initiale = %.6e (attendu: ≈ 0.5 depuis état nul)\n\n",
        compute_linf(s));

    /* ── Fenêtre + trajectoire ─────────────────────────────────────────── */
    StationarityWindow *sw = stationarity_window_create(WIN_CAPACITY);
    if (!sw) {
        LOG("[FATAL] stationarity_window_create echec\n");
        ns_solver_destroy(s);
        fclose(runlog);
        forensic_logger_destroy();
        return 1;
    }

    /* Trajectoire Linf pour T04C-9 (décroissance) */
    double *traj       = (double *)malloc((size_t)TRAJ_CAPACITY * sizeof(double));
    int    *traj_steps = (int    *)malloc((size_t)TRAJ_CAPACITY * sizeof(int));
    int     traj_n     = 0;

    if (!traj || !traj_steps) {
        LOG("[FATAL] allocation trajectoire echec\n");
        free(traj); free(traj_steps);
        stationarity_window_destroy(sw);
        ns_solver_destroy(s);
        fclose(runlog);
        forensic_logger_destroy();
        return 1;
    }

    int    total_steps  = 0;
    int    converged    = 0;
    double linf_final   = 0.0;

    LOG("=== BOUCLE SIMULATION (cold-start, u=v=0) ===\n");
    LOG("  [step]     [Linf]         [win_n]\n");

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

        /* Enregistrement trajectoire (sous-échantillonnage : 1 point / 5000 pas) */
        if (total_steps % 5000 == 0 && traj_n < TRAJ_CAPACITY) {
            traj[traj_n]       = linf;
            traj_steps[traj_n] = total_steps;
            traj_n++;
            LOG("  step=%-9d  Linf=%.6e  win_n=%d\n",
                total_steps, linf, sw->count);
        }

        /* Test stationnarité forte dès que la fenêtre est pleine */
        if (sw->count >= WIN_CAPACITY) {
            StationarityResult r = stationarity_analyze(sw);
            if (r.quasi_stationary) {
                converged = 1;
                LOG("\n  [CONVERGENCE] step=%d  Linf=%.6e — critère QS satisfait\n\n",
                    total_steps, linf);
                break;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double wall_s = (t_end.tv_sec  - t_start.tv_sec)
                  + (t_end.tv_nsec - t_start.tv_nsec) * 1e-9;

    LOG("=== FIN SIMULATION ===\n");
    LOG("  steps=%d | t_phys=%.4f s | wall=%.1f s | converged=%d\n\n",
        total_steps, (double)total_steps * dt, wall_s, converged);

    /* ── Analyse fenêtre finale ──────────────────────────────────────────── */
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
    LOG("  slope     = %.8e\n", final_r.slope);
    LOG("  slope_rel = %.6e  (seuil < %.4f)\n", final_r.slope_rel, (double)QS_SLOPE_REL);
    LOG("  DETAIL    : %s\n", final_r.reason);
    LOG("  VERDICT   : %s\n\n",
        final_r.quasi_stationary ? "QUASI_STATIONNAIRE" : "NON_STATIONNAIRE");

    /* ── T04C-9 : décroissance monotone de Linf ─────────────────────────── */
    /*
     * On vérifie que Linf a réellement décru sur au moins 80% de la
     * trajectoire enregistrée. Cela distingue une vraie convergence
     * (Linf part de ~0.5, descend vers ~0) d'un plateau bas initial.
     *
     * Méthode : compter les paires (traj[i], traj[i+1]) avec traj[i] > traj[i+1]
     * et exiger ratio >= 0.8.
     */
    int decreasing_pairs = 0;
    int total_pairs      = (traj_n > 1) ? traj_n - 1 : 0;
    for (int i = 0; i < total_pairs; i++)
        if (traj[i] > traj[i + 1]) decreasing_pairs++;

    double decreasing_ratio = (total_pairs > 0)
                            ? (double)decreasing_pairs / (double)total_pairs
                            : 0.0;

    LOG("=== T04C-9 : ANALYSE TRAJECTOIRE DECROISSANCE ===\n\n");
    if (traj_n > 0) {
        LOG("  Linf initial (step=%d) = %.6e\n", traj_steps[0], traj[0]);
        LOG("  Linf final   (step=%d) = %.6e\n", traj_steps[traj_n-1], traj[traj_n-1]);
    }
    LOG("  Points trajectoire enregistrés = %d\n", traj_n);
    LOG("  Paires décroissantes = %d / %d\n", decreasing_pairs, total_pairs);
    LOG("  Ratio décroissance = %.3f (seuil >= 0.80)\n\n", decreasing_ratio);

    /* ── Tests T04C-1 → T04C-9 ───────────────────────────────────────────── */
    LOG("=== TESTS T04C-1 → T04C-9 ===\n\n");

    double u_min, u_max;
    compute_urange(s, &u_min, &u_max);

    int t04c1 = (final_r.n_points >= QS_MIN_POINTS);
    int t04c2 = (final_r.range_rel < QS_RANGE_REL);
    int t04c3 = (final_r.std_rel   < QS_STD_REL);
    int t04c4 = (final_r.slope_rel < QS_SLOPE_REL);
    int t04c5 = final_r.quasi_stationary;
    int t04c6 = (linf_final < 0.05);
    int t04c7 = (u_max <= 1.0 + 1e-6);
    int t04c8 = (u_min >= 0.0 - 1e-6);
    int t04c9 = (decreasing_ratio >= 0.80);

    LOG("  T04C-1 — n_points >= %d            : %s  (n=%d)\n",
        QS_MIN_POINTS, t04c1 ? "PASS" : "FAIL", final_r.n_points);
    LOG("  T04C-2 — range_rel < %.4f           : %s  (%.6f)\n",
        (double)QS_RANGE_REL, t04c2 ? "PASS" : "FAIL", final_r.range_rel);
    LOG("  T04C-3 — std_rel < %.4f             : %s  (%.6f)\n",
        (double)QS_STD_REL, t04c3 ? "PASS" : "FAIL", final_r.std_rel);
    LOG("  T04C-4 — slope_rel < %.4f           : %s  (%.2e)\n",
        (double)QS_SLOPE_REL, t04c4 ? "PASS" : "FAIL", final_r.slope_rel);
    LOG("  T04C-5 — quasi_stationary == 1      : %s\n",
        t04c5 ? "PASS" : "FAIL");
    LOG("  T04C-6 — Linf_final < 0.05          : %s  (%.6e)\n",
        t04c6 ? "PASS" : "FAIL", linf_final);
    LOG("  T04C-7 — u_max <= 1.0 + 1e-6        : %s  (%.6f)\n",
        t04c7 ? "PASS" : "FAIL", u_max);
    LOG("  T04C-8 — u_min >= 0.0 - 1e-6        : %s  (%.6f)\n",
        t04c8 ? "PASS" : "FAIL", u_min);
    LOG("  T04C-9 — décroissance >= 80%%        : %s  (ratio=%.3f)\n\n",
        t04c9 ? "PASS" : "FAIL", decreasing_ratio);

    /* Forensic */
    uint64_t ts_abs = time_ns_get_absolute();
    char op_buf[128];
    snprintf(op_buf, sizeof(op_buf),
             "T04-COLDSTART:n=%d:Linf=%.3e:range_rel=%.4f:decr=%.3f:qs=%d",
             N_SIM, linf_final, final_r.range_rel, decreasing_ratio,
             final_r.quasi_stationary);
    uint64_t lum_id = ((uint64_t)(N_SIM & 0xFFFFU) << 48)
                    | ((uint64_t)(final_r.quasi_stationary & 0x1U) << 47)
                    | ((uint64_t)(t04c9 & 0x1U) << 46);
    forensic_log_individual_lum(lum_id, op_buf, ts_abs);

    /* ── Limites honnêtes ─────────────────────────────────────────────────── */
    LOG("=== LIMITES HONNETES ===\n\n");
    LOG("  - Cold-start u=v=0 : Linf initial ≈ 0.5 attendu.\n");
    LOG("  - Si Linf initial ≈ 0, le test est identique à S177 (état déjà proche).\n");
    LOG("  - Couette plan : advection inactive. Pour l'advection : MMS avec source.\n");
    LOG("  - LIMIT_STEPS=%d — si non convergé : documenter la limite.\n", LIMIT_STEPS);
    LOG("  - CERTIFIED_100=false | unique_human_proven=false\n\n");

    /* ── Verdict global ───────────────────────────────────────────────────── */
    int all_pass = t04c1 && t04c2 && t04c3 && t04c4 && t04c5
                && t04c6 && t04c7 && t04c8 && t04c9;

    LOG("[VERDICT] T04 COLD-START S178-B : %s\n",
        all_pass
        ? "PASS — 9/9 tests : convergence réelle depuis u=v=0 démontrée"
        : "FAIL honnête — voir T04C-1..T04C-9 ci-dessus");
    LOG("[NOTE] T04C-1=%s T04C-2=%s T04C-3=%s T04C-4=%s T04C-5=%s "
               "T04C-6=%s T04C-7=%s T04C-8=%s T04C-9=%s\n",
        t04c1?"PASS":"FAIL", t04c2?"PASS":"FAIL",
        t04c3?"PASS":"FAIL", t04c4?"PASS":"FAIL",
        t04c5?"PASS":"FAIL", t04c6?"PASS":"FAIL",
        t04c7?"PASS":"FAIL", t04c8?"PASS":"FAIL",
        t04c9?"PASS":"FAIL");
    LOG("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    free(traj);
    free(traj_steps);
    stationarity_window_destroy(sw);
    ns_solver_destroy(s);
    forensic_logger_destroy();
    fclose(runlog);

    return all_pass ? 0 : 1;

#undef LOG
}
