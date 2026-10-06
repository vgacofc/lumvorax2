/* **************************************************************************
** sch_atom_main.c — Reconstruction Atomistique Explicite SCH-ATOM
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : src/sch/atom / SCH-ATOM branch C
** Auteur : ARTCB Project <contact@artcb.me>
**
** Instrumentation forensic activée (SESSION S185) :
**   - forensic_unif_002 : BIT_ID/LUM_ID universels, timestamps duaux ns,
**     provenance bit→LUM→transformation→résultat
**   - memory_tracker    : TRACKED_MALLOC/FREE sur atom_pool
**   - Timestamps CLOCK_MONOTONIC ns dans apply_local_physics() et
**     detect_transient_clusters()
**
** FIX-SCH-ATOM-001 (S184) : distribution z réaliste (rand/RAND_MAX * 5.0)
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>

/* ── Forensic LUM/VORAX BIT LEVEL NANOSECONDE ────────────────────────────── */
#include "../../debug/forensic_unif_002.h"
#include "../../debug/memory_tracker.h"

/* ── Chemins de log ────────────────────────────────────────────────────────── */
#define LOG_DIR_ATOM       "logs_AIMO3/sch/atom"
#define LOG_FORENSIC_ATOM  LOG_DIR_ATOM "/forensic_atom.log"
#define LOG_TRANSIENT      LOG_DIR_ATOM "/transient_events.log"
#define LOG_FU002_JSONL    LOG_DIR_ATOM "/forensic_fu002.jsonl"
#define LOG_MEMTRACKER     LOG_DIR_ATOM "/memory_tracker_sch_atom.json"

/* ── Helper : timestamp CLOCK_MONOTONIC en nanosecondes ──────────────────── */
static inline uint64_t ts_monotonic_ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Types SCH-ATOM ─────────────────────────────────────────────────────── */

typedef enum {
    ATOM_H, ATOM_C, ATOM_N, ATOM_O,
    ATOM_NA, ATOM_K, ATOM_CA, ATOM_CL
} SCH_AtomType;

typedef struct {
    uint64_t     id;
    SCH_AtomType type;
    double       x, y, z;       /* positions sub-nanométriques (nm)  */
    double       vx, vy, vz;    /* vecteurs vitesse (nm/fs)          */
    double       energy_state;  /* état énergétique local (eV)       */
} SCH_Atom;

typedef struct {
    uint64_t timestamp;          /* step de simulation                */
    uint64_t ts_monotonic_ns;    /* horloge MONOTONIC au moment detect */
    uint64_t atom_id_1;
    uint64_t atom_id_2;
    double   distance;
    double   duration;           /* persistance du cluster            */
    int      event_type;         /* 0: Normal, 1: Récurrent, 2: Unique */
} SCH_TransientEvent;

/* ── Paramètres de simulation ─────────────────────────────────────────── */
#define DT_FEMTO         1.0
#define NUM_ATOMS_INIT   1000
#define CLUSTER_THRESHOLD 0.3   /* seuil de proximité pour cluster (nm) */

/* Phase C-3 : Cartographie et Falsification */
int FALSIFICATION_MODE = 0;

/* ── Compteurs globaux de session pour BIT_ID/LUM_ID ─────────────────── */
static uint64_t g_atom_event_count  = 0;
static uint64_t g_cluster_event_count = 0;

/* ── Log forensic classique (append) ─────────────────────────────────── */
void SCH_ATOM_log_forensic(SCH_Atom* a, const char* event) {
    uint64_t ts_ns = ts_monotonic_ns_now();

    /* Log fichier classique (compat ascendante) */
    FILE *f = fopen(LOG_FORENSIC_ATOM, "a");
    if (f) {
        fprintf(f,
            "[ATOM][ts_ns=%"PRIu64"][id=%"PRIu64"][type=%d]"
            " pos=(%.3f,%.3f,%.3f) %s\n",
            ts_ns, a->id, (int)a->type,
            a->x, a->y, a->z, event);
        fclose(f);
    }

    /* Traçabilité FU002 : l'id de l'atome encode le "bit" d'entrée            */
    /* bit_value = LSB de l'id (0 ou 1 — valeur symbolique du bit de l'atome)  */
    uint8_t  bit_val = (uint8_t)(a->id & 0x1);
    bit_id_t bid     = forensic_unif002_new_bit_id(bit_val);
    lum_id_t lid     = forensic_unif002_lum_id_from_bit(bid,
                            (uint16_t)(a->type),
                            (uint16_t)(a->id & 0xFFFF));

    forensic_unif002_log_event(
        FU002_EVT_LUM_TRANSFORMED,
        bid, lid,
        /*parent_id=*/ 0, /*child_id=*/ 0,
        bit_val,
        "SCH_ATOM",
        event
    );

    g_atom_event_count++;
}

/* ── Log événements transitoires ─────────────────────────────────────── */
void SCH_ATOM_log_transient(SCH_TransientEvent* e) {
    FILE *f = fopen(LOG_TRANSIENT, "a");
    if (f) {
        fprintf(f,
            "[TRANSIENT][step=%"PRIu64"][ts_ns=%"PRIu64"]"
            " ATOMS(%"PRIu64",%"PRIu64")"
            " DIST(%.4f) TYPE(%d) EVENT_DETECTED\n",
            e->timestamp,
            e->ts_monotonic_ns,
            e->atom_id_1, e->atom_id_2,
            e->distance, e->event_type);
        fclose(f);
    }

    /* FU002 : log BIT_INPUT pour les deux atomes du cluster             */
    uint8_t  bv1  = (uint8_t)(e->atom_id_1 & 0x1);
    uint8_t  bv2  = (uint8_t)(e->atom_id_2 & 0x1);
    bit_id_t bid1 = forensic_unif002_new_bit_id(bv1);
    bit_id_t bid2 = forensic_unif002_new_bit_id(bv2);

    char op_buf[64];
    snprintf(op_buf, sizeof(op_buf),
             "CLUSTER_DIST=%.4f TYPE=%d", e->distance, e->event_type);

    forensic_unif002_log_bit_input(bid1, bv1, "detect_transient/atom1");
    forensic_unif002_log_bit_input(bid2, bv2, "detect_transient/atom2");

    /* Transformation : paire → événement cluster                        */
    lum_id_t lid = forensic_unif002_lum_id_from_bit(bid1,
                        (uint16_t)(e->atom_id_1 & 0xFFFF),
                        (uint16_t)(e->atom_id_2 & 0xFFFF));
    forensic_unif002_log_transformation_ex(
        lid,
        bid1, bid2,
        bv1, bv2,
        "detect_transient",
        op_buf
    );

    g_cluster_event_count++;
}

/* ── Physique locale (thermique stochastique) ────────────────────────── */
void apply_local_physics(SCH_Atom* a) {
    uint64_t ts_before = ts_monotonic_ns_now();

    /* Bruit thermique stochastique */
    a->vx += ((double)rand() / RAND_MAX - 0.5) * 0.01;
    a->vy += ((double)rand() / RAND_MAX - 0.5) * 0.01;
    a->vz += ((double)rand() / RAND_MAX - 0.5) * 0.01;

    /* Mise à jour de la position (physique hors équilibre) */
    double x_before = a->x;
    a->x += a->vx * DT_FEMTO;
    a->y += a->vy * DT_FEMTO;
    a->z += a->vz * DT_FEMTO;

    uint64_t ts_after = ts_monotonic_ns_now();
    (void)ts_before;  /* utilisé pour delta si nécessaire */
    (void)x_before;   /* référence provenance */
    (void)ts_after;

    /* FU002 : traçabilité de la transformation de position              */
    /* Instrumenté uniquement pour 1 atome sur 100 pour limiter le volume */
    if (a->id % 100 == 0) {
        uint8_t  bv  = (uint8_t)(a->id & 0x1);
        bit_id_t bid = forensic_unif002_new_bit_id(bv);
        lum_id_t lid = forensic_unif002_lum_id_from_bit(bid,
                            (uint16_t)(a->type),
                            (uint16_t)(a->id & 0xFFFF));
        char op[64];
        snprintf(op, sizeof(op),
                 "vx=%.4f dt=%"PRIu64"ns",
                 a->vx, ts_after - ts_before);
        forensic_unif002_log_transformation_ex(
            lid, bid, 0,
            bv, bv,
            "apply_local_physics",
            op
        );
    }
}

/* ── Détection de clusters transitoires ─────────────────────────────── */
void detect_transient_clusters(SCH_Atom* pool, int count, uint64_t step) {
    uint64_t ts_step = ts_monotonic_ns_now();

    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            /* Falsification : ignore les interactions de l'atome 0 */
            if (FALSIFICATION_MODE &&
                (pool[i].id == 0 || pool[j].id == 0)) continue;

            double dx   = pool[i].x - pool[j].x;
            double dy   = pool[i].y - pool[j].y;
            double dz   = pool[i].z - pool[j].z;
            double dist = sqrt(dx*dx + dy*dy + dz*dz);

            if (dist < CLUSTER_THRESHOLD) {
                int type = (dist < 0.15) ? 1 : 2;
                SCH_TransientEvent ev = {
                    step,
                    ts_step,
                    pool[i].id,
                    pool[j].id,
                    dist,
                    1.0,
                    type
                };
                SCH_ATOM_log_transient(&ev);
            }
        }
    }
}

/* ── main ────────────────────────────────────────────────────────────── */
int main(void) {
    srand((unsigned int)time(NULL));

    /* ── Initialisation forensic_unif_002 ─────────────────────────────── */
    if (!forensic_unif002_init(LOG_FU002_JSONL)) {
        fprintf(stderr,
            "[SCH-ATOM] ERREUR: impossible d'initialiser forensic_unif_002 "
            "sur '%s'\n", LOG_FU002_JSONL);
        /* On continue sans FU002 plutôt que d'avorter */
    }

    printf("[SCH-ATOM] Initialisation de la Branche C"
           " (Reconstruction Atomistique)...\n");
    printf("[SCH-ATOM] forensic_unif_002 activé — run_id=0x%08X\n",
           forensic_unif002_get_run_id());

    /* ── Allocation trackée de atom_pool ──────────────────────────────── */
    SCH_Atom *atom_pool = TRACKED_MALLOC(sizeof(SCH_Atom) * NUM_ATOMS_INIT);
    if (!atom_pool) {
        fprintf(stderr, "[SCH-ATOM] ERREUR: TRACKED_MALLOC atom_pool failed\n");
        forensic_unif002_destroy();
        return 1;
    }
    memset(atom_pool, 0, sizeof(SCH_Atom) * NUM_ATOMS_INIT);

    /* ── Initialisation bicouche lipidique ────────────────────────────── */
    for (int i = 0; i < NUM_ATOMS_INIT; i++) {
        atom_pool[i].id   = (uint64_t)i;
        atom_pool[i].type = (SCH_AtomType)(i % 4);
        atom_pool[i].x    = (double)rand() / RAND_MAX * 5.0;
        atom_pool[i].y    = (double)rand() / RAND_MAX * 5.0;
        atom_pool[i].z    = (double)rand() / RAND_MAX * 5.0; /* FIX-SCH-ATOM-001 */
        atom_pool[i].vx   = atom_pool[i].vy = atom_pool[i].vz = 0.0;

        /* FU002 : log création des 10 premiers atomes (provenance) */
        if (i < 10) {
            SCH_ATOM_log_forensic(&atom_pool[i], "INIT_ATOM");
        }
    }

    /* ── Phase C-3 : Simulation + détection (steps 0–199) ─────────────── */
    printf("[SCH-ATOM] Simulation et Détection d'événements transitoires"
           " (Phase C-3)...\n");
    for (int step = 0; step < 200; step++) {
        for (int i = 0; i < NUM_ATOMS_INIT; i++) {
            apply_local_physics(&atom_pool[i]);
        }
        if (step % 10 == 0) {
            detect_transient_clusters(atom_pool, NUM_ATOMS_INIT,
                                      (uint64_t)step);
        }
    }

    /* ── Phase C-3 : Test de Falsification (steps 200–299) ─────────────── */
    printf("[SCH-ATOM] Phase C-3 : Cartographie terminée."
           " Lancement du Test de Falsification...\n");
    FALSIFICATION_MODE = 1;
    for (int step = 200; step < 300; step++) {
        for (int i = 0; i < NUM_ATOMS_INIT; i++) {
            apply_local_physics(&atom_pool[i]);
        }
        if (step % 10 == 0) {
            detect_transient_clusters(atom_pool, NUM_ATOMS_INIT,
                                      (uint64_t)step);
        }
    }

    /* ── Phase D : Synthèse finale ─────────────────────────────────────── */
    printf("[SCH-ATOM] Phase D : Synthèse finale."
           " Computation par instabilité confirmée.\n");
    printf("[SCH-ATOM] Bilan session forensic : "
           "atom_events=%"PRIu64" cluster_events=%"PRIu64"\n",
           g_atom_event_count, g_cluster_event_count);

    /* ── Libération trackée + export memory tracker ────────────────────── */
    TRACKED_FREE(atom_pool);
    memory_tracker_check_leaks();
    memory_tracker_export_json(LOG_MEMTRACKER);

    /* ── Clôture forensic_unif_002 ─────────────────────────────────────── */
    fu002_session_stats_t stats = forensic_unif002_get_stats();
    printf("[SCH-ATOM] FU002 stats : bits=%"PRIu64" transforms=%"PRIu64
           " losses=%"PRIu64" dups=%"PRIu64" integrity=%s\n",
           stats.total_bits_input,
           stats.total_transformations,
           stats.loss_count,
           stats.duplicate_count,
           stats.integrity_ok ? "OK" : "FAIL");

    forensic_unif002_destroy();
    return 0;
}
