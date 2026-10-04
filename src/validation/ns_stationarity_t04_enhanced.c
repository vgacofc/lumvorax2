/* **************************************************************************
** ns_stationarity_t04_enhanced.c — P3 : T04 renforcé fenêtre finale complète
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / T04-ENHANCED (P3)
** Auteur : LumVorax Project
**
** Objet : fermer le chantier P3 du registre 193 §9 :
**   « T04 renforcé : fenêtre finale complète (min/max/moy/écart-type/pente) »
**
** Différences fondamentales avec S177/S178-B :
**   S177 (ns_stationarity_t04.c)           : Couette + init exacte → Linf=0 PASS_MACHINE
**   S178-B (ns_stationarity_t04_coldstart.c): Couette cold-start → τ_diff=100s trop long
**   S183 (ce fichier)                       : LID-DRIVEN CAVITY Re=100 cold-start u=v=0
**
** Justification physique du choix Lid-Driven Cavity :
**   La Lid-Driven Cavity Re=100 atteint un état quasi-stationnaire en
**   τ_conv ≈ 5–15 unités de temps physique (cf. Ghia 1982 convergence).
**   Avec dt=4e-4 et N=32 (CFL_diff=0.082 << 0.5) : ~25 000–37 500 pas → 10–15 s.
**   Cela permet de démontrer la VRAIE convergence depuis l'état nul en temps
**   raisonnable, contrairement à Couette (τ=Re=100 >> temps simulé).
**
** Protocole fenêtre renforcée :
**   Observable : Linf = max|u_interior| (norme infinie du champ u intérieur)
**   Fenêtre : 300 points, collecte toutes les 50 itérations
**   Statistiques : min, max, mean, std, range_rel, std_rel, slope_rel (OLS)
**   Critère : 4 conditions simultanées (héritées de ns_stationarity_analysis.h)
**
** Tests T04E-1 → T04E-10 :
**   T04E-1  — n_points fenêtre finale >= QS_MIN_POINTS (>= 20)
**   T04E-2  — range_rel < QS_RANGE_REL (variation max-min relative < 5%)
**   T04E-3  — std_rel   < QS_STD_REL   (écart-type relatif < 2%)
**   T04E-4  — slope_rel < QS_SLOPE_REL (pente OLS normalisée < 2%/pt)
**   T04E-5  — quasi_stationary == 1 (verdict 4 critères combinés)
**   T04E-6  — Linf_final > 0.0 (champ Lid-Driven non nul en régime)
**   T04E-7  — u_max > 0.0 (couvercle lid a entraîné le fluide)
**   T04E-8  — u_max <= 1.0 + 1e-4 (pas de divergence physique)
**   T04E-9  — traj_n >= 10 points de trajectoire (convergence observable)
**   T04E-10 — mean fenêtre dans (0, 1) (plausibilité physique)
**
** Log forensic : logs/forensic/ns_stationarity_t04_enhanced.log
** Log brut     : logs/033_ns_stationarity_t04_enhanced.txt
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

/* Lid-Driven Cavity Re=100, grille 32x32 */
#define N_GRID         32
#define RE_LID         100.0
#define LX_LID         1.0
#define LY_LID         1.0

/*
 * Stabilité diffusion : dt_diff = Re * dx^2 / 2 = 100 * (1/32)^2 / 2 ≈ 4.88e-4
 * On prend dt = 4e-4 (< dt_diff → stable).
 * CFL_diff = dt/(Re*dx^2) = 4e-4 / (100 * (1/32)^2) = 0.082 ✓
 */
#define DT_SIM         4e-4

/* Arrêt anticipé si quasi-stationnarité détectée ; limite de sécurité */
#define LIMIT_STEPS    100000
#define TOL_POISSON    1e-8
#define MAX_POISSON    5000

/* Collecte et fenêtre */
#define POLL_INTERVAL  50       /* collecter Linf toutes les 50 itérations */
#define WIN_CAPACITY   300      /* 300 × 50 = 15 000 pas dans la fenêtre */

/* Trajectoire pour T04E-9 */
#define TRAJ_CAPACITY  2000

/* Paths */
#define LOG_FORENSIC   "logs/forensic/ns_stationarity_t04_enhanced.log"
#define LOG_RUN        "logs/033_ns_stationarity_t04_enhanced.txt"

/* ── Accesseurs MAC internes ─────────────────────────────────────────────── */

#define U(s, i, j)  ((s)->u[(i) * ((s)->params.ny + 2) + (j)])

/* ── Utilitaires ─────────────────────────────────────────────────────────── */

/* Norme Linf sur u intérieur (i=1..nx-1, j=1..ny) */
static double compute_linf_u(const NSSolver2D *s)
{
    int    nx  = s->params.nx;
    int    ny  = s->params.ny;
    double val = 0.0;

    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double a = fabs(U(s, i, j));
            if (a > val) val = a;
        }
    }
    return val;
}

/* u_max sur le domaine intérieur */
static double compute_umax(const NSSolver2D *s)
{
    int    nx  = s->params.nx;
    int    ny  = s->params.ny;
    double val = -1e30;

    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            if (U(s, i, j) > val) val = U(s, i, j);
        }
    }
    return val;
}

/* ── Macro de test ───────────────────────────────────────────────────────── */

#define CHECK_TEST(id, cond, msg_pass, msg_fail)                     \
    do {                                                              \
        int ok = (int)(cond);                                         \
        n_pass += ok;                                                 \
        n_total++;                                                    \
        fprintf(f_run, "%s : %s — %s\n",                            \
            ok ? "PASS" : "FAIL",                                    \
            (id), ok ? (msg_pass) : (msg_fail));                     \
        fprintf(stderr, "[T04E][%s] %s : %s\n",                     \
            ok ? "PASS" : "FAIL", (id),                              \
            ok ? (msg_pass) : (msg_fail));                           \
    } while (0)

/* ── Programme principal ─────────────────────────────────────────────────── */

int main(void)
{
    /* --- Ouverture log brut --- */
    FILE *f_run = fopen(LOG_RUN, "w");
    if (!f_run) {
        fprintf(stderr, "[T04E][ERROR] Impossible d'ouvrir %s\n", LOG_RUN);
        return 1;
    }

    /* --- Log forensic (API réelle : forensic_logger_init + forensic_log_individual_lum) --- */
    forensic_logger_init(LOG_FORENSIC);

    /* --- Horodatage début --- */
    uint64_t t_start_ns = time_ns_get_monotonic();
    time_t   t_wall     = time(NULL);
    char     t_str[32];
    strftime(t_str, sizeof(t_str), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t_wall));

    fprintf(f_run, "=== T04-ENHANCED P3 ===\n");
    fprintf(f_run, "Date     : %s\n", t_str);
    fprintf(f_run, "Solveur  : Lid-Driven Cavity Re=%.0f N=%d dt=%.1e\n",
            RE_LID, N_GRID, DT_SIM);
    fprintf(f_run, "Init     : u=v=0 (cold-start)\n");
    fprintf(f_run, "LIMIT    : %d steps\n", LIMIT_STEPS);
    fprintf(f_run, "Fenetre  : %d pts toutes les %d iter\n",
            WIN_CAPACITY, POLL_INTERVAL);
    fprintf(f_run, "\n");

    /* Log forensic start */
    {
        uint64_t ts = time_ns_get_absolute();
        char op[256];
        snprintf(op, sizeof(op),
            "T04E-START:n=%d:re=%.0f:dt=%.1e:limit=%d:poll=%d:win=%d",
            N_GRID, RE_LID, DT_SIM, LIMIT_STEPS, POLL_INTERVAL, WIN_CAPACITY);
        uint64_t lum_id = ((uint64_t)(N_GRID & 0xFFFFU) << 48)
                        | ((uint64_t)0xE4U << 40)
                        | (uint64_t)(0x0001U);
        forensic_log_individual_lum(lum_id, op, ts);
    }

    /* --- Création solveur --- */
    NSParams params;
    memset(&params, 0, sizeof(params));
    params.nx          = N_GRID;
    params.ny          = N_GRID;
    params.lx          = LX_LID;
    params.ly          = LY_LID;
    params.re          = RE_LID;
    params.dt          = DT_SIM;
    params.max_iter    = LIMIT_STEPS;
    params.tol         = TOL_POISSON;
    params.max_poisson = MAX_POISSON;
    params.debug       = 0;

    NSSolver2D *s = ns_solver_create(&params);
    if (!s) {
        fprintf(stderr, "[T04E][ERROR] ns_solver_create\n");
        forensic_logger_destroy();
        fclose(f_run);
        return 1;
    }

    /* Cold-start : u=v=0 → déjà initialisé par calloc dans ns_solver_create */

    /* --- Fenêtre stationnarité + trajectoire --- */
    StationarityWindow *sw = stationarity_window_create(WIN_CAPACITY);
    if (!sw) {
        fprintf(stderr, "[T04E][ERROR] stationarity_window_create\n");
        ns_solver_destroy(s);
        forensic_logger_destroy();
        fclose(f_run);
        return 1;
    }

    double traj_linf[TRAJ_CAPACITY];
    int    traj_step[TRAJ_CAPACITY];
    int    traj_n    = 0;

    fprintf(f_run, "[DEBUG] Linf_initial = 0.000000e+00 (cold-start u=v=0)\n");
    fprintf(stderr, "[T04E][DEBUG] Lid-Driven Cavity N=%d dt=%.1e Re=%.0f — cold-start\n",
            N_GRID, DT_SIM, RE_LID);

    /* --- Boucle temporelle --- */
    int    converged = 0;
    double linf_last = 0.0;

    for (int step = 1; step <= LIMIT_STEPS; step++) {
        ns_solver_step(s);

        /* Collecte Linf */
        if (step % POLL_INTERVAL == 0) {
            double linf = compute_linf_u(s);
            stationarity_window_push(sw, linf, step);
            linf_last = linf;

            /* Trajectoire (premiers TRAJ_CAPACITY points) */
            if (traj_n < TRAJ_CAPACITY) {
                traj_linf[traj_n] = linf;
                traj_step[traj_n] = step;
                traj_n++;
            }

            /* Vérification anticipée de convergence tous les 1000 pas */
            if (step % 1000 == 0) {
                StationarityResult r = stationarity_analyze(sw);
                fprintf(f_run,
                    "[step=%6d] Linf=%.4e qs=%d range_rel=%.4f std_rel=%.4f slope_rel=%.2e\n",
                    step, linf, r.quasi_stationary,
                    r.range_rel, r.std_rel, r.slope_rel);
                fprintf(stderr,
                    "[T04E] step=%6d Linf=%.4e qs=%d range_rel=%.4f\n",
                    step, linf, r.quasi_stationary, r.range_rel);

                if (r.quasi_stationary && sw->count >= WIN_CAPACITY / 2) {
                    fprintf(f_run,
                        "[CONVERGENCE] Quasi-stationnarite a step=%d "
                        "(Linf=%.4e %d pts fenetre)\n",
                        step, linf, sw->count);
                    fprintf(stderr,
                        "[T04E] Convergence a step=%d Linf=%.4e\n",
                        step, linf);
                    converged = 1;
                    break;
                }
            }
        }
    }

    /* --- Analyse finale de la fenêtre --- */
    StationarityResult final_r = stationarity_analyze(sw);
    stationarity_print_result(&final_r, "T04E-fenetre-finale");

    /* u_max pour T04E-8 */
    double u_max_final = compute_umax(s);

    /* Wall time */
    uint64_t t_end_ns = time_ns_get_monotonic();
    double   wall_s   = (double)(t_end_ns - t_start_ns) / 1e9;
    int      n_steps  = s->step;
    double   t_phys   = (double)n_steps * DT_SIM;

    fprintf(f_run, "\n=== ANALYSE FENETRE FINALE ===\n");
    fprintf(f_run, "n_steps  = %d\n", n_steps);
    fprintf(f_run, "t_phys   = %.4f s\n", t_phys);
    fprintf(f_run, "wall     = %.2f s\n", wall_s);
    fprintf(f_run, "converged= %d\n", converged);
    fprintf(f_run, "n_points = %d\n", final_r.n_points);
    fprintf(f_run, "min_val  = %.6e\n", final_r.min_val);
    fprintf(f_run, "max_val  = %.6e\n", final_r.max_val);
    fprintf(f_run, "mean     = %.6e\n", final_r.mean);
    fprintf(f_run, "std      = %.6e\n", final_r.std);
    fprintf(f_run, "range_rel= %.6f\n", final_r.range_rel);
    fprintf(f_run, "std_rel  = %.6f\n", final_r.std_rel);
    fprintf(f_run, "slope    = %.6e\n", final_r.slope);
    fprintf(f_run, "slope_rel= %.6e\n", final_r.slope_rel);
    fprintf(f_run, "quasi_qs = %d\n", final_r.quasi_stationary);
    fprintf(f_run, "Linf_last= %.6e\n", linf_last);
    fprintf(f_run, "u_max    = %.6e\n", u_max_final);
    fprintf(f_run, "\n");

    /* Log forensic résultats */
    {
        uint64_t ts = time_ns_get_absolute();
        char op[256];
        snprintf(op, sizeof(op),
            "T04E-RESULT:steps=%d:t_phys=%.4f:qs=%d:n_pts=%d:"
            "range_rel=%.6f:std_rel=%.6f:slope_rel=%.2e:"
            "linf=%.6e:u_max=%.6e:wall=%.2f",
            n_steps, t_phys, final_r.quasi_stationary, final_r.n_points,
            final_r.range_rel, final_r.std_rel, final_r.slope_rel,
            linf_last, u_max_final, wall_s);
        uint64_t l2b; memcpy(&l2b, &linf_last, sizeof(uint64_t));
        uint64_t lum_id = ((uint64_t)(N_GRID & 0xFFFFU) << 48)
                        | ((uint64_t)0xE4U << 40)
                        | (l2b & 0x7FFFU);
        forensic_log_individual_lum(lum_id, op, ts);
    }

    /* Trajectoire pour T04E-9 */
    double linf_first_traj = (traj_n > 0) ? traj_linf[0] : 0.0;
    double linf_last_traj  = (traj_n > 0) ? traj_linf[traj_n - 1] : 0.0;

    fprintf(f_run, "=== TRAJECTOIRE Linf (%d points) ===\n", traj_n);
    fprintf(f_run, "Linf[0]  = %.6e (step=%d)\n",
            linf_first_traj, (traj_n > 0 ? traj_step[0] : 0));
    fprintf(f_run, "Linf[-1] = %.6e (step=%d)\n",
            linf_last_traj, (traj_n > 0 ? traj_step[traj_n - 1] : 0));

    /* === BATTERIE DE TESTS === */
    int n_pass  = 0;
    int n_total = 0;

    fprintf(f_run, "\n=== TESTS T04E-1 -> T04E-10 ===\n");

    CHECK_TEST("T04E-1",
        final_r.n_points >= QS_MIN_POINTS,
        "n_points >= QS_MIN_POINTS",
        "n_points < QS_MIN_POINTS — fenetre insuffisante");

    CHECK_TEST("T04E-2",
        final_r.range_rel < QS_RANGE_REL,
        "range_rel < 0.05 (variation max-min faible)",
        "range_rel >= 0.05 — champ non stationnaire");

    CHECK_TEST("T04E-3",
        final_r.std_rel < QS_STD_REL,
        "std_rel < 0.02 (ecart-type faible)",
        "std_rel >= 0.02 — dispersion elevee");

    CHECK_TEST("T04E-4",
        final_r.slope_rel < QS_SLOPE_REL,
        "slope_rel < 0.02 (pente OLS faible)",
        "slope_rel >= 0.02 — derive residuelle");

    CHECK_TEST("T04E-5",
        final_r.quasi_stationary == 1,
        "quasi_stationary == 1 (4 criteres simultanes)",
        "quasi_stationary == 0 — convergence non atteinte");

    /* T04E-6 : Lid-Driven Cavity doit avoir un Linf non nul en régime */
    CHECK_TEST("T04E-6",
        linf_last > 0.0,
        "Linf_final > 0.0 (champ Lid-Driven non nul — physiquement attendu)",
        "Linf_final == 0.0 — champ nul suspect");

    /* T04E-7 : le couvercle doit avoir entraîné le fluide */
    CHECK_TEST("T04E-7",
        u_max_final > 0.0,
        "u_max > 0.0 (couvercle lid a entraine le fluide)",
        "u_max <= 0.0 — pas de mouvement");

    /* T04E-8 : pas de divergence physique */
    CHECK_TEST("T04E-8",
        u_max_final <= 1.0 + 1e-4,
        "u_max <= 1.0 + 1e-4 (pas de divergence)",
        "u_max > 1.0 + 1e-4 — divergence detectee");

    /* T04E-9 : trajectoire suffisamment longue */
    CHECK_TEST("T04E-9",
        traj_n >= 10,
        "trajectoire >= 10 points (convergence observable)",
        "trajectoire < 10 points — simulation trop courte");

    /* T04E-10 : mean fenêtre dans la plage physique (0, 1) */
    CHECK_TEST("T04E-10",
        final_r.mean > 0.0 && final_r.mean < 1.0,
        "mean fenetre dans (0, 1) — plausibilite physique Lid-Driven",
        "mean hors de (0, 1) — valeur suspecte");

    /* --- Résumé --- */
    fprintf(f_run, "\n=== RESUME ===\n");
    fprintf(f_run, "PASS : %d / %d\n", n_pass, n_total);
    fprintf(f_run, "VERDICT : T04-ENHANCED P3 : %s\n",
            (n_pass == n_total) ? "PASS" : "FAIL");
    fprintf(f_run, "CERTIFIED_100=false | unique_human_proven=false\n");

    fprintf(stderr,
        "[T04E] === VERDICT P3 : %s (%d/%d) ===\n"
        "  n_pts=%d range_rel=%.4f std_rel=%.4f slope_rel=%.2e qs=%d\n"
        "  Linf=%.4e u_max=%.4e t_phys=%.2f wall=%.1fs\n",
        (n_pass == n_total) ? "PASS" : "FAIL", n_pass, n_total,
        final_r.n_points, final_r.range_rel, final_r.std_rel,
        final_r.slope_rel, final_r.quasi_stationary,
        linf_last, u_max_final, t_phys, wall_s);

    /* Log forensic verdict */
    {
        uint64_t ts = time_ns_get_absolute();
        char op[256];
        snprintf(op, sizeof(op),
            "T04E-VERDICT:pass=%d:total=%d:verdict=%s",
            n_pass, n_total,
            (n_pass == n_total) ? "PASS" : "FAIL");
        uint64_t lum_id = ((uint64_t)(N_GRID & 0xFFFFU) << 48)
                        | ((uint64_t)0xE4U << 40)
                        | (uint64_t)(n_pass & 0x7FFFU);
        forensic_log_individual_lum(lum_id, op, ts);
    }

    /* --- Nettoyage --- */
    stationarity_window_destroy(sw);
    ns_solver_destroy(s);
    forensic_logger_destroy();
    fclose(f_run);

    /* Vérification log forensic produit */
    FILE *f_check = fopen(LOG_FORENSIC, "r");
    if (!f_check) {
        fprintf(stderr, "[T04E][ERROR] Log forensic manquant : %s\n",
                LOG_FORENSIC);
        return 1;
    }
    fclose(f_check);

    fprintf(stderr, "[T04E] Log forensic : %s OK\n", LOG_FORENSIC);
    fprintf(stderr, "[T04E] Log brut     : %s OK\n", LOG_RUN);

    return (n_pass == n_total) ? 0 : 1;
}
