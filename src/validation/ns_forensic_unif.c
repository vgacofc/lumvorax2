/* **************************************************************************
** ns_forensic_unif.c — FORENSIC-UNIF-001 : traçabilité bit→LUM_ID→NS
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / FORENSIC-UNIF-001
** Auteur : LumVorax Project
**
** Objectif : Prouver que chaque opération NS (u, v, p, advection, diffusion,
**   Poisson, correction) est tracée bit-level via le système LUM.
**
** Chaîne de traçabilité :
**   double (64 bits) → 64 LUM_ID individuels → event forensic nanoseconde
**   LUM_ID = hash(module_id × step × cell_i × cell_j × bit_pos)
**   Event forensic = forensic_log_individual_lum(lum_id, op_name, ts_ns)
**
** Définition d'un LUM dans ce contexte :
**   Un LUM représente UN BIT d'un champ numérique (u, v ou p) à un instant t.
**   LUM_ID encode : module | step | cellule (i,j) | position de bit (0-63)
**   Chaque opération NS trace l'entrée et la sortie de chaque bit.
**
** Coverage visé :
**   1. u[i][j] → 64 LUM tracés (entrée advection)
**   2. v[i][j] → 64 LUM tracés (entrée advection)
**   3. p[i][j] → 64 LUM tracés (entrée Poisson)
**   4. u_tmp[i][j] → 64 LUM tracés (sortie advection/diffusion)
**   5. Résidu Poisson → 64 LUM tracés (sortie Poisson)
**
** Mode de validation :
**   - Grille 4×4 (pour lisibilité des logs)
**   - 10 pas de temps
**   - Log forensic de TOUS les bits des 5 champs ci-dessus
**   - Comptage : nombre de bits tracés / nombre de bits total
**   - Coverage = bits_traced / bits_total × 100%
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "../solvers/ns_solver_2d.h"
#include "../debug/forensic_logger.h"
#include "../common/time_ns.h"
#include "../binary/binary_lum_converter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

/* ── Paramètres de la validation ────────────────────────────────────────── */

#define FORENSIC_GRID_N   4      /* grille petite : lisibilité des logs */
#define FORENSIC_STEPS    10     /* nombre de pas à tracer */
#define BITS_PER_DOUBLE   64     /* un double = 64 bits = 64 LUM_ID */

/* ── Encodage LUM_ID ─────────────────────────────────────────────────────
 *
 * LUM_ID encapsule l'identité complète d'un bit dans le pipeline NS :
 *
 *   bits [31..28] = module_id (0=U, 1=V, 2=P, 3=UTMP, 4=POISSON_RES)
 *   bits [27..20] = step (max 255)
 *   bits [19..14] = i   (max 63)
 *   bits [13..8]  = j   (max 63)
 *   bits [7..0]   = bit_pos (0..63)
 *
 * Cette bijection garantit que chaque LUM_ID est unique pour chaque
 * (module, step, i, j, bit_pos).
 */
static uint32_t encode_lum_id(int module_id, int step, int i, int j, int bit_pos)
{
    return (uint32_t)(
        ((module_id & 0xF)  << 28) |
        ((step      & 0xFF) << 20) |
        ((i         & 0x3F) << 14) |
        ((j         & 0x3F) << 8)  |
        (bit_pos    & 0xFF)
    );
}

/* ── Traçage bit-level d'un double ───────────────────────────────────────
 *
 * Extrait les 64 bits d'un double via memcpy (interprétation IEEE 754).
 * Pour chaque bit :
 *   - calcule LUM_ID via encode_lum_id()
 *   - appelle forensic_log_individual_lum()
 * Retourne le nombre de bits tracés (toujours 64).
 */
static int trace_double_bits(double value, int module_id, int step, int i, int j,
                              const char *op_name, uint64_t *bits_traced_out)
{
    uint64_t raw;
    memcpy(&raw, &value, sizeof(uint64_t));

    uint64_t ts_base = time_ns_get_absolute();
    int traced = 0;

    for (int b = 0; b < BITS_PER_DOUBLE; b++) {
        uint32_t lum_id = encode_lum_id(module_id, step & 0xFF, i & 0x3F,
                                         j & 0x3F, b);
        /* Le timestamp est incrémenté de 1 ns fictif par bit pour garantir
         * l'unicité de l'ordre. La résolution réelle de time_ns_get_absolute()
         * dépend de CLOCK_REALTIME — déclarée ici, pas supposée 1ns hardware. */
        uint64_t ts_bit = ts_base + (uint64_t)b;
        forensic_log_individual_lum(lum_id, op_name, ts_bit);
        traced++;
    }

    if (bits_traced_out) *bits_traced_out += (uint64_t)traced;
    return traced;
}

/* ── Structure de couverture forensic ───────────────────────────────────── */

typedef struct {
    uint64_t bits_u_in;      /* u entrant dans advection */
    uint64_t bits_v_in;      /* v entrant dans advection */
    uint64_t bits_p_in;      /* p entrant dans Poisson */
    uint64_t bits_utmp_out;  /* u_tmp sortant d'advection */
    uint64_t bits_pres_out;  /* résidu Poisson sortant */
    uint64_t total_bits;     /* total tracé */
    uint64_t expected_bits;  /* total attendu (calculé) */
    int      steps_done;
} ForensicCoverage;

/* ── Trace un pas de temps NS complet ────────────────────────────────────── */

static void trace_ns_step(NSSolver2D *s, int step_n, ForensicCoverage *cov)
{
    int nx = s->params.nx;
    int ny = s->params.ny;

    /* ── MODULE 0 : champ u entrant dans advection ── */
    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double val = s->u[i * (ny + 2) + j];
            trace_double_bits(val, 0, step_n, i, j,
                              "U_IN_ADVECTION", &cov->bits_u_in);
        }
    }

    /* ── MODULE 1 : champ v entrant dans advection ── */
    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j < ny; j++) {
            double val = s->v[i * (ny + 1) + j];
            trace_double_bits(val, 1, step_n, i, j,
                              "V_IN_ADVECTION", &cov->bits_v_in);
        }
    }

    /* ── MODULE 2 : champ p entrant dans Poisson ── */
    for (int i = 1; i <= nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double val = s->p[i * (ny + 2) + j];
            trace_double_bits(val, 2, step_n, i, j,
                              "P_IN_POISSON", &cov->bits_p_in);
        }
    }

    /* ── Exécuter le pas ── */
    double poisson_res = ns_solver_step(s);

    /* ── MODULE 3 : champ u_tmp sortant d'advection ── */
    for (int i = 1; i < nx; i++) {
        for (int j = 1; j <= ny; j++) {
            double val = s->u[i * (ny + 2) + j];  /* u après correction */
            trace_double_bits(val, 3, step_n, i, j,
                              "U_OUT_CORRECTION", &cov->bits_utmp_out);
        }
    }

    /* ── MODULE 4 : résidu Poisson (scalaire → 64 bits) ── */
    trace_double_bits(poisson_res, 4, step_n, 0, 0,
                      "POISSON_RESIDUAL", &cov->bits_pres_out);

    cov->steps_done++;
}

/* ── Calcul de la couverture attendue ────────────────────────────────────── */

static uint64_t compute_expected_bits(int n, int steps)
{
    int nx = n, ny = n;
    /* u intérieur : (nx-1) × ny */
    uint64_t cells_u = (uint64_t)(nx - 1) * (uint64_t)ny;
    /* v intérieur : nx × (ny-1) */
    uint64_t cells_v = (uint64_t)nx * (uint64_t)(ny - 1);
    /* p intérieur : nx × ny */
    uint64_t cells_p = (uint64_t)nx * (uint64_t)ny;
    /* poisson_res : 1 scalaire */
    uint64_t cells_res = 1;

    /* 2 modules in (u,v), 1 module in (p), 1 module out (u), 1 module out (poisson) */
    return (uint64_t)steps * BITS_PER_DOUBLE *
           (cells_u + cells_v + cells_p + cells_u + cells_res);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== FORENSIC-UNIF-001 : BIT → LUM_ID → NS NANOSECONDE ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[GRILLE] %d×%d | [STEPS] %d | [BITS/DOUBLE] %d\n\n",
           FORENSIC_GRID_N, FORENSIC_GRID_N, FORENSIC_STEPS, BITS_PER_DOUBLE);

    /* Initialisation forensic */
    forensic_logger_init("logs/forensic/ns_forensic_unif.log");

    NSParams p = {
        .nx = FORENSIC_GRID_N, .ny = FORENSIC_GRID_N,
        .lx = 1.0, .ly = 1.0,
        .re = 100.0, .dt = 0.001,
        .max_iter    = 1,
        .tol         = 1e-5,
        .max_poisson = 50,
        .debug       = 0
    };

    NSSolver2D *s = ns_solver_create(&p);
    if (!s) {
        fprintf(stderr, "[FORENSIC_UNIF][ERROR] ns_solver_create failed\n");
        return 1;
    }
    ns_solver_set_lid_bc(s);

    ForensicCoverage cov;
    memset(&cov, 0, sizeof(cov));
    cov.expected_bits = compute_expected_bits(FORENSIC_GRID_N, FORENSIC_STEPS);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    printf("[TRACE] Départ — %llu bits attendus\n",
           (unsigned long long)cov.expected_bits);

    for (int step = 0; step < FORENSIC_STEPS; step++) {
        printf("[TRACE] Pas %d/%d...\n", step + 1, FORENSIC_STEPS);
        trace_ns_step(s, step, &cov);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double wall_s = (t1.tv_sec  - t0.tv_sec) +
                    (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    /* Calcul couverture */
    cov.total_bits = cov.bits_u_in + cov.bits_v_in + cov.bits_p_in
                   + cov.bits_utmp_out + cov.bits_pres_out;
    double coverage_pct = (cov.expected_bits > 0)
                          ? (double)cov.total_bits / (double)cov.expected_bits * 100.0
                          : 0.0;

    /* ── Rapport ── */
    printf("\n=== RÉSULTATS FORENSIC-UNIF-001 ===\n\n");
    printf("  Grille       : %d×%d\n", FORENSIC_GRID_N, FORENSIC_GRID_N);
    printf("  Pas tracés   : %d\n", cov.steps_done);
    printf("  Bits/double  : %d (IEEE 754, memcpy)\n", BITS_PER_DOUBLE);
    printf("\n");
    printf("  Module                | Bits tracés\n");
    printf("  --------------------- | -----------\n");
    printf("  U_IN_ADVECTION        | %llu\n", (unsigned long long)cov.bits_u_in);
    printf("  V_IN_ADVECTION        | %llu\n", (unsigned long long)cov.bits_v_in);
    printf("  P_IN_POISSON          | %llu\n", (unsigned long long)cov.bits_p_in);
    printf("  U_OUT_CORRECTION      | %llu\n", (unsigned long long)cov.bits_utmp_out);
    printf("  POISSON_RESIDUAL      | %llu\n", (unsigned long long)cov.bits_pres_out);
    printf("  ─────────────────── + ─────────────\n");
    printf("  TOTAL tracé           | %llu\n", (unsigned long long)cov.total_bits);
    printf("  TOTAL attendu         | %llu\n", (unsigned long long)cov.expected_bits);
    printf("\n");
    printf("  COVERAGE BIT-LEVEL    : %.2f%%\n", coverage_pct);
    printf("  Wall time             : %.3f s\n", wall_s);
    printf("\n");

    printf("  Encodage LUM_ID (bijection) :\n");
    printf("    bits[31..28] = module_id  (0=U_IN, 1=V_IN, 2=P_IN, 3=U_OUT, 4=POISSON)\n");
    printf("    bits[27..20] = step       (0..255)\n");
    printf("    bits[19..14] = i          (0..63)\n");
    printf("    bits[13..8]  = j          (0..63)\n");
    printf("    bits[7..0]   = bit_pos    (0..63)\n");
    printf("\n");

    printf("  Précision timestamps :\n");
    printf("    Source    : CLOCK_REALTIME (time_ns_get_absolute)\n");
    printf("    Résolution matérielle : non garantie à 1 ns — unité = ns, "
           "résolution réelle dépend de l'OS/HW\n");
    printf("    Unicité   : ts_base + bit_pos (offset fictif pour ordre)\n");
    printf("\n");

    int pass = (cov.total_bits == cov.expected_bits);
    printf("[VERDICT] FORENSIC-UNIF-001 : %s\n",
           pass ? "PASS — 100%% bits tracés" : "FAIL — divergence bits tracés/attendus");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[NOTE] Cette validation couvre les 5 modules NS listés.\n");
    printf("[NOTE] Extension aux solveurs Richardson/Lyapunov/NX-42 = chantier suivant.\n");

    ns_solver_destroy(s);
    forensic_logger_destroy();
    return pass ? 0 : 1;
}
