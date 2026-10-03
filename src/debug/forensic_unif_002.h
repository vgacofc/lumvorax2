/* **************************************************************************
** forensic_unif_002.h — BIT_ID/LUM_ID universels, provenance bit-level,
**                        chaîne entrée→transformation→sortie
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : src/debug / FORENSIC-UNIF-002
** Auteur : ARTCB Project <contact@artcb.me>
**
** Objectif : Tracking bit-level universel avec identifiants globalement
**   uniques dans le domaine d'une session (run_id 32 bits + seq 32 bits),
**   run_id de session, parent_id/child_id, et détection de perte/duplication
**   dans la chaîne entrée→transformation→sortie.
**
** Architecture :
**   BIT_ID   = run_id (32 bits) | bit_global_seq (32 bits) — unique par session
**   LUM_ID   = run_id (32 bits) | group_index (16 bits) | bit_in_group (16 bits)
**   run_id   = généré à forensic_unif002_init() — différencier les sessions
**   parent_id = BIT_ID de l'entrée ayant produit ce LUM (provenance)
**   child_id  = BIT_ID du LUM résultant d'une transformation (sortie)
**
** Thread-safety :
**   Un mutex global unique protège dans une seule section critique :
**     - attribution event_seq
**     - construction de l'événement complet
**     - écriture FILE* (fprintf)
**     - fflush
**   Cette sérialisation garantit que l'ordre physique d'écriture dans le
**   fichier JSON-Lines correspond à l'ordre monotone des event_seq.
**   Cf. audit 186 §4 : « attribution seq → écriture → flush » doivent être
**   atomiques pour une preuve forensique acceptable.
**
** Détection perte/duplication (corrected FU002-FIX-001) :
**   Un flag has_last_seq_seen évite le faux positif DUPLICATE au démarrage
**   (au lieu d'initialiser last_seq_seen à UINT64_MAX qui provoquerait
**   un overflow sur l'addition last+1).
**
** Timestamps duaux (audit 186 §5) :
**   ts_realtime_ns = CLOCK_REALTIME  — corrélation externe (date civile)
**   ts_monotonic_ns = CLOCK_MONOTONIC — ordre causal (ne recule jamais)
**   Distinction explicite : l'unité est la nanoseconde, pas la résolution.
**
** Limites honnêtes :
**   - BIT_ID : 2^32 bits max par session, run_id non cryptographique
**   - LUM dimensions limitées à 16 bits (group_index, bit_in_group)
**   - Résolution réelle horloge non démontrée expérimentalement
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#ifndef FORENSIC_UNIF_002_H
#define FORENSIC_UNIF_002_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Identifiants ─────────────────────────────────────────────────────────── */

/* BIT_ID : identifiant d'un bit d'entrée dans la session courante.
 *   bits [63:32] = run_id  (session courante)
 *   bits [31:0]  = bit_global_seq (monotone croissant dans la session)
 * Domaine : unique dans la session, pas de garantie cryptographique globale.
 */
typedef uint64_t bit_id_t;

/* LUM_ID : identifiant d'un LUM dérivé d'un bit.
 *   bits [63:32] = run_id
 *   bits [31:16] = group_index (index dans le lum_group, max 65535)
 *   bits [15:0]  = bit_in_group (position du bit dans l'octet/groupe, max 65535)
 */
typedef uint64_t lum_id_t;

/* ── Types d'événements de traçabilité ────────────────────────────────────── */

typedef enum {
    FU002_EVT_BIT_INPUT        = 0x01,  /* bit lu depuis l'entrée brute      */
    FU002_EVT_LUM_CREATED      = 0x02,  /* LUM créé depuis un bit            */
    FU002_EVT_LUM_TRANSFORMED  = 0x03,  /* LUM passé dans un module optim    */
    FU002_EVT_LUM_GROUPED      = 0x04,  /* LUM ajouté à un groupe            */
    FU002_EVT_LUM_RESULT       = 0x05,  /* LUM produit en sortie             */
    FU002_EVT_LOSS_DETECTED    = 0x06,  /* gap dans la séquence = perte      */
    FU002_EVT_DUPLICATE        = 0x07,  /* même bit_id vu deux fois          */
    FU002_EVT_SESSION_START    = 0x10,  /* début de session (run_id généré)  */
    FU002_EVT_SESSION_END      = 0x11   /* fin de session (statistiques)     */
} fu002_event_type_e;

/* ── Enregistrement d'un événement ───────────────────────────────────────── */

typedef struct {
    fu002_event_type_e event_type;       /* type d'événement                  */
    bit_id_t           bit_id;           /* identifiant du bit concerné       */
    lum_id_t           lum_id;           /* identifiant du LUM concerné       */
    bit_id_t           parent_id;        /* BIT_ID de l'entrée parente        */
    bit_id_t           child_id;         /* BIT_ID de la sortie enfant        */
    uint64_t           ts_realtime_ns;   /* CLOCK_REALTIME (corrélation ext.) */
    uint64_t           ts_monotonic_ns;  /* CLOCK_MONOTONIC (ordre causal)    */
    uint64_t           event_seq;        /* séquence monotone globale         */
    uint32_t           run_id;           /* run_id de la session              */
    uint8_t            bit_value;        /* valeur brute du bit (0 ou 1)      */
    uint8_t            bit_value_before; /* valeur du bit AVANT transformation*/
    uint8_t            bit_value_after;  /* valeur du bit APRÈS transformation*/
    char               module_name[32];  /* nom du module (SIMD/MEM/PARALLEL) */
    char               operation[64];    /* description de l'opération        */
} fu002_event_t;

/* ── Statistiques de session ──────────────────────────────────────────────── */

typedef struct {
    uint64_t total_bits_input;       /* total bits lus en entrée           */
    uint64_t total_lums_created;     /* total LUM créés                    */
    uint64_t total_transformations;  /* transformations de LUM             */
    uint64_t total_results;          /* LUM résultats en sortie            */
    uint64_t loss_count;             /* gaps détectés (pertes)             */
    uint64_t duplicate_count;        /* duplications détectées             */
    uint64_t last_seq_seen;          /* dernière séquence observée         */
    bool     has_last_seq_seen;     /* flag : au moins 1 seq observée     */
    uint32_t run_id;                 /* run_id de la session               */
    bool     integrity_ok;          /* true si loss_count+dup_count == 0  */
} fu002_session_stats_t;

/* ── API publique ──────────────────────────────────────────────────────────── */

/* Initialise le module : génère run_id, ouvre le fichier log, écrit SESSION_START.
 * Thread-safe : sérialise attribution_seq + write + flush sous un seul mutex.
 * Retourne true en cas de succès. */
bool forensic_unif002_init(const char* log_path);

/* Ferme la session : écrit SESSION_END + statistiques, ferme le fichier.
 * Attend la fin de toutes les écritures en cours (mutex). */
void forensic_unif002_destroy(void);

/* Génère un BIT_ID unique pour un bit d'entrée.
 * Incrémente le compteur monotone bit_seq_counter.
 * bit_value = 0 ou 1. */
bit_id_t forensic_unif002_new_bit_id(uint8_t bit_value);

/* Génère un LUM_ID depuis un bit_id et la position dans le groupe. */
lum_id_t forensic_unif002_lum_id_from_bit(bit_id_t bit_id,
                                           uint16_t group_index,
                                           uint16_t bit_in_group);

/* Enregistre un événement de traçabilité complet.
 * parent_id = 0 si pas de parent connu.
 * child_id  = 0 si pas encore de sortie connue.
 * bit_value_before/after = 0 si non applicable (ex. SESSION_START).
 * Thread-safe : attribution seq + write + flush sous le même mutex. */
void forensic_unif002_log_event(fu002_event_type_e event_type,
                                bit_id_t            bit_id,
                                lum_id_t            lum_id,
                                bit_id_t            parent_id,
                                bit_id_t            child_id,
                                uint8_t             bit_value,
                                const char*         module_name,
                                const char*         operation);

/* Raccourcis pour les cas fréquents. */
void forensic_unif002_log_bit_input(bit_id_t bit_id, uint8_t bit_value,
                                    const char* source);
void forensic_unif002_log_lum_created(lum_id_t lum_id, bit_id_t parent_bit_id,
                                      const char* module);

/* Version avec provenance avant/après pour les transformations (audit 186 §6). */
void forensic_unif002_log_transformation_ex(lum_id_t    lum_id,
                                            bit_id_t    parent_id,
                                            bit_id_t    child_id,
                                            uint8_t     bit_before,
                                            uint8_t     bit_after,
                                            const char* module,
                                            const char* op);

/* Version simple (compat ascendante) — bit_before/after = 0. */
void forensic_unif002_log_transformation(lum_id_t lum_id, bit_id_t parent_id,
                                         bit_id_t child_id,
                                         const char* module, const char* op);
void forensic_unif002_log_result(lum_id_t lum_id, bit_id_t parent_id,
                                 const char* module);

/* Détection active de pertes/duplications (FU002-FIX-001).
 * Utilise un flag has_last_seq_seen pour éviter le faux positif
 * sur la séquence 0 au démarrage (pas d'initialisation à UINT64_MAX
 * qui provoquerait un overflow sur last+1).
 * Logique :
 *   - première séquence → enregistrée comme référence
 *   - seq == last+1     → normale
 *   - seq > last+1      → perte (gap = seq - last - 1)
 *   - seq == last       → duplication
 *   - seq < last        → anomalie classifiée
 */
void forensic_unif002_check_continuity(uint64_t seq_expected);

/* Retourne les statistiques de la session courante. */
fu002_session_stats_t forensic_unif002_get_stats(void);

/* Retourne le run_id courant. */
uint32_t forensic_unif002_get_run_id(void);

/* Retourne le compteur de séquence bit courant. */
uint64_t forensic_unif002_get_bit_seq(void);

#endif /* FORENSIC_UNIF_002_H */
