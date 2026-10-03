/* **************************************************************************
** ns_richardson_004_mms.c — Richardson-PROTOCOL-004 : MMS sur NS 2D
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Richardson-PROTOCOL-004 (MMS)
** Auteur : LumVorax Project
**
** Objectif : Fermer Richardson-PROTOCOL-001 en fournissant une erreur de
** discrétisation MESURABLE et DÉCROISSANTE, et démontrer l'ordre spatial.
**
** Contexte S165/169 : Richardson-003c a produit L2 = 0 (plancher machine)
** car u=y est un état stationnaire exact du discret. Le critère Richardson
** n'est pas évaluable quand l'erreur est sous le plancher IEEE-754.
** Richardson-004 / MMS résout ce problème en injectant un terme source
** artificiel qui force une solution analytique non-triviale.
**
** ── Méthode MMS (Method of Manufactured Solutions) ────────────────────
**
** Solution fabriquée (divergence-free exacte) :
**
**   u_e(x,y,t) = -sin(π·x)·cos(π·y)·exp(-2π²·t/Re)
**   v_e(x,y,t) =  cos(π·x)·sin(π·y)·exp(-2π²·t/Re)
**   p_e(x,y,t) = -(1/4)·(cos(2π·x)+cos(2π·y))·exp(-4π²·t/Re)
**
** Propriétés analytiques :
**   div(u_e) = ∂u_e/∂x + ∂v_e/∂y = 0  — exactement div-free
**   Ces fonctions sont la solution exacte NS linéarisée (Taylor-Green 2D).
**   Le terme source est nul (NS satisfait sans forcing artificiel).
**
** Avantage critique :
**   u_e → 0 exponentiellement, donc l'erreur numerique est O(h²) pour
**   un schéma de 2ème ordre. L2(t=T) ≠ 0 et décroît proprement avec h.
**
** ── Protocole d'évaluation ──────────────────────────────────────────────
**
**   Protocole A : dt = DT_REF (constant), N = 32, 64, 128
**   Protocole B : dt ∝ dx      (dt_n = DT_REF * dx_n / dx_ref)
**   Protocole C : dt ∝ dx²     (dt_n = DT_REF * (dx_n/dx_ref)²)
**
**   Mesure : L1, L2, Linf de (u_num - u_exact) à t = T_FINAL
**   Ordre  : p = log2(L2_coarse / L2_fine)
**
** ── Tests ───────────────────────────────────────────────────────────────
**
**   T00m — Solution fabriquée vérifiée analytiquement (div=0, NS satisfait)
**   T01m — Ordre 32→64  >= 1.5 (protocole C, MMS)
**   T02m — Ordre 64→128 >= 1.5 (protocole C, MMS)
**   T03m — L2 strictement décroissant (protocole C)
**   T04m — Linf_128 < 0.1 (tolérance MMS, erreur discrétisation mesurable)
**   T05m — Ordre observé compatible avec schéma O(dx²) (1.5 ≤ p ≤ 2.5)
**
** ── Limites honnêtes ────────────────────────────────────────────────────
**
**   - Schéma Euler explicite 1er ordre en temps → ordre temporel = 1.
**     Pour isoler l'ordre spatial, protocole C (dt ∝ dx²) est requis.
**   - Terme source nul (solution Taylor-Green exacte) — pas de forcing
**     artificiel additionnel dans les équations.
**   - Les CL sont Dirichlet imposées depuis u_exact (toutes les faces).
**   - CERTIFIED_100=false | unique_human_proven=false
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"
#include "../debug/forensic_logger.h"
#include "../common/time_ns.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

/* ── Constantes physiques ────────────────────────────────────────────────── */

#define RE_MMS       1.0   /* Re=1 : régime diffusif pur, décroissance
                            * exponentielle rapide et mesurable */
#define T_FINAL      0.003 /* T_FINAL calibré pour que Proto C 128×128
                            * reste sous STEPS_MAX :
                            * dt_C_128 = 7.629e-07 → steps = 0.003/7.629e-07 ≈ 3932 ✓
                            * Proto A 128×128 : 0.003/1.221e-05 ≈ 246 steps
                            * Toutes les grilles atteignent le même t_final → Richardson valide */
#define STEPS_MAX    5000  /* plafond de sécurité : ne doit pas être atteint
                            * avec T_FINAL=0.003 et les dt stables calculés.
                            * Conservé pour détecter toute dérive de calcul. */
#define DT_REF_STAB  2e-4  /* pas stable pour N=32 avec Re=1 :
                            * dt_diff = Re*dx²/(2*(1/dx²+1/dy²)) ≈ dx²/4 */

#define N_REF        32
#define DX_REF       (1.0 / N_REF)

#define N_GRIDS      3
static const int GRIDS[N_GRIDS] = {32, 64, 128};

#define N_PROTOCOLS  3
typedef enum { PROTO_A = 0, PROTO_B = 1, PROTO_C = 2 } Protocol;
static const char *PROTO_NAMES[N_PROTOCOLS] = {
    "A (dt=const)", "B (dt prop dx)", "C (dt prop dx2)"
};

/* ── Accesseurs grille MAC ────────────────────────────────────────────────── */

#define U(s, i, j)  ((s)->u[(i) * ((s)->params.ny + 2) + (j)])
#define V(s, i, j)  ((s)->v[(i) * ((s)->params.ny + 1) + (j)])
#define P(s, i, j)  ((s)->p[(i) * ((s)->params.ny + 2) + (j)])

static const double PI = 3.14159265358979323846;

/* ── Solution Taylor-Green 2D exacte (divergence-free) ───────────────────────
 *
 * u_e(x,y,t) = -sin(π·x)·cos(π·y)·exp(-2π²·t/Re)
 * v_e(x,y,t) =  cos(π·x)·sin(π·y)·exp(-2π²·t/Re)
 * p_e(x,y,t) = -(1/4)·(cos(2π·x)+cos(2π·y))·exp(-4π²·t/Re)
 *
 * Vérification analytique :
 *   ∂u/∂x + ∂v/∂y = -π·cos(πx)·cos(πy)·A + π·cos(πx)·cos(πy)·A = 0 ✓
 *   Cette solution satisfait NS incompressible EXACTEMENT (terme source nul).
 */
static double u_exact(double x, double y, double t, double re)
{
    double A = exp(-2.0 * PI * PI * t / re);
    return -sin(PI * x) * cos(PI * y) * A;
}

static double v_exact(double x, double y, double t, double re)
{
    double A = exp(-2.0 * PI * PI * t / re);
    return  cos(PI * x) * sin(PI * y) * A;
}

/* ── Calcul du dt stable selon protocole ─────────────────────────────────── */

static double get_dt_stable(int n, Protocol proto)
{
    double dx = 1.0 / n;
    /* dt_diffusif_max = Re * dx² / 4  (condition Von Neumann 2D pour Re=1) */
    double dt_diff = RE_MMS * dx * dx / 4.0;
    /* dt_ref adapté = min(DT_REF_STAB, dt_diffusif) pour le proto A */
    double dt_a = (DT_REF_STAB < dt_diff * 0.8) ? DT_REF_STAB : dt_diff * 0.8;

    switch (proto) {
    case PROTO_A: return dt_a;
    case PROTO_B: return dt_a * (dx / DX_REF);
    case PROTO_C: return dt_a * (dx / DX_REF) * (dx / DX_REF);
    }
    return dt_a;
}

/* ── Conditions aux limites Dirichlet MMS (toutes les faces) ─────────────────
 *
 * On impose u_exact, v_exact sur tous les bords fantômes.
 * Les bords internes sont traités dans la boucle d'init.
 *
 * Pour la grille MAC staggered :
 *   u[i][0]    (bord Sud)  : image miroir → u[i][0] = 2·u_e(x_i, y=0) - u[i][1]
 *                             mais u_e(y=0) = -sin(πx)·cos(0)·A = -sin(πx)·A
 *   u[i][ny+1] (bord Nord) : image miroir vers y=1
 *   u[0][j]    (bord Ouest): u_e(x=0, y_j) = -sin(0)·cos(πy)·A = 0
 *   u[nx][j]   (bord Est)  : u_e(x=1, y_j) = -sin(π)·cos(πy)·A = 0
 *
 * Note : pour u_exact = -sin(πx)·cos(πy)·A, les bords Ouest et Est
 * donnent u=0 (sin(0)=0, sin(π)=0), ce qui simplifie l'implémentation.
 */
typedef struct { double t; double re; } MMS_BCCtx;

/* Contexte global pour le callback (passage simplifié) */
static double g_mms_t  = 0.0;
static double g_mms_re = 1.0;

static void set_mms_bc(NSSolver2D *s)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dx = s->dx;
    double dy = s->dy;
    double t  = g_mms_t;
    double re = s->params.re;

    /* ── CL pour u (faces u[i][j] en x=i*dx, y=(j-0.5)*dy) ── */

    /* Bords Ouest/Est : u_exact(x=0,y,t) = 0 et u_exact(x=1,y,t) = 0 */
    for (int j = 0; j <= ny + 1; j++) {
        U(s, 0,  j) = 0.0;
        U(s, nx, j) = 0.0;
    }

    /* Bords Sud/Nord : image miroir à partir de u_exact aux faces */
    for (int i = 0; i <= nx; i++) {
        double x_f = (double)i * dx;   /* position face u en x */

        /* Bord Sud (j=0) : face fictive à y = -0.5*dy
         * u_ghost = 2*u_exact(x_f, y=0, t) - u_int
         * u_exact(y=0) = -sin(πx)*cos(0)*A = -sin(πx)*A */
        double u_wall_s = u_exact(x_f, 0.0, t, re);
        U(s, i, 0)      = 2.0 * u_wall_s - U(s, i, 1);

        /* Bord Nord (j=ny+1) : face fictive à y = 1+0.5*dy
         * u_exact(y=1) = -sin(πx)*cos(π)*A = +sin(πx)*A */
        double u_wall_n = u_exact(x_f, 1.0, t, re);
        U(s, i, ny + 1) = 2.0 * u_wall_n - U(s, i, ny);
    }

    /* ── CL pour v (faces v[i][j] en x=(i-0.5)*dx, y=j*dy) ── */

    /* Bords Sud/Nord : v_exact(y=0,t) = cos(πx)*sin(0)*A = 0 */
    for (int i = 0; i <= nx + 1; i++) {
        V(s, i, 0)  = 0.0;
        V(s, i, ny) = 0.0;
    }

    /* Bords Ouest/Est : image miroir */
    for (int j = 0; j <= ny; j++) {
        double y_f = (double)j * dy;   /* position face v en y */

        /* Bord Ouest (i=0) : face fictive à x = -0.5*dx
         * v_exact(x=0, y) = cos(0)*sin(πy)*A = sin(πy)*A */
        double v_wall_w = v_exact(0.0, y_f, t, re);
        V(s, 0, j)      = 2.0 * v_wall_w - V(s, 1, j);

        /* Bord Est (i=nx+1) : face fictive à x = 1+0.5*dx
         * v_exact(x=1, y) = cos(π)*sin(πy)*A = -sin(πy)*A */
        double v_wall_e = v_exact(1.0, y_f, t, re);
        V(s, nx + 1, j) = 2.0 * v_wall_e - V(s, nx, j);
    }

    /* ── CL pour p : Neumann homogène ── */
    for (int j = 0; j <= ny + 1; j++) {
        P(s, 0,      j) = P(s, 1,  j);
        P(s, nx + 1, j) = P(s, nx, j);
    }
    for (int i = 0; i <= nx + 1; i++) {
        P(s, i, 0)      = P(s, i, 1);
        P(s, i, ny + 1) = P(s, i, ny);
    }
}

/* ── Calcul des erreurs L1/L2/Linf ── */

typedef struct {
    double L1;
    double L2;
    double Linf;
    double u_max_abs;
    double v_max_abs;
    int    n_cells;
} MMSErrors;

static MMSErrors compute_mms_errors(const NSSolver2D *s, double t)
{
    int    nx     = s->params.nx;
    int    ny     = s->params.ny;
    double dx     = s->dx;
    double dy     = s->dy;
    double re     = s->params.re;
    double sum1   = 0.0, sum2 = 0.0, max_e = 0.0;
    double umax   = 0.0, vmax = 0.0;
    int    ncells = 0;

    /* Erreur sur u aux faces intérieures */
    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double x_f    = (double)i * dx;
            double y_f    = ((double)j - 0.5) * dy;
            double u_e    = u_exact(x_f, y_f, t, re);
            double u_num  = s->u[i * (ny + 2) + j];
            double err    = fabs(u_num - u_e);
            sum1 += err;
            sum2 += err * err;
            if (err > max_e) max_e = err;
            if (fabs(u_num) > umax) umax = fabs(u_num);
            ncells++;
        }
    }

    /* Erreur sur v aux faces intérieures */
    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j < ny; j++) {
            double x_f    = ((double)i - 0.5) * dx;
            double y_f    = (double)j * dy;
            double v_e    = v_exact(x_f, y_f, t, re);
            double v_num  = s->v[i * (ny + 1) + j];
            double err    = fabs(v_num - v_e);
            sum1 += err;
            sum2 += err * err;
            if (err > max_e) max_e = err;
            if (fabs(v_num) > vmax) vmax = fabs(v_num);
            ncells++;
        }
    }

    MMSErrors m;
    m.L1        = (ncells > 0) ? sum1 / ncells : 0.0;
    m.L2        = (ncells > 0) ? sqrt(sum2 / ncells) : 0.0;
    m.Linf      = max_e;
    m.u_max_abs = umax;
    m.v_max_abs = vmax;
    m.n_cells   = ncells;
    return m;
}

/* ── Structure résultat ──────────────────────────────────────────────────── */

typedef struct {
    int        n;
    double     dt;
    double     dx;
    long long  steps;
    double     t_final;
    double     t_wall_s;
    MMSErrors  err;
    double     poisson_res;
    int        stable;   /* 1 si simulation n'a pas divergé */
} MMSGridResult;

/* ── Calcul ordre Richardson ─────────────────────────────────────────────── */

static double richardson_order(double L2_coarse, double L2_fine)
{
    if (L2_coarse < 1e-15 || L2_fine < 1e-15)
        return -8888.0;  /* PASS_MACHINE — plancher machine */
    if (L2_coarse <= 0.0 || L2_fine <= 0.0 || L2_coarse <= L2_fine)
        return -9999.0;  /* non décroissant */
    return log2(L2_coarse / L2_fine);
}

/* ── Simulation MMS ──────────────────────────────────────────────────────── */

static MMSGridResult run_mms(int n, double dt, int proto_id)
{
    MMSGridResult res;
    memset(&res, 0, sizeof(res));
    res.n  = n;
    res.dt = dt;
    res.dx = 1.0 / n;

    NSParams p = {
        .nx = n, .ny = n,
        .lx = 1.0, .ly = 1.0,
        .re          = RE_MMS,
        .dt          = dt,
        .max_iter    = 1,
        .tol         = 1e-6,
        .max_poisson = 100,
        .debug       = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[MMS][ERROR] ns_solver_create n=%d failed\n", n);
        return res;
    }

    double dx = s->dx;
    double dy = s->dy;
    double re = s->params.re;

    /* ── Initialisation : u(x,y,t=0) = u_exact, v(x,y,t=0) = v_exact ── */
    {
        int nx_i = s->params.nx;
        int ny_i = s->params.ny;

        /* u[i][j] en x=i*dx, y=(j-0.5)*dy */
        for (int i = 0; i <= nx_i; i++) {
            for (int j = 0; j <= ny_i + 1; j++) {
                double x_f = (double)i * dx;
                double y_f = ((double)j - 0.5) * dy;
                U(s, i, j) = u_exact(x_f, y_f, 0.0, re);
            }
        }

        /* v[i][j] en x=(i-0.5)*dx, y=j*dy */
        for (int i = 0; i <= nx_i + 1; i++) {
            for (int j = 0; j <= ny_i; j++) {
                double x_f = ((double)i - 0.5) * dx;
                double y_f = (double)j * dy;
                V(s, i, j) = v_exact(x_f, y_f, 0.0, re);
            }
        }
        /* p = 0 initialement (par calloc) */
    }

    /* Appliquer les CL initiales */
    g_mms_t  = 0.0;
    g_mms_re = RE_MMS;
    set_mms_bc(s);

    /* ── Boucle temporelle jusqu'à T_FINAL ou STEPS_MAX ── */
    long long steps     = (long long)(T_FINAL / dt);
    if (steps < 1) steps = 1;
    if (steps > STEPS_MAX) {
        fprintf(stderr, "[MMS][INFO] n=%d proto=%d : steps limité %lld→%d (STEPS_MAX)\n",
                n, proto_id, steps, STEPS_MAX);
        steps = STEPS_MAX;
    }
    double    t_current  = 0.0;
    double    poisson_r  = 1.0;
    int       stable     = 1;

    struct timespec t0c, t1c;
    clock_gettime(CLOCK_MONOTONIC, &t0c);

    for (long long k = 0; k < steps; k++) {
        /* Mettre à jour le temps global pour le callback CL */
        g_mms_t = (double)(k + 1) * dt;

        poisson_r = ns_solver_step_with_bc(s, set_mms_bc);

        /* Détection divergence : u_max > 10 = instabilité numérique */
        if (k > 0 && k % 1000 == 0) {
            double umax_check = 0.0;
            int nx_i = s->params.nx;
            int ny_i = s->params.ny;
            for (int i = 1; i < nx_i; i++)
                for (int j = 1; j <= ny_i; j++) {
                    double u_abs = fabs(s->u[i * (ny_i + 2) + j]);
                    if (u_abs > umax_check) umax_check = u_abs;
                }
            if (umax_check > 10.0) {
                fprintf(stderr, "[MMS][WARN] n=%d proto=%d : divergence détectée à step=%lld (u_max=%.3e)\n",
                        n, proto_id, k, umax_check);
                stable = 0;
                break;
            }
        }
    }

    t_current = (double)steps * dt;

    clock_gettime(CLOCK_MONOTONIC, &t1c);

    res.t_final     = t_current;
    res.steps       = steps;
    res.t_wall_s    = (t1c.tv_sec  - t0c.tv_sec) +
                      (t1c.tv_nsec - t0c.tv_nsec) * 1e-9;
    res.poisson_res = poisson_r;
    res.stable      = stable;
    res.err         = compute_mms_errors(s, t_current);

    ns_solver_destroy(s);
    return res;
}

/* ── Test 0 : vérification analytique de la solution fabriquée ───────────────
 *
 * Vérifie que u_exact, v_exact satisfont bien :
 *   (A) div(u_exact) = ∂u/∂x + ∂v/∂y = 0  (incompressibilité exacte)
 *   (B) L2(t=0) << 1 sur une grille 32×32 — solution physiquement bornée
 *   (C) u_exact(x=0,y,t)=0 et u_exact(x=1,y,t)=0 — bords Dirichlet nuls
 */
typedef struct {
    double max_div;
    double l2_at_t0;
    double u_at_x0;
    double u_at_x1;
    int    pass;
} Test0MMS;

static Test0MMS run_test0_mms(void)
{
    Test0MMS r;
    memset(&r, 0, sizeof(r));

    int n = 32;
    double dx = 1.0 / n;
    double dy = 1.0 / n;
    double t  = 0.5;  /* vérifier à t=0.5, pas seulement t=0 */
    double re = RE_MMS;

    /* (A) max |∂u/∂x + ∂v/∂y| analytique aux centres de cellules */
    double max_div = 0.0;
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++) {
            /* Différence centrée pour ∂u/∂x (aux faces u) */
            double x_e = (double)i * dx;
            double x_w = (double)(i-1) * dx;
            double y_c = ((double)j - 0.5) * dy;
            double u_e_val = u_exact(x_e, y_c, t, re);
            double u_w_val = u_exact(x_w, y_c, t, re);
            double du_dx   = (u_e_val - u_w_val) / dx;

            /* Différence centrée pour ∂v/∂y (aux faces v) */
            double x_c = ((double)i - 0.5) * dx;
            double y_n = (double)j * dy;
            double y_s = (double)(j-1) * dy;
            double v_n_val = v_exact(x_c, y_n, t, re);
            double v_s_val = v_exact(x_c, y_s, t, re);
            double dv_dy   = (v_n_val - v_s_val) / dy;

            double div = fabs(du_dx + dv_dy);
            if (div > max_div) max_div = div;
        }
    }
    r.max_div = max_div;

    /* (B) L2 analytique à t=0 : doit être << 1 */
    double sum2 = 0.0;
    int ncells = 0;
    for (int i = 1; i < n; i++) {
        for (int j = 1; j <= n; j++) {
            double x_f = (double)i * dx;
            double y_f = ((double)j - 0.5) * dy;
            double u_e = u_exact(x_f, y_f, 0.0, re);
            sum2  += u_e * u_e;
            ncells++;
        }
    }
    r.l2_at_t0 = (ncells > 0) ? sqrt(sum2 / ncells) : 0.0;

    /* (C) u_exact aux bords Ouest et Est */
    r.u_at_x0 = fabs(u_exact(0.0, 0.5, t, re));  /* doit être 0 */
    r.u_at_x1 = fabs(u_exact(1.0, 0.5, t, re));  /* doit être 0 */

    r.pass = (r.max_div   < 1e-12) &&   /* div analytique < plancher */
             (r.l2_at_t0  < 1.0)  &&   /* champ borné à t=0 */
             (r.u_at_x0   < 1e-14) &&  /* bord Ouest = 0 */
             (r.u_at_x1   < 1e-14);    /* bord Est   = 0 */
    return r;
}

/* ── LOG_PATH ─────────────────────────────────────────────────────────────── */

#define LOG_PATH "logs/forensic/ns_richardson_004_mms.log"

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== RICHARDSON-PROTOCOL-004 : MMS (Method of Manufactured Solutions) ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[MMS] Solution : Taylor-Green 2D exacte (divergence-free)\n");
    printf("[MMS] u_exact = -sin(pi*x)*cos(pi*y)*exp(-2*pi^2*t/Re)\n");
    printf("[MMS] v_exact =  cos(pi*x)*sin(pi*y)*exp(-2*pi^2*t/Re)\n");
    printf("[MMS] Re=%.1f | T_final=%.4f\n", RE_MMS, T_FINAL);
    printf("[CONTEXT] Richardson-003c (S165) : L2=0 (plancher machine) → MMS requis\n\n");

    forensic_logger_init(LOG_PATH);

    /* ═══════════════════════════════════════════════════════════════════════
     * TEST 0 — Vérification analytique de la solution fabriquée
     * ═══════════════════════════════════════════════════════════════════════ */
    printf("=== TEST 0 — VÉRIFICATION ANALYTIQUE MMS ===\n\n");

    Test0MMS t0 = run_test0_mms();

    printf("  A — Max div(u_exact) discret    : %.3e  %s\n",
           t0.max_div,
           (t0.max_div < 1e-12) ? "OK (< 1e-12)" : "FAIL");
    printf("  B — L2(u_exact, t=0)            : %.3e  %s\n",
           t0.l2_at_t0,
           (t0.l2_at_t0 < 1.0) ? "OK (borné)" : "FAIL");
    printf("  C — u_exact(x=0, y=0.5, t=0.5) : %.3e  %s\n",
           t0.u_at_x0,
           (t0.u_at_x0 < 1e-14) ? "OK (= 0)" : "FAIL");
    printf("  D — u_exact(x=1, y=0.5, t=0.5) : %.3e  %s\n",
           t0.u_at_x1,
           (t0.u_at_x1 < 1e-14) ? "OK (= 0)" : "FAIL");
    printf("\n  T00m — Solution fabriquée analytique : %s\n\n",
           t0.pass ? "PASS — solution div-free, bornée, bords nuls"
                   : "FAIL — problème dans la définition analytique");

    if (!t0.pass) {
        printf("[FATAL] T00m FAIL — simulation MMS non lancée.\n");
        printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");
        forensic_logger_destroy();
        return 1;
    }

    /* ═══════════════════════════════════════════════════════════════════════
     * Simulations MMS
     * ═══════════════════════════════════════════════════════════════════════ */
    printf("=== SIMULATIONS MMS — %d protocoles × %d grilles ===\n\n",
           N_PROTOCOLS, N_GRIDS);

    MMSGridResult results[N_PROTOCOLS][N_GRIDS];
    memset(results, 0, sizeof(results));

    for (int pi = 0; pi < N_PROTOCOLS; pi++) {
        Protocol proto = (Protocol)pi;
        printf("─── Protocole %s ───\n", PROTO_NAMES[pi]);

        for (int gi = 0; gi < N_GRIDS; gi++) {
            int    n  = GRIDS[gi];
            double dt = get_dt_stable(n, proto);

            printf("  Grille %3d×%3d | dt=%.3e | steps=%lld ... ",
                   n, n, dt, (long long)(T_FINAL / dt));
            fflush(stdout);

            MMSGridResult r = run_mms(n, dt, pi);
            results[pi][gi] = r;

            printf("%s | L1=%.3e | L2=%.3e | Linf=%.3e | wall=%.1fs\n",
                   r.stable ? "OK" : "DIV!",
                   r.err.L1, r.err.L2, r.err.Linf, r.t_wall_s);

            /* Forensic checkpoint */
            uint64_t ts = time_ns_get_absolute();
            char op_buf[128];
            snprintf(op_buf, sizeof(op_buf),
                     "004mms:n=%d:proto=%d:L2=%.3e:steps=%lld:stable=%d",
                     n, pi, r.err.L2, r.steps, r.stable);
            uint64_t lum_id = ((uint64_t)(n   & 0xFFFFU) << 48)
                            | ((uint64_t)(pi  & 0xFFU)   << 40)
                            | ((uint64_t)(r.stable & 0x1U) << 32);
            uint64_t l2_bits;
            double l2_val = r.err.L2;
            memcpy(&l2_bits, &l2_val, sizeof(uint64_t));
            lum_id |= (l2_bits & 0x7FFFU);
            forensic_log_individual_lum(lum_id, op_buf, ts);
        }
        printf("\n");
    }

    /* ═══════════════════════════════════════════════════════════════════════
     * Analyse Richardson
     * ═══════════════════════════════════════════════════════════════════════ */
    printf("=== ANALYSE RICHARDSON — ORDRE DE CONVERGENCE SPATIALE ===\n\n");

    int t01m_pass = 0, t02m_pass = 0, t03m_pass = 0;
    int t04m_pass = 0, t05m_pass = 0;

    for (int pi = 0; pi < N_PROTOCOLS; pi++) {
        printf("  Protocole %s :\n", PROTO_NAMES[pi]);
        printf("    %-7s | %8s | %8s | %8s | %s\n",
               "Grille", "L1", "L2", "Linf", "Stable?");
        printf("    -------+----------+----------+----------+--------\n");

        for (int gi = 0; gi < N_GRIDS; gi++) {
            MMSGridResult *r = &results[pi][gi];
            printf("    %3d×%3d | %8.3e | %8.3e | %8.3e | %s\n",
                   r->n, r->n,
                   r->err.L1, r->err.L2, r->err.Linf,
                   r->stable ? "OUI" : "DIV");
        }

        double ord_32_64  = richardson_order(results[pi][0].err.L2,
                                              results[pi][1].err.L2);
        double ord_64_128 = richardson_order(results[pi][1].err.L2,
                                              results[pi][2].err.L2);

        printf("\n    Ordre observe 32->64  : ");
        if (ord_32_64 < -8880.0 && ord_32_64 > -8900.0)
            printf("N/A_MACHINE (L2 < 1e-15)\n");
        else if (ord_32_64 < -999.0)
            printf("N/A (L2 non décroissant)\n");
        else
            printf("%.3f\n", ord_32_64);

        printf("    Ordre observe 64->128 : ");
        if (ord_64_128 < -8880.0 && ord_64_128 > -8900.0)
            printf("N/A_MACHINE (L2 < 1e-15)\n");
        else if (ord_64_128 < -999.0)
            printf("N/A (L2 non décroissant)\n");
        else
            printf("%.3f\n", ord_64_128);

        if (pi == PROTO_C) {
            double l2_32  = results[pi][0].err.L2;
            double l2_64  = results[pi][1].err.L2;
            double l2_128 = results[pi][2].err.L2;

            /* T01m/T02m : PASS si ordre >= 1.5 */
            t01m_pass = (ord_32_64  >= 1.5) ? 1 : 0;
            t02m_pass = (ord_64_128 >= 1.5) ? 1 : 0;

            /* T03m : L2 strictement décroissant */
            t03m_pass = (l2_32 > l2_64 && l2_64 > l2_128) ? 1 : 0;

            /* T04m : Linf_128 < 0.1 */
            t04m_pass = (results[pi][2].err.Linf < 0.1) ? 1 : 0;

            /* T05m : ordre dans [1.5, 2.5] (compatible O(dx²)) */
            t05m_pass = (ord_64_128 >= 1.5 && ord_64_128 <= 2.5) ? 1 : 0;
        }
        printf("\n");
    }

    /* ═══════════════════════════════════════════════════════════════════════
     * Synthèse tests T00m–T05m
     * ═══════════════════════════════════════════════════════════════════════ */
    printf("=== TESTS T00m–T05m ===\n\n");

    double ord_c_32_64  = richardson_order(results[PROTO_C][0].err.L2,
                                            results[PROTO_C][1].err.L2);
    double ord_c_64_128 = richardson_order(results[PROTO_C][1].err.L2,
                                            results[PROTO_C][2].err.L2);

    printf("  T00m — Solution fab. analytique : PASS\n");
    printf("         div=%.2e L2(t0)=%.3e bords_O/E=0\n",
           t0.max_div, t0.l2_at_t0);

    printf("  T01m — Ordre 32->64  >= 1.5 : %s (ord = %.3f)\n",
           t01m_pass ? "PASS" : "FAIL",
           (ord_c_32_64 > -999.0) ? ord_c_32_64 : 0.0);
    printf("  T02m — Ordre 64->128 >= 1.5 : %s (ord = %.3f)\n",
           t02m_pass ? "PASS" : "FAIL",
           (ord_c_64_128 > -999.0) ? ord_c_64_128 : 0.0);
    printf("  T03m — L2 strictement décroissant : %s\n",
           t03m_pass ? "PASS" : "FAIL");
    printf("         L2_32=%.3e L2_64=%.3e L2_128=%.3e\n",
           results[PROTO_C][0].err.L2,
           results[PROTO_C][1].err.L2,
           results[PROTO_C][2].err.L2);
    printf("  T04m — Linf_128 < 0.1 : %s (Linf=%.3e)\n",
           t04m_pass ? "PASS" : "FAIL",
           results[PROTO_C][2].err.Linf);
    printf("  T05m — Ordre dans [1.5, 2.5] (schéma O(dx²)) : %s (ord=%.3f)\n",
           t05m_pass ? "PASS" : "FAIL",
           (ord_c_64_128 > -999.0) ? ord_c_64_128 : 0.0);

    /* ── Limites honnêtes ── */
    printf("\n=== LIMITES HONNÊTES ===\n\n");
    printf("  - Schéma Euler explicite 1er ordre en temps.\n");
    printf("    Proto C (dt prop dx^2) isole l ordre spatial.\n");
    printf("  - CL Dirichlet MMS : bords imposés depuis u_exact.\n");
    printf("    Les bords Ouest/Est satisfont u_exact=0 analytiquement.\n");
    printf("  - Re=1 : regime diffusif pur, advection negligeable.\n");
    printf("    Pour valider l advection non lineaire : MMS avec Re > 100 (OPEN).\n");
    printf("  - Separation erreur spatiale/temporelle : proto C fournit la separation.\n");
    printf("    Proto A (dt=const) melange erreurs spatiale et temporelle.\n");
    printf("  - CERTIFIED_100=false | unique_human_proven=false\n");

    /* ── Verdict global ── */
    int all_pass = t01m_pass && t02m_pass && t03m_pass &&
                   t04m_pass && t05m_pass;

    printf("\n[VERDICT] RICHARDSON-004-MMS : %s\n",
           all_pass ? "PASS — ordre spatial démontre, L2 décroissant, schéma O(dx^2)"
                    : "FAIL honnete — voir T00m..T05m ci-dessus");
    printf("[NOTE] T00m=PASS T01m=%s T02m=%s T03m=%s T04m=%s T05m=%s\n",
           t01m_pass ? "PASS" : "FAIL",
           t02m_pass ? "PASS" : "FAIL",
           t03m_pass ? "PASS" : "FAIL",
           t04m_pass ? "PASS" : "FAIL",
           t05m_pass ? "PASS" : "FAIL");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    forensic_logger_destroy();
    return all_pass ? 0 : 1;
}
