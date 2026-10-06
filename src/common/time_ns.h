#ifndef TIME_NS_H
#define TIME_NS_H

#include <stdint.h>

/* CLOCK_REALTIME — horloge civile (epoch). Utilisée pour corrélation externe. */
uint64_t time_ns_get_absolute(void);

/* CLOCK_MONOTONIC — horloge monotone (uptime). Utilisée pour ordre/durées.
 * FORENSIC-UNIF-004 BUG-3 FIX : tous les timestamps d'événements forensic
 * doivent utiliser cette fonction pour garantir la monotonie contractuelle. */
uint64_t time_ns_get_monotonic(void);

uint64_t time_ns_get_thread_cpu(void);

#endif
