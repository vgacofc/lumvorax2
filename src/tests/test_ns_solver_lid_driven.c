/* **************************************************************************
** test_ns_solver_lid_driven.c — Benchmark Lid-Driven Cavity Re=100
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/tests / Navier-Stokes validation
** Auteur : LumVorax Project
**
** Données de référence : Ghia U., Ghia K.N., Shin C.T. (1982).
**   "High-Re solutions for incompressible flow using the Navier-Stokes
**    equations and a multigrid method." J. Comput. Phys., 48, 387-411.
**   Table 1 (Re=100) — profil u(x=0.5, y) — 17 points
**   Table 2 (Re=100) — profil v(x, y=0.5) — 17 points
**
** Critère de validation : erreur L∞ ≤ 0.05 sur les 17 points Ghia.
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ── Données Ghia 1982, Table 1 — u(x=0.5, y), Re=100 ──────────────────── */
/* 17 points : y_ghia normalisé [0,1] */
static const double GHIA_Y[17] = {
    0.0000, 0.0547, 0.0625, 0.0703, 0.1016, 0.1719,
    0.2813, 0.4531, 0.5000, 0.6172, 0.7344, 0.8516,
    0.9531, 0.9609, 0.9688, 0.9766, 1.0000
};
/* u(x=0.5, y) normalisée par U_lid=1 */
static const double GHIA_U[17] = {
    0.00000, -0.03717, -0.04192, -0.04775, -0.06434, -0.10150,
   -0.15662, -0.21090, -0.20581, -0.13641,  0.00332,  0.23151,
    0.68717,  0.73722,  0.78871,  0.84123,  1.00000
};

/* ── Données Ghia 1982, Table 2 — v(x, y=0.5), Re=100 ──────────────────── */
static const double GHIA_X[17] = {
    0.0000, 0.0625, 0.0703, 0.0781, 0.0938, 0.1563,
    0.2266, 0.2344, 0.5000, 0.8047, 0.8594, 0.9063,
    0.9453, 0.9531, 0.9609, 0.9688, 1.0000
};
static const double GHIA_V[17] = {
    0.00000,  0.09233,  0.10091,  0.10890,  0.12317,  0.16077,
    0.17507,  0.17527,  0.05454, -0.24533, -0.22445, -0.16914,
   -0.10313, -0.08864, -0.07391, -0.05906,  0.00000
};

/* ── Interpolation linéaire pour extraire u/v aux positions Ghia ─────────── */
static double interp1d(const double *xs, const double *ys, int n, double x)
{
    if (x <= xs[0])    return ys[0];
    if (x >= xs[n-1])  return ys[n-1];
    for (int i = 0; i < n - 1; i++) {
        if (x >= xs[i] && x <= xs[i+1]) {
            double t = (x - xs[i]) / (xs[i+1] - xs[i]);
            return ys[i] * (1.0 - t) + ys[i+1] * t;
        }
    }
    return ys[n-1];
}

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    /* ── Horodatage ── */
    struct timespec ts0, ts1;
    clock_gettime(CLOCK_MONOTONIC, &ts0);

    printf("[NS_LID_DRIVEN][START] Benchmark Lid-Driven Cavity Re=100\n");
    printf("[REFERENCE] Ghia et al. 1982 — 17 points profil u + 17 profil v\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false\n\n");

    /* ── Paramètres ── */
    NSParams params = {
        .nx          = 64,        /* 64×64 cellules */
        .ny          = 64,
        .lx          = 1.0,
        .ly          = 1.0,
        .re          = 100.0,
        .dt          = 0.001,     /* dt=0.001 — CFL ≈ 0.065 (stable) */
        .max_iter    = 5000,      /* 5000 pas = 5 s simulés */
        .tol         = 1e-5,
        .max_poisson = 50,
        .debug       = 1
    };

    printf("[PARAMS] nx=%d ny=%d | Re=%.1f | dt=%.4f | steps=%d\n\n",
           params.nx, params.ny, params.re, params.dt, params.max_iter);

    /* ── Création et initialisation ── */
    NSSolver2D *solver = ns_solver_create(&params);
    if (!solver) {
        fprintf(stderr, "[NS_LID_DRIVEN][FATAL] Impossible de créer le solveur\n");
        return 1;
    }
    ns_solver_set_lid_bc(solver);

    /* ── Exécution ── */
    printf("[NS_LID_DRIVEN][RUN] Démarrage simulation...\n");
    int steps_done = ns_solver_run(solver);
    printf("[NS_LID_DRIVEN][RUN] %d pas effectués\n\n", steps_done);

    clock_gettime(CLOCK_MONOTONIC, &ts1);
    double wall_s = (double)(ts1.tv_sec - ts0.tv_sec)
                  + (double)(ts1.tv_nsec - ts0.tv_nsec) * 1e-9;

    /* ── Extraction des profils ── */
    int nx = params.nx;
    int ny = params.ny;

    /* ny+1 pour inclure le nœud couvercle (y=1.0, u=1.0) */
    double *y_sim = (double *)malloc((size_t)(ny + 1) * sizeof(double));
    double *u_sim = (double *)malloc((size_t)(ny + 1) * sizeof(double));
    double *x_sim = (double *)malloc((size_t)nx * sizeof(double));
    double *v_sim = (double *)malloc((size_t)nx * sizeof(double));

    if (!y_sim || !u_sim || !x_sim || !v_sim) {
        fprintf(stderr, "[NS_LID_DRIVEN][FATAL] malloc profils\n");
        ns_solver_destroy(solver);
        return 1;
    }

    ns_solver_u_centerline_y(solver, y_sim, u_sim);  /* ny+1 points */
    ns_solver_v_centerline_x(solver, x_sim, v_sim);

    /* ── Comparaison vs Ghia — profil u ── */
    printf("[VALIDATION] Profil u(x=0.5, y) — 17 points Ghia Re=100\n");
    printf("%-8s  %-12s  %-12s  %-10s\n", "y_Ghia", "u_Ghia", "u_sim", "erreur");
    double err_u_max = 0.0;
    double err_u_sum = 0.0;

    for (int k = 0; k < 17; k++) {
        double y_g = GHIA_Y[k];
        double u_g = GHIA_U[k];
        double u_s = interp1d(y_sim, u_sim, ny + 1, y_g);
        double err = fabs(u_s - u_g);
        if (err > err_u_max) err_u_max = err;
        err_u_sum += err;
        printf("  %.4f    %+.6f    %+.6f    %.4f%s\n",
               y_g, u_g, u_s, err, err > 0.05 ? "  <-- HORS TOLÉRANCE" : "");
    }
    printf("  L∞(u) = %.4f | Lmoy(u) = %.4f | Tolérance = 0.05\n\n",
           err_u_max, err_u_sum / 17.0);

    /* ── Comparaison vs Ghia — profil v ── */
    printf("[VALIDATION] Profil v(x, y=0.5) — 17 points Ghia Re=100\n");
    printf("%-8s  %-12s  %-12s  %-10s\n", "x_Ghia", "v_Ghia", "v_sim", "erreur");
    double err_v_max = 0.0;
    double err_v_sum = 0.0;

    for (int k = 0; k < 17; k++) {
        double x_g = GHIA_X[k];
        double v_g = GHIA_V[k];
        double v_s = interp1d(x_sim, v_sim, nx, x_g);
        double err = fabs(v_s - v_g);
        if (err > err_v_max) err_v_max = err;
        err_v_sum += err;
        printf("  %.4f    %+.6f    %+.6f    %.4f%s\n",
               x_g, v_g, v_s, err, err > 0.05 ? "  <-- HORS TOLÉRANCE" : "");
    }
    printf("  L∞(v) = %.4f | Lmoy(v) = %.4f | Tolérance = 0.05\n\n",
           err_v_max, err_v_sum / 17.0);

    /* ── Verdict final ── */
    int pass_u = (err_u_max <= 0.05);
    int pass_v = (err_v_max <= 0.05);
    int global_pass = pass_u && pass_v;

    printf("[VERDICT] Profil u : %s (L∞=%.4f)\n",
           pass_u ? "PASS ✓" : "FAIL ✗", err_u_max);
    printf("[VERDICT] Profil v : %s (L∞=%.4f)\n",
           pass_v ? "PASS ✓" : "FAIL ✗", err_v_max);
    printf("[VERDICT] GLOBAL : %s\n", global_pass ? "PASS ✓" : "FAIL ✗");
    printf("[WALL_TIME] %.3f s | [STEPS] %d\n", wall_s, steps_done);
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    /* ── Nettoyage ── */
    free(y_sim); free(u_sim);
    free(x_sim); free(v_sim);
    ns_solver_destroy(solver);

    return global_pass ? 0 : 1;
}
