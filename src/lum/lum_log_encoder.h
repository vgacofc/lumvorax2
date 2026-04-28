/* ============================================================================
 * LUM Log Encoder — Cycle C111
 * ----------------------------------------------------------------------------
 * Encode tout type de log (texte, JSON, CSV, binaire) au format LUM 100 %.
 *
 * Principe : chaque évènement de log devient une séquence de lum_t alignés
 * 64 octets, écrits dans un fichier .lum binaire monotone (append-only).
 *
 * Le format .lum est canonique pour le projet LumVorax2 ; les autres formats
 * (txt, json, csv) sont strictement réservés à la conversion / l'analyse
 * externe (cf. tools/lum_to_json.py qui sera ajouté en C112).
 * ============================================================================ */
#ifndef LUM_LOG_ENCODER_H_INCLUDED
#define LUM_LOG_ENCODER_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include "lum_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Types d'évènement loggés (encodés dans lum_t.structure_type).
 * NOTE C112 (REVERT C111) : renommage prophylactique LUM_LOG_KIND_* ANNULÉ
 * sur demande utilisateur — règle "renommer LES NOUVEAUX avec les ANCIENS noms,
 * jamais l'inverse (sinon casse modules existants)". Les noms LUM_LOG_INFO/
 * WARN/ERROR existent aussi dans src/logger/lum_logger.h (legacy, valeurs 0-3),
 * mais aucun .c lié au binaire BTC n'inclut SIMULTANÉMENT les deux headers
 * (vérifié C112 : seul memory_tracker.c inclut le legacy, et sans utiliser
 * ces enums). Valeurs différentes (10-50 ici vs 0-3 legacy) → toute future
 * double-inclusion provoquera une erreur de redéfinition claire au compile.
 * Les types METRIC/RECORD/DECISION/QUANTUM_OBS sont nouveaux et ne collisionnent
 * avec rien. */
typedef enum {
    LUM_LOG_INFO         = 10,
    LUM_LOG_WARN         = 11,
    LUM_LOG_ERROR        = 12,
    LUM_LOG_METRIC       = 20,  /* mesure quantitative (hashrate, etc.) */
    LUM_LOG_RECORD       = 30,  /* nouveau record (best_lz, etc.)       */
    LUM_LOG_DECISION     = 40,  /* décision NX48 / reasoning            */
    LUM_LOG_QUANTUM_OBS  = 50   /* observable quantique mesuré          */
} lum_log_kind_t;

typedef struct lum_log_writer lum_log_writer_t;

/* Crée un writer vers un fichier .lum (append). Retourne NULL si erreur. */
lum_log_writer_t* lum_log_writer_open(const char* lum_path);

/* Écrit un évènement texte court (≤ 40 octets de payload effectif). */
int lum_log_writer_write_text(lum_log_writer_t* w,
                               lum_log_kind_t kind,
                               const char* text);

/* Écrit une mesure scalaire (double encodé via cast bitwise). */
int lum_log_writer_write_metric(lum_log_writer_t* w,
                                 const char* metric_name,
                                 double value);

/* Écrit un record (key + valeur uint64). */
int lum_log_writer_write_record(lum_log_writer_t* w,
                                 const char* record_name,
                                 uint64_t value);

/* Ferme le writer + flush + fsync. */
int lum_log_writer_close(lum_log_writer_t* w);

#ifdef __cplusplus
}
#endif

#endif /* LUM_LOG_ENCODER_H_INCLUDED */
