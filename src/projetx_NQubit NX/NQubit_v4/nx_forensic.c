#define _POSIX_C_SOURCE 200809L
#include "nx_forensic.h"

#include <inttypes.h>
#include <time.h>

uint64_t nx_forensic_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

bool nx_forensic_open(nx_forensic_logger_t *logger, const char *path) {
    if (logger == NULL || path == NULL) {
        return false;
    }
    logger->file = fopen(path, "w");
    if (logger->file == NULL) {
        logger->enabled = false;
        return false;
    }
    logger->t0_ns = nx_forensic_now_ns();
    logger->enabled = true;
    fprintf(logger->file, "# NX FORENSIC LOG START t0_ns=%" PRIu64 "\n", logger->t0_ns);
    fflush(logger->file);
    return true;
}

void nx_forensic_close(nx_forensic_logger_t *logger) {
    if (logger == NULL || logger->file == NULL) {
        return;
    }
    fprintf(logger->file, "# NX FORENSIC LOG END t_ns=%" PRIu64 "\n", nx_forensic_now_ns());
    fclose(logger->file);
    logger->file = NULL;
    logger->enabled = false;
}

void nx_forensic_log(nx_forensic_logger_t *logger,
                     const char *layer,
                     const char *phase,
                     const char *subpoint,
                     const char *message,
                     double value) {
    if (logger == NULL || logger->file == NULL || !logger->enabled) {
        return;
    }
    const uint64_t now_ns = nx_forensic_now_ns();
    const uint64_t delta_ns = now_ns - logger->t0_ns;
    fprintf(logger->file,
            "t_ns=%" PRIu64 " delta_ns=%" PRIu64 " layer=%s phase=%s sub=%s msg=%s value=%.12f\n",
            now_ns,
            delta_ns,
            layer,
            phase,
            subpoint,
            message,
            value);
}

void nx_forensic_log_expert_question(nx_forensic_logger_t *logger,
                                     const char *phase,
                                     const char *question,
                                     const char *answer) {
    if (logger == NULL || logger->file == NULL || !logger->enabled) {
        return;
    }
    const uint64_t now_ns = nx_forensic_now_ns();
    const uint64_t delta_ns = now_ns - logger->t0_ns;
    fprintf(logger->file,
            "t_ns=%" PRIu64 " delta_ns=%" PRIu64 " layer=expert_review phase=%s question=\"%s\" answer=\"%s\"\n",
            now_ns,
            delta_ns,
            phase,
            question,
            answer);
}
