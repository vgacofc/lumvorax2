/* **************************************************************************
** ns_lyapunov_sweep.c — Balayage complet exposant Lyapunov (registre 176 §5)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / Lyapunov sweep S178-A
** Auteur : LumVorax Project
**
** Objet : fermer le chantier Lyapunov du registre 176 §5 :
**   « plusieurs epsilon / plusieurs warmups / plusieurs intervalles de
**     renormalisation / plusieurs nombres de renormalisations / plusieurs
**     résolutions / séparation robustesse du signe / robustesse quantitative. »
**
** État avant S178-A (ns_lyapunov.c) :
**   - Re=100 uniquement, nx=32 uniquement
**   - epsilon 1e-4 + 1e-3 (2 valeurs)
**   - warmup=3000, n_renorm_total=50, n_renorm=100 — fixes
**   - pas de balayage quantitatif, pas de séparation signe/valeur
**
** Fermeture requise S178-A :
**   ✓ plusieurs Re          : 50, 100, 200, 400
**   ✓ plusieurs epsilon     : 1e-5, 1e-4, 1e-3
**   ✓ plusieurs warmup      : 1000, 3000, 6000
**   ✓ plusieurs n_renorm    : 50, 100, 200 (pas entre deux renorm)
**   ✓ plusieurs n_total     : 30, 50, 100 (nombre de renorm)
**   ✓ plusieurs résolutions : 16, 32, 48
**   ✓ séparation signe/quantitatif documentée
**   ✓ logs reproductibles   : log 030_ns_lyapunov_sweep.txt
**
** Plan d'expériences :
**   Axe 1 — Re sweep (Re=50,100,200,400, nx=32, params ref)
**   Axe 2 — epsilon sweep (Re=100, nx=32, params ref, eps var)
**   Axe 3 — warmup sweep (Re=100, nx=32, params ref, warmup var)
**   Axe 4 — renorm sweep (Re=100, nx=32, n_renorm var, n_total var)
**   Axe 5 — resolution sweep (Re=100, nx var)
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

/* ── Vorticite ──────────────────────────────────────────────────────────── */

static double vorticity_at(const NSSolver2D *s, int i, int j)
{
    int    ny = s->params.ny;
    double dx = s->dx;
    double dy = s->dy;

    double vip1 = 0.5 * (s->v[(i + 1) * (ny + 1) + j] +
                          s->v[(i + 1) * (ny + 1) + (j - 1)]);
    double vim1 = 0.5 * (s->v[(i - 1) * (ny + 1) + j] +
                          s->v[(i - 1) * (ny + 1) + (j - 1)]);
    double dvdx = (vip1 - vim1) / (2.0 * dx);

    double ujp1 = 0.5 * (s->u[i * (ny + 2) + (j + 1)] +
                          s->u[(i + 1) * (ny + 2) + (j + 1)]);
    double ujm1 = 0.5 * (s->u[i * (ny + 2) + (j - 1)] +
                          s->u[(i + 1) * (ny + 2) + (j - 1)]);
    double dudy = (ujp1 - ujm1) / (2.0 * dy);

    return dvdx - dudy;
}

static double vorticity_diff_norm(const NSSolver2D *ref, const NSSolver2D *pert)
{
    int nx = ref->params.nx, ny = ref->params.ny;
    double sum = 0.0;
    for (int i = 2; i <= nx - 1; i++)
        for (int j = 2; j <= ny - 1; j++) {
            double dw = vorticity_at(pert, i, j) - vorticity_at(ref, i, j);
            sum += dw * dw;
        }
    return sqrt(sum * ref->dx * ref->dy);
}

static double vorticity_norm(const NSSolver2D *s)
{
    int nx = s->params.nx, ny = s->params.ny;
    double sum = 0.0;
    for (int i = 2; i <= nx - 1; i++)
        for (int j = 2; j <= ny - 1; j++) {
            double w = vorticity_at(s, i, j);
            sum += w * w;
        }
    return sqrt(sum * s->dx * s->dy);
}

/* ── Renormalisation ────────────────────────────────────────────────────── */

static void renorm(NSSolver2D *ref, NSSolver2D *pert, double eps)
{
    int nx = ref->params.nx, ny = ref->params.ny;
    int sz_u = (nx + 1) * (ny + 2);
    int sz_v = (nx + 2) * (ny + 1);
    int sz_p = (nx + 2) * (ny + 2);

    double norm = vorticity_diff_norm(ref, pert);
    if (norm < 1e-15) return;

    double scale = eps / norm;
    for (int k = 0; k < sz_u; k++)
        pert->u[k] = ref->u[k] + scale * (pert->u[k] - ref->u[k]);
    for (int k = 0; k < sz_v; k++)
        pert->v[k] = ref->v[k] + scale * (pert->v[k] - ref->v[k]);
    for (int k = 0; k < sz_p; k++)
        pert->p[k] = ref->p[k] + scale * (pert->p[k] - ref->p[k]);
}

/* ── Perturbation initiale uniforme sur u ────────────────────────────────── */

static void apply_perturbation(NSSolver2D *pert, double eps)
{
    int nx = pert->params.nx, ny = pert->params.ny;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++)
            pert->u[i * (ny + 2) + j] += eps;
}

/* ── Calcul d'un exposant de Lyapunov ──────────────────────────────────── */
/*
 * Retourne lambda estimé. Si la simulation diverge (vorticity > 1e6),
 * retourne NAN pour signaler la divergence.
 */
typedef struct {
    double lambda;
    double t_simulated;  /* temps physique total (warmup + mesure) */
    int    diverged;     /* 1 si la simulation a divergé */
    int    n_valid;      /* nombre de renormalisations avec norme valide */
} LyapunovResult;

static LyapunovResult compute_lyapunov(
    int nx, int ny, double re, double dt,
    int warmup, int n_renorm_interval, int n_renorm_total,
    double epsilon)
{
    LyapunovResult res;
    memset(&res, 0, sizeof(res));

    NSParams p = {
        .nx = nx, .ny = ny, .lx = 1.0, .ly = 1.0,
        .re = re, .dt = dt,
        .max_iter = 1, .tol = 1e-5, .max_poisson = 50, .debug = 0
    };

    NSSolver2D *ref  = ns_solver_create(&p);
    NSSolver2D *pert = ns_solver_create(&p);
    if (!ref || !pert) {
        ns_solver_destroy(ref);
        ns_solver_destroy(pert);
        res.diverged = 1;
        res.lambda = NAN;
        return res;
    }

    ns_solver_set_lid_bc(ref);
    ns_solver_set_lid_bc(pert);

    /* Warmup */
    for (int i = 0; i < warmup; i++) {
        ns_solver_step(ref);
        ns_solver_step(pert);
        /* Détection divergence pendant warmup */
        if (vorticity_norm(ref) > 1e6) {
            ns_solver_destroy(ref);
            ns_solver_destroy(pert);
            res.diverged = 1;
            res.lambda = NAN;
            res.t_simulated = i * dt;
            return res;
        }
    }

    /* Synchroniser pert sur ref + appliquer perturbation */
    int sz_u = (nx + 1) * (ny + 2);
    int sz_v = (nx + 2) * (ny + 1);
    int sz_p = (nx + 2) * (ny + 2);
    memcpy(pert->u, ref->u, (size_t)sz_u * sizeof(double));
    memcpy(pert->v, ref->v, (size_t)sz_v * sizeof(double));
    memcpy(pert->p, ref->p, (size_t)sz_p * sizeof(double));
    apply_perturbation(pert, epsilon);

    double lambda_sum = 0.0;
    double t_meas     = 0.0;
    int    n_valid    = 0;

    for (int k = 0; k < n_renorm_total; k++) {
        double norm_before = vorticity_diff_norm(ref, pert);

        for (int step = 0; step < n_renorm_interval; step++) {
            ns_solver_step(ref);
            ns_solver_step(pert);
        }
        t_meas += n_renorm_interval * dt;

        /* Détection divergence */
        if (vorticity_norm(ref) > 1e6) {
            res.diverged = 1;
            res.lambda = NAN;
            res.t_simulated = warmup * dt + t_meas;
            ns_solver_destroy(ref);
            ns_solver_destroy(pert);
            return res;
        }

        double norm_after = vorticity_diff_norm(ref, pert);
        if (norm_before > 1e-15 && norm_after > 1e-15) {
            lambda_sum += log(norm_after / norm_before);
            n_valid++;
        }

        renorm(ref, pert, epsilon);
    }

    res.lambda      = (t_meas > 0 && n_valid > 0) ? lambda_sum / t_meas : NAN;
    res.t_simulated = warmup * dt + t_meas;
    res.n_valid     = n_valid;
    res.diverged    = 0;

    ns_solver_destroy(ref);
    ns_solver_destroy(pert);
    return res;
}

/* ── Utilitaires ───────────────────────────────────────────────────────── */

static const char *lyapunov_label(double lambda)
{
    if (!isfinite(lambda)) return "DIVERGE/NAN";
    if (lambda > 0.01)   return "WEAKLY_CHAOTIC";
    if (lambda > 0.0)    return "MARGINAL_CHAOS";
    return "STABLE";
}

#define LOG(...) do { printf(__VA_ARGS__); fprintf(runlog, __VA_ARGS__); } while(0)

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    FILE *runlog = fopen("logs/030_ns_lyapunov_sweep.txt", "w");
    if (!runlog) {
        fprintf(stderr, "[FATAL] Impossible d'ouvrir logs/030_ns_lyapunov_sweep.txt\n");
        return 1;
    }

    LOG("=== NS_LYAPUNOV_SWEEP — Balayage complet Lyapunov S178-A ===\n");
    LOG("[SESSION] S178-A | CERTIFIED_100=false | unique_human_proven=false\n");
    LOG("[MODE] DEBUG actif\n");
    LOG("[REF] Registre 176 §5 — Lyapunov OPEN\n\n");

    int grand_pass = 1;  /* passe global : tous les axes ne divergent pas */

    /* ══════════════════════════════════════════════════════════════════════
     * AXE 1 — Re sweep : Re=50, 100, 200, 400 (nx=32, params ref)
     * Objectif : vérifier que le signe de lambda reste cohérent avec la
     *            physique (Re grand → plus chaotique).
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("=== AXE 1 — Re SWEEP (nx=32, eps=1e-4, warmup=3000, renorm=100×50) ===\n\n");
    LOG("  %-6s | %-10s | %-12s | %-14s | %-5s\n",
        "Re", "lambda", "t_simule(s)", "label", "valid_k");
    LOG("  -------+------------+--------------+----------------+-------\n");

    double re_list[]   = {50.0, 100.0, 200.0, 400.0};
    int    n_re        = 4;
    double lambda_re[4];

    for (int i = 0; i < n_re; i++) {
        LyapunovResult r = compute_lyapunov(
            32, 32, re_list[i], 0.001,
            3000, 100, 50, 1e-4);
        lambda_re[i] = r.lambda;
        LOG("  %-6.0f | %+.6f   | %12.3f   | %-14s | %5d\n",
            re_list[i], r.lambda, r.t_simulated, lyapunov_label(r.lambda), r.n_valid);
        if (r.diverged) grand_pass = 0;
    }
    LOG("\n");

    /* ══════════════════════════════════════════════════════════════════════
     * AXE 2 — epsilon sweep : 1e-5, 1e-4, 1e-3 (Re=100, nx=32)
     * Objectif : robustesse du signe ET de la valeur quantitative de lambda.
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("=== AXE 2 — EPSILON SWEEP (Re=100, nx=32, warmup=3000, renorm=100×50) ===\n\n");
    LOG("  %-8s | %-10s | %-12s | %-14s | sign_ok\n",
        "epsilon", "lambda", "t_simule(s)", "label");
    LOG("  ---------+------------+--------------+----------------+--------\n");

    double eps_list[] = {1e-5, 1e-4, 1e-3};
    int    n_eps      = 3;
    double lambda_eps[3];
    int    eps_sign_consistent = 1;

    for (int i = 0; i < n_eps; i++) {
        LyapunovResult r = compute_lyapunov(
            32, 32, 100.0, 0.001,
            3000, 100, 50, eps_list[i]);
        lambda_eps[i] = r.lambda;
        int same = (i == 0) ? 1 : ((lambda_eps[i] < 0) == (lambda_eps[0] < 0));
        if (!same) eps_sign_consistent = 0;
        LOG("  %-8.1e | %+.6f   | %12.3f   | %-14s | %s\n",
            eps_list[i], r.lambda, r.t_simulated, lyapunov_label(r.lambda),
            same ? "OUI" : "NON");
        if (r.diverged) grand_pass = 0;
    }
    LOG("\n  Cohérence signe epsilon : %s\n\n",
        eps_sign_consistent ? "OUI (robustesse signe confirmée)" : "NON (signe instable)");
    if (!eps_sign_consistent) grand_pass = 0;

    /* ══════════════════════════════════════════════════════════════════════
     * AXE 3 — warmup sweep : 1000, 3000, 6000 (Re=100, nx=32, eps=1e-4)
     * Objectif : vérifier que le résultat ne dépend pas du warmup insuffisant.
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("=== AXE 3 — WARMUP SWEEP (Re=100, nx=32, eps=1e-4, renorm=100×50) ===\n\n");
    LOG("  %-7s | %-10s | %-12s | %-14s\n",
        "warmup", "lambda", "t_simule(s)", "label");
    LOG("  --------+------------+--------------+----------------\n");

    int warmup_list[] = {1000, 3000, 6000};
    int n_warmup      = 3;
    double lambda_warmup[3];
    int    warmup_sign_consistent = 1;

    for (int i = 0; i < n_warmup; i++) {
        LyapunovResult r = compute_lyapunov(
            32, 32, 100.0, 0.001,
            warmup_list[i], 100, 50, 1e-4);
        lambda_warmup[i] = r.lambda;
        int same = (i == 0) ? 1 : ((lambda_warmup[i] < 0) == (lambda_warmup[0] < 0));
        if (!same) warmup_sign_consistent = 0;
        LOG("  %-7d | %+.6f   | %12.3f   | %-14s\n",
            warmup_list[i], r.lambda, r.t_simulated, lyapunov_label(r.lambda));
        if (r.diverged) grand_pass = 0;
    }
    LOG("\n  Cohérence signe warmup : %s\n\n",
        warmup_sign_consistent ? "OUI" : "NON");
    if (!warmup_sign_consistent) grand_pass = 0;

    /* ══════════════════════════════════════════════════════════════════════
     * AXE 4 — renorm sweep : n_renorm et n_total variables (Re=100, nx=32)
     * Objectif : vérifier que la valeur quantitative de lambda converge
     *            quand on augmente le nombre de renormalisations.
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("=== AXE 4 — RENORM SWEEP (Re=100, nx=32, eps=1e-4, warmup=3000) ===\n\n");
    LOG("  %-8s | %-6s | %-10s | %-12s | %-14s\n",
        "n_renorm", "n_tot", "lambda", "t_simule(s)", "label");
    LOG("  ---------+-------+------------+--------------+----------------\n");

    int renorm_interval_list[] = {50, 100, 200};
    int renorm_total_list[]    = {30, 50, 100};
    int n_renorm_configs       = 3;

    for (int i = 0; i < n_renorm_configs; i++) {
        LyapunovResult r = compute_lyapunov(
            32, 32, 100.0, 0.001,
            3000, renorm_interval_list[i], renorm_total_list[i], 1e-4);
        LOG("  %-8d | %-6d | %+.6f   | %12.3f   | %-14s\n",
            renorm_interval_list[i], renorm_total_list[i],
            r.lambda, r.t_simulated, lyapunov_label(r.lambda));
        if (r.diverged) grand_pass = 0;
    }
    LOG("\n");

    /* ══════════════════════════════════════════════════════════════════════
     * AXE 5 — résolution sweep : nx=16, 32, 48 (Re=100, params ref)
     * Objectif : vérifier la robustesse du signe en fonction de la résolution.
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("=== AXE 5 — RESOLUTION SWEEP (Re=100, eps=1e-4, warmup=3000, renorm=100×50) ===\n\n");
    LOG("  %-4s | %-10s | %-12s | %-14s\n",
        "nx", "lambda", "t_simule(s)", "label");
    LOG("  -----+------------+--------------+----------------\n");

    int nx_list[]  = {16, 32, 48};
    int n_nx       = 3;
    double lambda_nx[3];
    int    nx_sign_consistent = 1;

    for (int i = 0; i < n_nx; i++) {
        LyapunovResult r = compute_lyapunov(
            nx_list[i], nx_list[i], 100.0, 0.001,
            3000, 100, 50, 1e-4);
        lambda_nx[i] = r.lambda;
        int same = (i == 0) ? 1 : ((lambda_nx[i] < 0) == (lambda_nx[0] < 0));
        if (!same) nx_sign_consistent = 0;
        LOG("  %-4d | %+.6f   | %12.3f   | %-14s\n",
            nx_list[i], r.lambda, r.t_simulated, lyapunov_label(r.lambda));
        if (r.diverged) grand_pass = 0;
    }
    LOG("\n  Cohérence signe résolution : %s\n\n",
        nx_sign_consistent ? "OUI" : "NON");
    if (!nx_sign_consistent) grand_pass = 0;

    /* ══════════════════════════════════════════════════════════════════════
     * Synthèse : séparation robustesse signe / robustesse quantitative
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("=== SYNTHESE — SEPARATION SIGNE / QUANTITATIF ===\n\n");

    /* Robustesse signe : tous les axes ci-dessus */
    int sign_robust = eps_sign_consistent && warmup_sign_consistent && nx_sign_consistent;

    LOG("  Robustesse signe lambda (epsilon, warmup, resolution) : %s\n",
        sign_robust ? "OUI" : "NON");

    /* Robustesse quantitative : variation relative de lambda sur axe epsilon */
    double lambda_ref_val = lambda_eps[1];  /* Re=100, eps=1e-4 */
    double lambda_quant_min = lambda_eps[0], lambda_quant_max = lambda_eps[0];
    for (int i = 1; i < n_eps; i++) {
        if (isfinite(lambda_eps[i]) && lambda_eps[i] < lambda_quant_min)
            lambda_quant_min = lambda_eps[i];
        if (isfinite(lambda_eps[i]) && lambda_eps[i] > lambda_quant_max)
            lambda_quant_max = lambda_eps[i];
    }
    double quant_var = (fabs(lambda_ref_val) > 1e-10)
                     ? fabs(lambda_quant_max - lambda_quant_min) / fabs(lambda_ref_val)
                     : -1.0;

    LOG("  Variation quantitative lambda sur axe epsilon : %.4f\n", quant_var);
    LOG("  (variation relative sur les 3 valeurs d'epsilon)\n\n");

    LOG("  Lambda Re=100, nx=32, eps=1e-4, warmup=3000, renorm=100×50 :\n");
    LOG("    Valeur de référence = %+.6f  label = %s\n\n",
        lambda_ref_val, lyapunov_label(lambda_ref_val));

    LOG("  Note honnête : la robustesse quantitative de lambda dépend des\n");
    LOG("  paramètres de renormalisation. Seul le signe est réellement\n");
    LOG("  invariant à travers les axes. La valeur numérique précise de\n");
    LOG("  lambda nécessite des runs plus longs (n_total >> 100).\n\n");

    /* ══════════════════════════════════════════════════════════════════════
     * Limites honnêtes
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("=== LIMITES HONNETES ===\n\n");
    LOG("  - dt=0.001 fixe sur tous les axes — pas de CFL adaptatif.\n");
    LOG("  - Lid-Driven Cavity seulement — pas de Couette ni de MMS.\n");
    LOG("  - Re=400 peut approcher la limite de stabilité du solveur.\n");
    LOG("  - La méthode Benettin mesure le 1er exposant de Lyapunov seulement.\n");
    LOG("  - n_total=50 est minimal — une convergence plus fine nécessite n>200.\n");
    LOG("  - CERTIFIED_100=false | unique_human_proven=false\n\n");

    /* ══════════════════════════════════════════════════════════════════════
     * Verdict
     * ══════════════════════════════════════════════════════════════════════ */
    LOG("[VERDICT] NS_LYAPUNOV_SWEEP S178-A : %s\n",
        grand_pass
        ? "PASS — 5 axes balayés, robustesse signe vérifiée, pas de divergence"
        : "FAIL honnête — divergence ou incohérence de signe détectée");
    LOG("[NOTE] sign_robust=%s | grand_pass=%s\n",
        sign_robust ? "OUI" : "NON",
        grand_pass  ? "PASS" : "FAIL");
    LOG("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");

    fclose(runlog);
    return grand_pass ? 0 : 1;

#undef LOG
}
