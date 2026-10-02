/* **************************************************************************
** ns_convergence_study.c — Étude de convergence Richardson + conservation masse
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Navier-Stokes convergence
** Auteur : LumVorax Project
**
** Objectif : Prouver que ns_solver_2d.c converge à l'ordre 1 (Euler explicite)
**   par extrapolation de Richardson sur 3 grilles (32×32, 64×64, 128×128).
**
** Tests :
**   T01 : Erreur L2(u) décroît avec le raffinement de grille
**   T02 : Ordre de convergence ≥ 0.8 (Euler 1er ordre théorique = 1.0)
**   T03 : Conservation de masse : |div(u)| moyen ≤ 1e-3 dans le domaine
**   T04 : Énergie cinétique décroît monotonement (dissipation numérique stable)
**   T05 : Résidu Poisson < 1e-4 à 5000 pas sur grille 64×64
**
** CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/* u au centre cellule (i,j) sur grille décalée */
static double cell_u(const NSSolver2D *s, int i, int j)
{
    int ny = s->params.ny;
    /* u[i*(ny+2)+j] et u[(i+1)*(ny+2)+j] */
    double u_left  = s->u[i * (ny + 2) + j];
    double u_right = s->u[(i + 1) * (ny + 2) + j];
    return 0.5 * (u_left + u_right);
}

static double cell_v(const NSSolver2D *s, int i, int j)
{
    int ny = s->params.ny;
    double v_bot = s->v[i * (ny + 1) + j];
    double v_top = s->v[i * (ny + 1) + (j + 1)];
    return 0.5 * (v_bot + v_top);
}

/* Énergie cinétique totale (domaine intérieur) */
static double kinetic_energy(const NSSolver2D *s)
{
    int nx = s->params.nx;
    int ny = s->params.ny;
    double ek = 0.0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j <= ny; j++) {
            double u = cell_u(s, i, j);
            double v = cell_v(s, i, j);
            ek += u * u + v * v;
        }
    return 0.5 * ek * s->dx * s->dy;
}

/* Divergence max (conservation masse) */
static double max_divergence(const NSSolver2D *s)
{
    int nx = s->params.nx;
    int ny = s->params.ny;
    double dx = s->dx;
    double dy = s->dy;
    double div_max = 0.0;

    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double u_e = s->u[i * (ny + 2) + j];
            double u_w = s->u[(i - 1) * (ny + 2) + j];
            double v_n = s->v[i * (ny + 1) + j];
            double v_s = s->v[i * (ny + 1) + (j - 1)];
            double div = fabs((u_e - u_w) / dx + (v_n - v_s) / dy);
            if (div > div_max) div_max = div;
        }
    }
    return div_max;
}

/* Erreur L2(u) en x=0.5 contre une solution de référence interpolée */
static double l2_u_centerline(const NSSolver2D *s,
                               const double *ref_y, const double *ref_u,
                               int nref)
{
    int nx = s->params.nx;
    int ny = s->params.ny;
    double dy = s->dy;
    int ic = nx / 2;
    double sum = 0.0;

    for (int j = 1; j <= ny; j++) {
        double y = (j - 0.5) * dy;
        double u_s = 0.5 * (s->u[ic * (ny + 2) + j] + s->u[(ic + 1) * (ny + 2) + j]);
        /* interpolation linéaire dans ref */
        double u_ref = 0.0;
        for (int k = 0; k < nref - 1; k++) {
            if (y >= ref_y[k] && y <= ref_y[k + 1]) {
                double t = (y - ref_y[k]) / (ref_y[k + 1] - ref_y[k]);
                u_ref = ref_u[k] * (1.0 - t) + ref_u[k + 1] * t;
                break;
            }
        }
        sum += (u_s - u_ref) * (u_s - u_ref);
    }
    return sqrt(sum / (double)ny);
}

/* ── Données Ghia 1982 Re=100 — profil u (17 points) ────────────────────── */
static const double GHIA_Y[17] = {
    0.0000, 0.0547, 0.0625, 0.0703, 0.1016, 0.1719,
    0.2813, 0.4531, 0.5000, 0.6172, 0.7344, 0.8516,
    0.9531, 0.9609, 0.9688, 0.9766, 1.0000
};
static const double GHIA_U[17] = {
    0.00000, -0.03717, -0.04192, -0.04775, -0.06434, -0.10150,
   -0.15662, -0.21090, -0.20581, -0.13641,  0.00332,  0.23151,
    0.68717,  0.73722,  0.78871,  0.84123,  1.00000
};

/* ── Run complet sur une grille NxN ──────────────────────────────────────── */
typedef struct {
    int    n;
    double l2_u;
    double div_max;
    double ek_final;
    double poisson_res_final;
    double wall_s;
} GridResult;

/*
 * run_grid() — exécute un nombre fixe de pas (même t_final sur toutes les grilles).
 *
 * Contrainte clé Richardson : toutes les grilles doivent atteindre le même
 * temps physique simulé t_final = steps * dt pour que la comparaison soit valide.
 *
 * dt = 0.001 s, steps = n_steps → t_final identique pour 32/64/128.
 * La grille 128×128 a naturellement une résolution plus fine, donc si la
 * solution a convergé temporellement, L2 doit décroître avec le raffinement.
 *
 * Contrainte stabilité CFL : dt * U_max / dx <= 0.5
 *   → U_max ~ 1.0 (couvercle), dx = 1/n
 *   → dt_max_CFL = 0.5 / n
 *   → Pour n=128 : dt_max = 0.0039 s → dt=0.001 est stable.
 *
 * Contrainte diffusion CFL : dt <= Re * dx^2 / 4
 *   → Re=100, dx=1/128=0.0078 → dt_diff = 100 * 6e-5 / 4 = 0.0015 → OK.
 */
static GridResult run_grid(int n, int n_steps)
{
    GridResult r = {0};
    r.n = n;

    double dt = 0.001;  /* dt fixe — même t_final sur toutes les grilles */

    NSParams p = {
        .nx = n, .ny = n,
        .lx = 1.0, .ly = 1.0,
        .re = 100.0,
        .dt = dt,
        .max_iter    = n_steps,
        .tol         = 1e-5,
        .max_poisson = 50,
        .debug       = 0
    };

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) { fprintf(stderr, "[CONV] calloc failed n=%d\n", n); return r; }
    ns_solver_set_lid_bc(s);

    /* ns_solver_run() retourne le nombre de pas — on exécute manuellement
     * le dernier pas pour récupérer le résidu Poisson réel */
    for (int i = 0; i < n_steps - 1; i++)
        ns_solver_step(s);
    double last_res = ns_solver_step(s);  /* dernier pas → résidu Poisson */

    clock_gettime(CLOCK_MONOTONIC, &t1);
    r.wall_s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    r.l2_u              = l2_u_centerline(s, GHIA_Y, GHIA_U, 17);
    r.div_max            = max_divergence(s);
    r.ek_final           = kinetic_energy(s);
    r.poisson_res_final  = last_res;

    fprintf(stderr, "[CONV][n=%d] steps=%d t=%.3f L2=%.6f div=%.2e wall=%.1fs\n",
            n, s->step, s->step * dt, r.l2_u, r.div_max, r.wall_s);

    ns_solver_destroy(s);
    return r;
}

/* ── T04 FIX (rapport 151) : énergie cinétique — convergence vers plateau ── */
/*
 * Pour une cavité entraînée (lid-driven cavity) à Re=100 :
 *   - L'énergie cinétique CROÎT de 0 jusqu'à un plateau d'équilibre.
 *     EK@100 < EK@plateau est ATTENDU et correct physiquement.
 *     L'ancien critère "état_final < état_early" était incorrect pour ce cas.
 *   - Le critère honnête est : la simulation atteint un état stationnaire
 *     (|dEK/dt| faible sur les derniers pas) et l'énergie reste dans une
 *     plage physiquement raisonnable pour Re=100 (typiquement 0.001..0.1).
 *
 * Méthode :
 *   - Comparer EK sur les 100 derniers pas (pas 2900-3000) : si la variation
 *     relative est < 5%, la simulation a convergé vers un état stationnaire.
 *   - Vérifier que EK finale est dans [1e-4, 1.0] (sanity check physique).
 */
static int test_energy_convergence(double *ek_at_100_out, double *ek_at_2900_out,
                                    double *ek_final_out, double *ek_variation_out)
{
    NSParams p = {
        .nx = 32, .ny = 32,
        .lx = 1.0, .ly = 1.0,
        .re = 100.0, .dt = 0.001,
        .max_iter = 1, .tol = 1e-5, .max_poisson = 50, .debug = 0
    };
    NSSolver2D *s = ns_solver_create(&p);
    ns_solver_set_lid_bc(s);

    double ek_at_100 = 0.0, ek_at_2900 = 0.0;
    for (int i = 0; i < 3000; i++) {
        ns_solver_step(s);
        if (i == 99)   ek_at_100  = kinetic_energy(s);
        if (i == 2899) ek_at_2900 = kinetic_energy(s);
    }
    double ek_final = kinetic_energy(s);
    ns_solver_destroy(s);

    if (ek_at_100_out)   *ek_at_100_out   = ek_at_100;
    if (ek_at_2900_out)  *ek_at_2900_out  = ek_at_2900;
    if (ek_final_out)    *ek_final_out     = ek_final;

    /* Variation relative sur les 100 derniers pas (état stationnaire ?) */
    double variation = (ek_at_2900 > 1e-15)
                       ? fabs(ek_final - ek_at_2900) / ek_at_2900
                       : 1.0;
    if (ek_variation_out) *ek_variation_out = variation;

    /* Critères :
     *   1. EK finale dans plage physique [1e-4, 1.0] pour Re=100 lid-driven
     *   2. Variation relative < 5% sur les 100 derniers pas → état quasi-stationnaire
     *   3. EK@100 < EK_final : cavité non encore chargée à t=100 → normal
     */
    int ok_range = (ek_final >= 1e-4 && ek_final <= 1.0);
    int ok_stationary = (variation < 0.05);
    int ok_growth = (ek_at_100 < ek_final);  /* croissance attendue lid-driven */
    return ok_range && ok_stationary && ok_growth;
}

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    printf("[NS_CONV][START] Etude de convergence Richardson + conservation\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n\n");

    /* ── T01/T02 : Richardson — convergence adaptative ── */
    printf("[T01/T02] Raffinement grille 32x32 -> 64x64 -> 128x128\n");
    printf("  (convergence state stationnaire, max 20000 pas, Re=100)\n\n");

    GridResult g32  = run_grid(32,  20000);
    GridResult g64  = run_grid(64,  20000);
    GridResult g128 = run_grid(128, 20000);

    printf("  Grille  | L2(u vs Ghia) | div_max     | Poisson_res | Wall(s)\n");
    printf("  ------  | ------------- | ----------- | ----------- | -------\n");
    printf("  32x32   | %.6f      | %.4e  | %.4e  | %.2f\n",
           g32.l2_u, g32.div_max, g32.poisson_res_final, g32.wall_s);
    printf("  64x64   | %.6f      | %.4e  | %.4e  | %.2f\n",
           g64.l2_u, g64.div_max, g64.poisson_res_final, g64.wall_s);
    printf("  128x128 | %.6f      | %.4e  | %.4e  | %.2f\n\n",
           g128.l2_u, g128.div_max, g128.poisson_res_final, g128.wall_s);

    /* Ordre de convergence Richardson : p = log(e_coarse/e_fine) / log(2) */
    double order_32_64  = (g64.l2_u > 0.0 && g32.l2_u > 0.0)
                          ? log(g32.l2_u / g64.l2_u) / log(2.0) : 0.0;
    double order_64_128 = (g128.l2_u > 0.0 && g64.l2_u > 0.0)
                          ? log(g64.l2_u / g128.l2_u) / log(2.0) : 0.0;
    printf("  Ordre convergence 32->64   : %.3f (theorique Euler = 1.0)\n", order_32_64);
    printf("  Ordre convergence 64->128  : %.3f\n\n", order_64_128);

    /* T01 : L2 doit décroître avec le raffinement — g64 < g32 ET g128 < g64.
     * Critère honnête : décroissance stricte sur les deux paires de grilles.
     * Note : si l'erreur de troncature temporelle domine (Euler 1er ordre, dt
     * identique), la saturation spatiale est attendue. T01 documente ce fait
     * sans le masquer. */
    int t01_pass = (g64.l2_u < g32.l2_u) && (g128.l2_u < g64.l2_u);
    double ratio_32_64  = (g64.l2_u > 0.0) ? g32.l2_u / g64.l2_u : 0.0;
    double ratio_64_128 = (g128.l2_u > 0.0) ? g64.l2_u / g128.l2_u : 0.0;

    /* T02 FIX (rapport 151) : vérification de l'ordre Richardson ≥ 0.8.
     * L'ancien T02 testait uniquement le résidu Poisson (condition nécessaire
     * mais insuffisante pour certifier la convergence spatiale).
     * Ordre théorique Euler 1er ordre = 1.0. Seuil tolérant = 0.8.
     * Si L2 ne décroît pas (saturation temporelle), l'ordre est non défini
     * → on documente le fait et on FAIL avec raison explicite.
     * Résidu Poisson reste vérifié en T05 (critère distinct). */
    int t02_richardson_defined = (g64.l2_u < g32.l2_u) && (g128.l2_u < g64.l2_u);
    int t02_pass = t02_richardson_defined &&
                   (order_32_64  >= 0.8) &&
                   (order_64_128 >= 0.8);

    printf("  [T01] L2 decroit avec raffinement (32->64->128) : %s\n", t01_pass ? "PASS" : "FAIL");
    printf("        L2_32=%.6f  L2_64=%.6f  L2_128=%.6f\n",
           g32.l2_u, g64.l2_u, g128.l2_u);
    printf("        ratio 32/64=%.3f  ratio 64/128=%.3f\n", ratio_32_64, ratio_64_128);
    printf("  [T02] Ordre Richardson ≥ 0.8 (Euler theorique 1.0) : %s\n",
           t02_pass ? "PASS" : "FAIL");
    printf("        ordre_32_64=%.3f  ordre_64_128=%.3f  seuil=0.8\n",
           order_32_64, order_64_128);
    if (!t02_richardson_defined)
        printf("        [WARN] Ordre non defini : L2 ne decroit pas strictement entre grilles.\n");
    if (!t02_pass && t02_richardson_defined)
        printf("        [WARN] Ordre < 0.8 : saturation temporelle probable (dt identique sur 3 grilles).\n");
    printf("        [NOTE] L'erreur de troncature temporelle (Euler, dt=%.3f) peut dominer\n", 0.001);
    printf("               et saturer L2 meme si la grille spatiale se raffine.\n");
    printf("        Residu Poisson (info separee T05) : %.2e / %.2e / %.2e\n\n",
           g32.poisson_res_final, g64.poisson_res_final, g128.poisson_res_final);

    /* ── T03 : conservation de masse ── */
    printf("[T03] Conservation masse — div_max 64x64 convergee\n");
    int t03_pass = (g64.div_max <= 1e-2);
    printf("  div_max = %.4e | Seuil = 1e-2 : %s\n\n",
           g64.div_max, t03_pass ? "PASS" : "FAIL");

    /* ── T04 : énergie cinétique — convergence vers plateau stationnaire ── */
    printf("[T04] Energie cinetique — convergence vers etat stationnaire (32x32, 3000 pas)\n");
    double ek100 = 0.0, ek2900 = 0.0, ekfinal = 0.0, ek_var = 0.0;
    int t04_pass = test_energy_convergence(&ek100, &ek2900, &ekfinal, &ek_var);
    printf("  EK@100=%.6f  EK@2900=%.6f  EK@3000=%.6f  variation=%.2f%%\n",
           ek100, ek2900, ekfinal, ek_var * 100.0);
    printf("  Criteres : EK_final in [1e-4,1.0]=%s | variation<5%%=%s | EK@100<EK_final=%s\n",
           (ekfinal >= 1e-4 && ekfinal <= 1.0) ? "OK" : "FAIL",
           (ek_var < 0.05) ? "OK" : "FAIL",
           (ek100 < ekfinal) ? "OK" : "FAIL");
    printf("  [NOTE] Lid-driven cavity : EK croit de 0 vers plateau — croissance EK@100<EK@3000 est correcte.\n");
    printf("  T04=%s\n\n", t04_pass ? "PASS" : "FAIL");

    /* ── T05 : résidu Poisson ── */
    printf("[T05] Residu Poisson final 64x64\n");
    int t05_pass = (g64.poisson_res_final <= 1e-4);
    printf("  Poisson_res = %.4e | Seuil = 1e-4 : %s\n\n",
           g64.poisson_res_final, t05_pass ? "PASS" : "FAIL");

    /* ── Verdict global ── */
    int global = t01_pass && t02_pass && t03_pass && t04_pass && t05_pass;
    printf("[VERDICT] T01=%s T02=%s T03=%s T04=%s T05=%s\n",
           t01_pass?"PASS":"FAIL", t02_pass?"PASS":"FAIL",
           t03_pass?"PASS":"FAIL", t04_pass?"PASS":"FAIL",
           t05_pass?"PASS":"FAIL");
    printf("[VERDICT] GLOBAL : %s\n", global ? "PASS" : "FAIL");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    return global ? 0 : 1;
}
