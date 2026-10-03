/* **************************************************************************
** forensic_unif_002.c — Implémentation BIT_ID/LUM_ID universels
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : src/debug / FORENSIC-UNIF-002
** Auteur : ARTCB Project <contact@artcb.me>
**
** Corrections appliquées (audit 186) :
**
**   [P0 — SERIALIZE] forensic_unif002_log_event() :
**     AVANT : mutex pris pour event_seq++, relâché, construction hors mutex,
**             second mutex pour stats, puis _write_event() sans verrou.
**             Deux threads pouvaient entrelacer leurs écritures sur FILE*.
**     APRÈS : un seul mutex protège TOUTE la transaction :
**             event_seq++ → construction → stats → fprintf → fflush.
**             L'ordre d'écriture dans le fichier est maintenant identique
**             à l'ordre monotone des event_seq.
**
**   [P1 — CONTINUITY] forensic_unif002_check_continuity() :
**     AVANT : g_stats.last_seq_seen initialisé à 0 → faux DUPLICATE
**             sur la séquence 0 au démarrage (check_continuity(0) avant
**             le premier bit réel voyait 0==0 → DUPLICATE).
**     APRÈS : flag has_last_seq_seen (bool), initialisé à false.
**             Première séquence → référence, pas d'anomalie.
**             Pas d'initialisation à UINT64_MAX qui provoquerait un
**             overflow sur last+1.
**
**   [P2 — TIMESTAMPS] Dual timestamp :
**     AVANT : un seul timestamp_ns CLOCK_REALTIME.
**     APRÈS : ts_realtime_ns (CLOCK_REALTIME, corrélation externe) +
**             ts_monotonic_ns (CLOCK_MONOTONIC, ordre causal garanti).
**             Distinction explicite : unité nanoseconde ≠ résolution.
**
**   [P3 — BIT-VALUE] Provenance avant/après transformation :
**     AVANT : bit_value dans l'événement, pas de before/after.
**     APRÈS : bit_value_before + bit_value_after dans fu002_event_t.
**             Nouvelle API forensic_unif002_log_transformation_ex().
**
** Thread-safety garantie :
**   - Un seul mutex g_mutex pour tout log_event (attribution + write + flush)
**   - forensic_unif002_destroy() prend le mutex avant fclose()
**   - Pas de FILE* partagé hors mutex
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

/* ── Helper : dual timestamp ──────────────────────────────────────────────── */
/* Audit 186 §5 : l'unité est la nanoseconde — la résolution réelle peut être
 * supérieure. CLOCK_REALTIME = date civile. CLOCK_MONOTONIC = ordre causal. */

static uint64_t _now_realtime_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t _now_monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Helper : écriture JSON-Lines (DOIT être appelé sous g_mutex) ─────────── */
/* [P0 FIX] : cette fonction est appelée uniquement depuis l'intérieur du mutex
 * global dans log_event(). Elle ne doit JAMAIS être appelée sans g_mutex tenu. */

static void _write_event_locked(const fu002_event_t* ev)
{
    if (!g_log_file) return;

    fprintf(g_log_file,
        "{\"seq\":%" PRIu64
        ",\"run_id\":%u"
        ",\"ts_rt\":%" PRIu64
        ",\"ts_mono\":%" PRIu64
        ",\"event\":%d"
        ",\"bit_id\":%" PRIu64
        ",\"lum_id\":%" PRIu64
        ",\"parent_id\":%" PRIu64
        ",\"child_id\":%" PRIu64
        ",\"bit_val\":%u"
        ",\"bit_before\":%u"
        ",\"bit_after\":%u"
        ",\"module\":\"%s\""
        ",\"op\":\"%s\"}\n",
        ev->event_seq,
        ev->run_id,
        ev->ts_realtime_ns,
        ev->ts_monotonic_ns,
        (int)ev->event_type,
        ev->bit_id,
        ev->lum_id,
        ev->parent_id,
        ev->child_id,
        (unsigned)ev->bit_value,
        (unsigned)ev->bit_value_before,
        (unsigned)ev->bit_value_after,
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
    g_stats.run_id            = g_run_id;
    g_stats.integrity_ok      = true;
    g_stats.has_last_seq_seen = false;  /* [P1 FIX] flag init à false */

    /* Ouverture fichier log */
    if (log_path) {
        g_log_file = fopen(log_path, "a");  /* append — jamais écraser */
        if (!g_log_file) {
            fprintf(stderr, "[FU002][WARN] Impossible d'ouvrir %s\n", log_path);
        }
    }

    pthread_mutex_unlock(&g_mutex);

    fprintf(stderr, "[FU002][DEBUG] Init : run_id=0x%08X log=%s\n",
            g_run_id, log_path ? log_path : "(stderr only)");

    /* Écriture SESSION_START — sous mutex via log_event() */
    forensic_unif002_log_event(FU002_EVT_SESSION_START,
                               0, 0, 0, 0, 0,
                               "FORENSIC_UNIF_002", "session_start");
    return true;
}

void forensic_unif002_destroy(void)
{
    /* Écriture SESSION_END avec stats (avant de fermer le fichier) */
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

    /* [P0 FIX] Fermeture sous mutex : garantit qu'aucun thread producteur
     * ne peut écrire entre la dernière ligne et le fclose(). */
    pthread_mutex_lock(&g_mutex);
    if (g_log_file) {
        fflush(g_log_file);
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

    /* Capture des timestamps HORS du mutex (appels système peuvent être lents) */
    uint64_t rt  = _now_realtime_ns();
    uint64_t mono = _now_monotonic_ns();

    ev.event_type        = event_type;
    ev.bit_id            = bit_id;
    ev.lum_id            = lum_id;
    ev.parent_id         = parent_id;
    ev.child_id          = child_id;
    ev.ts_realtime_ns    = rt;
    ev.ts_monotonic_ns   = mono;
    ev.bit_value         = bit_value;
    ev.bit_value_before  = 0;  /* défaut — overridé par log_transformation_ex */
    ev.bit_value_after   = 0;

    strncpy(ev.module_name, module_name ? module_name : "?", 31);
    ev.module_name[31] = '\0';
    strncpy(ev.operation, operation ? operation : "?", 63);
    ev.operation[63] = '\0';

    /* [P0 FIX] Section critique unique :
     *   attribution event_seq + màj stats + écriture fichier + flush.
     * L'ordre d'écriture dans le fichier est strictement monotone. */
    pthread_mutex_lock(&g_mutex);

    ev.event_seq = g_event_seq++;
    ev.run_id    = g_run_id;

    /* Mise à jour statistiques */
    switch (event_type) {
        case FU002_EVT_BIT_INPUT:       /* comptabilisé dans new_bit_id() */ break;
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

    /* Écriture JSON-Lines + flush sous le même mutex */
    _write_event_locked(&ev);

    pthread_mutex_unlock(&g_mutex);
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

/* [P3 FIX] Version avec valeurs bit avant/après transformation */
void forensic_unif002_log_transformation_ex(lum_id_t    lum_id,
                                            bit_id_t    parent_id,
                                            bit_id_t    child_id,
                                            uint8_t     bit_before,
                                            uint8_t     bit_after,
                                            const char* module,
                                            const char* op_desc)
{
    char op[80];
    snprintf(op, sizeof(op),
             "transform_ex:%s:before=%u:after=%u:parent=0x%08" PRIX64
             ":child=0x%08" PRIX64,
             op_desc ? op_desc : "?",
             (unsigned)bit_before, (unsigned)bit_after,
             (uint64_t)(parent_id & 0xFFFFFFFFULL),
             (uint64_t)(child_id  & 0xFFFFFFFFULL));

    fu002_event_t ev;
    memset(&ev, 0, sizeof(ev));

    uint64_t rt   = _now_realtime_ns();
    uint64_t mono = _now_monotonic_ns();

    ev.event_type       = FU002_EVT_LUM_TRANSFORMED;
    ev.bit_id           = parent_id;
    ev.lum_id           = lum_id;
    ev.parent_id        = parent_id;
    ev.child_id         = child_id;
    ev.ts_realtime_ns   = rt;
    ev.ts_monotonic_ns  = mono;
    ev.bit_value        = bit_before;  /* valeur d'entrée */
    ev.bit_value_before = bit_before;
    ev.bit_value_after  = bit_after;

    strncpy(ev.module_name, module ? module : "TRANSFORM", 31);
    ev.module_name[31] = '\0';
    strncpy(ev.operation, op, 63);
    ev.operation[63] = '\0';

    pthread_mutex_lock(&g_mutex);
    ev.event_seq = g_event_seq++;
    ev.run_id    = g_run_id;
    g_stats.total_transformations++;
    _write_event_locked(&ev);
    pthread_mutex_unlock(&g_mutex);
}

/* Version simple (compat ascendante) — bit_before/after = 0 */
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

/* [P1 FIX] check_continuity avec flag has_last_seq_seen
 *
 * AVANT : last_seq_seen init à 0 → check_continuity(0) → 0==0 → DUPLICATE
 * APRÈS : has_last_seq_seen=false au démarrage → première séquence acceptée
 *         sans anomalie, pas de UINT64_MAX (overflow sur last+1).
 *
 * Logique complète :
 *   - !has_last_seq_seen      → première séquence, enregistrer comme ref
 *   - seq == last + 1         → normale (séquence continue)
 *   - seq > last + 1          → perte (gap entre last+1 et seq-1)
 *   - seq == last             → duplication
 *   - seq < last              → anomalie classifiée (rétrogradation)
 */
void forensic_unif002_check_continuity(uint64_t seq_expected)
{
    pthread_mutex_lock(&g_mutex);
    bool     has_last = g_stats.has_last_seq_seen;
    uint64_t last     = g_stats.last_seq_seen;
    pthread_mutex_unlock(&g_mutex);

    if (!has_last) {
        /* Première observation : on accepte sans anomalie */
        pthread_mutex_lock(&g_mutex);
        g_stats.last_seq_seen     = seq_expected;
        g_stats.has_last_seq_seen = true;
        pthread_mutex_unlock(&g_mutex);
        return;
    }

    if (seq_expected == last + 1) {
        /* Séquence normale — juste mettre à jour */
    } else if (seq_expected > last + 1) {
        /* Gap : bits manquants */
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
        /* Duplication */
        char op[64];
        snprintf(op, sizeof(op), "DUPLICATE:seq=%" PRIu64, seq_expected);
        forensic_unif002_log_event(FU002_EVT_DUPLICATE,
                                   0, 0, 0, 0, 0,
                                   "CONTINUITY_CHECK", op);
        fprintf(stderr, "[FU002][WARN] DUPLICATE : seq=%" PRIu64 "\n",
                seq_expected);
    } else {
        /* seq_expected < last : rétrogradation (anomalie) */
        char op[128];
        snprintf(op, sizeof(op),
                 "RETROGRADE:expected=%" PRIu64 ":last=%" PRIu64,
                 seq_expected, last);
        forensic_unif002_log_event(FU002_EVT_LOSS_DETECTED,
                                   0, 0, 0, 0, 0,
                                   "CONTINUITY_CHECK", op);
        fprintf(stderr, "[FU002][WARN] RETROGRADE : seq=%" PRIu64
                " < last=%" PRIu64 "\n", seq_expected, last);
    }

    /* Mise à jour last uniquement si la séquence avance */
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
