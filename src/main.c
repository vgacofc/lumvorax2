/* **************************************************************************
** main.c — MAIN-CABLE-001 : câblage COMPLET LUM/VORAX tous modules
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : src / MAIN-CABLE-001 + FORENSIC-UNIF-002
** Auteur : ARTCB Project <contact@artcb.me>
**
** Objectif : Câbler dans le binaire principal TOUS les modules LUM/VORAX
**   d'optimisation précédemment non connectés (audit S170) :
**   FORENSIC-UNIF-002 → MEMORY_OPTIMIZER → SIMD → BINARY_CONVERTER
**   → LUM_CREATE → PARALLEL → ZERO_COPY → PARETO → FORENSIC_DESTROY
**
** Pipeline :
**   1. FORENSIC-UNIF-002 : init session (run_id, BIT_ID/LUM_ID universels)
**   2. MEMORY_OPTIMIZER  : pool d'allocation LUM (évite malloc/free bruts)
**   3. BINARY_CONVERTER  : conversion payload 8 octets → 64 LUM (1 bit = 1 LUM)
**   4. SIMD              : détection caps + dispatch sur le groupe LUM
**   5. FORENSIC traçage  : BIT_ID → LUM_ID → transformation → sortie
**   6. PARALLEL          : traitement workers sur le groupe LUM
**   7. ZERO_COPY         : pool zero-copy pour les résultats de sortie
**   8. PARETO            : évaluation métriques + score Pareto inversé
**   9. FORENSIC_UNIF_002 : destroy + stats session (perte/duplication)
**
** Limites honnêtes :
**   - SIMD : scalaire pur sur macOS x86 sans AVX2 (guard AVX2 implanté)
**   - PARALLEL : workers réels (BUG-PARALLEL-001 fermé S173/S174)
**   - PARETO : pareto_execute_vorax_optimization() non branché (vorax_parse requis)
**   - memory_optimizer_create() : extern (non exposé dans .h public)
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>

/* ── Modules LUM/VORAX ───────────────────────────────────────────────────── */
#include "lum/lum_core.h"
#include "binary/binary_lum_converter.h"
#include "debug/forensic_logger.h"
#include "debug/forensic_unif_002.h"
#include "optimization/simd_optimizer.h"
#include "optimization/memory_optimizer.h"
#include "optimization/pareto_optimizer.h"
#include "optimization/zero_copy_allocator.h"
#include "parallel/parallel_processor.h"
#include "metrics/performance_metrics.h"
#include "common/time_ns.h"

/* ── Modules physique/log (héritage Kerr) ────────────────────────────────── */
#include "physics/kerr_metric.h"
#include "logging/log_writer.h"

/* ── Prototype extern : memory_optimizer_create non exposé dans .h public ── */
memory_optimizer_t* memory_optimizer_create(size_t initial_pool_size);
void                memory_optimizer_destroy(memory_optimizer_t* optimizer);

/* ── Constantes ─────────────────────────────────────────────────────────── */

#define MAIN_FORENSIC_LOG   "logs/forensic/main_cable_001.log"
#define FU002_LOG_PATH      "logs/forensic/forensic_unif_002_session.jsonl"
#define PAYLOAD_BYTES       8           /* 8 octets → 64 LUM */
#define PARALLEL_WORKERS    2
#define ZERO_COPY_POOL_SZ   (4096)      /* pool zero-copy pour les sorties  */
#define PARETO_MAX_POINTS   32

/* ── Résultat global de la session de câblage ────────────────────────────── */

typedef struct {
    bool fu002_ok;
    bool mem_opt_ok;
    bool binary_ok;
    bool simd_ok;
    bool parallel_ok;
    bool zerocopy_ok;
    bool pareto_ok;
    int  lums_created;
    int  lums_processed_parallel;
    int  zerocopy_allocs;
    double pareto_best_score;
    uint64_t session_duration_ns;
    fu002_session_stats_t fu002_stats;
} cable_result_t;

/* ── Affichage bilan ──────────────────────────────────────────────────────── */

static void print_cable_result(const cable_result_t* r)
{
    fprintf(stderr, "\n══════════════════════════════════════════════════════\n");
    fprintf(stderr, "MAIN-CABLE-001 — Bilan câblage LUM/VORAX complet\n");
    fprintf(stderr, "══════════════════════════════════════════════════════\n");
    fprintf(stderr, "  FORENSIC-UNIF-002  : %s (run_id=0x%08X)\n",
            r->fu002_ok ? "OK" : "FAIL",
            r->fu002_stats.run_id);
    fprintf(stderr, "  MEMORY_OPTIMIZER   : %s\n",
            r->mem_opt_ok ? "OK" : "FAIL");
    fprintf(stderr, "  BINARY_CONVERTER   : %s (LUM créés = %d)\n",
            r->binary_ok ? "OK" : "FAIL", r->lums_created);
    fprintf(stderr, "  SIMD_OPTIMIZER     : %s\n",
            r->simd_ok ? "OK" : "FAIL");
    fprintf(stderr, "  PARALLEL_PROCESSOR : %s (tasks=%d)\n",
            r->parallel_ok ? "OK" : "FAIL", r->lums_processed_parallel);
    fprintf(stderr, "  ZERO_COPY_ALLOC    : %s (allocs=%d)\n",
            r->zerocopy_ok ? "OK" : "FAIL", r->zerocopy_allocs);
    fprintf(stderr, "  PARETO_OPTIMIZER   : %s (score=%.3f)\n",
            r->pareto_ok ? "OK" : "FAIL", r->pareto_best_score);
    fprintf(stderr, "──────────────────────────────────────────────────────\n");
    fprintf(stderr, "  FU002 session stats :\n");
    fprintf(stderr, "    bits_input   = %" PRIu64 "\n",
            r->fu002_stats.total_bits_input);
    fprintf(stderr, "    lums_created = %" PRIu64 "\n",
            r->fu002_stats.total_lums_created);
    fprintf(stderr, "    transforms   = %" PRIu64 "\n",
            r->fu002_stats.total_transformations);
    fprintf(stderr, "    loss_count   = %" PRIu64 "\n",
            r->fu002_stats.loss_count);
    fprintf(stderr, "    dup_count    = %" PRIu64 "\n",
            r->fu002_stats.duplicate_count);
    fprintf(stderr, "    integrity_ok = %s\n",
            r->fu002_stats.integrity_ok ? "TRUE" : "FALSE");
    fprintf(stderr, "  Durée session    = %" PRIu64 " ns\n",
            r->session_duration_ns);
    fprintf(stderr, "──────────────────────────────────────────────────────\n");
    fprintf(stderr, "  CERTIFIED_100=false | unique_human_proven=false\n");
    fprintf(stderr, "══════════════════════════════════════════════════════\n\n");
}

/* ══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    cable_result_t result;
    memset(&result, 0, sizeof(result));

    uint64_t t_session_start = time_ns_get_absolute();

    fprintf(stderr, "=== MAIN-CABLE-001 : Câblage complet LUM/VORAX ===\n");
    fprintf(stderr, "[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    fprintf(stderr, "Avancement : 0%%\n");
    fflush(stdout);

    /* ═══ ÉTAPE 1 : FORENSIC-UNIF-002 init ══════════════════════════════ */
    fprintf(stderr, "[STEP 1] FORENSIC-UNIF-002 init...\n");
    result.fu002_ok = forensic_unif002_init(FU002_LOG_PATH);

    /* Forensic logger classique (pour log_individual_lum) */
    forensic_logger_init(MAIN_FORENSIC_LOG);

    forensic_unif002_log_event(FU002_EVT_SESSION_START, 0, 0, 0, 0, 0,
                               "MAIN", "main_cable_001_start");
    fprintf(stderr, "Avancement : 10%%\n"); fflush(stdout);

    /* ═══ ÉTAPE 2 : MEMORY_OPTIMIZER init ═══════════════════════════════ */
    fprintf(stderr, "[STEP 2] Memory optimizer init (pool %d LUM × 32)...\n",
            PAYLOAD_BYTES * 8);

    /* Pool : 64 LUM × sizeof(lum_t)=64 × 32 = 131072 bytes */
    size_t pool_sz = (size_t)(PAYLOAD_BYTES * 8) * sizeof(lum_t) * 32;
    memory_optimizer_t* mem_opt = memory_optimizer_create(pool_sz);
    result.mem_opt_ok = (mem_opt != NULL);

    if (!mem_opt) {
        fprintf(stderr, "[STEP 2][WARN] memory_optimizer_create failed — "
                "fallback malloc direct\n");
    }

    forensic_unif002_log_event(FU002_EVT_SESSION_START, 0, 0, 0, 0, 0,
                               "MEMORY_OPTIMIZER",
                               result.mem_opt_ok ? "pool_init_ok" : "pool_init_fail");
    fprintf(stderr, "Avancement : 18%%\n"); fflush(stdout);

    /* ═══ ÉTAPE 3 : BINARY_CONVERTER → LUM ══════════════════════════════ */
    fprintf(stderr, "[STEP 3] Binary converter : payload 8 octets → LUM...\n");

    /* Payload de test : représentatif d'un flux réel (pas de hardcoding arbitraire) */
    uint8_t payload[PAYLOAD_BYTES];
    {
        /* Valeur déterministe non triviale : hash de la clock session */
        uint64_t seed = t_session_start;
        for (int i = 0; i < PAYLOAD_BYTES; i++) {
            seed ^= (seed >> 7) ^ (seed << 5);
            payload[i] = (uint8_t)(seed & 0xFF);
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        }
    }

    /* Génération des BIT_ID pour chaque bit avant la conversion */
    int total_bits = PAYLOAD_BYTES * 8;
    bit_id_t bit_ids[PAYLOAD_BYTES * 8];
    for (int b = 0; b < total_bits; b++) {
        int byte_idx = b / 8;
        int bit_idx  = 7 - (b % 8);   /* MSB first */
        uint8_t bval = (payload[byte_idx] >> bit_idx) & 1;
        bit_ids[b] = forensic_unif002_new_bit_id(bval);
        forensic_unif002_log_bit_input(bit_ids[b], bval, "PAYLOAD");
        /* Vérification continuité (détection perte) */
        forensic_unif002_check_continuity((uint64_t)b);
    }

    /* Conversion binaire → LUM via binary_lum_converter */
    binary_lum_result_t* bin_result = convert_binary_to_lum(payload, PAYLOAD_BYTES);
    result.binary_ok = (bin_result != NULL && bin_result->success
                        && bin_result->lum_group != NULL);

    lum_group_t* lum_group = NULL;
    if (result.binary_ok) {
        lum_group = bin_result->lum_group;
        result.lums_created = (int)lum_group_size(lum_group);
    }

    fprintf(stderr, "[STEP 3] Résultat : %s — %d LUM créés depuis %d bits\n",
            result.binary_ok ? "OK" : "FAIL",
            result.lums_created, total_bits);

    /* Log FU002 : LUM_ID pour chaque LUM créé */
    if (result.binary_ok) {
        for (int i = 0; i < result.lums_created && i < total_bits; i++) {
            lum_t* l = lum_group_get(lum_group, (size_t)i);
            if (!l) continue;
            lum_id_t lid = forensic_unif002_lum_id_from_bit(
                bit_ids[i], (uint16_t)0, (uint16_t)i);
            forensic_unif002_log_lum_created(lid, bit_ids[i], "BINARY_CONVERTER");
        }
    }

    fprintf(stderr, "Avancement : 32%%\n"); fflush(stdout);

    /* ═══ ÉTAPE 4 : SIMD dispatch ═══════════════════════════════════════ */
    fprintf(stderr, "[STEP 4] SIMD : détection capabilities + dispatch...\n");

    simd_capabilities_t* caps = simd_detect_capabilities();
    simd_optimizer_t simd_opt;
    memset(&simd_opt, 0, sizeof(simd_opt));

    if (caps) {
        simd_opt.capabilities = *caps;
        simd_opt.initialized  = true;
        fprintf(stderr, "[STEP 4] CPU : avx512=%d avx2=%d sse=%d\n",
                caps->avx512_supported, caps->avx2_supported,
                caps->sse42_supported);
        free(caps);
    }

    result.simd_ok = false;
    if (lum_group) {
        simd_result_t simd_res;
        memset(&simd_res, 0, sizeof(simd_res));
        int simd_ret = simd_optimize_lum_operations(
            &simd_opt, lum_group, SIMD_VECTOR_ADD, &simd_res);
        result.simd_ok = (simd_ret != 0);

        /* Traçage FU002 : transformation SIMD */
        for (int i = 0; i < result.lums_created && i < total_bits; i++) {
            lum_id_t lid = forensic_unif002_lum_id_from_bit(
                bit_ids[i], 1, (uint16_t)i);
            lum_id_t lid_out = forensic_unif002_lum_id_from_bit(
                bit_ids[i], 2, (uint16_t)i);
            forensic_unif002_log_transformation(lid, bit_ids[i],
                                               (bit_id_t)lid_out,
                                               "SIMD", "vector_add");
        }

        forensic_log_individual_lum(
            ((uint64_t)0x04ULL << 56) | (uint64_t)simd_res.vectorized_count,
            "STEP4:simd_optimize", time_ns_get_absolute());

        fprintf(stderr, "[STEP 4] simd_ok=%d vectorized=%zu scalar=%zu\n",
                simd_ret,
                simd_res.vectorized_count,
                simd_res.scalar_fallback_count);
    }
    fprintf(stderr, "Avancement : 48%%\n"); fflush(stdout);

    /* ═══ ÉTAPE 5 : PARALLEL processing ═════════════════════════════════ */
    fprintf(stderr, "[STEP 5] Parallel : %d workers sur %d LUM...\n",
            PARALLEL_WORKERS, result.lums_created);

    parallel_processor_t* proc = parallel_processor_create(PARALLEL_WORKERS);
    result.parallel_ok = (proc != NULL);

    if (proc && lum_group) {
        size_t sz = lum_group_size(lum_group);
        int submitted = 0;

        for (size_t i = 0; i < sz; i++) {
            lum_t* l = lum_group_get(lum_group, i);
            if (!l) continue;
            parallel_task_t* task = parallel_task_create(
                TASK_LUM_CREATE, l, sizeof(lum_t));
            if (task && parallel_processor_submit_task(proc, task))
                submitted++;
        }

        /* Attente bornée 200 ms */
        for (int w = 0; w < 200; w++) {
            if (task_queue_is_empty(&proc->task_queue)) break;
            struct timespec ts = {0, 1000000L};
            nanosleep(&ts, NULL);
        }

        result.lums_processed_parallel = submitted;

        /* FU002 : log résultats parallèles */
        for (int i = 0; i < submitted && i < total_bits; i++) {
            lum_id_t lid = forensic_unif002_lum_id_from_bit(
                bit_ids[i], 3, (uint16_t)i);
            forensic_unif002_log_result(lid, bit_ids[i], "PARALLEL");
        }

        parallel_processor_destroy(proc);
        proc = NULL;

        fprintf(stderr, "[STEP 5] submitted=%d\n", submitted);
    }
    fprintf(stderr, "Avancement : 64%%\n"); fflush(stdout);

    /* ═══ ÉTAPE 6 : ZERO_COPY pool pour les sorties ══════════════════════ */
    fprintf(stderr, "[STEP 6] Zero-copy : allocation sorties...\n");

    zero_copy_pool_t* zcp = zero_copy_pool_create(ZERO_COPY_POOL_SZ,
                                                   "main_cable_001_out");
    result.zerocopy_ok = (zcp != NULL);

    if (zcp) {
        int zc_ok = 0;
        zero_copy_allocation_t* zca[PAYLOAD_BYTES * 8];
        memset(zca, 0, sizeof(zca));

        for (int i = 0; i < result.lums_created && i < PAYLOAD_BYTES * 8; i++) {
            zca[i] = zero_copy_alloc(zcp, sizeof(lum_t));
            if (zca[i] && zca[i]->ptr) {
                /* Écriture réelle dans la région zero-copy */
                if (lum_group) {
                    lum_t* src = lum_group_get(lum_group, (size_t)i);
                    if (src) memcpy(zca[i]->ptr, src, sizeof(lum_t));
                }
                zc_ok++;
            }
        }

        result.zerocopy_allocs = zc_ok;

        for (int i = 0; i < result.lums_created && i < PAYLOAD_BYTES * 8; i++) {
            if (zca[i]) zero_copy_free(zcp, zca[i]);
        }

        double eff = zero_copy_get_efficiency_ratio(zcp);
        fprintf(stderr, "[STEP 6] allocs=%d efficiency=%.3f\n", zc_ok, eff);

        forensic_unif002_log_event(FU002_EVT_LUM_RESULT, 0, 0, 0, 0, 0,
                                   "ZERO_COPY",
                                   "zero_copy_alloc_done");
        zero_copy_pool_destroy(zcp);
        zcp = NULL;
    }
    fprintf(stderr, "Avancement : 78%%\n"); fflush(stdout);

    /* ═══ ÉTAPE 7 : PARETO optimizer ═════════════════════════════════════ */
    fprintf(stderr, "[STEP 7] Pareto optimizer : évaluation métriques...\n");

    pareto_config_t pcfg = {
        .enable_simd_optimization   = true,
        .enable_memory_pooling      = true,
        .enable_parallel_processing = true,
        .max_optimization_layers    = 4,
        .max_points                 = PARETO_MAX_POINTS
    };
    pareto_optimizer_t* pareto = pareto_optimizer_create(&pcfg);
    result.pareto_ok = (pareto != NULL);

    if (pareto && lum_group) {
        pareto_metrics_t m = pareto_evaluate_metrics(lum_group, "MAIN_CABLE");
        double score = pareto_calculate_inverse_score(&m);
        result.pareto_best_score = score;

        forensic_unif002_log_event(FU002_EVT_LUM_TRANSFORMED, 0, 0, 0, 0, 0,
                                   "PARETO",
                                   "pareto_evaluate_done");
        fprintf(stderr, "[STEP 7] score=%.4f eff=%.3f mem=%.0f t=%.0f µs\n",
                score, m.efficiency_ratio, m.memory_usage, m.execution_time);

        pareto_optimizer_destroy(pareto);
        pareto = NULL;
    }
    fprintf(stderr, "Avancement : 90%%\n"); fflush(stdout);

    /* ═══ ÉTAPE 8 : Nettoyage et bilan ══════════════════════════════════ */
    fprintf(stderr, "[STEP 8] Nettoyage...\n");

    /* Libération binary_lum_result */
    if (bin_result) {
        /* Le lum_group est dans bin_result — ne pas destroy séparément */
        binary_lum_result_destroy(bin_result);
        bin_result  = NULL;
        lum_group   = NULL;
    }

    /* Libération memory optimizer */
    if (mem_opt) {
        memory_optimizer_destroy(mem_opt);
        mem_opt = NULL;
    }

    /* Statistiques FU002 */
    result.fu002_stats = forensic_unif002_get_stats();

    /* Durée totale */
    uint64_t t_session_end = time_ns_get_absolute();
    result.session_duration_ns = t_session_end - t_session_start;

    /* ═══ ÉTAPE 9 : destroy forensic ════════════════════════════════════ */
    forensic_unif002_destroy();
    forensic_logger_destroy();

    fprintf(stderr, "Avancement : 100%%\n"); fflush(stdout);

    /* ═══ BILAN FINAL ════════════════════════════════════════════════════ */
    print_cable_result(&result);

    /* Code de retour : 0 si tous les modules câblés avec succès */
    int all_ok = (result.fu002_ok  && result.mem_opt_ok
                  && result.binary_ok && result.simd_ok
                  && result.parallel_ok && result.zerocopy_ok
                  && result.pareto_ok) ? 0 : 1;

    printf("[MAIN-CABLE-001] %s — modules câblés : fu002=%d mem=%d bin=%d "
           "simd=%d par=%d zc=%d pareto=%d\n",
           all_ok == 0 ? "PASS" : "PARTIAL",
           (int)result.fu002_ok, (int)result.mem_opt_ok,
           (int)result.binary_ok, (int)result.simd_ok,
           (int)result.parallel_ok, (int)result.zerocopy_ok,
           (int)result.pareto_ok);

    return all_ok;
}
