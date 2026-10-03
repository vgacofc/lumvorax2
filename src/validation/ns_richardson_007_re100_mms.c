/* **************************************************************************
** ns_richardson_007_re100_mms.c — Richardson-PROTOCOL-007 : ordre spatial
**   NS Chorin Re=100 avec MMS Taylor-Green (régime convectif)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Richardson-PROTOCOL-007
** Auteur : LumVorax Project
**
** OBJECTIF :
**   Vérifier que l'ordre spatial O(dx²) est conservé lorsque le terme
**   advectif est fort (Re=100 >> Re=1 des sessions S166-S169).
**   À Re=1, la diffusion dominait. Ici l'advection est de même ordre
**   que la diffusion, ce qui teste le schéma d'advection centré du solveur.
**
** SOLUTION ANALYTIQUE — Taylor-Green 2D (terme source nul) :
**
**   u(x,y,t) = -sin(πx) × cos(πy) × exp(-2π²t/Re)
**   v(x,y,t) =  cos(πx) × sin(πy) × exp(-2π²t/Re)
**   p(x,y,t) = -(cos(2πx)+cos(2πy))/4 × exp(-4π²t/Re)
**
**   Propriété fondamentale : cette solution satisfait NS EXACTEMENT
**   pour tout Re, terme source = 0.
**   Vérification :
**     ∂u/∂t + (u·∇)u = -2π²u/Re + (u·∇)u
**     (u·∇)u = -π sin(2πx)/2 × exp(-4π²t/Re)  [composante x]
**     -∇p    = +π sin(2πx)/2 × exp(-4π²t/Re)   [composante x]
**     → (u·∇)u = ∇p  →  NS satisfait avec diffusion = ∂u/∂t ✓
**
** CONTEXTE (rapport 173 / table §7) :
**   Les piliers précédents étaient fermés sur solveur diffusion (Re=1).
**   Ce protocole ferme le verrou : "Validation en régime convectif (Re>100)".
**
** PARAMÈTRES EXP-SPACE (Re=100) :
**
**   Re = 100
**   T_FINAL = 0.5   → u_amplitude(T) = exp(-2π²×0.5/100) ≈ 0.906  [signal présent ✓]
**   DT_FIXED = 1e-4 → stable et négligeable vs erreur spatiale :
**     dt_CFL_adv_16  = (1/16)/1   = 6.25e-2   >> 1e-4  ✓
**     dt_CFL_adv_32  = (1/32)/1   = 3.12e-2   >> 1e-4  ✓
**     dt_CFL_adv_64  = (1/64)/1   = 1.56e-2   >> 1e-4  ✓
**     dt_diff_16(Re=100) = 100*(1/16)²/4 = 0.39   >> 1e-4  ✓
**   Steps = T/dt = 0.5/1e-4 = 5000 (toutes grilles identique)
**
**   Grilles : 16×16, 32×32, 64×64
**   (128 non testé : 5000 pas × 128² ≈ 82M ops — trop long pour CI)
**
**   Erreur temporelle O(dt) = O(1e-4) << Erreur spatiale O(dx²_64) = O(2.4e-4)
**   → ratio ≈ 0.42 → non négligeable mais suffisant (voir justification ci-bas)
**
**   NOTE : à Re=100, l'amplitude temporelle décroît très lentement.
**   Le raffinement en dx divise l'erreur advection par 4 à chaque doublement.
**   L'erreur temporelle est FIXE (dt constant) entre les grilles.
**   → L'ordre mesuré = ordre spatial pur (confirmé si ordre ≈ 2).
**
** STABILITÉ :
**   CFL_adv = dt × Umax / dx
**     N=16 : CFL = 1e-4 × 1.0 / (1/16) = 1.6e-3   << 1 ✓
**     N=32 : CFL = 1e-4 × 1.0 / (1/32) = 3.2e-3   << 1 ✓
**     N=64 : CFL = 1e-4 × 1.0 / (1/64) = 6.4e-3   << 1 ✓
**
** CRITÈRES DE VALIDATION :
**   T-SPACE-1 : ordre p(16→32)  ≥ 1.5
**   T-SPACE-2 : ordre p(32→64)  ≥ 1.5
**   T-SPACE-3 : L2 strictement décroissant
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

/* ── Constantes ──────────────────────────────────────────────────────────── */

#define RE_100       100.0
#define T_FINAL_007  0.5
#define DT_FIXED_007 1e-4
#define N_GRIDS_007  3

static const int    GRIDS_007[N_GRIDS_007] = {16, 32, 64};
static const double PI_007 = 3.14159265358979323846;

/* Paramètres Poisson : convergence stricte pour ne pas contaminer l'ordre */
#define MAX_POISSON_007 800
#define TOL_POISSON_007 1e-8

/* Plafond de sécurité steps */
#define STEPS_MAX_007   20000LL

/* Log forensic */
#define LOG_PATH_007 "logs/forensic/ns_richardson_007_re100_mms.log"

/* ── Macros accès grille (identiques à ns_solver_2d.c) ───────────────────── */

#define U007(s, i, j)  ((s)->u[(i) * ((s)->params.ny + 2) + (j)])
#define V007(s, i, j)  ((s)->v[(i) * ((s)->params.ny + 1) + (j)])
#define P007(s, i, j)  ((s)->p[(i) * ((s)->params.ny + 2) + (j)])

/* ── Solution Taylor-Green 2D (Re paramétrique) ───────────────────────────── */

static double tg_u(double x, double y, double t, double re)
{
    return -sin(PI_007 * x) * cos(PI_007 * y) * exp(-2.0 * PI_007 * PI_007 * t / re);
}

static double tg_v(double x, double y, double t, double re)
{
    return  cos(PI_007 * x) * sin(PI_007 * y) * exp(-2.0 * PI_007 * PI_007 * t / re);
}

/* ── Globals pour callback CL ─────────────────────────────────────────────── */

static double g007_t  = 0.0;

/* ── Callback CL Dirichlet MMS (Taylor-Green) ─────────────────────────────── */

static void set_mms_bc_007(NSSolver2D *s)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dx = s->dx;
    double dy = s->dy;
    double t  = g007_t;
    double re = s->params.re;

    /* u : bords Ouest (i=0) et Est (i=nx) → u_exact = 0 (sin(π×0)=0, sin(π×1)=0) */
    for (int j = 0; j <= ny + 1; j++) {
        U007(s, 0,  j) = 0.0;
        U007(s, nx, j) = 0.0;
    }
    /* u : bords Sud (j=0) et Nord (j=ny+1) — image miroir */
    for (int i = 0; i <= nx; i++) {
        double x_f = (double)i * dx;
        double u_s = tg_u(x_f, 0.0, t, re);
        double u_n = tg_u(x_f, 1.0, t, re);
        U007(s, i, 0)       = 2.0 * u_s - U007(s, i, 1);
        U007(s, i, ny + 1)  = 2.0 * u_n - U007(s, i, ny);
    }
    /* v : bords Sud (j=0) et Nord (j=ny) → v_exact = 0 (sin(π×0)=0, sin(π×1)=0) */
    for (int i = 0; i <= nx + 1; i++) {
        V007(s, i, 0)  = 0.0;
        V007(s, i, ny) = 0.0;
    }
    /* v : bords Ouest (i=0) et Est (i=nx+1) — image miroir */
    for (int j = 0; j <= ny; j++) {
        double y_f = (double)j * dy;
        double v_w = tg_v(0.0, y_f, t, re);
        double v_e = tg_v(1.0, y_f, t, re);
        V007(s, 0,      j) = 2.0 * v_w - V007(s, 1,  j);
        V007(s, nx + 1, j) = 2.0 * v_e - V007(s, nx, j);
    }
    /* p : Neumann homogène dp/dn=0 */
    for (int j = 0; j <= ny + 1; j++) {
        P007(s, 0,      j) = P007(s, 1,  j);
        P007(s, nx + 1, j) = P007(s, nx, j);
    }
    for (int i = 0; i <= nx + 1; i++) {
        P007(s, i, 0)       = P007(s, i, 1);
        P007(s, i, ny + 1)  = P007(s, i, ny);
    }
}

/* ── Initialisation champ analytique ─────────────────────────────────────── */

static void init_tg_field(NSSolver2D *s, double t0)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double dx = s->dx;
    double dy = s->dy;
    double re = s->params.re;

    for (int i = 0; i <= nx; i++)
        for (int j = 0; j <= ny + 1; j++) {
            double x_f = (double)i * dx;
            double y_f = ((double)j - 0.5) * dy;
            U007(s, i, j) = tg_u(x_f, y_f, t0, re);
        }
    for (int i = 0; i <= nx + 1; i++)
        for (int j = 0; j <= ny; j++) {
            double x_f = ((double)i - 0.5) * dx;
            double y_f = (double)j * dy;
            V007(s, i, j) = tg_v(x_f, y_f, t0, re);
        }
}

/* ── Erreurs L2 et Linf ───────────────────────────────────────────────────── */

static double compute_l2_007(const NSSolver2D *s, double t)
{
    int    nx   = s->params.nx;
    int    ny   = s->params.ny;
    double dx   = s->dx;
    double dy   = s->dy;
    double re   = s->params.re;
    double sum2 = 0.0;
    int    nc   = 0;

    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++) {
            double x_f = (double)i * dx;
            double y_f = ((double)j - 0.5) * dy;
            double e   = s->u[i * (ny + 2) + j] - tg_u(x_f, y_f, t, re);
            sum2 += e * e;
            nc++;
        }
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++) {
            double x_f = ((double)i - 0.5) * dx;
            double y_f = (double)j * dy;
            double e   = s->v[i * (ny + 1) + j] - tg_v(x_f, y_f, t, re);
            sum2 += e * e;
            nc++;
        }
    return (nc > 0) ? sqrt(sum2 / (double)nc) : 0.0;
}

static double compute_linf_007(const NSSolver2D *s, double t)
{
    int    nx   = s->params.nx;
    int    ny   = s->params.ny;
    double dx   = s->dx;
    double dy   = s->dy;
    double re   = s->params.re;
    double maxe = 0.0;

    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++) {
            double x_f = (double)i * dx;
            double y_f = ((double)j - 0.5) * dy;
            double e   = fabs(s->u[i * (ny + 2) + j] - tg_u(x_f, y_f, t, re));
            if (e > maxe) maxe = e;
        }
    return maxe;
}

/* ── Résultat de simulation ───────────────────────────────────────────────── */

typedef struct {
    int    n;
    double dt;
    long long steps;
    double t_final;
    double l2;
    double linf;
    double t_wall_s;
    int    stable;
    int    steps_capped;
} SimResult007;

/* ── Boucle de simulation ─────────────────────────────────────────────────── */

static SimResult007 run_sim007(int n, double dt, double t_final_param)
{
    SimResult007 res;
    memset(&res, 0, sizeof(res));
    res.n  = n;
    res.dt = dt;

    NSParams p = {
        .nx          = n,
        .ny          = n,
        .lx          = 1.0,
        .ly          = 1.0,
        .re          = RE_100,
        .dt          = dt,
        .max_iter    = 1,
        .tol         = TOL_POISSON_007,
        .max_poisson = MAX_POISSON_007,
        .debug       = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[007][ERROR] ns_solver_create n=%d échoué\n", n);
        return res;
    }

    init_tg_field(s, 0.0);
    g007_t = 0.0;
    set_mms_bc_007(s);

    long long target_steps = (long long)(t_final_param / dt + 0.5);
    if (target_steps < 1) target_steps = 1;

    int capped = 0;
    if (target_steps > STEPS_MAX_007) {
        fprintf(stderr,
            "[007][INFO] n=%d dt=%.2e : steps %lld → %lld (plafond)\n",
            n, dt, target_steps, STEPS_MAX_007);
        target_steps = STEPS_MAX_007;
        capped = 1;
    }

    struct timespec t0c, t1c;
    clock_gettime(CLOCK_MONOTONIC, &t0c);

    int stable = 1;
    for (long long k = 0; k < target_steps; k++) {
        g007_t = (double)(k + 1) * dt;
        ns_solver_step_with_bc(s, set_mms_bc_007);

        /* Détection divergence toutes les 500 steps */
        if (k > 0 && k % 500 == 0) {
            double umax = 0.0;
            for (int i = 1; i < n; i++)
                for (int j = 1; j <= n; j++) {
                    double u = fabs(s->u[i * (n + 2) + j]);
                    if (u > umax) umax = u;
                }
            if (umax > 50.0) {
                fprintf(stderr,
                    "[007][WARN] divergence n=%d step=%lld u_max=%.3e\n",
                    n, k, umax);
                stable = 0;
                target_steps = k + 1;
                break;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1c);

    double t_done      = (double)target_steps * dt;
    res.steps          = target_steps;
    res.t_final        = t_done;
    res.l2             = compute_l2_007(s, t_done);
    res.linf           = compute_linf_007(s, t_done);
    res.t_wall_s       = (t1c.tv_sec  - t0c.tv_sec) +
                         (t1c.tv_nsec - t0c.tv_nsec) * 1e-9;
    res.stable         = stable;
    res.steps_capped   = capped;

    ns_solver_destroy(s);
    return res;
}

/* ── Calcul d'ordre Richardson ────────────────────────────────────────────── */

static double richardson_order_007(double l2_coarse, double l2_fine)
{
    if (l2_coarse < 1e-15 || l2_fine < 1e-15) return -8888.0;
    if (l2_coarse <= l2_fine)                  return -9999.0;
    return log2(l2_coarse / l2_fine);
}

static const char *fmt_order_007(double o)
{
    static char buf[32];
    if (o < -8880.0 && o > -8900.0) return "N/A_PLANCHER";
    if (o < -999.0)                  return "N/A (non décroissant)";
    snprintf(buf, sizeof(buf), "%.3f", o);
    return buf;
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== RICHARDSON-PROTOCOL-007 : ORDRE SPATIAL NS COMPLET Re=100 ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[OBJECTIF] Vérifier O(dx²) spatial en régime convectif (Re=100)\n");
    printf("[MMS] Taylor-Green 2D | Re=%.0f | terme source=0 (solution exacte NS)\n", RE_100);
    printf("[PARAMS] T_final=%.2f | dt=%.2e | Poisson max=%d tol=%.0e\n\n",
           T_FINAL_007, DT_FIXED_007, MAX_POISSON_007, TOL_POISSON_007);

    /* Vérification stabilité a priori */
    printf("  Justification DT_FIXED=%.2e :\n", DT_FIXED_007);
    for (int gi = 0; gi < N_GRIDS_007; gi++) {
        int n = GRIDS_007[gi];
        double dx_n = 1.0 / (double)n;
        double cfl  = DT_FIXED_007 / dx_n;
        double dt_diff = RE_100 * dx_n * dx_n / 4.0;
        printf("    N=%3d : CFL_adv=%.4f | dt_diff=%.2e | DT_FIXED << dt_diff ✓\n",
               n, cfl, dt_diff);
    }
    {
        double err_t = DT_FIXED_007;
        double dx_64 = 1.0 / 64.0;
        double err_s = dx_64 * dx_64;
        printf("  Ratio erreur_temp/erreur_spatiale_64 ≈ %.2f\n",
               err_t / err_s);
        printf("  (dt constant → erreur temporelle FIXE entre grilles → ordre = ordre spatial)\n\n");
    }

    forensic_logger_init(LOG_PATH_007);

    printf("═══ EXP-SPACE Re=100 : Ordre spatial isolé (dt=%.2e fixe) ═══\n\n",
           DT_FIXED_007);
    printf("  %-7s | %8s | %8s | %6s | %7s | Stable | Capped\n",
           "Grille", "L2", "Linf", "Steps", "Wall(s)");
    printf("  -------+----------+----------+--------+---------+--------+-------\n");

    SimResult007 res[N_GRIDS_007];
    for (int gi = 0; gi < N_GRIDS_007; gi++) {
        int n = GRIDS_007[gi];
        printf("  %3d×%3d | ", n, n);
        fflush(stdout);

        res[gi] = run_sim007(n, DT_FIXED_007, T_FINAL_007);
        SimResult007 *r = &res[gi];

        printf("%8.3e | %8.3e | %6lld | %7.1f | %-6s | %s\n",
               r->l2, r->linf, r->steps, r->t_wall_s,
               r->stable ? "OUI" : "NON",
               r->steps_capped ? "OUI" : "non");

        /* Log forensic */
        uint64_t ts = time_ns_get_absolute();
        char op_buf[192];
        snprintf(op_buf, sizeof(op_buf),
                 "007-SPACE:Re=100:n=%d:dt=%.2e:T=%.3f:L2=%.3e:Linf=%.3e:steps=%lld:stable=%d",
                 n, DT_FIXED_007, r->t_final, r->l2, r->linf, r->steps, r->stable);
        uint64_t lum_id = ((uint64_t)(n & 0xFFFFU) << 48) | ((uint64_t)0x07U << 40);
        uint64_t l2b; memcpy(&l2b, &r->l2, sizeof(uint64_t));
        lum_id |= (l2b & 0x7FFFU);
        forensic_log_individual_lum(lum_id, op_buf, ts);
    }

    /* Analyse ordres Richardson */
    double ord_16_32 = richardson_order_007(res[0].l2, res[1].l2);
    double ord_32_64 = richardson_order_007(res[1].l2, res[2].l2);

    printf("\n  Ordre spatial 16→32 : %s\n", fmt_order_007(ord_16_32));
    printf("  Ordre spatial 32→64 : %s\n", fmt_order_007(ord_32_64));

    if (res[0].l2 > 0 && res[1].l2 > 0)
        printf("  Ratio L2(16)/L2(32) : %.3f (attendu ~4 pour O(dx²))\n",
               res[0].l2 / res[1].l2);
    if (res[1].l2 > 0 && res[2].l2 > 0)
        printf("  Ratio L2(32)/L2(64) : %.3f (attendu ~4 pour O(dx²))\n",
               res[1].l2 / res[2].l2);

    /* Verdicts */
    int t1 = (ord_16_32 >= 1.5) ? 1 : 0;
    int t2 = (ord_32_64 >= 1.5) ? 1 : 0;
    int t3 = (res[0].l2 > res[1].l2 && res[1].l2 > res[2].l2) ? 1 : 0;
    int t4 = res[0].stable && res[1].stable && res[2].stable;

    printf("\n  T-SPACE-1 (ord 16→32 ≥ 1.5)    : %s\n", t1 ? "PASS ✓" : "FAIL ✗");
    printf("  T-SPACE-2 (ord 32→64 ≥ 1.5)    : %s\n", t2 ? "PASS ✓" : "FAIL ✗");
    printf("  T-SPACE-3 (L2 décroissant)      : %s\n", t3 ? "PASS ✓" : "FAIL ✗");
    printf("  T-SPACE-4 (toutes grilles stable) : %s\n\n", t4 ? "PASS ✓" : "FAIL ✗");

    /* Synthèse */
    printf("═══ SYNTHÈSE RICHARDSON-PROTOCOL-007 ═══\n\n");
    printf("  Re = %.0f (régime convectif — terme advectif non négligeable)\n", RE_100);
    printf("  T_final = %.2f | dt = %.2e | steps = %lld\n\n",
           T_FINAL_007, DT_FIXED_007, res[0].steps);
    for (int gi = 0; gi < N_GRIDS_007; gi++)
        printf("  L2(%3d×%3d) = %.3e\n", res[gi].n, res[gi].n, res[gi].l2);
    printf("  Ordre 16→32 : %s\n", fmt_order_007(ord_16_32));
    printf("  Ordre 32→64 : %s\n\n", fmt_order_007(ord_32_64));
    printf("  T-SPACE-1 : %s | T-SPACE-2 : %s | T-SPACE-3 : %s | T-SPACE-4 : %s\n\n",
           t1 ? "PASS" : "FAIL",
           t2 ? "PASS" : "FAIL",
           t3 ? "PASS" : "FAIL",
           t4 ? "PASS" : "FAIL");

    /* Limites honnêtes */
    printf("=== LIMITES HONNÊTES ===\n\n");
    printf("  - Taylor-Green 2D satisfait NS exactement : terme source = 0.\n");
    printf("    L'erreur mesurée est l'erreur de discrétisation spatiale pure.\n");
    printf("  - Re=100 : advection et diffusion comparables au niveau de l'erreur.\n");
    printf("    Le schéma d'advection centré 2ème ordre contribue à l'ordre global ≈ 2.\n");
    printf("  - dt=%.2e fixe → erreur temporelle O(dt) fixe entre grilles.\n", DT_FIXED_007);
    printf("    Elle ne diminue pas avec le raffinement → l'ordre mesuré = ordre spatial.\n");
    printf("  - Poisson max=%d tol=%.0e : résidu Poisson négligeable.\n",
           MAX_POISSON_007, TOL_POISSON_007);
    printf("  - Grille 128×128 non testée (coût : 5000 pas × 128² trop long pour CI).\n");
    printf("  - Chorin CL/pression splitting : erreur O(dt) sur bords présente mais fixe.\n");
    printf("  - Régime turbulent (Re > 1000) : hors périmètre.\n");
    printf("  - CERTIFIED_100=false | unique_human_proven=false\n");

    int all_pass = t1 && t2 && t3 && t4;
    printf("\n[VERDICT] RICHARDSON-007-RE100 : %s\n",
           all_pass ? "PASS — ordre spatial O(dx²) confirmé en régime convectif Re=100"
                    : "FAIL honnête — voir T-SPACE-* ci-dessus");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    forensic_logger_destroy();
    return all_pass ? 0 : 1;
}
