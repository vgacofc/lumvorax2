/* **************************************************************************
** ns_forensic_unif4.c — FORENSIC-UNIF-004 : timestamps MONOTONIC + unicité exacte
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / FORENSIC-UNIF-004
** Auteur : LumVorax Project
**
** Historique des corrections :
**
**   S162 (UNICITE-001) :
**     BUG-3 FINAL CLOSE : timestamps CLOCK_MONOTONIC via time_ns_get_monotonic().
**     UNICITE-001 CLOSE : hash set LUM_ID (schéma v2 — cell_idx intégré) → 0 doublon.
**
**   S164 (UNICITE-003) — audit 165 PC2/PC3/UNIF-004 :
**     MIGRATION v2→v3 : suppression de encode_lum_id_64() local, LumIDHashSet,
**       HASH_EMPTY=UINT64_MAX, make_run_id() XOR.
**     Remplacement par lum_id_v3_encode(), LumIDHashSetV3, lum_id_v3_new_run_seq()
**       depuis lum_id_schema.h (schéma v3 partagé — garanties PC3/PC4 correctes).
**     RAISON : le header était déjà inclus mais les fonctions locales v2 n'avaient
**       pas été supprimées — l'inclusion était sans effet opérationnel (audit 165).
**     PC2 : garde step <= LUM_ID_V3_MAX_STEP_VALUE ajoutée dans la boucle main().
**     run_id → run_seq dans tout le fichier (vocabulaire cohérent avec v3).
**
** Format log FORENSIC-UNIF-004 :
**   [ts_mono_ns] [seq=N] [lum_id=0xXXXXXXXXXXXXXXXX] op_name
**   Timestamp principal = CLOCK_MONOTONIC.
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"
#include "../debug/forensic_logger.h"
#include "../common/time_ns.h"
#include "lum_id_schema.h"   /* UNICITE-002 : schéma v3 partagé */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>

/* ── Paramètres ─────────────────────────────────────────────────────────── */

#define UNIF4_GRID_N    4
#define UNIF4_STEPS     10
#define BITS_PER_DOUBLE 64
#define PROTOCOL_UNIF4  2   /* protocol_id = 2 pour UNIF4_NS */

#define MOD_U_IN       0
#define MOD_V_IN       1
#define MOD_P_IN       2
#define MOD_UTMP       3
#define MOD_VTMP       4
#define MOD_U_OUT      5
#define MOD_POISSON    6
#define MOD_COUNT      7

static const char *MOD_NAMES[MOD_COUNT] = {
    "U_IN", "V_IN", "P_IN", "UTMP", "VTMP", "U_OUT", "POISSON_RES"
};

/* ── UNICITE-003 S164 : encode_lum_id_64 / LumIDHashSet / HASH_EMPTY supprimés ─
 *
 * Ces définitions locales (schéma v2) ont été supprimées car :
 *   1. lum_id_schema.h (inclus ligne 41) fournit déjà lum_id_v3_encode() (v3, PC4 fix)
 *      et LumIDHashSetV3 avec sentinelle=0x0 (PC4 fix) et lum_id_v3_new_run_seq()
 *      (PC3 fix fail-hard).
 *   2. L'inclusion du header en S163 était sans effet opérationnel : les fonctions
 *      locales masquaient les fonctions v3 du header (audit 165).
 *   3. HASH_EMPTY=UINT64_MAX est risqué — UINT64_MAX est un LUM_ID v2 théoriquement
 *      possible (run_id=0xFFFF, protocol=0xF, ...). La sentinelle v3 = 0x0 est sûre
 *      car run_seq ≥ 1 → LUM_ID ≥ 0x0001_0000_0000_0000 > 0.
 *   4. make_run_id() XOR n'est pas injective (collision de timestamps possible).
 *      lum_id_v3_new_run_seq() est séquentielle et injective dans la session.
 *
 * Schéma v3 utilisé ici = identique au schéma v2 sauf :
 *   - run_seq séquentiel (v3) au lieu de run_id XOR (v2)
 *   - hash set avec sentinelle=0x0 (v3) au lieu de UINT64_MAX (v2)
 *   - fail-hard sur wrap-around (v3) au lieu de wrap silencieux (v2)
 */

/* ── Couverture par module ───────────────────────────────────────────────── */

typedef struct {
    uint64_t bits_traced;
    uint64_t bits_expected;
    uint64_t ones_count;
    uint64_t zeros_count;
} ModCoverage;

/* ── Contexte global ────────────────────────────────────────────────────── */

typedef struct {
    ModCoverage    mod[MOD_COUNT];
    LumIDHashSetV3 *hs;       /* UNICITE-003 : v3 (sentinelle=0x0, PC4 safe) */
    uint16_t       run_seq;   /* UNICITE-003 : run_seq séquentiel (v3) au lieu de run_id XOR */
    uint64_t       ts_mono_start;
    uint64_t       ts_real_start;
    uint64_t       total_expected;
    uint64_t       total_traced;
    int            steps_done;
    int            pairs_verified;
} Unif4Context;

/* ── Trace 64 bits d'un double ─────────────────────────────────────────────
 *
 * BUG-3 FINAL CLOSE : ts_mono = time_ns_get_monotonic() passé comme timestamp
 * principal. Monotonie stricte garantie par le noyau.
 *
 * UNICITE-003 S164 : migration v2→v3 :
 *   - run_id → run_seq (uint16_t, séquentiel, injective)
 *   - encode_lum_id_64() → lum_id_v3_encode() (schéma v3, sentinelle=0x0)
 *   - LumIDHashSet → LumIDHashSetV3 (PC4 safe)
 *   - hashset_insert() → lum_hashset_v3_insert()
 */
static void trace_double_bits_mono(double value, uint16_t run_seq, int module,
                                    uint16_t step, uint16_t cell_idx,
                                    uint64_t ts_mono,
                                    ModCoverage *cov, LumIDHashSetV3 *hs)
{
    uint64_t raw;
    memcpy(&raw, &value, sizeof(uint64_t));

    char op_buf[64];

    for (int b = 0; b < BITS_PER_DOUBLE; b++) {
        int bit_val = (int)((raw >> b) & 1ULL);

        /* UNICITE-003 : lum_id_v3_encode() — schéma v3 partagé depuis lum_id_schema.h */
        uint64_t lum_id = lum_id_v3_encode(run_seq, PROTOCOL_UNIF4,
                                            module, step, cell_idx, b);

        snprintf(op_buf, sizeof(op_buf), "%s:c%u:val=%d",
                 MOD_NAMES[module], (unsigned)cell_idx, bit_val);
        forensic_log_individual_lum(lum_id, op_buf, ts_mono);

        /* UNICITE-003 : lum_hashset_v3_insert() (sentinelle=0x0, PC4 safe) */
        lum_hashset_v3_insert(hs, lum_id);

        cov->bits_traced++;
        if (bit_val) cov->ones_count++;
        else         cov->zeros_count++;
    }
}

/* ── Helpers trace champs NS ─────────────────────────────────────────────── */
/* UNICITE-003 S164 : signatures mises à jour LumIDHashSet → LumIDHashSetV3,
 * run_id → run_seq. Logique identique — seul le type de hash set change. */

static void trace_field_u(const NSSolver2D *s, uint16_t run_seq, int module,
                           uint16_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_mono(s->u[i*(ny+2)+j], run_seq, module, step, cidx, ts, cov, hs);
}

static void trace_field_v(const NSSolver2D *s, uint16_t run_seq, int module,
                           uint16_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++, cidx++)
            trace_double_bits_mono(s->v[i*(ny+1)+j], run_seq, module, step, cidx, ts, cov, hs);
}

static void trace_field_p(const NSSolver2D *s, uint16_t run_seq, int module,
                           uint16_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_mono(s->p[i*(ny+2)+j], run_seq, module, step, cidx, ts, cov, hs);
}

static void trace_field_utmp(const NSSolver2D *s, uint16_t run_seq, int module,
                              uint16_t step, uint64_t ts, ModCoverage *cov,
                              LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_mono(s->u_tmp[i*(ny+2)+j], run_seq, module, step, cidx, ts, cov, hs);
}

static void trace_field_vtmp(const NSSolver2D *s, uint16_t run_seq, int module,
                              uint16_t step, uint64_t ts, ModCoverage *cov,
                              LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++, cidx++)
            trace_double_bits_mono(s->v_tmp[i*(ny+1)+j], run_seq, module, step, cidx, ts, cov, hs);
}

/* ── Calcul des bits attendus ────────────────────────────────────────────── */

static uint64_t cells_u(int n) { return (uint64_t)(n-1)*(uint64_t)n; }
static uint64_t cells_v(int n) { return (uint64_t)n*(uint64_t)(n-1); }
static uint64_t cells_p(int n) { return (uint64_t)n*(uint64_t)n; }

static void compute_expected(int n, int steps, Unif4Context *ctx)
{
    uint64_t cu   = cells_u(n);
    uint64_t cv   = cells_v(n);
    uint64_t cp   = cells_p(n);
    uint64_t bits = (uint64_t)BITS_PER_DOUBLE;

    ctx->mod[MOD_U_IN   ].bits_expected = (uint64_t)steps * cu * bits;
    ctx->mod[MOD_V_IN   ].bits_expected = (uint64_t)steps * cv * bits;
    ctx->mod[MOD_P_IN   ].bits_expected = (uint64_t)steps * cp * bits;
    ctx->mod[MOD_UTMP   ].bits_expected = (uint64_t)steps * cu * bits;
    ctx->mod[MOD_VTMP   ].bits_expected = (uint64_t)steps * cv * bits;
    ctx->mod[MOD_U_OUT  ].bits_expected = (uint64_t)steps * cu * bits;
    ctx->mod[MOD_POISSON].bits_expected = (uint64_t)steps * 1  * bits;

    ctx->total_expected = 0;
    for (int m = 0; m < MOD_COUNT; m++)
        ctx->total_expected += ctx->mod[m].bits_expected;
}

/* ── UNICITE-003 S164 : make_run_id() XOR supprimée ─────────────────────────
 * Cette fonction calculait run_id = XOR des 4 quarts de 16 bits du timestamp.
 * Problème : non injective — deux timestamps différents peuvent produire le
 * même run_id (ex : ts1 et ts1 XOR masqué). PC3 de l'audit 163 et 165.
 * Remplacée par lum_id_v3_new_run_seq() (séquentielle, injective dans la session).
 */

/* ── Trace un pas NS complet avec timestamps MONOTONIC ──────────────────── */

static void trace_ns_step_unif4(NSSolver2D *s, uint32_t step_n,
                                  Unif4Context *ctx)
{
    uint16_t rseq = ctx->run_seq;  /* UNICITE-003 : run_seq séquentiel */
    uint16_t step = (uint16_t)(step_n & 0xFFFFU);

    uint64_t ts_before = time_ns_get_monotonic();

    trace_field_u(s, rseq, MOD_U_IN,  step, ts_before, &ctx->mod[MOD_U_IN], ctx->hs);
    trace_field_v(s, rseq, MOD_V_IN,  step, ts_before, &ctx->mod[MOD_V_IN], ctx->hs);
    trace_field_p(s, rseq, MOD_P_IN,  step, ts_before, &ctx->mod[MOD_P_IN], ctx->hs);

    double poisson_res = ns_solver_step(s);

    uint64_t ts_after = time_ns_get_monotonic();

    trace_field_utmp(s, rseq, MOD_UTMP, step, ts_after, &ctx->mod[MOD_UTMP], ctx->hs);
    trace_field_vtmp(s, rseq, MOD_VTMP, step, ts_after, &ctx->mod[MOD_VTMP], ctx->hs);
    trace_field_u(s, rseq, MOD_U_OUT,   step, ts_after, &ctx->mod[MOD_U_OUT], ctx->hs);
    trace_double_bits_mono(poisson_res, rseq, MOD_POISSON,
                            step, 0, ts_after, &ctx->mod[MOD_POISSON], ctx->hs);

    ctx->pairs_verified += (int)(cells_u(s->params.nx));
    ctx->steps_done++;
}

/* ── Vérification post-campagne du log (event_seq + ts_monotonic) ─────────── */

static int verify_log_monotonic(const char *log_path, uint64_t total_expected,
                                  uint64_t *out_seq_count, uint64_t *out_gaps,
                                  uint64_t *out_non_monotonic)
{
    FILE *f = fopen(log_path, "r");
    if (!f) {
        fprintf(stderr, "[VERIFY] Cannot open: %s\n", log_path);
        return 0;
    }

    char line[512];
    uint64_t prev_seq  = 0;
    uint64_t prev_ts   = 0;
    uint64_t seq_count = 0;
    uint64_t gap_count = 0;
    uint64_t non_mono  = 0;
    int first = 1;

    while (fgets(line, sizeof(line), f)) {
        char *p_seq = strstr(line, "[seq=");
        if (!p_seq) continue;

        uint64_t seq = 0;
        if (sscanf(p_seq, "[seq=%" SCNu64 "]", &seq) != 1) continue;

        /* Extraire le timestamp monotonic (premier champ entre crochets) */
        uint64_t ts = 0;
        if (sscanf(line, "[%" SCNu64 "]", &ts) != 1) ts = 0;

        if (!first) {
            if (seq != prev_seq + 1) gap_count++;
            if (ts < prev_ts)        non_mono++;   /* régression horloge */
        }
        first    = 0;
        prev_seq = seq;
        prev_ts  = ts;
        seq_count++;
    }
    fclose(f);

    *out_seq_count     = seq_count;
    *out_gaps          = gap_count;
    *out_non_monotonic = non_mono;

    return (seq_count == total_expected) && (gap_count == 0) && (non_mono == 0);
}

/* ── main ────────────────────────────────────────────────────────────────── */

#define LOG_PATH "logs/forensic/ns_forensic_unif4.log"

int main(void)
{
    printf("=== FORENSIC-UNIF-004 : TIMESTAMPS MONOTONIC + UNICITE EXACTE (v3) ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[GRILLE] %d×%d | [STEPS] %d | [MODULES] %d | [BITS/DOUBLE] %d\n\n",
           UNIF4_GRID_N, UNIF4_GRID_N, UNIF4_STEPS, MOD_COUNT, BITS_PER_DOUBLE);
    printf("[BUG-3 FINAL] Événements = CLOCK_MONOTONIC (time_ns_get_monotonic())\n");
    printf("[UNICITE-003] LUM_ID schéma v3 — run_seq séquentiel, sentinelle=0x0\n\n");

    if (!forensic_logger_init(LOG_PATH)) {
        fprintf(stderr, "[UNIF4][ERROR] forensic_logger_init failed\n");
        return 1;
    }

    /* UNICITE-003 : run_seq séquentiel (injective) au lieu de make_run_id() XOR */
    uint16_t run_seq       = lum_id_v3_new_run_seq();
    uint64_t ts_mono_start = time_ns_get_monotonic();
    uint64_t ts_real_start = time_ns_get_absolute();

    printf("[RUN_SEQ]     %u (compteur séquentiel — injective)\n", run_seq);
    printf("[TS_MONO]     %" PRIu64 " ns (CLOCK_MONOTONIC — uptime)\n", ts_mono_start);
    printf("[TS_REAL]     %" PRIu64 " ns (CLOCK_REALTIME  — corrélation civile)\n\n",
           ts_real_start);

    /* ── Contexte ── */
    Unif4Context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.run_seq        = run_seq;  /* UNICITE-003 */
    ctx.ts_mono_start  = ts_mono_start;
    ctx.ts_real_start  = ts_real_start;
    compute_expected(UNIF4_GRID_N, UNIF4_STEPS, &ctx);

    printf("[ATTENDU] %" PRIu64 " bits totaux\n\n", ctx.total_expected);

    /* UNICITE-003 : lum_hashset_v3_create (sentinelle=0x0, PC4 safe) */
    ctx.hs = lum_hashset_v3_create(ctx.total_expected);
    if (!ctx.hs) {
        fprintf(stderr, "[UNIF4][ERROR] lum_hashset_v3_create failed\n");
        forensic_logger_destroy();
        return 1;
    }

    /* ── Solveur NS ── */
    NSParams p = {
        .nx = UNIF4_GRID_N, .ny = UNIF4_GRID_N,
        .lx = 1.0, .ly = 1.0,
        .re = 100.0, .dt = 0.001,
        .max_iter = 1, .tol = 1e-5,
        .max_poisson = 50, .debug = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[UNIF4][ERROR] ns_solver_create failed\n");
        lum_hashset_v3_destroy(ctx.hs);  /* UNICITE-003 : v3 */
        forensic_logger_destroy();
        return 1;
    }
    ns_solver_set_lid_bc(s);

    /* ── Boucle ── */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int step = 0; step < UNIF4_STEPS; step++) {
        /* PC2 FIX S164 : garde step <= LUM_ID_V3_MAX_STEP_VALUE */
        if ((uint32_t)step > (uint32_t)LUM_ID_V3_MAX_STEP_VALUE) {
            fprintf(stderr,
                "[UNIF4][FATAL] step=%d dépasse LUM_ID_V3_MAX_STEP_VALUE=%u"
                " — encodage LUM_ID impossible. Arrêt.\n",
                step, LUM_ID_V3_MAX_STEP_VALUE);
            ns_solver_destroy(s);
            lum_hashset_v3_destroy(ctx.hs);
            forensic_logger_destroy();
            return 1;
        }
        printf("[TRACE] Pas %d/%d ...\n", step + 1, UNIF4_STEPS);
        trace_ns_step_unif4(s, (uint32_t)step, &ctx);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double wall_s = (t1.tv_sec  - t0.tv_sec) +
                    (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    ctx.total_traced = 0;
    for (int m = 0; m < MOD_COUNT; m++)
        ctx.total_traced += ctx.mod[m].bits_traced;

    uint64_t logger_seq = forensic_get_event_seq();
    ns_solver_destroy(s);
    forensic_logger_destroy();

    /* ── Résultats par module ── */
    printf("\n=== RÉSULTATS FORENSIC-UNIF-004 (UNICITE-003 v3) ===\n\n");
    printf("  run_seq         : %u (séquentiel injective)\n", run_seq);
    printf("  event_seq total : %" PRIu64 "\n\n", logger_seq);

    printf("  %-14s | %8s | %8s | %6s | %8s | %8s\n",
           "Module", "Attendu", "Tracé", "Perdu", "1s", "0s");
    printf("  %-14s-+-%8s-+-%8s-+-%6s-+-%8s-+-%8s\n",
           "--------------", "--------", "--------",
           "------", "--------", "--------");

    int all_ok = 1;
    for (int m = 0; m < MOD_COUNT; m++) {
        ModCoverage *mc = &ctx.mod[m];
        uint64_t lost  = (mc->bits_expected >= mc->bits_traced)
                         ? mc->bits_expected - mc->bits_traced : 0;
        int mod_ok = (mc->bits_traced == mc->bits_expected);
        if (!mod_ok) all_ok = 0;
        printf("  %-14s | %8" PRIu64 " | %8" PRIu64 " | %6" PRIu64
               " | %8" PRIu64 " | %8" PRIu64 " %s\n",
               MOD_NAMES[m], mc->bits_expected, mc->bits_traced, lost,
               mc->ones_count, mc->zeros_count, mod_ok ? "OK" : "FAIL");
    }
    printf("  %-14s   %8" PRIu64 "   %8" PRIu64 "   %6" PRIu64 "\n\n",
           "TOTAL", ctx.total_expected, ctx.total_traced,
           (ctx.total_expected >= ctx.total_traced)
               ? ctx.total_expected - ctx.total_traced : 0);

    /* ── Unicité exacte (hash set v3) ── */
    printf("=== UNICITE-003 — HASH SET LUM_ID v3 ===\n\n");
    lum_hashset_v3_print_summary(ctx.hs);
    int uniqueness_pass = lum_hashset_v3_is_unique(ctx.hs, ctx.total_expected);
    printf("  Unicité exacte        : %s\n\n",
           uniqueness_pass ? "PASS — tous les LUM_ID sont distincts (v3, 0 doublon)"
                           : "FAIL — voir doublons ci-dessus");
    lum_hashset_v3_destroy(ctx.hs);

    /* ── Vérification post-campagne du log ── */
    printf("=== VÉRIFICATION LOG (event_seq + monotonie timestamps) ===\n\n");
    uint64_t seq_count = 0, gap_count = 0, non_mono = 0;
    int log_pass = verify_log_monotonic(LOG_PATH, ctx.total_expected,
                                         &seq_count, &gap_count, &non_mono);
    printf("  Événements lus        : %" PRIu64 "\n", seq_count);
    printf("  Gaps séquence (pertes): %" PRIu64 " %s\n",
           gap_count, (gap_count == 0) ? "✓" : "⚠");
    printf("  Régressions timestamp : %" PRIu64 " %s\n",
           non_mono, (non_mono == 0) ? "✓ MONOTONE" : "⚠ NON MONOTONE");

    printf("\n  BUG-3 FINAL CLOSE : timestamps = CLOCK_MONOTONIC : %s\n",
           (non_mono == 0) ? "PROUVÉ" : "ECHEC (voir régressions)");

    printf("\n  Wall time             : %.3f s\n", wall_s);
    printf("  Cohérence event_seq   : %s\n",
           (logger_seq == ctx.total_traced) ? "OK" : "FAIL");

    /* ── Verdict ── */
    int pass = all_ok
               && (ctx.total_traced == ctx.total_expected)
               && (logger_seq == ctx.total_traced)
               && uniqueness_pass
               && log_pass
               && (non_mono == 0);

    printf("\n[VERDICT] FORENSIC-UNIF-004 : %s\n",
           pass ? "PASS — schéma v3, timestamps MONOTONIC, unicité exacte, 0 gap, 0 régression"
                : "FAIL — voir résultats ci-dessus");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[NOTE] run_seq=%u | ts_mono_start=%" PRIu64 " ns\n",
           run_seq, ts_mono_start);

    return pass ? 0 : 1;
}
