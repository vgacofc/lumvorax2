#ifndef FORENSIC_LOGGER_H
#define FORENSIC_LOGGER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include "../common/common_types.h"

// Using unified forensic levels from common_types.h
typedef unified_forensic_level_e forensic_level_e;

#include "../lum/lum_core.h"

/* ── FORENSIC-UNIF-003 : compteur séquentiel global ─────────────────────────
 * Chaque appel à forensic_log_individual_lum() incrémente un compteur
 * atomique (uint64_t protégé par mutex) et l'écrit dans le log.
 * Permet la détection de pertes (gap dans la séquence) et de duplications
 * (même event_seq deux fois).
 * Lecture via forensic_get_event_seq() pour les tests de campagne.
 */
uint64_t forensic_get_event_seq(void);

/* ── BUG-3 FIX : horloge unifiée ─────────────────────────────────────────────
 * forensic_get_monotonic_ns() retourne CLOCK_MONOTONIC — utilisé pour l'ordre
 * et les durées. Le header de log peut ainsi utiliser la même source que les
 * événements.
 * L'horloge civile (CLOCK_REALTIME) reste disponible via time_ns_get_absolute()
 * pour la corrélation externe, mais elle n'est plus mélangée avec MONOTONIC.
 */
uint64_t forensic_get_monotonic_ns(void);

// Macros timing différenciées selon usage
#define FORENSIC_TIMING_START(timer_var) \
    struct timespec timer_var##_start, timer_var##_end; \
    clock_gettime(CLOCK_MONOTONIC, &timer_var##_start)

#define FORENSIC_TIMING_END(timer_var) \
    clock_gettime(CLOCK_MONOTONIC, &timer_var##_end)

#define FORENSIC_TIMING_CALC_NS(timer_var) \
    ((timer_var##_end.tv_sec - timer_var##_start.tv_sec) * 1000000000ULL + \
     (timer_var##_end.tv_nsec - timer_var##_start.tv_nsec))

#define FILE_TIMESTAMP_GET() \
    ({ \
        struct timespec ts; \
        clock_gettime(CLOCK_REALTIME, &ts); \
        ts.tv_sec * 1000000000ULL + ts.tv_nsec; \
    })

bool forensic_logger_init(const char* filename);
bool forensic_logger_init_individual_files(void);
void forensic_log_memory_operation(const char* operation, void* ptr, size_t size);
void forensic_log_lum_operation(const char* operation, uint64_t lum_count, double duration_ns);

/* FORENSIC-UNIF-003 : lum_id est maintenant uint64_t (corrige BUG-1 troncature 64→32).
 * La signature est rétrocompatible au niveau appel si le code appelant utilisait un cast
 * explicite (uint32_t) — tous les sites d'appel DOIVENT être mis à jour pour passer le
 * uint64_t complet sans troncature.
 *
 * Sémantique du timestamp_ns : CLOCK_REALTIME (horloge civile) pour corrélation externe.
 * event_seq (compteur monotone interne) garantit l'ordre même si deux événements partagent
 * le même timestamp_ns (résolution OS ≥ 1 µs en pratique).
 */
void forensic_log_individual_lum(uint64_t lum_id, const char* operation, uint64_t timestamp_ns);

void forensic_logger_destroy(void);

// General forensic logging function
void forensic_log(forensic_level_e level, const char* function, const char* format, ...);

#endif // FORENSIC_LOGGER_H