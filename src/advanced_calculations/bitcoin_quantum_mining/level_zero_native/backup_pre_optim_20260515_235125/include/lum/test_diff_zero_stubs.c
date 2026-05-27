/* ============================================================================
 * test_diff_zero_stubs.c — Cycle C133
 * ----------------------------------------------------------------------------
 * Stubs minimaux pour les symboles forensic exigés par lum_core.c
 * dans le contexte du test unitaire `test_bit_level_diff_zero`.
 *
 * Le test diff=0 mesure la fidélité bit-à-bit du pipeline LUM trace/reconstruct.
 * Il n'a PAS besoin de la chaîne forensic complète (ultra_forensic_logger,
 * memory_tracker enrichi, async_logger). Ces stubs fournissent une
 * implémentation minimale des appels (malloc/free directs, log silencieux).
 *
 * IMPORTANT : ces stubs sont destinés UNIQUEMENT au binaire de test.
 * Le binaire BTC (`btc_mining_runner`) continue d'utiliser les implémentations
 * réelles via son propre Makefile — ces stubs ne sont jamais linkés en prod.
 * ============================================================================ */
#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── memory_tracker.h ─────────────────────────────────────────────────────── */
void* tracked_malloc(size_t size, const char* file, int line, const char* func) {
    (void)file; (void)line; (void)func;
    return malloc(size);
}

void tracked_free(void* ptr, const char* file, int line, const char* func) {
    (void)file; (void)line; (void)func;
    free(ptr);
}

void* tracked_calloc(size_t nmemb, size_t size,
                     const char* file, int line, const char* func) {
    (void)file; (void)line; (void)func;
    return calloc(nmemb, size);
}

void* tracked_realloc(void* ptr, size_t size,
                      const char* file, int line, const char* func) {
    (void)file; (void)line; (void)func;
    return realloc(ptr, size);
}

/* ── unified_logging.h / forensic_logger.h ────────────────────────────────── */
void unified_forensic_log(const char* module, const char* message, ...) {
    (void)module;
    (void)message;
    /* Silencieux par défaut. Si l'utilisateur veut tracer, exporter
     * C133_VERBOSE_STUBS=1 redirige vers stderr (utile en debug). */
    static int verbose = -1;
    if (verbose == -1) verbose = (getenv("C133_VERBOSE_STUBS") != NULL);
    if (verbose) {
        va_list ap;
        va_start(ap, message);
        fprintf(stderr, "[STUB-LOG][%s] ", module ? module : "?");
        vfprintf(stderr, message, ap);
        fputc('\n', stderr);
        va_end(ap);
    }
}

/* forensic_logger.h : signature avec arguments (uint32_t, const char*, uint64_t) */
void forensic_log_lum_operation(const char* operation,
                                 uint64_t lum_count,
                                 double duration_ns) {
    (void)operation; (void)lum_count; (void)duration_ns;
}

void forensic_log_individual_lum(uint32_t lum_id,
                                  const char* operation,
                                  uint64_t timestamp_ns) {
    (void)lum_id; (void)operation; (void)timestamp_ns;
}
