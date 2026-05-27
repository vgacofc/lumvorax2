#include "time_ns.h"
#include <time.h>
#include <stdint.h>
#include <stdio.h>

/**
 * time_ns_get_absolute
 * Returns the absolute time in nanoseconds since the epoch.
 * Standard Name: time_ns_get_absolute
 */
uint64_t time_ns_get_absolute(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

/**
 * time_ns_get_thread_cpu
 * Returns the CPU time for the current thread in nanoseconds.
 */
uint64_t time_ns_get_thread_cpu(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}
