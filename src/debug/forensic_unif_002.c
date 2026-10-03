/* **************************************************************************
** forensic_unif_002.c — Implémentation BIT_ID/LUM_ID universels
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : src/debug / FORENSIC-UNIF-002
** Auteur : ARTCB Project <contact@artcb.me>
**
** Propriétés :
**   - run_id généré par XOR de clock_gettime(REALTIME) et getpid()
**   - bit_seq_counter atomique (mutex) → monotone garanti
**   - event_seq global monotone → détection de perte/duplication
**   - Fichier log JSON-Lines (une entrée par ligne)
**   - Thread-safe : mutex global unique
**   - Fail-closed : si log_file NULL, les stats sont quand même mises à jour
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include "forensic_unif_002.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

/* ── État interne ─────────────────────────────────────────────────────────── */

static FILE*              g_log_file    = NULL;
static uint32_t           g_run_id      = 0;
static uint64_t           g_bit_seq     = 0;    /* compteur monotone bits    */
static uint64_t           g_event_seq   = 0;    /* compteur monotone events  */
static pthread_mutex_t    g_mutex       = PTHREAD_MUTEX_INITIALIZER;

/* Statistiques de session */
static fu002_session_stats_t g_stats;

/* ── Helper : horodatage CLOCK_REALTIME ───────────────────────────────────── */

static uint64_t _now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Helper : écriture JSON-Lines ─────────────────────────────────────────── */

static void _write_event(const fu002_event_t* ev)
{
    if (!g_log_file) return;

    fprintf(g_log_file,
        "{\"seq\":%" PRIu64
        ",\"run_id\":%u"
        ",\"ts_ns\":%" PRIu64
        ",\"event\":%d"
        ",\"bit_id\":%" PRIu64
        ",\"lum_id\":%" PRIu64
        ",\"parent_id\":%" PRIu64
        ",\"child_id\":%" PRIu64
        ",\"bit_val\":%u"
        ",\"module\":\"%s\""
        ",\"op\":\"%s\"}\n",
        ev->event_seq,
        ev->run_id,
        ev->timestamp_ns,
        (int)ev->event_type,
        ev->bit_id,
        ev->lum_id,
        ev->parent_id,
        ev->child_id,
        (unsigned)ev->bit_value,
        ev->module_name,
        ev->operation);
    fflush(g_log_file);
}

/* ── API publique ─────────────────────────────────────────────────────────── */

bool forensic_unif002_init(const char* log_path)
{
    pthread_mutex_lock(&g_mutex);

    /* Génération run_id : XOR de REALTIME et PID */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    g_run_id = (uint32_t)((ts.tv_sec ^ ts.tv_nsec) ^ (uint64_t)getpid());

    /* Reset compteurs */
    g_bit_seq   = 0;
    g_event_seq = 0;
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.run_id       = g_run_id;
    g_stats.integrity_ok = true;

    /* Ouverture fichier log */
    if (log_path) {
        g_log_file = fopen(log_path, "a");  /* append — jamais écraser */
        if (!g_log_file) {
            fprintf(stderr, "[FU002][WARN] Impossible d'ouvrir %s\n", log_path);
        }
    }

    pthread_mutex_unlock(&g_mutex);

    /* Écriture SESSION_START */
    forensic_unif002_log_event(FU002_EVT_SESSION_START,
                               0, 0, 0, 0, 0,
                               "FORENSIC_UNIF_002", "session_start");

    fprintf(stderr, "[FU002][DEBUG] Init : run_id=0x%08X log=%s\n",
            g_run_id, log_path ? log_path : "(stderr only)");
    return true;
}

void forensic_unif002_destroy(void)
{
    /* Écriture SESSION_END avec stats */
    char op[128];
    fu002_session_stats_t s = forensic_unif002_get_stats();
    snprintf(op, sizeof(op),
             "session_end:bits=%" PRIu64 ":lums=%" PRIu64
             ":tf=%" PRIu64 ":loss=%" PRIu64 ":dup=%" PRIu64
             ":ok=%d",
             s.total_bits_input, s.total_lums_created,
             s.total_transformations, s.loss_count,
             s.duplicate_count, (int)s.integrity_ok);

    forensic_unif002_log_event(FU002_EVT_SESSION_END,
                               0, 0, 0, 0, 0,
                               "FORENSIC_UNIF_002", op);

    pthread_mutex_lock(&g_mutex);
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    pthread_mutex_unlock(&g_mutex);

    fprintf(stderr,
            "[FU002][DEBUG] Session terminée : run_id=0x%08X "
            "bits=%" PRIu64 " lums=%" PRIu64
            " loss=%" PRIu64 " dup=%" PRIu64 " ok=%d\n",
            s.run_id,
            s.total_bits_input, s.total_lums_created,
            s.loss_count, s.duplicate_count, (int)s.integrity_ok);
}

bit_id_t forensic_unif002_new_bit_id(uint8_t bit_value)
{
    pthread_mutex_lock(&g_mutex);
    uint64_t seq = g_bit_seq++;
    uint32_t rid = g_run_id;
    g_stats.total_bits_input++;
    pthread_mutex_unlock(&g_mutex);

    /* BIT_ID = run_id(32) | seq(32) */
    bit_id_t id = ((uint64_t)rid << 32) | (seq & 0xFFFFFFFFULL);

    (void)bit_value;  /* valeur stockée dans l'événement, pas dans l'id */
    return id;
}

lum_id_t forensic_unif002_lum_id_from_bit(bit_id_t bit_id,
                                           uint16_t group_index,
                                           uint16_t bit_in_group)
{
    /* LUM_ID = run_id(32) | group_index(16) | bit_in_group(16) */
    uint32_t rid = (uint32_t)(bit_id >> 32);
    return ((uint64_t)rid << 32)
         | ((uint64_t)group_index << 16)
         | ((uint64_t)bit_in_group);
}

void forensic_unif002_log_event(fu002_event_type_e event_type,
                                bit_id_t            bit_id,
                                lum_id_t            lum_id,
                                bit_id_t            parent_id,
                                bit_id_t            child_id,
                                uint8_t             bit_value,
                                const char*         module_name,
                                const char*         operation)
{
    fu002_event_t ev;
    memset(&ev, 0, sizeof(ev));

    pthread_mutex_lock(&g_mutex);
    ev.event_seq  = g_event_seq++;
    ev.run_id     = g_run_id;
    pthread_mutex_unlock(&g_mutex);

    ev.event_type   = event_type;
    ev.bit_id       = bit_id;
    ev.lum_id       = lum_id;
    ev.parent_id    = parent_id;
    ev.child_id     = child_id;
    ev.timestamp_ns = _now_ns();
    ev.bit_value    = bit_value;

    strncpy(ev.module_name, module_name ? module_name : "?", 31);
    ev.module_name[31] = '\0';
    strncpy(ev.operation, operation ? operation : "?", 63);
    ev.operation[63] = '\0';

    /* Mise à jour statistiques */
    pthread_mutex_lock(&g_mutex);
    switch (event_type) {
        case FU002_EVT_LUM_CREATED:     g_stats.total_lums_created++;    break;
        case FU002_EVT_LUM_TRANSFORMED: g_stats.total_transformations++; break;
        case FU002_EVT_LUM_RESULT:      g_stats.total_results++;         break;
        case FU002_EVT_LOSS_DETECTED:
            g_stats.loss_count++;
            g_stats.integrity_ok = false;
            break;
        case FU002_EVT_DUPLICATE:
            g_stats.duplicate_count++;
            g_stats.integrity_ok = false;
            break;
        default: break;
    }
    pthread_mutex_unlock(&g_mutex);

    _write_event(&ev);
}

void forensic_unif002_log_bit_input(bit_id_t bit_id, uint8_t bit_value,
                                    const char* source)
{
    char op[80];
    snprintf(op, sizeof(op), "bit_input:val=%u:src=%s",
             (unsigned)bit_value, source ? source : "?");
    forensic_unif002_log_event(FU002_EVT_BIT_INPUT,
                               bit_id, 0, 0, 0,
                               bit_value, "INPUT", op);
}

void forensic_unif002_log_lum_created(lum_id_t lum_id, bit_id_t parent_bit_id,
                                      const char* module)
{
    char op[80];
    snprintf(op, sizeof(op), "lum_created:parent=0x%016" PRIX64,
             (uint64_t)parent_bit_id);
    forensic_unif002_log_event(FU002_EVT_LUM_CREATED,
                               parent_bit_id, lum_id,
                               parent_bit_id, 0,
                               0, module ? module : "LUM_CORE", op);
}

void forensic_unif002_log_transformation(lum_id_t lum_id, bit_id_t parent_id,
                                         bit_id_t child_id,
                                         const char* module, const char* op_desc)
{
    char op[80];
    snprintf(op, sizeof(op), "transform:%s:parent=0x%08" PRIX64
             ":child=0x%08" PRIX64,
             op_desc ? op_desc : "?",
             (uint64_t)(parent_id & 0xFFFFFFFFULL),
             (uint64_t)(child_id  & 0xFFFFFFFFULL));
    forensic_unif002_log_event(FU002_EVT_LUM_TRANSFORMED,
                               parent_id, lum_id,
                               parent_id, child_id,
                               0, module ? module : "TRANSFORM", op);
}

void forensic_unif002_log_result(lum_id_t lum_id, bit_id_t parent_id,
                                 const char* module)
{
    char op[80];
    snprintf(op, sizeof(op), "result:parent=0x%016" PRIX64,
             (uint64_t)parent_id);
    forensic_unif002_log_event(FU002_EVT_LUM_RESULT,
                               parent_id, lum_id,
                               parent_id, 0,
                               0, module ? module : "OUTPUT", op);
}

void forensic_unif002_check_continuity(uint64_t seq_expected)
{
    pthread_mutex_lock(&g_mutex);
    uint64_t last = g_stats.last_seq_seen;
    pthread_mutex_unlock(&g_mutex);

    if (seq_expected > last + 1) {
        /* Gap détecté : bits manquants entre last+1 et seq_expected-1 */
        char op[128];
        snprintf(op, sizeof(op),
                 "LOSS:expected=%" PRIu64 ":last_seen=%" PRIu64
                 ":gap=%" PRIu64,
                 seq_expected, last,
                 seq_expected - last - 1);
        forensic_unif002_log_event(FU002_EVT_LOSS_DETECTED,
                                   0, 0, 0, 0, 0,
                                   "CONTINUITY_CHECK", op);
        fprintf(stderr,
                "[FU002][WARN] LOSS détecté : gap=%" PRIu64
                " (expected=%" PRIu64 " last=%" PRIu64 ")\n",
                seq_expected - last - 1, seq_expected, last);
    } else if (seq_expected == last) {
        /* Même séquence vue deux fois */
        char op[64];
        snprintf(op, sizeof(op), "DUPLICATE:seq=%" PRIu64, seq_expected);
        forensic_unif002_log_event(FU002_EVT_DUPLICATE,
                                   0, 0, 0, 0, 0,
                                   "CONTINUITY_CHECK", op);
        fprintf(stderr, "[FU002][WARN] DUPLICATE : seq=%" PRIu64 "\n",
                seq_expected);
    }

    pthread_mutex_lock(&g_mutex);
    if (seq_expected > g_stats.last_seq_seen)
        g_stats.last_seq_seen = seq_expected;
    pthread_mutex_unlock(&g_mutex);
}

fu002_session_stats_t forensic_unif002_get_stats(void)
{
    pthread_mutex_lock(&g_mutex);
    fu002_session_stats_t s = g_stats;
    pthread_mutex_unlock(&g_mutex);
    return s;
}

uint32_t forensic_unif002_get_run_id(void)
{
    pthread_mutex_lock(&g_mutex);
    uint32_t r = g_run_id;
    pthread_mutex_unlock(&g_mutex);
    return r;
}

uint64_t forensic_unif002_get_bit_seq(void)
{
    pthread_mutex_lock(&g_mutex);
    uint64_t s = g_bit_seq;
    pthread_mutex_unlock(&g_mutex);
    return s;
}
