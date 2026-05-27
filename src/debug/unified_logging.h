#ifndef UNIFIED_LOGGING_H
#define UNIFIED_LOGGING_H

#include <stdio.h>
#include <stdarg.h>

void unified_forensic_log(const char* module, const char* message, ...);
void forensic_log_individual_lum(const char* info);

#endif
