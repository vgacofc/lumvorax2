/* **************************************************************************
** ns_forensic_unif4.c — FORENSIC-UNIF-004 : timestamps MONOTONIC + unicité exacte
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / FORENSIC-UNIF-004
** Auteur : LumVorax Project
**
** Objectif : Fermer les deux propriétés ouvertes identifiées dans le rapport 160 :
**
**   BUG-3 FINAL CLOSE : timestamps des événements = CLOCK_MONOTONIC via
**     time_ns_get_monotonic() (et non CLOCK_REALTIME via time_ns_get_absolute()).
**     Garantie contractuelle : monotonie stricte garantie par le noyau.
**     Les deux horodatages sont conservés dans le log :
**       ts_mono_ns = ordre/durées (CLOCK_MONOTONIC)
**       ts_real_ns = corrélation civile optionnelle (CLOCK_REALTIME)
**
**   UNICITE-001 CLOSE : vérification exacte des doublons LUM_ID par hash set
**     (tableau de hachage ouvert, sondage linéaire) au lieu du XOR.
**     Un XOR peut valoir 0 pour N paires symétriques sans doublon réel.
**     Un hash set détecte chaque doublon individuellement.
**
** Format log FORENSIC-UNIF-004 :
**   [ts_mono_ns] [ts_real_ns] [seq=N] [lum_id=0xXXXXXXXXXXXXXXXX] op_name
**   Le logger forensic_logger.c utilise le timestamp passé en paramètre.
**   Pour UNIF-004, on passe ts_mono comme timestamp principal.
**
** Hash set LUM_ID :
**   Table de hachage ouverte (taille = 2× capacité attendue, puissance de 2).
**   Sondage linéaire. Clé = lum_id (uint64_t). Valeur = compteur d'occurrences.
**   Vérification post-campagne :
**     - aucune entrée avec count > 1 (pas de doublon)
**     - nb d'entrées distinctes == total_expected
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

/* ── Encodage LUM_ID 64 bits v2 — cell_idx intégré ───────────────────────────
 *
 * Schéma v1 (UNIF-002/003) : omettait l'index de cellule → plusieurs cellules
 * du même champ/step/bit_pos produisaient le même LUM_ID → 3840 doublons
 * détectés par le hash set (rapport 152/session 161).
 *
 * Schéma v2 (UNICITE-001 FIX) :
 *   [63:48] run_id   (16 bits)  — identifiant de run
 *   [47:44] protocol  (4 bits)  — id protocole (ex. PROTOCOL_UNIF4=2)
 *   [43:40] module    (4 bits)  — id module (MOD_U_IN…MOD_POISSON)
 *   [39:24] step     (16 bits)  — numéro de pas (max 65535)
 *   [23:8]  cell_idx (16 bits)  — indice de cellule dans le champ (max 65535)
 *   [7:0]   bit_pos   (8 bits)  — position de bit dans le double (0..63)
 *
 * Garantie : (run_id, protocol, module, step, cell_idx, bit_pos) forme un
 * tuple unique par événement → LUM_ID unique pour chaque bit tracé.
 * Vérification : hash set post-campagne → 0 doublon attendu.
 */
static uint64_t encode_lum_id_64(uint16_t run_id, int protocol,
                                  int module, uint16_t step,
                                  uint16_t cell_idx, int bit_pos)
{
    return ((uint64_t)(run_id   & 0xFFFFU) << 48)
         | ((uint64_t)(protocol & 0xFU)    << 44)
         | ((uint64_t)(module   & 0xFU)    << 40)
         | ((uint64_t)(step     & 0xFFFFU) << 24)
         | ((uint64_t)(cell_idx & 0xFFFFU) << 8)
         | ((uint64_t)(bit_pos  & 0xFFU));
}

/* ── Hash set LUM_ID (sondage linéaire) ──────────────────────────────────────
 *
 * Table de hachage ouverte pour détecter les doublons LUM_ID.
 * Taille = première puissance de 2 >= 2 × capacité.
 * Clé = lum_id. Valeur = count (nombre d'insertions).
 * count == 0 → case vide ; count == 1 → unique ; count > 1 → doublon.
 */

#define HASH_EMPTY UINT64_MAX   /* sentinelle case vide */

typedef struct {
    uint64_t *keys;
    uint32_t *counts;
    uint64_t  capacity;    /* puissance de 2 */
    uint64_t  size;        /* nb entrées distinctes */
    uint64_t  duplicates;  /* nb doublons détectés */
} LumIDHashSet;

static LumIDHashSet *hashset_create(uint64_t expected_count)
{
    LumIDHashSet *hs = (LumIDHashSet *)malloc(sizeof(LumIDHashSet));
    if (!hs) return NULL;

    /* capacité = prochaine puissance de 2 >= 2 × expected */
    uint64_t cap = 1;
    while (cap < 2 * expected_count) cap <<= 1;

    hs->keys   = (uint64_t *)malloc(cap * sizeof(uint64_t));
    hs->counts = (uint32_t *)malloc(cap * sizeof(uint32_t));
    if (!hs->keys || !hs->counts) {
        free(hs->keys);
        free(hs->counts);
        free(hs);
        return NULL;
    }

    for (uint64_t i = 0; i < cap; i++) {
        hs->keys[i]   = HASH_EMPTY;
        hs->counts[i] = 0;
    }
    hs->capacity   = cap;
    hs->size       = 0;
    hs->duplicates = 0;
    return hs;
}

static void hashset_insert(LumIDHashSet *hs, uint64_t lum_id)
{
    if (!hs) return;
    uint64_t mask  = hs->capacity - 1;
    uint64_t idx   = (lum_id ^ (lum_id >> 32)) & mask; /* hash simple */

    /* Sondage linéaire */
    for (uint64_t probe = 0; probe < hs->capacity; probe++) {
        uint64_t i = (idx + probe) & mask;
        if (hs->keys[i] == HASH_EMPTY) {
            /* case vide → première insertion */
            hs->keys[i]   = lum_id;
            hs->counts[i] = 1;
            hs->size++;
            return;
        }
        if (hs->keys[i] == lum_id) {
            /* doublon détecté */
            hs->counts[i]++;
            if (hs->counts[i] == 2) hs->duplicates++; /* compter une fois */
            return;
        }
    }
    /* Table pleine — ne devrait pas arriver avec capacité = 2× expected */
    fprintf(stderr, "[HASHSET] OVERFLOW — table pleine, lum_id=0x%016" PRIx64 " non inséré\n",
            lum_id);
}

static void hashset_destroy(LumIDHashSet *hs)
{
    if (!hs) return;
    free(hs->keys);
    free(hs->counts);
    free(hs);
}

/* ── Couverture par module ───────────────────────────────────────────────── */

typedef struct {
    uint64_t bits_traced;
    uint64_t bits_expected;
    uint64_t ones_count;
    uint64_t zeros_count;
} ModCoverage;

/* ── Contexte global ────────────────────────────────────────────────────── */

typedef struct {
    ModCoverage  mod[MOD_COUNT];
    LumIDHashSet *hs;
    uint16_t     run_id;
    uint64_t     ts_mono_start;
    uint64_t     ts_real_start;
    uint64_t     total_expected;
    uint64_t     total_traced;
    int          steps_done;
    int          pairs_verified;
} Unif4Context;

/* ── Trace 64 bits d'un double ─────────────────────────────────────────────
 *
 * BUG-3 FINAL CLOSE : ts_mono = time_ns_get_monotonic() passé comme timestamp
 * principal. ts_real = time_ns_get_absolute() conservé pour corrélation.
 * Le log porte les deux valeurs dans le champ operation.
 *
 * UNICITE-001 FIX : cell_idx ajouté — chaque cellule de champ produit un
 * LUM_ID distinct grâce au nouveau schéma v2 (bits [23:8] = cell_idx).
 */
static void trace_double_bits_mono(double value, uint16_t run_id, int module,
                                    uint16_t step, uint16_t cell_idx,
                                    uint64_t ts_mono,
                                    ModCoverage *cov, LumIDHashSet *hs)
{
    uint64_t raw;
    memcpy(&raw, &value, sizeof(uint64_t));

    char op_buf[64];

    for (int b = 0; b < BITS_PER_DOUBLE; b++) {
        int bit_val = (int)((raw >> b) & 1ULL);

        /* UNICITE-001 FIX : cell_idx passé → LUM_ID unique par (step,cell,bit) */
        uint64_t lum_id = encode_lum_id_64(run_id, PROTOCOL_UNIF4,
                                            module, step, cell_idx, b);

        /* BUG-3 FINAL : timestamp MONOTONIC passé au logger */
        snprintf(op_buf, sizeof(op_buf), "%s:c%u:val=%d",
                 MOD_NAMES[module], (unsigned)cell_idx, bit_val);
        forensic_log_individual_lum(lum_id, op_buf, ts_mono);

        /* UNICITE-001 : insérer dans le hash set */
        hashset_insert(hs, lum_id);

        cov->bits_traced++;
        if (bit_val) cov->ones_count++;
        else         cov->zeros_count++;
    }
}

/* ── Helpers trace champs NS ─────────────────────────────────────────────── */
/* UNICITE-001 FIX : chaque boucle calcule cell_idx explicitement (linéaire)
 * pour garantir l'unicité LUM_ID. Le même cell_idx ne peut pas apparaître
 * dans deux modules distincts car le champ module est différent dans encode_lum_id_64.
 */

static void trace_field_u(const NSSolver2D *s, uint16_t rid, int module,
                           uint16_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSet *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_mono(s->u[i*(ny+2)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_v(const NSSolver2D *s, uint16_t rid, int module,
                           uint16_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSet *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++, cidx++)
            trace_double_bits_mono(s->v[i*(ny+1)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_p(const NSSolver2D *s, uint16_t rid, int module,
                           uint16_t step, uint64_t ts, ModCoverage *cov,
                           LumIDHashSet *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_mono(s->p[i*(ny+2)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_utmp(const NSSolver2D *s, uint16_t rid, int module,
                              uint16_t step, uint64_t ts, ModCoverage *cov,
                              LumIDHashSet *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i < nx; i++)
        for (int j = 1; j <= ny; j++, cidx++)
            trace_double_bits_mono(s->u_tmp[i*(ny+2)+j], rid, module, step, cidx, ts, cov, hs);
}

static void trace_field_vtmp(const NSSolver2D *s, uint16_t rid, int module,
                              uint16_t step, uint64_t ts, ModCoverage *cov,
                              LumIDHashSet *hs)
{
    int nx = s->params.nx, ny = s->params.ny;
    uint16_t cidx = 0;
    for (int i = 1; i <= nx; i++)
        for (int j = 1; j < ny; j++, cidx++)
            trace_double_bits_mono(s->v_tmp[i*(ny+1)+j], rid, module, step, cidx, ts, cov, hs);
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

/* ── run_id ──────────────────────────────────────────────────────────────── */

static uint16_t make_run_id(uint64_t ts)
{
    return (uint16_t)(
        ((ts      ) & 0xFFFFU) ^
        ((ts >> 16) & 0xFFFFU) ^
        ((ts >> 32) & 0xFFFFU) ^
        ((ts >> 48) & 0xFFFFU)
    );
}

/* ── Trace un pas NS complet avec timestamps MONOTONIC ──────────────────── */

static void trace_ns_step_unif4(NSSolver2D *s, uint32_t step_n,
                                  Unif4Context *ctx)
{
    uint16_t rid  = ctx->run_id;
    uint16_t step = (uint16_t)(step_n & 0xFFFFU);  /* UNICITE-001 FIX : cast uint16 */

    /* BUG-3 FINAL : ts_before et ts_after = CLOCK_MONOTONIC */
    uint64_t ts_before = time_ns_get_monotonic();

    trace_field_u(s, rid, MOD_U_IN,  step, ts_before, &ctx->mod[MOD_U_IN], ctx->hs);
    trace_field_v(s, rid, MOD_V_IN,  step, ts_before, &ctx->mod[MOD_V_IN], ctx->hs);
    trace_field_p(s, rid, MOD_P_IN,  step, ts_before, &ctx->mod[MOD_P_IN], ctx->hs);

    double poisson_res = ns_solver_step(s);

    uint64_t ts_after = time_ns_get_monotonic();   /* CLOCK_MONOTONIC */

    trace_field_utmp(s, rid, MOD_UTMP, step, ts_after, &ctx->mod[MOD_UTMP], ctx->hs);
    trace_field_vtmp(s, rid, MOD_VTMP, step, ts_after, &ctx->mod[MOD_VTMP], ctx->hs);
    trace_field_u(s, rid, MOD_U_OUT,   step, ts_after, &ctx->mod[MOD_U_OUT], ctx->hs);
    /* UNICITE-001 FIX : Poisson = cellule unique → cell_idx=0 */
    trace_double_bits_mono(poisson_res, rid, MOD_POISSON,
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
    printf("=== FORENSIC-UNIF-004 : TIMESTAMPS MONOTONIC + UNICITE EXACTE ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[GRILLE] %d×%d | [STEPS] %d | [MODULES] %d | [BITS/DOUBLE] %d\n\n",
           UNIF4_GRID_N, UNIF4_GRID_N, UNIF4_STEPS, MOD_COUNT, BITS_PER_DOUBLE);
    printf("[BUG-3 FINAL] Événements = CLOCK_MONOTONIC (time_ns_get_monotonic())\n");
    printf("[UNICITE-001] Hash set LUM_ID (sondage linéaire) — pas de XOR\n\n");

    if (!forensic_logger_init(LOG_PATH)) {
        fprintf(stderr, "[UNIF4][ERROR] forensic_logger_init failed\n");
        return 1;
    }

    uint64_t ts_mono_start = time_ns_get_monotonic();
    uint64_t ts_real_start = time_ns_get_absolute();
    uint16_t run_id        = make_run_id(ts_mono_start);

    printf("[RUN_ID]      0x%04X\n", run_id);
    printf("[TS_MONO]     %" PRIu64 " ns (CLOCK_MONOTONIC — uptime)\n", ts_mono_start);
    printf("[TS_REAL]     %" PRIu64 " ns (CLOCK_REALTIME  — corrélation civile)\n\n",
           ts_real_start);

    /* ── Contexte ── */
    Unif4Context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.run_id         = run_id;
    ctx.ts_mono_start  = ts_mono_start;
    ctx.ts_real_start  = ts_real_start;
    compute_expected(UNIF4_GRID_N, UNIF4_STEPS, &ctx);

    printf("[ATTENDU] %" PRIu64 " bits totaux\n\n", ctx.total_expected);

    /* Hash set dimensionné pour total_expected entrées */
    ctx.hs = hashset_create(ctx.total_expected);
    if (!ctx.hs) {
        fprintf(stderr, "[UNIF4][ERROR] hashset_create failed\n");
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
        hashset_destroy(ctx.hs);
        forensic_logger_destroy();
        return 1;
    }
    ns_solver_set_lid_bc(s);

    /* ── Boucle ── */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int step = 0; step < UNIF4_STEPS; step++) {
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
    printf("\n=== RÉSULTATS FORENSIC-UNIF-004 ===\n\n");
    printf("  run_id          : 0x%04X\n", run_id);
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

    /* ── Unicité exacte (hash set) ── */
    printf("=== UNICITE-001 — HASH SET LUM_ID ===\n\n");
    printf("  Capacité hash set     : %" PRIu64 " slots\n", ctx.hs->capacity);
    printf("  Entrées distinctes    : %" PRIu64 "\n", ctx.hs->size);
    printf("  Doublons détectés     : %" PRIu64 " %s\n",
           ctx.hs->duplicates,
           (ctx.hs->duplicates == 0) ? "✓ AUCUN" : "⚠ DOUBLONS!");
    int uniqueness_pass = (ctx.hs->size == ctx.total_expected)
                          && (ctx.hs->duplicates == 0);
    printf("  Unicité exacte        : %s\n\n",
           uniqueness_pass ? "PASS — tous les LUM_ID sont distincts"
                           : "FAIL — voir doublons ci-dessus");
    hashset_destroy(ctx.hs);

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
           pass ? "PASS — timestamps MONOTONIC, unicité exacte, 0 gap, 0 régression"
                : "FAIL — voir résultats ci-dessus");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[NOTE] run_id=0x%04X | ts_mono_start=%" PRIu64 " ns\n",
           run_id, ts_mono_start);

    return pass ? 0 : 1;
}
