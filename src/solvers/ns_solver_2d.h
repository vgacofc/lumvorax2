/* **************************************************************************
** ns_solver_2d.h — Solveur Navier-Stokes 2D incompressible (méthode FD)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/solvers / Navier-Stokes 2D
** Auteur : LumVorax Project
**
** Méthode : Différences finies centrées, schéma de projection de Chorin.
** Validation cible : Lid-Driven Cavity Re=100 — données Ghia et al. 1982.
**
** Référence : Ghia U., Ghia K.N., Shin C.T. (1982).
**   "High-Re solutions for incompressible flow using the Navier-Stokes
**    equations and a multigrid method." J. Comput. Phys., 48, 387-411.
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#ifndef NS_SOLVER_2D_H
#define NS_SOLVER_2D_H

#include <stddef.h>

/* ── Paramètres physiques ── */
typedef struct {
    int    nx;          /* nombre de cellules en x (sans les bords) */
    int    ny;          /* nombre de cellules en y (sans les bords) */
    double lx;          /* longueur du domaine en x (m) */
    double ly;          /* longueur du domaine en y (m) */
    double re;          /* nombre de Reynolds */
    double dt;          /* pas de temps (s) */
    int    max_iter;    /* nombre max d'itérations temporelles */
    double tol;         /* tolérance convergence pression (Poisson) */
    int    max_poisson; /* itérations max solveur Poisson interne */
    int    debug;       /* 1 = afficher stats toutes les 100 itérations */
} NSParams;

/* ── État du solveur (grille décalée — staggered grid) ──
 *
 *  u[i][j] = composante vitesse en x au point (i+1/2, j)
 *  v[i][j] = composante vitesse en y au point (i, j+1/2)
 *  p[i][j] = pression au centre de la cellule (i, j)
 *
 *  Tailles :
 *    u : (nx+1) × (ny+2)   — inclut les bords fantômes
 *    v : (nx+2) × (ny+1)   — inclut les bords fantômes
 *    p : (nx+2) × (ny+2)   — inclut les bords fantômes
 */
typedef struct {
    NSParams params;
    double  *u;     /* [i*(ny+2)+j] */
    double  *v;     /* [i*(ny+1)+j] */
    double  *p;     /* [i*(ny+2)+j] */
    double  *u_tmp; /* buffer temporaire pour étape advection/diffusion */
    double  *v_tmp;
    double   dx;    /* lx / nx */
    double   dy;    /* ly / ny */
    int      step;  /* itération courante */
} NSSolver2D;

/* ── API publique ── */

/**
 * ns_solver_create() — Alloue et initialise le solveur.
 * @params : paramètres physiques (copié dans le solveur).
 * @return : pointeur alloué (à libérer avec ns_solver_destroy), NULL si échec.
 */
NSSolver2D *ns_solver_create(const NSParams *params);

/**
 * ns_solver_destroy() — Libère toutes les ressources.
 */
void ns_solver_destroy(NSSolver2D *s);

/**
 * ns_solver_set_lid_bc() — Applique les conditions aux limites Lid-Driven Cavity.
 *   - Parois Ouest/Est/Sud = no-slip (u=v=0)
 *   - Couvercle Nord = u=1 (vitesse imposée), v=0
 *   - Pression : Neumann homogène sur tous les bords (dp/dn=0)
 */
void ns_solver_set_lid_bc(NSSolver2D *s);

/**
 * ns_solver_step() — Exécute une itération temporelle (schéma de projection).
 *   Étape 1 : vitesses intermédiaires u*, v* (advection + diffusion explicite)
 *   Étape 2 : solveur Poisson pour la pression (Gauss-Seidel SOR)
 *   Étape 3 : correction des vitesses (u = u* - dt/rho * dp/dx)
 *   Étape 4 : application des conditions aux limites
 * @return : résidu Poisson de la dernière itération interne.
 */
double ns_solver_step(NSSolver2D *s);

/**
 * ns_solver_run() — Exécute max_iter pas de temps.
 * @return : nombre de pas effectués.
 */
int ns_solver_run(NSSolver2D *s);

/**
 * ns_solver_u_centerline_y() — Extrait u(x=0.5, y) sur ny+1 points.
 * Utilisé pour la comparaison Ghia 1982 (profil u sur la ligne centrale verticale).
 * @out_y   : tableau de taille ny+1, rempli avec les coordonnées y normalisées [0,1]
 *            Dernier point (ny) = face couvercle y=1.0, u=1.0 imposée.
 * @out_u   : tableau de taille ny+1, rempli avec la vitesse u normalisée [-1,1]
 */
void ns_solver_u_centerline_y(const NSSolver2D *s, double *out_y, double *out_u);

/**
 * ns_solver_v_centerline_x() — Extrait v(x, y=0.5) sur nx points.
 * Utilisé pour la comparaison Ghia 1982 (profil v sur la ligne centrale horizontale).
 */
void ns_solver_v_centerline_x(const NSSolver2D *s, double *out_x, double *out_v);

/**
 * ns_solver_print_stats() — Affiche les statistiques courantes (DEBUG).
 */
void ns_solver_print_stats(const NSSolver2D *s, double poisson_residual);

#endif /* NS_SOLVER_2D_H */
