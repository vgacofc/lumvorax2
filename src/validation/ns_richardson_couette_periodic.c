/* **************************************************************************
** ns_richardson_couette_periodic.c — Richardson-PROTOCOL-003b : Couette périodique
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Richardson-PROTOCOL-003b
** Auteur : LumVorax Project
**
** Objectif : Résoudre le problème de causalité identifié dans le rapport 158 §5 :
**
**   PROTOCOL-003 utilisait ns_solver_set_lid_bc() qui impose des parois
**   latérales no-slip. La solution exacte du problème résolu ≠ u = y.
**
** ═══ Solution retenue : Couette plan avec CL périodiques en x ═══
**
**   Géométrie : domaine [0,1]×[0,1]
**   Conditions aux limites :
**     y=0 (j=0, paroi basse)  : u = 0  (no-slip Sud)
**     y=1 (j=ny, paroi haute) : u = 1  (plaque mobile Nord = couvercle)
**     x=0 / x=Lx              : PÉRIODIQUES (u[0,j] = u[nx,j])
**     v = 0 sur tous les bords
**     p : Neumann homogène
**
**   Solution stationnaire exacte (Couette plan) :
**     u_exact(x, y) = y   (profil linéaire)
**     v_exact = 0
**     p_exact = 0
**
**   POURQUOI ça marche avec les périodiques :
**     Avec u[0,j]=u[nx,j], les parois latérales sont transparentes.
**     La solution stationnaire satisfait Δu = 0 (terme de diffusion nul
**     puisque d²u/dy² = 0 pour u=y). L'advection est nulle (u·∇u = 0
**     pour u=u(y) uniforme en x). Donc u = y est bien la solution exacte.
**
** ═══ BUG-2 FIX : coordonnée staggered MAC correcte ═══
**
**   AVANT (PROTOCOL-003) : y_node = j * dy      → erreur systématique
**   APRÈS (PROTOCOL-003b): y_node = (j-0.5)*dy  → position réelle de u[i][j]
**
**   Convention MAC : u[i][j] se situe à la face horizontale entre les cellules
**   j-1 et j. Sa position physique en y est (j - 0.5) * dy, pas j * dy.
**
**   Le rapport 158 signale que la causalité n'est pas démontrée (BUG-2 seul
**   ne suffit pas à expliquer Linf > 1). Ce fichier mesure min/max(u) pour
**   vérifier la présence éventuelle d'overshoot.
**
** ═══ Mesure diagnostique min/max ═══
**
**   Pour chaque grille et protocole, on mesure :
**     u_min = min(u[i][j]) sur tous les nœuds intérieurs
**     u_max = max(u[i][j]) sur tous les nœuds intérieurs
**   Si u_max > 1 ou u_min < 0 → overshoot/undershoot → Linf > 1 possible
**   indépendamment de la coordonnée de référence.
**
** ═══ CL périodiques en x : implémentation ═══
**
**   On substitue ns_solver_set_lid_bc() par set_couette_periodic_bc() qui :
**     - Conserve les CL Nord/Sud identiques à lid_bc (u=1 Nord, u=0 Sud)
**     - Remplace les bords Ouest/Est no-slip par la périodicité :
**         u[0][j]    = u[nx-1][j]   (bord Ouest = colonne intérieure droite)
**         u[nx][j]   = u[1][j]      (bord Est   = colonne intérieure gauche)
**     - v = 0 sur tous les bords (flux nul aux parois et en périodique)
**     - p : Neumann homogène inchangé
**
**   Note : On appelle set_couette_periodic_bc() après chaque ns_solver_step()
**   via un wrapper run_step_periodic() pour forcer les bords périodiques à
**   chaque pas — le solveur interne appelle ns_solver_set_lid_bc() dans step(),
**   on le remplace par nos propres bords après.
**
** ═══ Tests T01b–T04b ═══
**
**   T01b — Ordre spatial 32→64  ≥ 1.5 (protocole C, CL périodiques)
**   T02b — Ordre spatial 64→128 ≥ 1.5 (protocole C, CL périodiques)
**   T03b — L2 strictement décroissant (protocole C)
**   T04b — Linf_128 < 0.05 (protocole C)
**   T05b — u_max_128 ≤ 1.0 + 1e-6 (pas d'overshoot)
**   T06b — u_min_128 ≥ 0.0 - 1e-6 (pas d'undershoot)
**
** ═══ Limites honnêtes ═══
**
**   - La périodicité est appliquée APRÈS ns_solver_step() car set_lid_bc()
**     est appelé en interne. Une implémentation plus propre modifierait le
**     solveur pour accepter un callback de CL — noté comme OPEN.
**   - v = 0 en périodique ne garantit pas que le solveur converge vers
**     v = 0 exactement — on mesure |v_max| dans le rapport.
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

/* ── Paramètres ─────────────────────────────────────────────────────────── */

#define RE_COUETTE    100.0
#define DT_REF        1e-4
#define N_REF         32
#define DX_REF        (1.0 / N_REF)

#define EPS_CONV      1e-3
#define WIN_CHECKS    100
#define POLL_INTERVAL 50

#define LIMIT_STEPS_A  40000
#define LIMIT_STEPS_BC 500000

#define N_GRIDS   3
static const int GRIDS[N_GRIDS] = {32, 64, 128};

#define N_PROTOCOLS 3
typedef enum { PROTO_A = 0, PROTO_B = 1, PROTO_C = 2 } Protocol;
static const char *PROTO_NAMES[N_PROTOCOLS] = {
    "A (dt=const)", "B (dt∝dx)", "C (dt∝dx²)"
};

/* ── Solution exacte Couette plan ────────────────────────────────────────── */

static double couette_u_exact(double x, double y)
{
    (void)x;
    return y;
}

/* ── CL Couette périodiques en x ─────────────────────────────────────────── */

/* Macros locales (copie de ns_solver_2d.c) pour accès direct */
#define U(s, i, j)  ((s)->u[(i) * ((s)->params.ny + 2) + (j)])
#define V(s, i, j)  ((s)->v[(i) * ((s)->params.ny + 1) + (j)])
#define P(s, i, j)  ((s)->p[(i) * ((s)->params.ny + 2) + (j)])

static void set_couette_periodic_bc(NSSolver2D *s)
{
    int nx = s->params.nx;
    int ny = s->params.ny;

    /* ── Bords Nord/Sud identiques à lid_bc ── */
    for (int i = 0; i <= nx; i++) {
        U(s, i, 0)      = -U(s, i, 1);          /* no-slip Sud (image miroir) */
        U(s, i, ny + 1) = 2.0 - U(s, i, ny);    /* couvercle Nord : u=1 */
    }

    /* ── Bords Ouest/Est PÉRIODIQUES pour u ── */
    for (int j = 0; j <= ny + 1; j++) {
        U(s, 0,  j) = U(s, nx - 1, j);  /* Ouest copie de nx-1 (dernier intérieur) */
        U(s, nx, j) = U(s, 1,      j);  /* Est   copie de 1    (premier intérieur) */
    }

    /* ── v = 0 sur tous les bords ── */
    for (int i = 0; i <= nx + 1; i++) {
        V(s, i, 0)  = 0.0;
        V(s, i, ny) = 0.0;
    }
    for (int j = 0; j <= ny; j++) {
        V(s, 0,      j) = 0.0;   /* Ouest périodique : flux nul */
        V(s, nx + 1, j) = 0.0;   /* Est   périodique : flux nul */
    }

    /* ── Pression : Neumann homogène ── */
    for (int j = 0; j <= ny + 1; j++) {
        P(s, 0,      j) = P(s, 1,  j);
        P(s, nx + 1, j) = P(s, nx, j);
    }
    for (int i = 0; i <= nx + 1; i++) {
        P(s, i, 0)      = P(s, i, 1);
        P(s, i, ny + 1) = P(s, i, ny);
    }
}

/* ── Calcul des erreurs L1/L2/Linf — BUG-2 FIX : y_node = (j-0.5)*dy ─────── */

typedef struct {
    double L1;
    double L2;
    double Linf;
    double u_min;    /* min(u) intérieur — diagnostic overshoot */
    double u_max;    /* max(u) intérieur — diagnostic overshoot */
    double v_max;    /* max(|v|) intérieur — doit être ~0 pour Couette */
    int    n_cells;
} ErrorMetrics;

static ErrorMetrics compute_errors_u(const NSSolver2D *s)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dy = s->dy;
    double dx = s->dx;

    double sum_abs = 0.0;
    double sum_sq  = 0.0;
    double max_err = 0.0;
    double u_min   =  1e30;
    double u_max   = -1e30;
    double v_max   =  0.0;
    int    n_cells = 0;

    /* BUG-2 FIX : y_node = (j - 0.5) * dy
     * Convention MAC : u[i][j] est à la face horizontale entre j-1 et j.
     * Position physique : y = (j - 0.5) * dy  pour j = 1..ny.
     *
     * AVANT (PROTOCOL-003) : y_node = j * dy  →  pour j=1, y=dy  (bord intérieur sup)
     * APRÈS (PROTOCOL-003b): y_node = (j-0.5)*dy  → pour j=1, y=0.5*dy  (centre cell)
     */
    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double x_node     = (i + 0.5) * dx;     /* non utilisé pour Couette */
            double y_node     = (j - 0.5) * dy;     /* BUG-2 FIX */
            double u_exact_val = couette_u_exact(x_node, y_node);
            double u_num       = s->u[i * (ny + 2) + j];
            double err         = fabs(u_num - u_exact_val);

            sum_abs += err;
            sum_sq  += err * err;
            if (err > max_err) max_err = err;
            if (u_num < u_min) u_min = u_num;
            if (u_num > u_max) u_max = u_num;
            n_cells++;
        }
    }

    /* v intérieur — doit être ~0 pour Couette plan */
    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j < ny; j++) {
            double v = fabs(s->v[i * (ny + 1) + j]);
            if (v > v_max) v_max = v;
        }
    }

    ErrorMetrics m;
    m.L1      = (n_cells > 0) ? sum_abs / n_cells : 0.0;
    m.L2      = (n_cells > 0) ? sqrt(sum_sq / n_cells) : 0.0;
    m.Linf    = max_err;
    m.u_min   = u_min;
    m.u_max   = u_max;
    m.v_max   = v_max;
    m.n_cells = n_cells;
    return m;
}

/* ── Observables stationnarité ───────────────────────────────────────────── */

static double compute_u_max_field(const NSSolver2D *s)
{
    int nx = s->params.nx, ny = s->params.ny;
    double umax = 0.0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++) {
            double u = fabs(s->u[i*(ny+2)+j]);
            if (u > umax) umax = u;
        }
    return umax;
}

static double compute_kinetic_energy(const NSSolver2D *s)
{
    int nx = s->params.nx, ny = s->params.ny;
    double ek = 0.0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++) {
            double u = s->u[i*(ny+2)+j];
            ek += u * u;
        }
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++) {
            double v = s->v[i*(ny+1)+j];
            ek += v * v;
        }
    return 0.5 * ek;
}

/* ── Structure résultat ──────────────────────────────────────────────────── */

typedef struct {
    int          n;
    double       dt;
    double       dx;
    int          steps_to_converge;
    int          converged;
    double       t_physical;
    double       t_wall_s;
    ErrorMetrics err;
    double       poisson_res_final;
} GridResult;

/* ── Calcul du dt selon le protocole ─────────────────────────────────────── */

static double get_dt(int n, Protocol proto)
{
    double dx = 1.0 / n;
    switch (proto) {
    case PROTO_A: return DT_REF;
    case PROTO_B: return DT_REF * (dx / DX_REF);
    case PROTO_C: return DT_REF * (dx / DX_REF) * (dx / DX_REF);
    }
    return DT_REF;
}

/* ── Simulation jusqu'à stationnarité avec CL périodiques ────────────────── */

static GridResult run_to_steady_periodic(int n, double dt, int protocol_id)
{
    GridResult res;
    memset(&res, 0, sizeof(res));
    res.n  = n;
    res.dt = dt;
    res.dx = 1.0 / n;

    NSParams p = {
        .nx = n, .ny = n,
        .lx = 1.0, .ly = 1.0,
        .re = RE_COUETTE,
        .dt = dt,
        .max_iter    = 1,
        .tol         = 1e-6,
        .max_poisson = 100,
        .debug       = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[PROTO003b][ERROR] ns_solver_create failed n=%d\n", n);
        res.converged = 0;
        return res;
    }

    /* Initialisation avec CL périodiques */
    set_couette_periodic_bc(s);

    double *win_umax = (double *)calloc(WIN_CHECKS, sizeof(double));
    double *win_ek   = (double *)calloc(WIN_CHECKS, sizeof(double));
    int     win_idx  = 0;
    int     win_full = 0;

    double poisson_res = 1.0;
    int    total_steps = 0;
    int    converged   = 0;
    int    limit_steps = (protocol_id == PROTO_A) ? LIMIT_STEPS_A : LIMIT_STEPS_BC;

    while (total_steps < limit_steps) {
        for (int k = 0; k < POLL_INTERVAL && total_steps < limit_steps; k++) {
            /* ns_solver_step() appelle set_lid_bc() en interne.
             * On réapplique nos CL périodiques après chaque pas. */
            poisson_res = ns_solver_step(s);
            set_couette_periodic_bc(s);
            total_steps++;
        }

        double umax = compute_u_max_field(s);
        double ek   = compute_kinetic_energy(s);

        win_umax[win_idx] = umax;
        win_ek[win_idx]   = ek;
        win_idx = (win_idx + 1) % WIN_CHECKS;
        if (win_idx == 0) win_full = 1;

        if (!win_full) continue;

        int old_idx = win_idx;
        double umax_old = win_umax[old_idx];
        double ek_old   = win_ek[old_idx];

        double var_umax = (umax > 1e-12) ? fabs(umax - umax_old) / umax : 0.0;
        double var_ek   = (ek   > 1e-12) ? fabs(ek   - ek_old)   / ek   : 0.0;

        if (var_umax < EPS_CONV && var_ek < EPS_CONV && poisson_res < 1e-4) {
            converged = 1;
            break;
        }
    }

    free(win_umax);
    free(win_ek);

    res.converged         = converged;
    res.steps_to_converge = total_steps;
    res.t_physical        = total_steps * dt;
    res.poisson_res_final = poisson_res;
    res.err               = compute_errors_u(s);

    (void)protocol_id;
    ns_solver_destroy(s);
    return res;
}

/* ── Forensic checkpoint ─────────────────────────────────────────────────── */

static void log_forensic_checkpoint_b(int n, int protocol_id,
                                       double L2, int steps, int converged)
{
    uint64_t ts = time_ns_get_absolute();
    char op_buf[80];
    snprintf(op_buf, sizeof(op_buf),
             "PROTO003b:n=%d:proto=%d:L2=%.6f:conv=%d",
             n, protocol_id, L2, converged);

    /* FORENSIC-UNIF-003 BUG-1 : lum_id uint64_t complet */
    uint64_t lum_id = ((uint64_t)(n           & 0xFFFFU) << 48)
                    | ((uint64_t)(protocol_id & 0xFFU)   << 40)
                    | ((uint64_t)((steps/1000) & 0xFFU)  << 32)
                    | ((uint64_t)(converged   & 0x1U)    << 31);
    uint64_t l2_bits;
    memcpy(&l2_bits, &L2, sizeof(uint64_t));
    lum_id |= (l2_bits & 0x7FFFU);

    forensic_log_individual_lum(lum_id, op_buf, ts);
}

/* ── Calcul ordre Richardson ─────────────────────────────────────────────── */

static double richardson_order(double L2_coarse, double L2_fine)
{
    if (L2_coarse <= 0.0 || L2_fine <= 0.0 || L2_coarse <= L2_fine)
        return -9999.0;
    return log2(L2_coarse / L2_fine);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== RICHARDSON-PROTOCOL-003b : COUETTE PÉRIODIQUE ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[REF] Couette plan : u(x,y) = y  | CL x = PÉRIODIQUES\n");
    printf("[BUG-2 FIX] y_node = (j-0.5)*dy  (convention MAC staggered correcte)\n");
    printf("[DIAGNOSTIC] u_min/u_max mesurés pour détecter overshoot/undershoot\n\n");
    printf("[DT] Protocole A = constant = %.2e\n", DT_REF);
    printf("[DT] Protocole B = dt∝dx\n");
    printf("[DT] Protocole C = dt∝dx²\n\n");

    forensic_logger_init("logs/forensic/ns_richardson_couette_periodic.log");

    GridResult results[N_PROTOCOLS][N_GRIDS];
    memset(results, 0, sizeof(results));
    struct timespec t0, t1;

    for (int pi = 0; pi < N_PROTOCOLS; pi++) {
        Protocol proto = (Protocol)pi;
        printf("─── Protocole %s ───\n", PROTO_NAMES[pi]);

        int n_grids_proto = (pi == PROTO_A) ? 2 : N_GRIDS;

        for (int gi = 0; gi < n_grids_proto; gi++) {
            int    n  = GRIDS[gi];
            double dt = get_dt(n, proto);

            printf("  Grille %3d×%3d | dt=%.3e ... ", n, n, dt);
            fflush(stdout);

            clock_gettime(CLOCK_MONOTONIC, &t0);
            GridResult r = run_to_steady_periodic(n, dt, pi);
            clock_gettime(CLOCK_MONOTONIC, &t1);

            r.t_wall_s = (t1.tv_sec  - t0.tv_sec) +
                         (t1.tv_nsec - t0.tv_nsec) * 1e-9;
            results[pi][gi] = r;

            printf("%s | steps=%d | L1=%.6f | L2=%.6f | Linf=%.6f\n"
                   "                       u_min=%.4f u_max=%.4f v_max=%.2e | wall=%.1f s\n",
                   r.converged ? "CONV" : "LIMIT",
                   r.steps_to_converge,
                   r.err.L1, r.err.L2, r.err.Linf,
                   r.err.u_min, r.err.u_max, r.err.v_max,
                   r.t_wall_s);

            log_forensic_checkpoint_b(n, pi, r.err.L2,
                                      r.steps_to_converge, r.converged);
        }
        if (pi == PROTO_A)
            printf("  [NOTE] Grille 128×128 exclue du protocole A : dt=1e-4 → coût trop élevé\n");
        printf("\n");
    }

    /* ── Analyse Richardson ── */
    printf("=== ANALYSE RICHARDSON — ORDRE DE CONVERGENCE SPATIALE ===\n\n");

    int t01b_pass = 0, t02b_pass = 0, t03b_pass = 0;
    int t04b_pass = 0, t05b_pass = 0, t06b_pass = 0;

    for (int pi = 0; pi < N_PROTOCOLS; pi++) {
        printf("  Protocole %s :\n", PROTO_NAMES[pi]);
        printf("    %-6s | %8s | %8s | %8s | %6s | %6s | %6s | %s\n",
               "Grille", "L1", "L2", "Linf", "u_min", "u_max", "v_max", "Conv?");
        printf("    ------+----------+----------+----------+--------+--------+--------+------\n");

        for (int gi = 0; gi < N_GRIDS; gi++) {
            GridResult *r = &results[pi][gi];
            printf("    %3d×%3d | %8.6f | %8.6f | %8.6f | %6.3f | %6.3f | %.2e | %s\n",
                   r->n, r->n,
                   r->err.L1, r->err.L2, r->err.Linf,
                   r->err.u_min, r->err.u_max, r->err.v_max,
                   r->converged ? "OUI" : "NON");
        }

        double ord_32_64  = richardson_order(results[pi][0].err.L2,
                                              results[pi][1].err.L2);
        double ord_64_128 = richardson_order(results[pi][1].err.L2,
                                              results[pi][2].err.L2);

        printf("\n    Ordre observé 32→64   : ");
        if (ord_32_64 < -999.0)  printf("N/A (L2 non décroissant)\n");
        else                     printf("%.3f\n", ord_32_64);

        printf("    Ordre observé 64→128  : ");
        if (ord_64_128 < -999.0) printf("N/A (L2 non décroissant)\n");
        else                     printf("%.3f\n", ord_64_128);

        if (pi == PROTO_C) {
            t01b_pass = (ord_32_64  >= 1.5) ? 1 : 0;
            t02b_pass = (ord_64_128 >= 1.5) ? 1 : 0;
            t03b_pass = (results[pi][0].err.L2 > results[pi][1].err.L2 &&
                         results[pi][1].err.L2 > results[pi][2].err.L2) ? 1 : 0;
            t04b_pass = (results[pi][2].err.Linf < 0.05) ? 1 : 0;
            /* T05b/T06b : pas d'overshoot/undershoot */
            t05b_pass = (results[pi][2].err.u_max <= 1.0 + 1e-6) ? 1 : 0;
            t06b_pass = (results[pi][2].err.u_min >= 0.0 - 1e-6) ? 1 : 0;
        }
        printf("\n");
    }

    /* ── Tableau de synthèse ── */
    printf("=== TESTS T01b–T06b (protocole C, CL périodiques, y_node=(j-0.5)*dy) ===\n\n");
    printf("  T01b — Ordre 32→64  ≥ 1.5 : %s (ord = %.3f)\n",
           t01b_pass ? "PASS" : "FAIL",
           richardson_order(results[PROTO_C][0].err.L2, results[PROTO_C][1].err.L2));
    printf("  T02b — Ordre 64→128 ≥ 1.5 : %s (ord = %.3f)\n",
           t02b_pass ? "PASS" : "FAIL",
           richardson_order(results[PROTO_C][1].err.L2, results[PROTO_C][2].err.L2));
    printf("  T03b — L2 strictement décroissant : %s\n",
           t03b_pass ? "PASS" : "FAIL");
    printf("  T04b — Linf_128 < 0.05           : %s (Linf=%.6f)\n",
           t04b_pass ? "PASS" : "FAIL",
           results[PROTO_C][2].err.Linf);
    printf("  T05b — u_max_128 ≤ 1.0+1e-6      : %s (u_max=%.6f)\n",
           t05b_pass ? "PASS" : "FAIL",
           results[PROTO_C][2].err.u_max);
    printf("  T06b — u_min_128 ≥ 0.0-1e-6      : %s (u_min=%.6f)\n",
           t06b_pass ? "PASS" : "FAIL",
           results[PROTO_C][2].err.u_min);

    /* ── Limites documentées ── */
    printf("\n=== LIMITES HONNÊTES ===\n\n");
    printf("  - set_couette_periodic_bc() ré-appliqué après chaque ns_solver_step()\n");
    printf("    car step() appelle set_lid_bc() en interne. Une architecture propre\n");
    printf("    fournirait un callback de CL — noté OPEN.\n");
    printf("  - Couette plan : advection inactive (u·∇u = 0). Pour valider l'advection\n");
    printf("    non linéaire, une MMS avec terme source est requise (OPEN).\n");
    printf("  - CERTIFIED_100=false | unique_human_proven=false\n");

    /* ── Verdict global ── */
    int all_pass = t01b_pass && t02b_pass && t03b_pass &&
                   t04b_pass && t05b_pass && t06b_pass;

    printf("\n[VERDICT] RICHARDSON-PROTOCOL-003b : %s\n",
           all_pass ? "PASS — ordre spatial démontré, CL périodiques, BUG-2 corrigé"
                    : "FAIL honnête — voir T01b..T06b");
    printf("[NOTE] T01b=%s T02b=%s T03b=%s T04b=%s T05b=%s T06b=%s\n",
           t01b_pass ? "PASS" : "FAIL",
           t02b_pass ? "PASS" : "FAIL",
           t03b_pass ? "PASS" : "FAIL",
           t04b_pass ? "PASS" : "FAIL",
           t05b_pass ? "PASS" : "FAIL",
           t06b_pass ? "PASS" : "FAIL");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    forensic_logger_destroy();
    return all_pass ? 0 : 1;
}
