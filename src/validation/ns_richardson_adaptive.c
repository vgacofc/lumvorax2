/* **************************************************************************
** ns_richardson_adaptive.c — Richardson-PROTOCOL-002 : convergence adaptative
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Richardson-PROTOCOL-002
** Auteur : LumVorax Project
**
** Principe :
**   Aucune limite artificielle de pas de temps.
**   Chaque grille est simulée jusqu'à ce que son propre critère de stationnarité
**   soit satisfait. L2 est mesuré une fois et une seule fois : APRÈS stationnarité.
**
**   Critère de stationnarité (all-of) :
**     1. |u_max(t) - u_max(t - WIN)| / u_max(t) < EPS_CONV
**     2. |EK(t) - EK(t - WIN)| / EK(t) < EPS_CONV
**     3. poisson_residual < 1e-4
**
**   WIN  = 200 pas de contrôle (mesure toutes les POLL_INTERVAL pas)
**   EPS_CONV = 1e-3 (variation relative < 0.1%)
**   LIMIT_STEPS = 500 000 pas (limite de sécurité — JAMAIS un PASS)
**     Si atteinte : résultat marqué NON_CONVERGÉ.
**
**   FORENSIC : chaque grille logue un event forensic LUM par pas de contrôle
**   via forensic_log_individual_lum() — FORENSIC-UNIF-001 (partiel).
**
** Grilles : 32×32 | 64×64 | 128×128
** Protocoles de dt : A (constant) | B (∝dx) | C (∝dx²)
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

/* ── Paramètres adaptatifs ───────────────────────────────────────────────── */

#define RE          100.0
#define DT_REF      0.001       /* dt grille 32×32 protocole A */
#define N_REF       32

/* Critère de stationnarité */
#define EPS_CONV       1e-3     /* variation relative < 0.1% */
#define WIN_CHECKS     200      /* fenêtre de contrôle : 200 points de mesure */
#define POLL_INTERVAL  50       /* mesure l'état tous les N pas */

/* Limite de sécurité absolue (jamais un PASS si atteinte) */
#define LIMIT_STEPS    500000

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

/* ── Observables dynamiques ──────────────────────────────────────────────── */

static double compute_u_max(const NSSolver2D *s)
{
    int nx = s->params.nx;
    int ny = s->params.ny;
    double umax = 0.0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++) {
            double u = 0.5 * (s->u[i * (ny + 2) + j] +
                              s->u[(i + 1) * (ny + 2) + j]);
            if (fabs(u) > umax) umax = fabs(u);
        }
    return umax;
}

static double compute_kinetic_energy(const NSSolver2D *s)
{
    int    nx = s->params.nx;
    int    ny = s->params.ny;
    double ek = 0.0;
    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double u = 0.5 * (s->u[i       * (ny + 2) + j] +
                              s->u[(i + 1) * (ny + 2) + j]);
            double v = 0.5 * (s->v[i * (ny + 1) + j] +
                              s->v[i * (ny + 1) + (j + 1)]);
            ek += u * u + v * v;
        }
    }
    return 0.5 * ek * s->dx * s->dy;
}

static double compute_l2_ghia(const NSSolver2D *s)
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

static double compute_div_max(const NSSolver2D *s)
{
    int    nx  = s->params.nx;
    int    ny  = s->params.ny;
    double dx  = s->dx;
    double dy  = s->dy;
    double dmax = 0.0;

    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double ue = s->u[i       * (ny + 2) + j];
            double uw = s->u[(i - 1) * (ny + 2) + j];
            double vn = s->v[i * (ny + 1) + j];
            double vs = s->v[i * (ny + 1) + (j - 1)];
            double d  = fabs((ue - uw) / dx + (vn - vs) / dy);
            if (d > dmax) dmax = d;
        }
    }
    return dmax;
}

/* ── Résultat d'un run adaptatif ─────────────────────────────────────────── */

typedef struct {
    int    n;
    double dt;
    int    steps_to_conv;   /* -1 = LIMIT atteinte */
    double t_phys;          /* temps physique à convergence */
    double l2_u;
    double div_max;
    double ek_final;
    double u_max_final;
    double poisson_res;
    double wall_s;
    int    converged;       /* 1 = convergé | 0 = LIMIT_STEPS atteinte */
    /* Forensic */
    uint64_t ts_start_ns;
    uint64_t ts_end_ns;
    int    forensic_events; /* nombre d'events LUM loggés */
} AdaptiveResult;

/* ── Run adaptatif principal ─────────────────────────────────────────────── */
/*
 * Aucune limite artificielle sauf LIMIT_STEPS (sécurité).
 * On mesure l'état tous les POLL_INTERVAL pas.
 * Convergence = WIN_CHECKS mesures consécutives stables.
 *
 * FORENSIC : forensic_log_individual_lum() appelé à chaque point de contrôle
 * avec lum_id = hash(grille × step × observable) pour traçabilité.
 */
static AdaptiveResult run_adaptive(int n, double dt, const char *proto_name)
{
    AdaptiveResult r;
    memset(&r, 0, sizeof(r));
    r.n            = n;
    r.dt           = dt;
    r.converged    = 0;
    r.steps_to_conv = -1;

    /* Forensic : timestamp de début */
    r.ts_start_ns = time_ns_get_absolute();

    NSParams p = {
        .nx          = n,
        .ny          = n,
        .lx          = 1.0,
        .ly          = 1.0,
        .re          = RE,
        .dt          = dt,
        .max_iter    = 1,       /* géré manuellement */
        .tol         = 1e-5,
        .max_poisson = 50,
        .debug       = 0
    };

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[ADAPT][ERROR] ns_solver_create failed n=%d dt=%.2e\n", n, dt);
        return r;
    }
    ns_solver_set_lid_bc(s);

    /*
     * Fenêtre glissante de WIN_CHECKS points de contrôle.
     * On stocke u_max et EK aux WIN_CHECKS derniers points.
     */
    double *buf_umax = (double *)calloc((size_t)WIN_CHECKS, sizeof(double));
    double *buf_ek   = (double *)calloc((size_t)WIN_CHECKS, sizeof(double));
    if (!buf_umax || !buf_ek) {
        fprintf(stderr, "[ADAPT][ERROR] calloc bufs failed\n");
        ns_solver_destroy(s);
        free(buf_umax); free(buf_ek);
        return r;
    }

    int    check_idx    = 0;     /* position circulaire dans la fenêtre */
    int    checks_done  = 0;     /* nombre total de points de contrôle */
    double last_poisson = 1.0;
    int    forensic_evts = 0;

    fprintf(stderr,
        "[ADAPT][n=%d][dt=%.2e][%s] départ — critère: rel<%.0e sur %d checks (poll=%d)\n",
        n, dt, proto_name, EPS_CONV, WIN_CHECKS, POLL_INTERVAL);

    int total_steps = 0;
    int converged   = 0;

    while (total_steps < LIMIT_STEPS && !converged) {
        /* Exécuter POLL_INTERVAL pas */
        for (int k = 0; k < POLL_INTERVAL && total_steps < LIMIT_STEPS; k++) {
            last_poisson = ns_solver_step(s);
            total_steps++;
        }

        /* Mesure des observables */
        double umax = compute_u_max(s);
        double ek   = compute_kinetic_energy(s);

        /* Enregistrement dans la fenêtre circulaire */
        buf_umax[check_idx % WIN_CHECKS] = umax;
        buf_ek  [check_idx % WIN_CHECKS] = ek;
        checks_done++;
        check_idx++;

        /* FORENSIC-UNIF-003 BUG-1 FIX : lum_id uint64_t complet.
         * Encodage : n(16) | checks_done(32) | reserved(16) */
        uint64_t lum_id = ((uint64_t)(n & 0xFFFFU) << 48)
                        | ((uint64_t)(checks_done & 0xFFFFFFFFU) << 16);
        forensic_log_individual_lum(lum_id, "NS_ADAPTIVE_CHECKPOINT",
                                    time_ns_get_absolute());
        forensic_evts++;

        /* Test de convergence : seulement si on a WIN_CHECKS points */
        if (checks_done >= WIN_CHECKS) {
            /* Variation relative max-min sur la fenêtre glissante */
            double umax_min = buf_umax[0], umax_max = buf_umax[0];
            double ek_min   = buf_ek[0],   ek_max   = buf_ek[0];

            for (int k = 1; k < WIN_CHECKS; k++) {
                if (buf_umax[k] < umax_min) umax_min = buf_umax[k];
                if (buf_umax[k] > umax_max) umax_max = buf_umax[k];
                if (buf_ek[k]   < ek_min)   ek_min   = buf_ek[k];
                if (buf_ek[k]   > ek_max)   ek_max   = buf_ek[k];
            }

            double rel_umax = (umax > 1e-15) ? (umax_max - umax_min) / umax : 1.0;
            double rel_ek   = (ek   > 1e-15) ? (ek_max   - ek_min)   / ek   : 1.0;

            if (rel_umax < EPS_CONV && rel_ek < EPS_CONV && last_poisson < 1e-4) {
                converged = 1;
                fprintf(stderr,
                    "[ADAPT][n=%d] CONVERGÉ step=%d t=%.2f "
                    "rel_umax=%.2e rel_ek=%.2e poisson=%.2e\n",
                    n, total_steps, total_steps * dt,
                    rel_umax, rel_ek, last_poisson);
            }
        }

        /* Rapport de progression tous les 5000 pas */
        if (total_steps % 5000 == 0) {
            fprintf(stderr,
                "[ADAPT][n=%d] step=%6d t=%7.2f umax=%.4f ek=%.6f poisson=%.2e\n",
                n, total_steps, total_steps * dt, umax, ek, last_poisson);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    r.ts_end_ns = time_ns_get_absolute();

    r.wall_s         = (t1.tv_sec - t0.tv_sec) +
                       (t1.tv_nsec - t0.tv_nsec) * 1e-9;
    r.steps_to_conv  = converged ? total_steps : -1;
    r.t_phys         = total_steps * dt;
    r.converged      = converged;
    r.forensic_events = forensic_evts;

    /* Mesure finale des métriques */
    r.l2_u           = compute_l2_ghia(s);
    r.div_max        = compute_div_max(s);
    r.ek_final       = compute_kinetic_energy(s);
    r.u_max_final    = compute_u_max(s);
    r.poisson_res    = last_poisson;

    fprintf(stderr,
        "[ADAPT][n=%d][dt=%.2e] %s steps=%d t=%.2f L2=%.6f "
        "umax=%.4f ek=%.6f div=%.2e poisson=%.2e wall=%.1fs forensic_evts=%d\n",
        n, dt,
        converged ? "CONV" : "LIMIT",
        total_steps, r.t_phys, r.l2_u,
        r.u_max_final, r.ek_final, r.div_max, r.poisson_res,
        r.wall_s, forensic_evts);

    free(buf_umax);
    free(buf_ek);
    ns_solver_destroy(s);
    return r;
}

/* ── Analyse Richardson sur 3 résultats convergés ────────────────────────── */

typedef struct {
    const char *name;
    double l2_32, l2_64, l2_128;
    double order_32_64, order_64_128;
    int    t01_pass;
    int    t02_pass;
    int    all_converged; /* 1 = les 3 grilles ont convergé */
} RichardsonResult;

static RichardsonResult compute_richardson(const char *name,
                                            AdaptiveResult r32,
                                            AdaptiveResult r64,
                                            AdaptiveResult r128)
{
    RichardsonResult rr;
    rr.name  = name;
    rr.l2_32  = r32.l2_u;
    rr.l2_64  = r64.l2_u;
    rr.l2_128 = r128.l2_u;
    rr.all_converged = r32.converged && r64.converged && r128.converged;

    rr.order_32_64  = (r64.l2_u > 0.0 && r32.l2_u > 0.0)
                      ? log(r32.l2_u / r64.l2_u) / log(2.0) : 0.0;
    rr.order_64_128 = (r128.l2_u > 0.0 && r64.l2_u > 0.0)
                      ? log(r64.l2_u / r128.l2_u) / log(2.0) : 0.0;

    rr.t01_pass = rr.all_converged
                  && (r64.l2_u < r32.l2_u)
                  && (r128.l2_u < r64.l2_u);
    rr.t02_pass = rr.t01_pass
                  && (rr.order_32_64  >= 0.8)
                  && (rr.order_64_128 >= 0.8);
    return rr;
}

static void print_adaptive_result(const char *label, AdaptiveResult r)
{
    printf("  %-9s | %s | steps=%7d | t=%7.2f s | L2=%.6f | "
           "umax=%.4f | ek=%.6f | wall=%.1fs | forensic=%d\n",
           label,
           r.converged ? "CONV  " : "LIMIT ",
           r.converged ? r.steps_to_conv : -1,
           r.t_phys, r.l2_u, r.u_max_final, r.ek_final,
           r.wall_s, r.forensic_events);
}

static void print_richardson_result(const RichardsonResult *rr,
                                     AdaptiveResult r32,
                                     AdaptiveResult r64,
                                     AdaptiveResult r128)
{
    printf("\n  ── %s ──\n", rr->name);
    printf("  Grille  | Statut | Steps     | t_phys   | L2(u)    | u_max  | "
           "EK       | Wall  | Forensic\n");
    print_adaptive_result("32×32  ", r32);
    print_adaptive_result("64×64  ", r64);
    print_adaptive_result("128×128", r128);
    printf("\n");

    if (!rr->all_converged)
        printf("  [WARN] Au moins une grille n'a pas convergé → Richardson non valide.\n");

    printf("  Ordre Richardson 32→64   : %+.3f\n", rr->order_32_64);
    printf("  Ordre Richardson 64→128  : %+.3f\n", rr->order_64_128);
    printf("  T01 (L2 décroît)         : %s\n", rr->t01_pass ? "PASS" : "FAIL");
    printf("  T02 (ordre ≥ 0.8)        : %s\n", rr->t02_pass ? "PASS" : "FAIL");

    if (!rr->t01_pass && rr->all_converged)
        printf("  [NOTE] Toutes les grilles convergées mais L2 ne décroît pas "
               "→ solveur non convergent en espace.\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== RICHARDSON-PROTOCOL-002 — CONVERGENCE ADAPTATIVE ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[CRITERE] rel_umax < %.0e ET rel_ek < %.0e ET poisson < 1e-4\n",
           EPS_CONV, EPS_CONV);
    printf("[FENETRE] %d checkpoints (POLL=%d pas chacun)\n", WIN_CHECKS, POLL_INTERVAL);
    printf("[LIMITE]  LIMIT_STEPS=%d — si atteinte : NON_CONVERGÉ, jamais PASS\n\n",
           LIMIT_STEPS);

    /* Initialisation forensic */
    forensic_logger_init("logs/forensic/ns_adaptive_forensic.log");

    /* ── PROTOCOLE A : dt constant ── */
    printf("=== PROTOCOLE A : dt constant (dt=%.3f) ===\n", DT_REF);
    AdaptiveResult a32  = run_adaptive(32,  DT_REF, "A");
    AdaptiveResult a64  = run_adaptive(64,  DT_REF, "A");
    AdaptiveResult a128 = run_adaptive(128, DT_REF, "A");
    RichardsonResult rA = compute_richardson("PROTOCOLE A — dt constant", a32, a64, a128);
    print_richardson_result(&rA, a32, a64, a128);

    /* ── PROTOCOLE B : dt ∝ dx ── */
    double dt_B64  = DT_REF * ((double)N_REF / 64.0);
    double dt_B128 = DT_REF * ((double)N_REF / 128.0);
    printf("\n=== PROTOCOLE B : dt ∝ dx ===\n");
    printf("  dt(32)=%.4f | dt(64)=%.4f | dt(128)=%.5f\n",
           DT_REF, dt_B64, dt_B128);
    AdaptiveResult b32  = run_adaptive(32,  DT_REF, "B");
    AdaptiveResult b64  = run_adaptive(64,  dt_B64,  "B");
    AdaptiveResult b128 = run_adaptive(128, dt_B128, "B");
    RichardsonResult rB = compute_richardson("PROTOCOLE B — dt ∝ dx", b32, b64, b128);
    print_richardson_result(&rB, b32, b64, b128);

    /* ── PROTOCOLE C : dt ∝ dx² ── */
    double dt_C64  = DT_REF * ((double)N_REF / 64.0)  * ((double)N_REF / 64.0);
    double dt_C128 = DT_REF * ((double)N_REF / 128.0) * ((double)N_REF / 128.0);
    printf("\n=== PROTOCOLE C : dt ∝ dx² ===\n");
    printf("  dt(32)=%.4f | dt(64)=%.5f | dt(128)=%.7f\n",
           DT_REF, dt_C64, dt_C128);
    AdaptiveResult c32  = run_adaptive(32,  DT_REF, "C");
    AdaptiveResult c64  = run_adaptive(64,  dt_C64,  "C");
    AdaptiveResult c128 = run_adaptive(128, dt_C128, "C");
    RichardsonResult rC = compute_richardson("PROTOCOLE C — dt ∝ dx²", c32, c64, c128);
    print_richardson_result(&rC, c32, c64, c128);

    /* ── Synthèse ── */
    printf("\n=== SYNTHÈSE RICHARDSON-PROTOCOL-002 ===\n\n");
    printf("  %-38s | Convergé | T01  | T02  | ord 32/64 | ord 64/128\n",
           "Protocole");
    printf("  %-38s | -------- | ---- | ---- | --------- | ----------\n",
           "--------------------------------------");
    printf("  %-38s | %-8s | %-4s | %-4s | %+.3f     | %+.3f\n",
           rA.name, rA.all_converged?"OUI":"NON",
           rA.t01_pass?"PASS":"FAIL", rA.t02_pass?"PASS":"FAIL",
           rA.order_32_64, rA.order_64_128);
    printf("  %-38s | %-8s | %-4s | %-4s | %+.3f     | %+.3f\n",
           rB.name, rB.all_converged?"OUI":"NON",
           rB.t01_pass?"PASS":"FAIL", rB.t02_pass?"PASS":"FAIL",
           rB.order_32_64, rB.order_64_128);
    printf("  %-38s | %-8s | %-4s | %-4s | %+.3f     | %+.3f\n",
           rC.name, rC.all_converged?"OUI":"NON",
           rC.t01_pass?"PASS":"FAIL", rC.t02_pass?"PASS":"FAIL",
           rC.order_32_64, rC.order_64_128);

    int global_pass = rA.t02_pass || rB.t02_pass || rC.t02_pass;
    printf("\n[VERDICT] RICHARDSON-PROTOCOL-002 : %s\n",
           global_pass
           ? "AU MOINS UN PROTOCOLE DÉMONTRE ORDRE >= 0.8"
           : "OPEN — convergence atteinte mais ordre < 0.8 sur tous les protocoles");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    forensic_logger_destroy();
    return global_pass ? 0 : 1;
}
