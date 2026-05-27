#ifndef NX_FORENSIC_H
#define NX_FORENSIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
    FILE *file;
    uint64_t t0_ns;
    bool enabled;
} nx_forensic_logger_t;

bool nx_forensic_open(nx_forensic_logger_t *logger, const char *path);
void nx_forensic_close(nx_forensic_logger_t *logger);
uint64_t nx_forensic_now_ns(void);

void nx_forensic_log(nx_forensic_logger_t *logger,
                     const char *layer,
                     const char *phase,
                     const char *subpoint,
                     const char *message,
                     double value);

void nx_forensic_log_expert_question(nx_forensic_logger_t *logger,
                                     const char *phase,
                                     const char *question,
                                     const char *answer);

#endif
