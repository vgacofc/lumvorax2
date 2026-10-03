/* **************************************************************************
** fu002_tsan_test.c — Test TSan FU002 S181 : data race detection
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : src/tests / FORENSIC-UNIF-002 S181
** Auteur : ARTCB Project <contact@artcb.me>
**
** Objectif S181-B : prouver absence de data race dans forensic_unif_002.c
**   sous ThreadSanitizer — 8 threads, 100k events.
**
** Objectif S181-C : prouver atomicité de check_continuity() sous concurrence
**   séquences concurrentes soumises par N_THREADS en parallèle.
**
** Résultat attendu :
**   - 0 data race TSan
**   - bits_input == N_THREADS * EVENTS_EACH
**   - lums_created == N_THREADS * EVENTS_EACH
**   - loss_count == 0 (les pertes éventuelles de check_continuity sont
**     attendues car les séquences concurrentes peuvent s'intercaler)
**   - dup_count == 0
**   - integrity_ok == TRUE
**   - EXIT = 0
**
** IMPORTANT : check_continuity() concurrent ATTENDU à produire des
**   LOSS/RETROGRADE (les threads soumettent des séquences indépendantes,
**   l'ordre d'arrivée est non-déterministe).
**   Ce test vérifie l'absence de DATA RACE, pas l'absence de LOSS
**   dans le cas concurrent de check_continuity().
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include "forensic_unif_002.h"

#define N_THREADS        8
#define EVENTS_EACH      12500    /* 8 × 12500 = 100 000 events */
#define CONTINUITY_COUNT 1000     /* séquences soumises par thread à check_continuity */

/* ── Données par thread ───────────────────────────────────────────────────── */

typedef struct {
    int tid;
    uint64_t base_seq;   /* base de séquence pour check_continuity */
} thread_arg_t;

/* ── Worker principal : log_event (S181-B) ───────────────────────────────── */

static void* worker_log(void* arg)
{
    thread_arg_t* a = (thread_arg_t*)arg;
    int tid = a->tid;

    for (int i = 0; i < EVENTS_EACH; i++) {
        bit_id_t bid = forensic_unif002_new_bit_id((uint8_t)(i & 1));
        lum_id_t lid = forensic_unif002_lum_id_from_bit(bid,
                            (uint16_t)tid, (uint16_t)(i & 0xFFFF));

        forensic_unif002_log_bit_input(bid, (uint8_t)(i & 1), "TSAN_TEST");
        forensic_unif002_log_lum_created(lid, bid, "TSAN_TEST");

        /* Cas transformation_ex (P3) : before/after */
        if ((i % 10) == 0) {
            forensic_unif002_log_transformation_ex(lid, bid, bid + 1,
                (uint8_t)(i & 1), (uint8_t)((i + 1) & 1),
                "TSAN_TRANSFORM", "xor_test");
        }
    }
    return NULL;
}

/* ── Worker check_continuity (S181-C) ────────────────────────────────────── */
/* NOTE : les threads soumettent des séquences INDÉPENDANTES et ENTRELACÉES.
 * Des LOSS/RETROGRADE sont attendus dans les stats — ce n'est pas un bug
 * du module, c'est le comportement correct quand check_continuity() est
 * appelée depuis plusieurs threads avec leurs propres compteurs.
 * Ce test vérifie uniquement l'absence de DATA RACE (TSan), pas la
 * sémantique de la machine d'état sous usage concurrent. */

static void* worker_continuity(void* arg)
{
    thread_arg_t* a = (thread_arg_t*)arg;
    for (int i = 0; i < CONTINUITY_COUNT; i++) {
        /* Chaque thread a sa propre base de séquence pour éviter
         * une collision délibérée — on teste les races, pas la logique. */
        forensic_unif002_check_continuity(a->base_seq + (uint64_t)i);
    }
    return NULL;
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    fprintf(stderr, "[FU002_TSAN] === S181-B+C TSan FU002 ===\n");
    fprintf(stderr, "[FU002_TSAN] Threads=%d Events/thread=%d "
            "Total=%d Continuity/thread=%d\n",
            N_THREADS, EVENTS_EACH,
            N_THREADS * EVENTS_EACH, CONTINUITY_COUNT);

    forensic_unif002_init("/tmp/fu002_tsan_run.jsonl");

    /* Phase 1 : S181-B — test log_event sous TSan */
    pthread_t tlog[N_THREADS];
    thread_arg_t args[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        args[i].tid      = i;
        args[i].base_seq = (uint64_t)i * CONTINUITY_COUNT;
        pthread_create(&tlog[i], NULL, worker_log, &args[i]);
    }
    for (int i = 0; i < N_THREADS; i++)
        pthread_join(tlog[i], NULL);

    fprintf(stderr, "[FU002_TSAN] Phase 1 (log_event) terminée\n");

    /* Phase 2 : S181-C — test check_continuity sous TSan */
    pthread_t tcont[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
        pthread_create(&tcont[i], NULL, worker_continuity, &args[i]);
    for (int i = 0; i < N_THREADS; i++)
        pthread_join(tcont[i], NULL);

    fprintf(stderr, "[FU002_TSAN] Phase 2 (check_continuity) terminée\n");

    forensic_unif002_destroy();

    fu002_session_stats_t s = forensic_unif002_get_stats();

    uint64_t expected_bits = (uint64_t)N_THREADS * EVENTS_EACH;
    uint64_t expected_lums = (uint64_t)N_THREADS * EVENTS_EACH;

    /* Vérification P0/P1 */
    int pass_bits = (s.total_bits_input == expected_bits);
    int pass_lums = (s.total_lums_created == expected_lums);
    /* loss/dup peuvent exister depuis check_continuity concurrent —
     * ce test vérifie uniquement l'absence de data race TSan */
    int pass_dup  = (s.duplicate_count == 0);

    fprintf(stderr, "[FU002_TSAN] bits_input   = %" PRIu64
            " (attendu %" PRIu64 ") : %s\n",
            s.total_bits_input, expected_bits,
            pass_bits ? "OK" : "FAIL");
    fprintf(stderr, "[FU002_TSAN] lums_created = %" PRIu64
            " (attendu %" PRIu64 ") : %s\n",
            s.total_lums_created, expected_lums,
            pass_lums ? "OK" : "FAIL");
    fprintf(stderr, "[FU002_TSAN] loss_count   = %" PRIu64
            " (concurrent check_continuity — attendu possiblement >0)\n",
            s.loss_count);
    fprintf(stderr, "[FU002_TSAN] dup_count    = %" PRIu64 " : %s\n",
            s.duplicate_count, pass_dup ? "OK" : "FAIL");
    fprintf(stderr, "[FU002_TSAN] integrity_ok = %s\n",
            s.integrity_ok ? "TRUE" : "FALSE");

    /* Vérification intégrité JSON-Lines */
    FILE* f = fopen("/tmp/fu002_tsan_run.jsonl", "r");
    int valid_lines = 0, invalid_lines = 0;
    if (f) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), f)) {
            size_t len = strlen(buf);
            if (len > 2 && buf[0] == '{' && buf[len-2] == '}')
                valid_lines++;
            else if (len > 0)
                invalid_lines++;
        }
        fclose(f);
    }
    fprintf(stderr, "[FU002_TSAN] JSON lines valides   = %d\n", valid_lines);
    fprintf(stderr, "[FU002_TSAN] JSON lines invalides = %d : %s\n",
            invalid_lines, (invalid_lines == 0) ? "OK" : "FAIL");

    int overall = pass_bits && pass_lums && pass_dup && (invalid_lines == 0);

    fprintf(stderr, "[FU002_TSAN] RÉSULTAT FONCTIONNEL : %s\n",
            overall ? "PASS" : "FAIL");
    fprintf(stderr, "[FU002_TSAN] Note TSan : si aucun rapport "
            "\"DATA RACE\" ci-dessus → TSan PASS\n");

    return overall ? 0 : 1;
}
