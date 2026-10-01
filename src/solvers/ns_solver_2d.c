/* **************************************************************************
** ns_solver_2d.c — Solveur Navier-Stokes 2D incompressible (méthode FD)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/solvers / Navier-Stokes 2D
** Auteur : LumVorax Project
**
** Algorithme : Méthode de projection de Chorin (1968) sur grille décalée.
**   Schéma temporel : Euler explicite (1er ordre).
**   Advection      : différences centrées du 2ème ordre.
**   Diffusion      : différences centrées du 2ème ordre.
**   Pression       : Gauss-Seidel SOR (ω=1.5).
**
** Condition de stabilité CFL : dt ≤ min(Re*dx²*dy²/(2*(dx²+dy²)), dx/U, dy/V)
**
** Validation : Lid-Driven Cavity Re=100 — données Ghia et al. 1982.
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "ns_solver_2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Macros d'accès aux grilles ──────────────────────────────────────────── */

/* u : dimensions (nx+1) × (ny+2)  → i=0..nx, j=0..ny+1 */
#define U(s, i, j)   ((s)->u[(i) * ((s)->params.ny + 2) + (j)])
#define UT(s, i, j)  ((s)->u_tmp[(i) * ((s)->params.ny + 2) + (j)])

/* v : dimensions (nx+2) × (ny+1)  → i=0..nx+1, j=0..ny */
#define V(s, i, j)   ((s)->v[(i) * ((s)->params.ny + 1) + (j)])
#define VT(s, i, j)  ((s)->v_tmp[(i) * ((s)->params.ny + 1) + (j)])

/* p : dimensions (nx+2) × (ny+2)  → i=0..nx+1, j=0..ny+1 */
#define P(s, i, j)   ((s)->p[(i) * ((s)->params.ny + 2) + (j)])

/* ── Allocation / libération ─────────────────────────────────────────────── */

NSSolver2D *ns_solver_create(const NSParams *params)
{
    if (!params || params->nx < 2 || params->ny < 2 ||
        params->re <= 0.0 || params->dt <= 0.0) {
        fprintf(stderr, "[NS_SOLVER][ERROR] Paramètres invalides\n");
        return NULL;
    }

    NSSolver2D *s = (NSSolver2D *)calloc(1, sizeof(NSSolver2D));
    if (!s) {
        fprintf(stderr, "[NS_SOLVER][ERROR] calloc NSSolver2D\n");
        return NULL;
    }

    s->params = *params;
    s->dx     = params->lx / (double)params->nx;
    s->dy     = params->ly / (double)params->ny;
    s->step   = 0;

    int nx = params->nx;
    int ny = params->ny;

    /* tailles des tableaux */
    size_t sz_u = (size_t)(nx + 1) * (size_t)(ny + 2);
    size_t sz_v = (size_t)(nx + 2) * (size_t)(ny + 1);
    size_t sz_p = (size_t)(nx + 2) * (size_t)(ny + 2);

    s->u     = (double *)calloc(sz_u, sizeof(double));
    s->u_tmp = (double *)calloc(sz_u, sizeof(double));
    s->v     = (double *)calloc(sz_v, sizeof(double));
    s->v_tmp = (double *)calloc(sz_v, sizeof(double));
    s->p     = (double *)calloc(sz_p, sizeof(double));

    if (!s->u || !s->u_tmp || !s->v || !s->v_tmp || !s->p) {
        fprintf(stderr, "[NS_SOLVER][ERROR] calloc grilles\n");
        ns_solver_destroy(s);
        return NULL;
    }

    if (params->debug) {
        fprintf(stderr, "[NS_SOLVER][DEBUG] Grille %dx%d | dx=%.4f dy=%.4f | Re=%.1f | dt=%.6f\n",
                nx, ny, s->dx, s->dy, params->re, params->dt);
        fprintf(stderr, "[NS_SOLVER][DEBUG] Mémoire : u=%zu v=%zu p=%zu doubles\n",
                sz_u, sz_v, sz_p);
    }

    return s;
}

void ns_solver_destroy(NSSolver2D *s)
{
    if (!s) return;
    free(s->u);
    free(s->u_tmp);
    free(s->v);
    free(s->v_tmp);
    free(s->p);
    free(s);
}

/* ── Conditions aux limites Lid-Driven Cavity ────────────────────────────── */

void ns_solver_set_lid_bc(NSSolver2D *s)
{
    int nx = s->params.nx;
    int ny = s->params.ny;

    /*
     * Convention grille décalée :
     *   u[0][j]    = bord Ouest  (u=0)
     *   u[nx][j]   = bord Est    (u=0)
     *   u[i][0]    = bord Sud    (u=0)
     *   u[i][ny+1] = bord Nord   (u=1 : couvercle)
     *
     *   v[i][0]    = bord Sud    (v=0)
     *   v[i][ny]   = bord Nord   (v=0)
     *   v[0][j]    = bord Ouest  (v=0)
     *   v[nx+1][j] = bord Est    (v=0)
     */

    /* bords Ouest et Est pour u */
    for (int j = 0; j <= ny + 1; j++) {
        U(s, 0,  j) = 0.0;
        U(s, nx, j) = 0.0;
    }

    /* bords Sud et Nord pour u */
    for (int i = 0; i <= nx; i++) {
        U(s, i, 0)     = -U(s, i, 1);           /* no-slip Sud  (image miroir) */
        U(s, i, ny + 1) = 2.0 - U(s, i, ny);    /* couvercle Nord : u=1 */
    }

    /* bords Sud et Nord pour v */
    for (int i = 0; i <= nx + 1; i++) {
        V(s, i, 0)  = 0.0;
        V(s, i, ny) = 0.0;
    }

    /* bords Ouest et Est pour v */
    for (int j = 0; j <= ny; j++) {
        V(s, 0,      j) = -V(s, 1,  j);          /* no-slip Ouest (image miroir) */
        V(s, nx + 1, j) = -V(s, nx, j);           /* no-slip Est  (image miroir) */
    }

    /* pression : Neumann homogène dp/dn=0 (bords = cellule voisine intérieure) */
    for (int j = 0; j <= ny + 1; j++) {
        P(s, 0,      j) = P(s, 1,      j);
        P(s, nx + 1, j) = P(s, nx,     j);
    }
    for (int i = 0; i <= nx + 1; i++) {
        P(s, i, 0)      = P(s, i, 1);
        P(s, i, ny + 1) = P(s, i, ny);
    }
}

/* ── Étape 1 : vitesses intermédiaires (advection + diffusion) ───────────── */

static void compute_intermediate_velocity(NSSolver2D *s)
{
    int    nx  = s->params.nx;
    int    ny  = s->params.ny;
    double dx  = s->dx;
    double dy  = s->dy;
    double dt  = s->params.dt;
    double re  = s->params.re;
    double dx2 = dx * dx;
    double dy2 = dy * dy;

    /* ---- composante u ---- */
    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            /* advection u.du/dx : différences centrées */
            double u_e = 0.5 * (U(s, i, j) + U(s, i + 1, j));
            double u_w = 0.5 * (U(s, i, j) + U(s, i - 1, j));
            double adv_x = (u_e * u_e - u_w * u_w) / dx;

            /* advection v.du/dy : interpolation */
            double v_n = 0.5 * (V(s, i, j)     + V(s, i + 1, j));
            double v_s = 0.5 * (V(s, i, j - 1) + V(s, i + 1, j - 1));
            double u_n = 0.5 * (U(s, i, j)     + U(s, i, j + 1));
            double u_s = 0.5 * (U(s, i, j)     + U(s, i, j - 1));
            double adv_y = (v_n * u_n - v_s * u_s) / dy;

            /* diffusion (1/Re) * laplacien(u) */
            double diff = (1.0 / re) * (
                (U(s, i + 1, j) - 2.0 * U(s, i, j) + U(s, i - 1, j)) / dx2 +
                (U(s, i, j + 1) - 2.0 * U(s, i, j) + U(s, i, j - 1)) / dy2
            );

            UT(s, i, j) = U(s, i, j) + dt * (-adv_x - adv_y + diff);
        }
    }

    /* ---- composante v ---- */
    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j < ny; j++) {
            /* advection u.dv/dx */
            double u_e = 0.5 * (U(s, i, j)     + U(s, i, j + 1));
            double u_w = 0.5 * (U(s, i - 1, j) + U(s, i - 1, j + 1));
            double v_e = 0.5 * (V(s, i, j)     + V(s, i + 1, j));
            double v_w = 0.5 * (V(s, i, j)     + V(s, i - 1, j));
            double adv_x = (u_e * v_e - u_w * v_w) / dx;

            /* advection v.dv/dy */
            double v_n = 0.5 * (V(s, i, j) + V(s, i, j + 1));
            double v_s = 0.5 * (V(s, i, j) + V(s, i, j - 1));
            double adv_y = (v_n * v_n - v_s * v_s) / dy;

            /* diffusion */
            double diff = (1.0 / re) * (
                (V(s, i + 1, j) - 2.0 * V(s, i, j) + V(s, i - 1, j)) / dx2 +
                (V(s, i, j + 1) - 2.0 * V(s, i, j) + V(s, i, j - 1)) / dy2
            );

            VT(s, i, j) = V(s, i, j) + dt * (-adv_x - adv_y + diff);
        }
    }
}

/* ── Étape 2 : solveur Poisson pour la pression (Gauss-Seidel SOR) ──────── */

static double solve_pressure_poisson(NSSolver2D *s)
{
    int    nx  = s->params.nx;
    int    ny  = s->params.ny;
    double dx  = s->dx;
    double dy  = s->dy;
    double dt  = s->params.dt;
    double dx2 = dx * dx;
    double dy2 = dy * dy;
    double omega = 1.5;  /* facteur de sur-relaxation SOR */
    double residual = 0.0;

    int max_iter = s->params.max_poisson;
    double tol   = s->params.tol;

    for (int iter = 0; iter < max_iter; iter++) {
        residual = 0.0;

        for (int i = 1; i <= nx; i++) {
            for (int j = 1; j <= ny; j++) {
                /* divergence des vitesses intermédiaires */
                double div = (UT(s, i, j) - UT(s, i - 1, j)) / dx
                           + (VT(s, i, j) - VT(s, i, j - 1)) / dy;

                /* terme source Poisson : div/dt */
                double rhs = div / dt;

                /* coefficient central */
                double coeff = 2.0 / dx2 + 2.0 / dy2;

                /* valeur Gauss-Seidel */
                double p_new = (
                    (P(s, i + 1, j) + P(s, i - 1, j)) / dx2 +
                    (P(s, i, j + 1) + P(s, i, j - 1)) / dy2 - rhs
                ) / coeff;

                /* SOR */
                double dp = omega * (p_new - P(s, i, j));
                P(s, i, j) += dp;
                residual += dp * dp;
            }
        }

        /* conditions aux limites pression (Neumann) */
        for (int j = 0; j <= ny + 1; j++) {
            P(s, 0,      j) = P(s, 1,  j);
            P(s, nx + 1, j) = P(s, nx, j);
        }
        for (int i = 0; i <= nx + 1; i++) {
            P(s, i, 0)      = P(s, i, 1);
            P(s, i, ny + 1) = P(s, i, ny);
        }

        residual = sqrt(residual / (double)(nx * ny));
        if (residual < tol) break;
    }

    return residual;
}

/* ── Étape 3 : correction des vitesses ──────────────────────────────────── */

static void correct_velocity(NSSolver2D *s)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dx = s->dx;
    double dy = s->dy;
    double dt = s->params.dt;

    /* u = u* - dt * dp/dx */
    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            U(s, i, j) = UT(s, i, j) - dt * (P(s, i + 1, j) - P(s, i, j)) / dx;
        }
    }

    /* v = v* - dt * dp/dy */
    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j < ny; j++) {
            V(s, i, j) = VT(s, i, j) - dt * (P(s, i, j + 1) - P(s, i, j)) / dy;
        }
    }
}

/* ── API : un pas de temps ───────────────────────────────────────────────── */

double ns_solver_step(NSSolver2D *s)
{
    /* copie u_tmp = u, v_tmp = v pour les bords */
    int nx = s->params.nx;
    int ny = s->params.ny;
    memcpy(s->u_tmp, s->u, (size_t)(nx + 1) * (size_t)(ny + 2) * sizeof(double));
    memcpy(s->v_tmp, s->v, (size_t)(nx + 2) * (size_t)(ny + 1) * sizeof(double));

    compute_intermediate_velocity(s);
    double residual = solve_pressure_poisson(s);
    correct_velocity(s);
    ns_solver_set_lid_bc(s);
    s->step++;

    if (s->params.debug && (s->step % 100 == 0))
        ns_solver_print_stats(s, residual);

    return residual;
}

/* ── API : run complet ───────────────────────────────────────────────────── */

int ns_solver_run(NSSolver2D *s)
{
    for (int i = 0; i < s->params.max_iter; i++)
        ns_solver_step(s);
    return s->step;
}

/* ── Extraction des profils pour validation Ghia ────────────────────────── */

void ns_solver_u_centerline_y(const NSSolver2D *s, double *out_y, double *out_u)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dy = s->dy;
    int    i  = nx / 2;  /* colonne centrale x=0.5 */

    /*
     * On extrait ny+1 points :
     *   j=1..ny  : centres des cellules intérieures (y = (j-0.5)*dy)
     *   j=ny+1   : face Nord couvercle (y = 1.0, u = 1.0 imposée)
     * Le tableau out_y/out_u doit donc être alloué pour ny+1 éléments.
     */
    for (int j = 1; j <= ny; j++) {
        out_y[j - 1] = ((double)j - 0.5) * dy;
        /* interpolation u au centre de la cellule (i, j) */
        out_u[j - 1] = 0.5 * (U(s, i, j) + U(s, i + 1, j));
    }
    /* nœud couvercle : u = 1.0 imposée en condition aux limites */
    out_y[ny] = 1.0;
    out_u[ny] = 1.0;
}

void ns_solver_v_centerline_x(const NSSolver2D *s, double *out_x, double *out_v)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dx = s->dx;
    int    j  = ny / 2;  /* ligne centrale y=0.5 */

    for (int i = 1; i <= nx; i++) {
        out_x[i - 1] = ((double)i - 0.5) * dx;
        /* interpolation v au centre de la cellule (i, j) */
        out_v[i - 1] = 0.5 * (V(s, i, j) + V(s, i, j + 1));
    }
}

/* ── Affichage DEBUG ─────────────────────────────────────────────────────── */

void ns_solver_print_stats(const NSSolver2D *s, double poisson_residual)
{
    int nx = s->params.nx;
    int ny = s->params.ny;

    /* vitesse max dans le domaine intérieur */
    double u_max = 0.0;
    double v_max = 0.0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++)
            if (fabs(U(s, i, j)) > u_max) u_max = fabs(U(s, i, j));
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++)
            if (fabs(V(s, i, j)) > v_max) v_max = fabs(V(s, i, j));

    fprintf(stderr,
        "[NS][step=%05d] Poisson_res=%.2e | u_max=%.4f | v_max=%.4f\n",
        s->step, poisson_residual, u_max, v_max);
}
