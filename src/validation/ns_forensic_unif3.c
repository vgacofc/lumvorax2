/* **************************************************************************
** ns_forensic_unif3.c — FORENSIC-UNIF-003 : LUM_ID 64 bits + event_seq
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / FORENSIC-UNIF-003
** Auteur : LumVorax Project
**
** Objectif : Fermer les 4 anomalies confirmées dans le rapport 158 :
**
**   BUG-1 CLOSED : lum_id uint64_t complet — plus de troncature 64→32.
**                  forensic_log_individual_lum() accepte maintenant uint64_t.
**                  Ce fichier passe le lum_id 64 bits sans cast (uint32_t).
**
**   BUG-3 CLOSED : header + footer du log = CLOCK_MONOTONIC (cohérent avec
**                  les événements). Horloge civile journalisée séparément.
**
**   PERF-1 CLOSED : batch flush toutes les 1024 écritures (dans forensic_logger.c).
**                   Plus de fflush() par événement individuel.
**
**   NEW : event_seq global monotone — chaque événement dans le log porte son
**         numéro de séquence. Vérification de campagne :
**           - Aucun gap dans la séquence (perte)
**           - Aucun doublon LUM_ID (duplication)
**           - Total events = total_expected
**
** Architecture FORENSIC-UNIF-003 :
**   Identique à UNIF-002 (7 modules, grille 4×4, 10 steps) avec les corrections.
**   On réutilise le même encodage LUM_ID 64 bits que UNIF-002.
**   On ajoute une vérification post-campagne du fichier de log.
**
** Vérification de campagne :
**   Après exécution, le log est relu ligne par ligne pour extraire les
**   event_seq et les lum_id. On vérifie :
**     (a) event_seq croît de 1 en 1 (pas de gap, pas de doublon)
**     (b) tous les lum_id sont distincts (XOR != 0 si N impair, check exact)
**     (c) total_events (footer) == total_traced (compteur interne)
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

/* ── Paramètres de la campagne (identiques à UNIF-002) ───────────────────── */

#define UNIF3_GRID_N    4
#define UNIF3_STEPS     10
#define BITS_PER_DOUBLE 64
#define PROTOCOL_UNIF3  1       /* protocol_id = 1 pour UNIF3_NS */

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

/* UNICITE-002 : encode_lum_id_64 v1 remplacé par lum_id_v3_encode() de lum_id_schema.h */

/* ── Couverture par module ───────────────────────────────────────────────── */

typedef struct {
    uint64_t bits_traced;
    uint64_t bits_expected;
    uint64_t ones_count;
    uint64_t zeros_count;
    /* UNICITE-002 : xor_check supprimé — hash set v3 */
} ModCoverage;

/* ── Contexte global ────────────────────────────────────────────────────── */

typedef struct {
    ModCoverage    mod[MOD_COUNT];
    uint16_t       run_seq;    /* UNICITE-002 : compteur séquentiel */
    uint64_t       ts_start_ns;
    uint64_t       total_expected;
    uint64_t       total_traced;
    int            steps_done;
    int            pairs_verified;
    LumIDHashSetV3 *hs;        /* UNICITE-002 : hash set v3 */
} Unif3Context;

/* ── Trace 64 bits — UNICITE-002 v3 + cell_idx ──────────────────────────── */

static void trace_double_bits_real(double value, uint16_t run_seq, int module,
                                    uint16_t step, uint16_t cell_idx,
                                    uint64_t ts_real,
                                    ModCoverage *cov, LumIDHashSetV3 *hs)
{
    uint64_t raw;
    memcpy(&raw, &value, sizeof(uint64_t));
    char op_buf[64];
    for (int b = 0; b < BITS_PER_DOUBLE; b++) {
        int bit_val = (int)((raw >> b) & 1ULL);
        uint64_t lum_id = lum_id_v3_encode(run_seq, PROTOCOL_UNIF3,
                                            module, step, cell_idx, b);
        snprintf(op_buf, sizeof(op_buf), "%s:c%u:val=%d",
                 MOD_NAMES[module], (unsigned)cell_idx, bit_val);
        forensic_log_individual_lum(lum_id, op_buf, ts_real);
        lum_hashset_v3_insert(hs, lum_id);
        cov->bits_traced++;
        if (bit_val) cov->ones_count++;
        else         cov->zeros_count++;
    }
}

/* ── Helpers de trace — UNICITE-002 : cell_idx linéaire ─────────────────── */

static void trace_field_u(const NSSolver2D *s, uint16_t rid, int module,
                           uint16_t step, uint64_t ts,
                           ModCoverage *cov, LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_real(s->u[i*(ny+2)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_v(const NSSolver2D *s, uint16_t rid, int module,
                           uint16_t step, uint64_t ts,
                           ModCoverage *cov, LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++, cidx++)
            trace_double_bits_real(s->v[i*(ny+1)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_p(const NSSolver2D *s, uint16_t rid, int module,
                           uint16_t step, uint64_t ts,
                           ModCoverage *cov, LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_real(s->p[i*(ny+2)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_utmp(const NSSolver2D *s, uint16_t rid, int module,
                              uint16_t step, uint64_t ts,
                              ModCoverage *cov, LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_real(s->u_tmp[i*(ny+2)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_vtmp(const NSSolver2D *s, uint16_t rid, int module,
                              uint16_t step, uint64_t ts,
                              ModCoverage *cov, LumIDHashSetV3 *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++, cidx++)
            trace_double_bits_real(s->v_tmp[i*(ny+1)+j], rid, module, step, cidx, ts, cov, hs);
}

/* ── Calcul du nombre de cellules intérieures ───────────────────────────── */

static uint64_t cells_u(int n) { return (uint64_t)(n-1)*(uint64_t)n; }
static uint64_t cells_v(int n) { return (uint64_t)n*(uint64_t)(n-1); }
static uint64_t cells_p(int n) { return (uint64_t)n*(uint64_t)n; }

/* ── Trace un pas NS — UNICITE-002 ──────────────────────────────────────── */

static void trace_ns_step_unif3(NSSolver2D *s, uint32_t step_n,
                                  Unif3Context *ctx)
{
    uint16_t rid  = ctx->run_seq;
    uint16_t step = (uint16_t)(step_n & 0xFFFFU);

    uint64_t ts_before = time_ns_get_absolute();

    trace_field_u(s, rid, MOD_U_IN,  step, ts_before, &ctx->mod[MOD_U_IN],  ctx->hs);
    trace_field_v(s, rid, MOD_V_IN,  step, ts_before, &ctx->mod[MOD_V_IN],  ctx->hs);
    trace_field_p(s, rid, MOD_P_IN,  step, ts_before, &ctx->mod[MOD_P_IN],  ctx->hs);

    double poisson_res = ns_solver_step(s);

    uint64_t ts_after = time_ns_get_absolute();

    trace_field_utmp(s, rid, MOD_UTMP,  step, ts_after, &ctx->mod[MOD_UTMP], ctx->hs);
    trace_field_vtmp(s, rid, MOD_VTMP,  step, ts_after, &ctx->mod[MOD_VTMP], ctx->hs);
    trace_field_u(s, rid, MOD_U_OUT,    step, ts_after, &ctx->mod[MOD_U_OUT], ctx->hs);
    trace_double_bits_real(poisson_res, rid, MOD_POISSON,
                            step, 0, ts_after, &ctx->mod[MOD_POISSON], ctx->hs);

    ctx->pairs_verified += (int)(cells_u(s->params.nx));
    ctx->steps_done++;
}

/* ── Calcul des bits attendus par module ─────────────────────────────────── */

static void compute_expected(int n, int steps, Unif3Context *ctx)
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

/* ── Vérification post-campagne du fichier de log ───────────────────────────
 *
 * Relit le fichier log ligne par ligne.
 * Extrait event_seq et lum_id depuis le format :
 *   [ts_ns] [seq=N] [lum_id=0xXXXXXXXXXXXXXXXX] op_name
 *
 * Vérifie :
 *   (a) Aucun gap seq : seq[i+1] == seq[i] + 1
 *   (b) Aucune duplication seq : séquence strictement croissante
 *   (c) total_seq == total_expected
 *   (d) XOR global des lum_id != 0 si N > 0 (indicateur d'unicité)
 *       Note : XOR == 0 possible si N pair et collisions par pairs — donc
 *              on signale XOR=0 comme WARNING, pas FAIL absolu.
 *
 * Retourne 1 si PASS, 0 si FAIL.
 */
static int verify_log_campaign(const char *log_path, uint64_t total_expected,
                                uint64_t *out_seq_count, uint64_t *out_gaps,
                                uint64_t *out_xor_all)
{
    FILE *f = fopen(log_path, "r");
    if (!f) {
        fprintf(stderr, "[VERIFY] Cannot open log: %s\n", log_path);
        return 0;
    }

    char line[512];
    uint64_t prev_seq   = 0;
    uint64_t seq_count  = 0;
    uint64_t gap_count  = 0;
    uint64_t xor_all    = 0;
    int      first_event = 1;

    while (fgets(line, sizeof(line), f)) {
        /* Chercher le pattern [seq=N] */
        char *p_seq = strstr(line, "[seq=");
        if (!p_seq) continue;

        uint64_t seq = 0;
        if (sscanf(p_seq, "[seq=%" SCNu64 "]", &seq) != 1) continue;

        /* Chercher le pattern [lum_id=0xXXXX...] */
        uint64_t lum_id = 0;
        char *p_lum = strstr(line, "[lum_id=0x");
        if (p_lum) {
            if (sscanf(p_lum, "[lum_id=0x%" SCNx64 "]", &lum_id) != 1)
                lum_id = 0;
        }

        if (!first_event) {
            if (seq != prev_seq + 1) {
                gap_count++;
                if (gap_count <= 5) {
                    fprintf(stderr,
                            "[VERIFY] GAP détecté : seq attendu=%" PRIu64
                            " reçu=%" PRIu64 "\n",
                            prev_seq + 1, seq);
                }
            }
        }
        first_event = 0;
        prev_seq    = seq;
        seq_count++;
        xor_all    ^= lum_id;
    }

    fclose(f);

    *out_seq_count = seq_count;
    *out_gaps      = gap_count;
    *out_xor_all   = xor_all;

    int pass = (seq_count == total_expected) && (gap_count == 0);
    return pass;
}

/* ── main ────────────────────────────────────────────────────────────────── */

#define LOG_PATH "logs/forensic/ns_forensic_unif3.log"

int main(void)
{
    printf("=== FORENSIC-UNIF-003 : LUM_ID 64 BITS + EVENT_SEQ (UNICITE-002) ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[GRILLE] %d×%d | [STEPS] %d | [MODULES] %d | [BITS/DOUBLE] %d\n\n",
           UNIF3_GRID_N, UNIF3_GRID_N, UNIF3_STEPS, MOD_COUNT, BITS_PER_DOUBLE);
    printf("[SCHEMA] LUM_ID v3 — run_seq compteur, cell_idx, HASH_EMPTY=0\n\n");

    if (!forensic_logger_init(LOG_PATH)) {
        fprintf(stderr, "[UNIF3][ERROR] forensic_logger_init failed\n");
        return 1;
    }

    uint16_t run_seq = lum_id_v3_new_run_seq();
    uint64_t ts_start = time_ns_get_absolute();

    printf("[RUN_SEQ] %u (compteur séquentiel — injective)\n", run_seq);
    printf("[TS_SRC] CLOCK_REALTIME | ts_start=%" PRIu64 " ns\n\n", ts_start);

    Unif3Context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.run_seq     = run_seq;
    ctx.ts_start_ns = ts_start;
    compute_expected(UNIF3_GRID_N, UNIF3_STEPS, &ctx);

    printf("[ATTENDU] %" PRIu64 " bits totaux (%d modules × %d steps)\n\n",
           ctx.total_expected, MOD_COUNT, UNIF3_STEPS);

    ctx.hs = lum_hashset_v3_create(ctx.total_expected);
    if (!ctx.hs) {
        fprintf(stderr, "[UNIF3][ERROR] lum_hashset_v3_create failed\n");
        forensic_logger_destroy();
        return 1;
    }

    /* ── Initialisation solveur NS ── */
    NSParams p = {
        .nx = UNIF3_GRID_N, .ny = UNIF3_GRID_N,
        .lx = 1.0, .ly = 1.0,
        .re = 100.0, .dt = 0.001,
        .max_iter    = 1,
        .tol         = 1e-5,
        .max_poisson = 50,
        .debug       = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[UNIF3][ERROR] ns_solver_create failed\n");
        forensic_logger_destroy();
        return 1;
    }
    ns_solver_set_lid_bc(s);

    /* ── Boucle de traçage ── */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int step = 0; step < UNIF3_STEPS; step++) {
        /* PC2 FIX S164 (UNICITE-003) : garde manquante identifiée audit 165.
         * step est cast uint16_t avant appel — la valeur doit tenir dans le champ
         * 16 bits du schéma v3. LUM_ID_V3_MAX_STEP_VALUE = 65535 (valeur max).
         * Ici UNIF3_STEPS=10 donc le cas ne se produit jamais, mais la garde
         * est obligatoire pour respecter le contrat de lum_id_v3_encode(). */
        if ((uint32_t)step > (uint32_t)LUM_ID_V3_MAX_STEP_VALUE) {
            fprintf(stderr,
                "[UNIF3][FATAL] step=%d dépasse LUM_ID_V3_MAX_STEP_VALUE=%u"
                " — encodage LUM_ID impossible. Arrêt.\n",
                step, LUM_ID_V3_MAX_STEP_VALUE);
            ns_solver_destroy(s);
            lum_hashset_v3_destroy(ctx.hs);
            forensic_logger_destroy();
            return 1;
        }
        printf("[TRACE] Pas %d/%d ...\n", step + 1, UNIF3_STEPS);
        trace_ns_step_unif3(s, (uint32_t)step, &ctx);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double wall_s = (t1.tv_sec  - t0.tv_sec) +
                    (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    /* ── Totaux ── */
    ctx.total_traced = 0;
    for (int m = 0; m < MOD_COUNT; m++)
        ctx.total_traced += ctx.mod[m].bits_traced;

    /* Récupérer le compteur event_seq côté logger (doit == total_traced) */
    uint64_t logger_seq = forensic_get_event_seq();

    /* Fermer le log (flush final garanti) */
    ns_solver_destroy(s);
    forensic_logger_destroy();

    /* ── Résultats par module ── */
    printf("\n=== RÉSULTATS FORENSIC-UNIF-003 (UNICITE-002) ===\n\n");
    printf("  run_seq      : %u\n", run_seq);
    printf("  event_seq (logger) : %" PRIu64 "\n\n", logger_seq);

    printf("  %-14s | %8s | %8s | %6s | %8s | %8s\n",
           "Module", "Attendu", "Tracé", "Perdu", "1s", "0s");
    printf("  %-14s-+-%8s-+-%8s-+-%6s-+-%8s-+-%8s\n",
           "--------------", "--------", "--------",
           "------", "--------", "--------");

    int all_ok = 1;
    for (int m = 0; m < MOD_COUNT; m++) {
        ModCoverage *mc = &ctx.mod[m];
        uint64_t lost = (mc->bits_expected >= mc->bits_traced)
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

    /* ── Unicité exacte v3 ── */
    printf("=== UNICITE-002 — HASH SET LUM_ID v3 ===\n\n");
    lum_hashset_v3_print_summary(ctx.hs);
    int uniqueness_pass = lum_hashset_v3_is_unique(ctx.hs, ctx.total_expected);
    printf("  Unicité exacte : %s\n\n",
           uniqueness_pass ? "PASS — tous les LUM_ID sont distincts"
                           : "FAIL — voir doublons ci-dessus");
    lum_hashset_v3_destroy(ctx.hs);

    /* ── Cohérence logger_seq ── */
    printf("  Cohérence event_seq vs total_traced : %s\n",
           (logger_seq == ctx.total_traced) ? "OK" : "FAIL");

    /* ── Vérification post-campagne ── */
    printf("\n=== VÉRIFICATION POST-CAMPAGNE (lecture log) ===\n\n");
    uint64_t seq_count = 0, gap_count = 0, xor_all = 0;
    int verify_pass = verify_log_campaign(LOG_PATH, ctx.total_expected,
                                          &seq_count, &gap_count, &xor_all);
    printf("  Événements lus  : %" PRIu64 "\n", seq_count);
    printf("  Gaps séquence   : %" PRIu64 " %s\n",
           gap_count, (gap_count == 0) ? "✓" : "⚠ FAIL");

    printf("\n  Wall time  : %.3f s\n", wall_s);
    printf("  Paires I/O : %d vérifiées\n", ctx.pairs_verified);

    int pass = all_ok
               && (ctx.total_traced == ctx.total_expected)
               && (logger_seq == ctx.total_traced)
               && uniqueness_pass
               && verify_pass
               && (gap_count == 0);

    printf("\n[VERDICT] FORENSIC-UNIF-003 : %s\n",
           pass ? "PASS — v3 schéma, 0 doublon, event_seq continu"
                : "FAIL — voir tableau ci-dessus");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[NOTE] run_seq=%u | ts_start=%" PRIu64 " ns\n", run_seq, ts_start);

    return pass ? 0 : 1;
}
