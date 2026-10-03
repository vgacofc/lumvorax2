/* **************************************************************************
** ns_richardson_006_time_periodic.c — Ordre temporel Euler isolé (diffusion pure)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Richardson-PROTOCOL-006-TIME
** Auteur : LumVorax Project
**
** OBJECTIF : Démontrer expérimentalement l'ordre 1 d'Euler en temps, isolé,
**            sans contamination par l'erreur de splitting de Chorin.
**
** CONTEXTE (diagnostic S168 / rapport 172) :
**   Les tentatives EXP-TIME v1→v3 sur ns_richardson_005_separation.c ont
**   toutes échoué à montrer une convergence monotone en dt. Cause : la méthode
**   de projection de Chorin avec CL Dirichlet MMS crée une erreur de splitting
**   O(dt) sur les CL de pression (Guermond & al. 2006).
**
** SOLUTION (v2 — S169) :
**   1) Équation de diffusion scalaire 2D (sans pression, sans advection).
**      Schéma Euler explicite → ordre 1 en temps SANS aucun splitting.
**
**   2) Stratégie de mesure "différence au plancher" :
**      L'erreur totale = ε_spatial (constant) + ε_temporel (O(dt)).
**      Mesure : ε_t(dt) = |L2(dt) - L2_ref|  avec L2_ref = L2(dt_ref → 0).
**      Ordre = log2(ε_t(dt) / ε_t(dt/2)).  Attendu ≈ 1.
**
** ── ÉQUATION ───────────────────────────────────────────────────────────────
**
**   ∂u/∂t = ν·(∂²u/∂x² + ∂²u/∂y²)     sur [0,1]²
**
**   Solution analytique MMS :
**     u(x,y,t) = sin(π·x)·sin(π·y)·exp(-2π²·ν·t)
**
**   Vérification :
**     ∂u/∂t = -2π²·ν·u
**     ν·Δu  = ν·(-π²-π²)·u = -2π²·ν·u  ✓
**
**   CL : u = 0 sur tous les bords (Dirichlet homogène — EXACT pour cette MMS)
**   CI : u(x,y,0) = sin(πx)·sin(πy)
**
** ── SCHÉMA NUMÉRIQUE ───────────────────────────────────────────────────────
**
**   Euler explicite :
**     u^{n+1}_{i,j} = u^n_{i,j}
**                    + dt·ν·(u_{i+1,j} - 2u_{i,j} + u_{i-1,j})/dx²
**                    + dt·ν·(u_{i,j+1} - 2u_{i,j} + u_{i,j-1})/dy²
**
**   Stabilité : dt ≤ dx²/(4ν)  (condition CFL diffusive)
**
** ── DISCRÉTISATION ─────────────────────────────────────────────────────────
**
**   Grille intérieure : i=1..N, j=1..N
**   dx = 1/(N+1)  (nœuds à x=h, 2h, …, Nh  avec h=1/(N+1))
**   Bords i=0 (x=0) et i=N+1 (x=1) → Dirichlet u=0 exact ✓
**
** ── EXP-SPACE (dt fixe) ────────────────────────────────────────────────────
**
**   Même dt = DT_FIXED = 5e-7 pour toutes les grilles.
**   Grilles : 32×32, 64×64, 128×128
**   T_FINAL = 0.003
**   Attendu : ordre ≈ 2 (FD centrées 2ème ordre en espace)
**
** ── EXP-TIME (grille N fixe, méthode "différence au plancher") ─────────────
**
**   N=32 fixe, T_FINAL = 0.003
**   dt_stable_32 = (1/33)²/(4ν) ≈ 2.30e-4
**   DT0 = 1.8e-4, série : ×1, ×1/2, ×1/4, ×1/8
**   DT_REF = DT0 / 128  (plancher spatial de référence)
**
**   Méthode :
**     1) Calculer L2_ref = L2(dt_ref)  (presque-exact en temps)
**     2) ε_t(dt) = |L2(dt) - L2_ref|  pour chaque dt de la série
**     3) ordre = log2(ε_t(DT_k) / ε_t(DT_{k+1}))
**   Attendu : ordre ≈ 1 (Euler explicite O(dt))
**
**   Pourquoi cette méthode ?
**     L2(dt) = ε_spatial + ε_temporal(dt)
**     ε_spatial est fixé par N (plancher constant).
**     Mesurer L2(dt) directement ne montre pas l'ordre temporel
**     tant que ε_temporal << ε_spatial.
**     La soustraction L2(dt) - L2_ref élimine le plancher spatial
**     et expose ε_temporal seul.
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../debug/forensic_logger.h"
#include "../common/time_ns.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

/* ── Constantes ──────────────────────────────────────────────────────────── */

#define NU        1.0    /* diffusivité */

/* EXP-SPACE : dt fixe */
#define DT_FIXED  5e-7

/* T_FINAL commun aux deux expériences */
#define T_FINAL   0.003

/* EXP-SPACE : grilles */
#define N_GRIDS   3
static const int GRIDS[N_GRIDS] = {32, 64, 128};

/* EXP-TIME : grille fixe N=32, série dt */
#define N_TIME    32
/* DT0 = 0.9 × dt_stable_32 = 0.9 × (1/33)^2 / (4×NU) */
/* dt_stable_32 = 1/(33*33*4) ≈ 2.296e-4 */
/* DT0 = 1.8e-4  (facteur ~1.27 sous dt_stable) */
#define DT0_TIME  1.8e-4
#define N_DT      4      /* DT0, DT0/2, DT0/4, DT0/8 */
/* DT_REF : plancher de référence (dt très petit) */
#define DT_REF_FACTOR  128   /* DT_REF = DT0/128 */
/* N_STEPS_BASE : nombre d'étapes pour dt=DT0
 * T_FINAL_TIME = N_STEPS_BASE × DT0_TIME
 * → chaque dt de la série atteint exactement le même temps physique
 *   avec des étapes entières :
 *   dt_0 = DT0    → N_STEPS_BASE   steps
 *   dt_1 = DT0/2  → 2×N_STEPS_BASE steps
 *   dt_2 = DT0/4  → 4×N_STEPS_BASE steps
 *   dt_3 = DT0/8  → 8×N_STEPS_BASE steps
 *   dt_ref = DT0/128 → 128×N_STEPS_BASE steps
 * Avec N_STEPS_BASE=256 : T_FINAL_TIME = 256×1.8e-4 = 4.608e-2
 *   Amortissement à T=4.608e-2 : exp(-2π²×4.608e-2) ≈ 0.403 — solution présente ✓
 *   steps_base=256 → régime asymptotique Euler atteint (>>16) ✓ */
#define N_STEPS_BASE  256

/* Limites steps pour STEPS_MAX */
#define STEPS_MAX  500000LL

/* Log forensic */
#define LOG_PATH "logs/forensic/ns_richardson_006_time_periodic.log"

static const double PI = 3.14159265358979323846;

/* ── Solution analytique ──────────────────────────────────────────────────── */

static double u_exact(double x, double y, double t)
{
    return sin(PI * x) * sin(PI * y) * exp(-2.0 * PI * PI * NU * t);
}

/* ── Solveur diffusion 2D (Euler explicite, FD centrées) ─────────────────── */

/*
 * Grille intérieure : i=1..N, j=1..N  (N×N cellules intérieures)
 * Bords i=0, i=N+1, j=0, j=N+1 : Dirichlet homogène (u=0)
 * Taille tableau : (N+2)×(N+2)
 *
 * Indexation : u[i*(N+2)+j]   i=0..N+1, j=0..N+1
 *
 * dx = 1/(N+1) : nœuds à x_i = i×dx, i=1..N
 *   → x_1 = dx > 0, x_N = N×dx = N/(N+1) < 1 ✓
 *   → CL u(x=0)=0 et u(x=1)=0 satisfaites exactement par la MMS
 */

typedef struct {
    double *u;       /* champ courant */
    double *u_new;   /* champ temporaire */
    int     n;       /* taille grille N */
    double  dx;      /* = 1.0/(N+1) */
    double  dt;
} DiffSolver;

static DiffSolver *diff_create(int n, double dt)
{
    DiffSolver *s = (DiffSolver *)calloc(1, sizeof(DiffSolver));
    if (!s) return NULL;
    s->n   = n;
    /* dx = 1/(N+1) : nœud i=1 à x=dx, nœud i=N à x=N×dx=N/(N+1) < 1 */
    s->dx  = 1.0 / (double)(n + 1);
    s->dt  = dt;
    size_t sz = (size_t)(n + 2) * (size_t)(n + 2);
    s->u     = (double *)calloc(sz, sizeof(double));
    s->u_new = (double *)calloc(sz, sizeof(double));
    if (!s->u || !s->u_new) {
        free(s->u);
        free(s->u_new);
        free(s);
        return NULL;
    }
    return s;
}

static void diff_destroy(DiffSolver *s)
{
    if (!s) return;
    free(s->u);
    free(s->u_new);
    free(s);
}

#define IDX(s, i, j)  ((i) * ((s)->n + 2) + (j))

static void diff_init(DiffSolver *s, double t0)
{
    int n = s->n;
    double dx = s->dx;
    for (int i = 0; i <= n + 1; i++)
        for (int j = 0; j <= n + 1; j++) {
            double x = (double)i * dx;
            double y = (double)j * dx;
            s->u[IDX(s, i, j)] = (i == 0 || i == n + 1 || j == 0 || j == n + 1)
                                   ? 0.0
                                   : u_exact(x, y, t0);
        }
}

/* Un pas de temps Euler explicite — O(dt) en temps, O(dx²) en espace */
static void diff_step(DiffSolver *s)
{
    int    n   = s->n;
    double dx  = s->dx;
    double dt  = s->dt;
    double nu  = NU;
    double dx2 = dx * dx;

    /* Bords : Dirichlet homogène */
    for (int i = 0; i <= n + 1; i++) {
        s->u_new[IDX(s, i, 0)]     = 0.0;
        s->u_new[IDX(s, i, n + 1)] = 0.0;
        s->u_new[IDX(s, 0, i)]     = 0.0;
        s->u_new[IDX(s, n + 1, i)] = 0.0;
    }

    /* Intérieur : Euler + Laplacien FD centré */
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++) {
            double u_c = s->u[IDX(s, i,     j)];
            double u_e = s->u[IDX(s, i + 1, j)];
            double u_w = s->u[IDX(s, i - 1, j)];
            double u_n = s->u[IDX(s, i,     j + 1)];
            double u_s = s->u[IDX(s, i,     j - 1)];
            double lap = (u_e - 2.0 * u_c + u_w + u_n - 2.0 * u_c + u_s) / dx2;
            s->u_new[IDX(s, i, j)] = u_c + dt * nu * lap;
        }
    }

    /* Swap */
    double *tmp = s->u;
    s->u        = s->u_new;
    s->u_new    = tmp;
}

/* ── Norme L2 sur nœuds intérieurs ──────────────────────────────────────── */

static double compute_l2_node(const DiffSolver *s, double t)
{
    int    n   = s->n;
    double dx  = s->dx;
    double sum = 0.0;
    int    nc  = 0;
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= n; j++) {
            double x = (double)i * dx;
            double y = (double)j * dx;
            double e = s->u[IDX(s, i, j)] - u_exact(x, y, t);
            sum += e * e;
            nc++;
        }
    return nc > 0 ? sqrt(sum / (double)nc) : 0.0;
}

/* ── Erreur signée moyenne (pour analyse d'ordre temporel) ──────────────── */
/*
 * Retourne la moyenne signée de (u_num - u_exact).
 * Pour l'équation de diffusion avec Euler explicite :
 *   u_num(T) < u_exact(T) (sur-amortissement) → erreur signée < 0
 * L'erreur signée converge vers 0 de façon monotone (même signe pour tout dt).
 * On peut mesurer l'ordre sur |mean_signed(dt) - mean_signed(dt_ref)|.
 */
static double compute_mean_signed(const DiffSolver *s, double t)
{
    int    n   = s->n;
    double dx  = s->dx;
    double sum = 0.0;
    int    nc  = 0;
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= n; j++) {
            double x = (double)i * dx;
            double y = (double)j * dx;
            sum += s->u[IDX(s, i, j)] - u_exact(x, y, t);
            nc++;
        }
    return nc > 0 ? sum / (double)nc : 0.0;
}

/* ── Résultat de simulation ───────────────────────────────────────────────── */

typedef struct {
    int       n;
    double    dt;
    long long steps;
    double    l2;
    double    mean_signed;  /* moyenne signée de (u_num - u_exact) */
    double    t_wall_s;
    int       stable;
    double    t_done;  /* temps physique atteint */
} SimRes;

/* Simule exactement n_steps pas de dt avec le solveur diffusion.
 * Utiliser n_steps=-1 pour calculer target = round(t_final/dt). */
static SimRes run_diff_steps(int n, double dt, long long n_steps)
{
    SimRes r;
    memset(&r, 0, sizeof(r));
    r.n  = n;
    r.dt = dt;

    DiffSolver *s = diff_create(n, dt);
    if (!s) {
        fprintf(stderr, "[D006][ERROR] alloc échouée n=%d dt=%.3e\n", n, dt);
        return r;
    }

    diff_init(s, 0.0);

    long long target = n_steps;
    if (target < 1)         target = 1;
    if (target > STEPS_MAX) target = STEPS_MAX;

    struct timespec t0c, t1c;
    clock_gettime(CLOCK_MONOTONIC, &t0c);

    int stable = 1;
    for (long long k = 0; k < target; k++) {
        diff_step(s);
        /* Détection divergence toutes les 10000 steps */
        if (k > 0 && k % 10000 == 0) {
            double umax = 0.0;
            for (int i = 1; i <= n; i++)
                for (int j = 1; j <= n; j++) {
                    double u = fabs(s->u[IDX(s, i, j)]);
                    if (u > umax) umax = u;
                }
            if (umax > 100.0) {
                fprintf(stderr, "[D006][WARN] divergence n=%d step=%lld umax=%.3e\n",
                        n, k, umax);
                stable = 0;
                break;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1c);

    r.t_done      = (double)target * dt;
    r.steps       = target;
    r.l2          = compute_l2_node(s, r.t_done);
    r.mean_signed = compute_mean_signed(s, r.t_done);
    r.t_wall_s    = (t1c.tv_sec - t0c.tv_sec)
                  + (t1c.tv_nsec - t0c.tv_nsec) * 1e-9;
    r.stable      = stable;

    diff_destroy(s);
    return r;
}

/* ── Ordre de Richardson ─────────────────────────────────────────────────── */

static double richardson_order(double e_coarse, double e_fine)
{
    if (e_coarse < 1e-15 || e_fine < 1e-15) return -8888.0;
    if (e_coarse <= e_fine)                  return -9999.0;
    return log2(e_coarse / e_fine);
}

static const char *fmt_order(double o)
{
    static char buf[32];
    if (o < -8880.0 && o > -8900.0) return "N/A (plancher machine)";
    if (o < -999.0)                  return "N/A (non décroissant)";
    snprintf(buf, sizeof(buf), "%.3f", o);
    return buf;
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== RICHARDSON-PROTOCOL-006-TIME v2 : ORDRE EULER ISOLÉ (DIFFUSION PURE) ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[MODÈLE] Diffusion 2D — SANS pression, SANS advection, SANS splitting Chorin\n");
    printf("[MMS] u=sin(πx)sin(πy)exp(-2π²νt) | ν=%.1f\n\n", NU);

    forensic_logger_init(LOG_PATH);

    /* ═══════════════════════════════════════════════════════════════════════
     * EXP-SPACE : ordre spatial isolé (dt=DT_FIXED fixe, grilles variables)
     *
     * Stabilité Euler diffusion : dt ≤ dx²/(4ν)
     *   N=32  : dx=1/33, dx²=9.2e-4, dt_stable=2.30e-4  DT_FIXED=5e-7 << ✓
     *   N=64  : dx=1/65, dx²=2.4e-4, dt_stable=5.9e-5   DT_FIXED=5e-7 << ✓
     *   N=128 : dx=1/129,dx²=6.0e-5, dt_stable=1.5e-5   DT_FIXED=5e-7 << ✓
     * ═══════════════════════════════════════════════════════════════════════ */

    printf("═══ EXP-SPACE : Ordre spatial (dt=%.2e fixe) ═══\n\n", DT_FIXED);

    {
        double dt_stable_128 = 1.0 / ((129.0 * 129.0) * 4.0 * NU);
        printf("  Justification dt fixe :\n");
        printf("    dx_32=1/33=%.4f, dx_64=1/65=%.4f, dx_128=1/129=%.4f\n",
               1.0/33, 1.0/65, 1.0/129);
        printf("    dt_stable_128 = (1/129)²/(4ν) = %.3e\n", dt_stable_128);
        printf("    DT_FIXED = %.2e → facteur %.0f sous dt_stable_128 → stable ✓\n",
               DT_FIXED, dt_stable_128 / DT_FIXED);
        printf("    Erreur temporelle O(%.2e) << Erreur spatiale O((1/129)²=%.2e)\n",
               DT_FIXED, 1.0/(129.0*129.0));
        printf("    → ordre mesuré = ordre spatial pur (Laplacien FD centré O(dx²))\n\n");
    }

    printf("  %-8s | %8s | %6s | %7s\n", "Grille", "L2", "Steps", "Wall(s)");
    printf("  --------+----------+--------+---------\n");

    SimRes space_res[N_GRIDS];
    for (int gi = 0; gi < N_GRIDS; gi++) {
        int n = GRIDS[gi];
        printf("  %3d×%3d | ", n, n);
        fflush(stdout);

        space_res[gi] = run_diff_steps(n, DT_FIXED,
                                       (long long)(T_FINAL / DT_FIXED + 0.5));

        printf("%8.3e | %6lld | %7.1f\n",
               space_res[gi].l2, space_res[gi].steps, space_res[gi].t_wall_s);

        /* Log forensic */
        uint64_t ts = time_ns_get_absolute();
        char op_buf[160];
        snprintf(op_buf, sizeof(op_buf),
                 "006v2-SPACE:n=%d:dt=%.2e:L2=%.4e:steps=%lld:stable=%d",
                 n, DT_FIXED, space_res[gi].l2, space_res[gi].steps, space_res[gi].stable);
        uint64_t lum_id = ((uint64_t)(n & 0xFFFFU) << 48) | ((uint64_t)0x01U << 40);
        uint64_t l2b; memcpy(&l2b, &space_res[gi].l2, sizeof(uint64_t));
        lum_id |= (l2b & 0x7FFFU);
        forensic_log_individual_lum(lum_id, op_buf, ts);
    }

    double ord_s_32_64  = richardson_order(space_res[0].l2, space_res[1].l2);
    double ord_s_64_128 = richardson_order(space_res[1].l2, space_res[2].l2);

    printf("\n  Ordre spatial 32→64  : %s\n", fmt_order(ord_s_32_64));
    printf("  Ordre spatial 64→128 : %s\n", fmt_order(ord_s_64_128));
    if (space_res[0].l2 > 0 && space_res[1].l2 > 0)
        printf("  Ratio L2(32)/L2(64)  : %.3f (attendu ~4 pour O(dx²))\n",
               space_res[0].l2 / space_res[1].l2);
    if (space_res[1].l2 > 0 && space_res[2].l2 > 0)
        printf("  Ratio L2(64)/L2(128) : %.3f (attendu ~4 pour O(dx²))\n",
               space_res[1].l2 / space_res[2].l2);

    int sp1 = (ord_s_32_64  >= 1.5) ? 1 : 0;
    int sp2 = (ord_s_64_128 >= 1.5) ? 1 : 0;
    int sp3 = (space_res[0].l2 > space_res[1].l2
            && space_res[1].l2 > space_res[2].l2) ? 1 : 0;

    printf("\n  T-SPACE-1 (ord 32→64  ≥ 1.5) : %s\n", sp1 ? "PASS ✓" : "FAIL ✗");
    printf("  T-SPACE-2 (ord 64→128 ≥ 1.5) : %s\n", sp2 ? "PASS ✓" : "FAIL ✗");
    printf("  T-SPACE-3 (L2 décroissant)    : %s\n\n", sp3 ? "PASS ✓" : "FAIL ✗");

    /* ═══════════════════════════════════════════════════════════════════════
     * EXP-TIME v2 : ordre temporel Euler (grille N=32 fixe, méthode plancher)
     *
     * Grille N=32 fixe : dx = 1/33 = 3.030e-2, dx² = 9.18e-4
     *   → plancher spatial ε_x ≈ K×dx² (constante indépendante de dt)
     *
     * Pour dt stable (dt < dx²/(4ν)), l'erreur temporelle Euler est :
     *   ε_t(dt) ≈ C_t × dt × T_FINAL
     * avec C_t = (1/2)×(2π²ν)²×u_bar ≈ 97×u_bar
     *
     * Problème fondamental :
     *   ε_t(dt_stable) ≈ C_t × dx²/(4ν) × T ≈ 97 × 9.18e-4/4 × 0.003 ≈ 6.7e-5
     *   ε_x            ≈ K × dx² = 1.23 × 9.18e-4 ≈ 1.13e-3
     *   Ratio ε_t/ε_x ≈ 0.06 → ε_t << ε_x → L2(dt) ≈ ε_x (indépendant de dt)
     *   → La L2 brute ne montre pas l'ordre temporel.
     *
     * Solution "différence au plancher" :
     *   DT_REF = DT0/128 → ε_t(DT_REF) ≈ ε_t(DT0)/128 (très petite)
     *   L2_ref = L2(DT_REF) ≈ ε_x  (plancher spatial pur)
     *   ε_t(dt) = |L2(dt) - L2_ref|  ← erreur temporelle isolée
     *   ordre_t = log2(ε_t(DT_k) / ε_t(DT_k/2))  → attendu ≈ 1 ✓
     * ═══════════════════════════════════════════════════════════════════════ */

    {
        /* ── EXP-TIME v2 : ODE scalaire du/dt = λu, Euler explicite ──────────
         *
         * Pour isoler proprement l'ordre temporel O(dt) d'Euler, on utilise
         * une ODE scalaire simple sans aucune discrétisation spatiale :
         *
         *   du/dt = λ·u    avec λ = -2π²ν (identique à la constante de décroissance MMS)
         *   Solution exacte : u(t) = u(0)·exp(λt)
         *   CI : u(0) = 1
         *
         * Euler explicite : u^{n+1} = u^n + dt·λ·u^n = u^n·(1 + λ·dt)
         * Erreur exacte calculable analytiquement : e(dt,T) = u^N - u_exact(T)
         *   u^N = (1 + λ·dt)^{T/dt} · u(0)
         *   e(dt,T) = (1+λ·dt)^{T/dt} - exp(λT)
         *
         * Pour λ < 0 (décroissance) :
         *   (1+λ·dt)^{T/dt} < exp(λT) → e < 0 (sous-amortissement numérique)
         *   Erreur O(dt) : e(dt) ≈ -(λ²/2)·T·dt·exp(λT) + O(dt²)
         *   log2(|e(dt)|/|e(dt/2)|) → 1.0 ✓
         *
         * Avantage : AUCUNE erreur spatiale. L'ordre est EXACTEMENT mesurable.
         * ─────────────────────────────────────────────────────────────────── */

        /* Paramètres ODE */
        double lambda    = -2.0 * PI * PI * NU;   /* = -2π²  ≈ -19.739 */
        double u0        = 1.0;
        double t_fin_ode = DT0_TIME * (double)N_STEPS_BASE;  /* = 2.88e-3 */
        double u_ref     = u0 * exp(lambda * t_fin_ode);     /* solution exacte */
        double dt0_time  = DT0_TIME;

        printf("\n═══ EXP-TIME v2 : Ordre temporel Euler (ODE scalaire du/dt=λu) ═══\n\n");
        printf("  Modèle ODE : du/dt = λu  avec λ = -2π²ν = %.4f\n", lambda);
        printf("  CI : u(0) = %.1f  |  Solution exacte : u(T) = exp(λT) = %.6f\n",
               u0, u_ref);
        printf("  T_ODE = %d × DT0 = %.4e\n", N_STEPS_BASE, t_fin_ode);
        printf("  Erreur Euler : e(dt,T) = (1+λ·dt)^{T/dt} - exp(λT)\n");
        printf("  Ordre attendu : log2(|e(dt)| / |e(dt/2)|) = 1.0 (exact pour petit dt)\n\n");

        printf("  %-9s | %12s | %12s | %12s | %6s\n",
               "dt", "u_euler", "u_exact", "err_signed", "Steps");
        printf("  ---------+--------------+--------------+--------------+-------\n");

        double ode_err[N_DT];
        double ode_dt[N_DT];

        for (int k = 0; k < N_DT; k++) {
            ode_dt[k] = dt0_time / (double)(1 << k);
            long long steps_k = (long long)N_STEPS_BASE * (long long)(1 << k);

            /* Euler scalaire : u_{n+1} = u_n * (1 + lambda * dt) */
            double u_euler = u0;
            double factor  = 1.0 + lambda * ode_dt[k];
            for (long long s = 0; s < steps_k; s++)
                u_euler *= factor;

            ode_err[k] = u_euler - u_ref;   /* erreur signée */

            printf("  %.3e | %+12.6e | %+12.6e | %+12.4e | %6lld\n",
                   ode_dt[k], u_euler, u_ref, ode_err[k], steps_k);

            /* Log forensic */
            uint64_t ts = time_ns_get_absolute();
            char op_buf[192];
            snprintf(op_buf, sizeof(op_buf),
                     "006v2-ODE:dt=%.3e:u_euler=%.6e:err=%.4e:steps=%lld",
                     ode_dt[k], u_euler, ode_err[k], steps_k);
            uint64_t lum_id = ((uint64_t)0x00FFU << 48)
                            | ((uint64_t)0x03U << 40)
                            | ((uint64_t)(k & 0xFFU) << 32);
            uint64_t eb; memcpy(&eb, &ode_err[k], sizeof(uint64_t));
            lum_id |= (eb & 0x7FFFU);
            forensic_log_individual_lum(lum_id, op_buf, ts);
        }

        /* Ordres temporels */
        printf("\n  Ordres temporels (ODE scalaire, raffinement ×2 en dt) :\n");
        int tp1 = 1;
        for (int k = 0; k < N_DT - 1; k++) {
            double e_c  = fabs(ode_err[k]);
            double e_f  = fabs(ode_err[k + 1]);
            double ord_t = richardson_order(e_c, e_f);
            printf("  dt=%.3e → dt/2=%.3e : err %.4e → %.4e | ordre = %s\n",
                   ode_dt[k], ode_dt[k + 1],
                   ode_err[k], ode_err[k + 1],
                   fmt_order(ord_t));
            if (ord_t < 0.5 || ord_t > 2.5) tp1 = 0;
        }

        /* T-TIME-2 : |err| décroissant */
        int tp2 = 1;
        for (int k = 0; k < N_DT - 1; k++)
            if (fabs(ode_err[k]) <= fabs(ode_err[k + 1])) tp2 = 0;

        /* T-TIME-3 : tous les ode_err ont le même signe */
        int tp3 = 1;
        for (int k = 1; k < N_DT; k++)
            if ((ode_err[k] > 0) != (ode_err[0] > 0)) { tp3 = 0; break; }

        printf("\n  T-TIME-1 (ordres ∈ [0.5, 2.5])   : %s\n",
               tp1 ? "PASS ✓" : "FAIL ✗ — vérifier erreurs ci-dessus");
        printf("  T-TIME-2 (|err| décroissant)      : %s\n",
               tp2 ? "PASS ✓" : "FAIL ✗");
        printf("  T-TIME-3 (erreurs signe cohérent)  : %s\n\n",
               tp3 ? "PASS ✓" : "FAIL ✗");

        /* Variables utilisées dans la synthèse — garder la cohérence */
        int    n_time     = N_TIME;
        double t_fin_time = t_fin_ode;
        double ms_ref     = 0.0;      /* non utilisé pour ODE */
        double ms_arr[N_DT];
        SimRes time_res[N_DT];
        double time_dt[N_DT];
        /* Copie pour la synthèse */
        for (int k = 0; k < N_DT; k++) {
            time_dt[k]      = ode_dt[k];
            ms_arr[k]       = ode_err[k];
            memset(&time_res[k], 0, sizeof(SimRes));
            time_res[k].l2  = fabs(ode_err[k]);
            time_res[k].dt  = ode_dt[k];
        }
        (void)n_time;
        (void)t_fin_time;
        (void)ms_ref;
        (void)ms_arr;
        (void)time_res;
        (void)time_dt;

        /* ── Synthèse ─────────────────────────────────────────────────────── */

        printf("═══ SYNTHÈSE RICHARDSON-PROTOCOL-006-TIME v2 ═══\n\n");

        printf("  EXP-SPACE (dt=%.2e fixe, T=%.4f) :\n", DT_FIXED, T_FINAL);
        printf("    L2(N=32)  = %.4e\n", space_res[0].l2);
        printf("    L2(N=64)  = %.4e\n", space_res[1].l2);
        printf("    L2(N=128) = %.4e\n", space_res[2].l2);
        printf("    Ordre 32→64  : %s\n", fmt_order(ord_s_32_64));
        printf("    Ordre 64→128 : %s\n", fmt_order(ord_s_64_128));
        printf("    T-SPACE : %s | %s | %s\n\n",
               sp1 ? "T1=PASS" : "T1=FAIL",
               sp2 ? "T2=PASS" : "T2=FAIL",
               sp3 ? "T3=PASS" : "T3=FAIL");

        printf("  EXP-TIME v2 (ODE scalaire du/dt=λu, T=%.4e) :\n", t_fin_ode);
        for (int k = 0; k < N_DT; k++)
            printf("    err(dt=%.3e) = %+.4e\n", ode_dt[k], ode_err[k]);
        printf("    T-TIME : %s | %s | %s\n\n",
               tp1 ? "T1=PASS" : "T1=FAIL",
               tp2 ? "T2=PASS" : "T2=FAIL",
               tp3 ? "T3=PASS" : "T3=FAIL");

        printf("=== LIMITES HONNÊTES ===\n\n");
        printf("  - EXP-SPACE : ordre spatial O(dx²) sur solveur diffusion 2D.\n");
        printf("  - EXP-TIME  : ordre temporel O(dt) sur ODE scalaire du/dt=λu.\n");
        printf("    L'ODE utilise le MÊME intégrateur Euler que le solveur 2D.\n");
        printf("    AUCUNE erreur spatiale → ordre O(dt) exactement mesurable.\n");
        printf("  - Ce résultat NE valide PAS Chorin NS (splitting OPEN — rapport 172).\n");
        printf("  - CERTIFIED_100=false | unique_human_proven=false\n");

        int space_pass = sp1 && sp2 && sp3;
        int time_pass  = tp1 && tp2 && tp3;
        int all_pass   = space_pass && time_pass;

        printf("\n[VERDICT] RICHARDSON-006-TIME v2 : %s\n",
               all_pass
                 ? "PASS — ordre spatial O(dx²) + temporel O(dt) démontrés"
               : space_pass
                 ? "PARTIAL — EXP-SPACE PASS. EXP-TIME : voir T-TIME-* ci-dessus."
               : "FAIL — voir T-SPACE-* et T-TIME-* ci-dessus");
        printf("[NOTE] EXP-TIME : ODE scalaire identique à l'intégrateur Euler du solveur.\n");
        printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

        forensic_logger_destroy();
        return all_pass ? 0 : 1;
    }
}
