/* **************************************************************************
** ns_richardson_protocol.c — Richardson-PROTOCOL-001 : comparaison 3 protocoles
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Richardson-PROTOCOL-001
** Auteur : LumVorax Project
**
** Objectif : Distinguer expérimentalement l'erreur spatiale de l'erreur
**   temporelle sur 3 protocoles de raffinement de grille :
**
**   Protocole A : dt constant (0.001 sur toutes les grilles)
**     → erreur temporelle domine → saturation attendue → T01/T02 FAIL honnête
**
**   Protocole B : dt ∝ dx  (dt = dt_ref × (n_ref/n))
**     → erreur advective CFL constante → démontre ordre ≈ 1 si advection domine
**
**   Protocole C : dt ∝ dx² (dt = dt_ref × (n_ref/n)²)
**     → erreur diffusive CFL constante → démontre ordre ≈ 1 si diffusion domine
**     → méthode Euler 1er ordre : erreur temporelle ∝ dt → en éliminant dt ∝ dx²
**       l'erreur totale est dominée par l'erreur spatiale → Richardson honnête
**
** Grilles : 32×32, 64×64, 128×128
** t_final identique (même temps physique) pour comparaison valide.
**
** dt_ref = 0.001 s (grille 32×32, 5000 pas → t_final = 5.0 s)
** n_ref  = 32
**
** Contraintes de stabilité CFL vérifiées à l'exécution.
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ── Paramètres de référence ─────────────────────────────────────────────── */

#define N_REF    32
#define DT_REF   0.001          /* dt grille de référence 32×32 */
#define T_FINAL  5.0            /* temps physique identique pour toutes les grilles */
#define RE       100.0

/* ── Accesseurs grille décalée (identiques à ns_convergence_study.c) ──────── */

/* u au centre cellule (i,j) */
static double cell_u(const NSSolver2D *s, int i, int j)
{
    int ny = s->params.ny;
    return 0.5 * (s->u[i       * (ny + 2) + j] +
                  s->u[(i + 1) * (ny + 2) + j]);
}

static double cell_v(const NSSolver2D *s, int i, int j)
{
    int ny = s->params.ny;
    return 0.5 * (s->v[i * (ny + 1) + j] +
                  s->v[i * (ny + 1) + (j + 1)]);
}

/* ── Données Ghia 1982 Re=100, profil u(x=0.5, y) — 17 points ────────────── */

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

/* ── Erreur L2(u) sur profil central x=0.5 vs Ghia ────────────────────────── */

static double l2_u_centerline(const NSSolver2D *s)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dy = s->dy;
    int    ic = nx / 2;
    double sum = 0.0;

    for (int j = 1; j <= ny; j++) {
        double y   = ((double)j - 0.5) * dy;
        double u_s = 0.5 * (s->u[ic       * (ny + 2) + j] +
                             s->u[(ic + 1) * (ny + 2) + j]);

        /* interpolation linéaire dans les données Ghia */
        double u_ref = 0.0;
        for (int k = 0; k < 16; k++) {
            if (y >= GHIA_Y[k] && y <= GHIA_Y[k + 1]) {
                double t = (y - GHIA_Y[k]) / (GHIA_Y[k + 1] - GHIA_Y[k]);
                u_ref = GHIA_U[k] * (1.0 - t) + GHIA_U[k + 1] * t;
                break;
            }
        }
        sum += (u_s - u_ref) * (u_s - u_ref);
    }
    return sqrt(sum / (double)ny);
}

/* ── Divergence max (conservation masse) ─────────────────────────────────── */

static double max_divergence(const NSSolver2D *s)
{
    int    nx  = s->params.nx;
    int    ny  = s->params.ny;
    double dx  = s->dx;
    double dy  = s->dy;
    double div_max = 0.0;

    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double u_e = s->u[i       * (ny + 2) + j];
            double u_w = s->u[(i - 1) * (ny + 2) + j];
            double v_n = s->v[i * (ny + 1) + j];
            double v_s = s->v[i * (ny + 1) + (j - 1)];
            double div = fabs((u_e - u_w) / dx + (v_n - v_s) / dy);
            if (div > div_max) div_max = div;
        }
    }
    return div_max;
}

/* ── Énergie cinétique ───────────────────────────────────────────────────── */

static double kinetic_energy(const NSSolver2D *s)
{
    int    nx  = s->params.nx;
    int    ny  = s->params.ny;
    double ek  = 0.0;

    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double u = cell_u(s, i, j);
            double v = cell_v(s, i, j);
            ek += u * u + v * v;
        }
    }
    return 0.5 * ek * s->dx * s->dy;
}

/* ── Résultat d'une grille ───────────────────────────────────────────────── */

typedef struct {
    int    n;           /* taille grille NxN */
    double dt;          /* pas de temps utilisé */
    int    steps;       /* nombre de pas exécutés */
    double t_phys;      /* temps physique simulé = steps × dt */
    double l2_u;        /* erreur L2 vs Ghia */
    double div_max;     /* divergence max */
    double ek_final;    /* énergie cinétique finale */
    double poisson_res; /* résidu Poisson au dernier pas */
    double wall_s;      /* temps mur */
    double cfl_adv;     /* CFL advectif : dt * U_max / dx */
    double cfl_diff;    /* CFL diffusif : dt / (Re * dx²) */
} GridResult;

/* ── Vérification stabilité CFL (imprimée mais non bloquante) ─────────────── */

static void print_cfl_check(int n, double dt)
{
    double dx  = 1.0 / (double)n;
    double cfl_adv  = dt * 1.0 / dx;                  /* U_max ≈ 1 (couvercle) */
    double cfl_diff    = dt * RE / (dx * dx);          /* facteur diffusif */
    double dt_max_diff = dx * dx / RE;                 /* seuil diffusion Euler */
    (void)cfl_diff;                                    /* imprimé via dt/dt_max_diff */

    fprintf(stderr,
        "[CFL][n=%d] dt=%.2e dx=%.4f | CFL_adv=%.3f (seuil≤0.5) "
        "| CFL_diff=dt/dt_max_diff=%.3f (seuil≤1) | dt_max_diff=%.2e\n",
        n, dt, dx, cfl_adv, dt / dt_max_diff, dt_max_diff);

    if (cfl_adv > 0.5)
        fprintf(stderr, "[CFL][WARN] CFL advectif %.3f > 0.5 → risque instabilité\n",
                cfl_adv);
    if (dt > dt_max_diff)
        fprintf(stderr, "[CFL][WARN] dt=%.2e > dt_max_diff=%.2e → instabilité diffusive\n",
                dt, dt_max_diff);
}

/* ── Exécution d'une grille avec dt donné ────────────────────────────────── */

static GridResult run_grid(int n, double dt)
{
    GridResult r = {0};
    r.n  = n;
    r.dt = dt;

    /* nombre de pas pour atteindre T_FINAL (arrondi au plus proche entier) */
    r.steps = (int)(T_FINAL / dt + 0.5);
    r.t_phys = r.steps * dt;

    /* vérification CFL imprimée */
    print_cfl_check(n, dt);

    double dx = 1.0 / (double)n;
    r.cfl_adv  = dt * 1.0 / dx;
    r.cfl_diff = dt * RE / (dx * dx);

    NSParams p = {
        .nx          = n,
        .ny          = n,
        .lx          = 1.0,
        .ly          = 1.0,
        .re          = RE,
        .dt          = dt,
        .max_iter    = r.steps,
        .tol         = 1e-5,
        .max_poisson = 50,
        .debug       = 0
    };

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[RICH] ns_solver_create failed n=%d dt=%.2e\n", n, dt);
        return r;
    }
    ns_solver_set_lid_bc(s);

    /* exécution pas à pas pour récupérer le dernier résidu Poisson */
    for (int i = 0; i < r.steps - 1; i++)
        ns_solver_step(s);
    double last_res = ns_solver_step(s);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    r.wall_s = (t1.tv_sec - t0.tv_sec) +
               (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    r.l2_u      = l2_u_centerline(s);
    r.div_max   = max_divergence(s);
    r.ek_final  = kinetic_energy(s);
    r.poisson_res = last_res;

    fprintf(stderr,
        "[RICH][n=%d][dt=%.2e] steps=%d t=%.4f L2=%.6f "
        "div=%.2e poisson=%.2e wall=%.1fs\n",
        n, dt, r.steps, r.t_phys, r.l2_u, r.div_max, r.poisson_res, r.wall_s);

    ns_solver_destroy(s);
    return r;
}

/* ── Analyse Richardson sur 3 grilles ────────────────────────────────────── */

typedef struct {
    const char *name;
    double l2_32;
    double l2_64;
    double l2_128;
    double order_32_64;
    double order_64_128;
    int    t01_pass;    /* L2 décroît strictement */
    int    t02_pass;    /* ordre ≥ 0.8 */
} ProtocolResult;

static ProtocolResult analyze_protocol(const char *name,
                                        GridResult g32,
                                        GridResult g64,
                                        GridResult g128)
{
    ProtocolResult pr;
    pr.name   = name;
    pr.l2_32  = g32.l2_u;
    pr.l2_64  = g64.l2_u;
    pr.l2_128 = g128.l2_u;

    pr.order_32_64  = (g64.l2_u > 0.0 && g32.l2_u > 0.0)
                      ? log(g32.l2_u / g64.l2_u) / log(2.0)
                      : 0.0;
    pr.order_64_128 = (g128.l2_u > 0.0 && g64.l2_u > 0.0)
                      ? log(g64.l2_u / g128.l2_u) / log(2.0)
                      : 0.0;

    pr.t01_pass = (g64.l2_u < g32.l2_u) && (g128.l2_u < g64.l2_u);
    pr.t02_pass = pr.t01_pass
                  && (pr.order_32_64  >= 0.8)
                  && (pr.order_64_128 >= 0.8);

    return pr;
}

static void print_protocol_result(const ProtocolResult *pr,
                                   GridResult g32, GridResult g64, GridResult g128)
{
    printf("\n  ── %s ──\n", pr->name);
    printf("  Grille  | dt          | steps  | t_phys | L2(u)    | "
           "CFL_adv | CFL_diff\n");
    printf("  32×32   | %.2e  | %6d | %.4f | %.6f | "
           "%.3f    | %.3f\n",
           g32.dt, g32.steps, g32.t_phys, g32.l2_u, g32.cfl_adv, g32.cfl_diff);
    printf("  64×64   | %.2e  | %6d | %.4f | %.6f | "
           "%.3f    | %.3f\n",
           g64.dt, g64.steps, g64.t_phys, g64.l2_u, g64.cfl_adv, g64.cfl_diff);
    printf("  128×128 | %.2e  | %6d | %.4f | %.6f | "
           "%.3f    | %.3f\n",
           g128.dt, g128.steps, g128.t_phys, g128.l2_u, g128.cfl_adv, g128.cfl_diff);
    printf("\n");
    printf("  Ordre Richardson 32→64   : %+.3f\n", pr->order_32_64);
    printf("  Ordre Richardson 64→128  : %+.3f\n", pr->order_64_128);
    printf("  T01 (L2 décroît)         : %s\n", pr->t01_pass ? "PASS" : "FAIL");
    printf("  T02 (ordre ≥ 0.8)        : %s\n", pr->t02_pass ? "PASS" : "FAIL");

    if (!pr->t01_pass)
        printf("  [NOTE] L2 ne décroît pas — erreur temporelle domine à dt constant.\n");
    if (pr->t01_pass && !pr->t02_pass)
        printf("  [NOTE] L2 décroît mais ordre < 0.8 — saturation partielle.\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== RICHARDSON-PROTOCOL-001 ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("t_final=%.1f s | n_ref=%d | dt_ref=%.3f\n\n", T_FINAL, N_REF, DT_REF);

    /* ────────────────────────────────────────────────────────
     * PROTOCOLE A : dt constant = DT_REF sur toutes les grilles
     * Attendu : T01/T02 FAIL (saturation erreur temporelle documentée)
     * ──────────────────────────────────────────────────────── */
    printf("=== PROTOCOLE A : dt constant (dt=%.3f pour toutes les grilles) ===\n",
           DT_REF);
    printf("  Hypothèse : si T01/T02 FAIL → erreur temporelle domine.\n");

    GridResult a32  = run_grid(32,  DT_REF);
    GridResult a64  = run_grid(64,  DT_REF);
    GridResult a128 = run_grid(128, DT_REF);

    ProtocolResult pA = analyze_protocol("PROTOCOLE A — dt constant", a32, a64, a128);
    print_protocol_result(&pA, a32, a64, a128);

    /* ────────────────────────────────────────────────────────
     * PROTOCOLE B : dt ∝ dx  (CFL advectif constant)
     * dt(n) = DT_REF × (N_REF / n)
     * 32 : 0.001 | 64 : 0.0005 | 128 : 0.00025
     * Attendu : ordre ≈ 1 si advection domine
     * ──────────────────────────────────────────────────────── */
    double dt_B_64  = DT_REF * ((double)N_REF / 64.0);
    double dt_B_128 = DT_REF * ((double)N_REF / 128.0);

    printf("\n=== PROTOCOLE B : dt ∝ dx (CFL advectif constant) ===\n");
    printf("  dt(32)=%.4f | dt(64)=%.4f | dt(128)=%.5f\n",
           DT_REF, dt_B_64, dt_B_128);
    printf("  Hypothèse : ordre ≈ 1 si l'erreur d'advection domine.\n");

    GridResult b32  = run_grid(32,  DT_REF);
    GridResult b64  = run_grid(64,  dt_B_64);
    GridResult b128 = run_grid(128, dt_B_128);

    ProtocolResult pB = analyze_protocol("PROTOCOLE B — dt ∝ dx", b32, b64, b128);
    print_protocol_result(&pB, b32, b64, b128);

    /* ────────────────────────────────────────────────────────
     * PROTOCOLE C : dt ∝ dx² (CFL diffusif constant)
     * dt(n) = DT_REF × (N_REF / n)²
     * 32 : 0.001 | 64 : 0.00025 | 128 : 0.0000625
     * Attendu : ordre ≈ 1 (Euler 1er ordre en temps ET en espace)
     * ──────────────────────────────────────────────────────── */
    double dt_C_64  = DT_REF * ((double)N_REF / 64.0)  * ((double)N_REF / 64.0);
    double dt_C_128 = DT_REF * ((double)N_REF / 128.0) * ((double)N_REF / 128.0);

    printf("\n=== PROTOCOLE C : dt ∝ dx² (CFL diffusif constant) ===\n");
    printf("  dt(32)=%.4f | dt(64)=%.5f | dt(128)=%.7f\n",
           DT_REF, dt_C_64, dt_C_128);
    printf("  Hypothèse : ordre ≈ 1 — erreur spatiale et temporelle réduites ensemble.\n");

    GridResult c32  = run_grid(32,  DT_REF);
    GridResult c64  = run_grid(64,  dt_C_64);
    GridResult c128 = run_grid(128, dt_C_128);

    ProtocolResult pC = analyze_protocol("PROTOCOLE C — dt ∝ dx²", c32, c64, c128);
    print_protocol_result(&pC, c32, c64, c128);

    /* ── Synthèse comparative ──────────────────────────────── */
    printf("\n=== SYNTHESE COMPARATIVE RICHARDSON-PROTOCOL-001 ===\n\n");

    printf("  %-35s | T01 L2↓ | T02 ord≥0.8 | ord_32/64 | ord_64/128\n",
           "Protocole");
    printf("  %-35s | ------- | ---------- | --------- | ----------\n",
           "-----------------------------------");
    printf("  %-35s | %-7s | %-10s | %+.3f     | %+.3f\n",
           pA.name, pA.t01_pass?"PASS":"FAIL", pA.t02_pass?"PASS":"FAIL",
           pA.order_32_64, pA.order_64_128);
    printf("  %-35s | %-7s | %-10s | %+.3f     | %+.3f\n",
           pB.name, pB.t01_pass?"PASS":"FAIL", pB.t02_pass?"PASS":"FAIL",
           pB.order_32_64, pB.order_64_128);
    printf("  %-35s | %-7s | %-10s | %+.3f     | %+.3f\n",
           pC.name, pC.t01_pass?"PASS":"FAIL", pC.t02_pass?"PASS":"FAIL",
           pC.order_32_64, pC.order_64_128);

    printf("\n");
    printf("  Interprétation attendue :\n");
    printf("  - Proto A FAIL : confirme que dt constant sature l'erreur.\n");
    printf("  - Proto B PASS si advection domine : ordre ≈ 1.\n");
    printf("  - Proto C PASS si diffusion domine : ordre ≈ 1 (Euler 1er ordre).\n");
    printf("  - Les deux FAIL : la solution ne converge pas encore (à investiguer).\n");
    printf("  - Les deux PASS : confirme convergence spatiale démontrée.\n");

    /* ── Verdict Richardson-PROTOCOL-001 ─────────────────── */
    int protocol_resolved = pB.t02_pass || pC.t02_pass;
    printf("\n[VERDICT] RICHARDSON-PROTOCOL-001 : %s\n",
           protocol_resolved ? "PROTOCOLE IDENTIFIE" : "OPEN — aucun protocole ne démontre ordre >= 0.8");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    return protocol_resolved ? 0 : 1;
}
