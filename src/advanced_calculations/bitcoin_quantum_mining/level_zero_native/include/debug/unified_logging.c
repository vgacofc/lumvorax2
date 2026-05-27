#include "unified_logging.h"
#include <time.h>
#include <string.h>

void unified_forensic_log(const char* module, const char* message, ...) {
    va_list args;
    va_start(args, message);
    printf("[FORENSIC][%s] ", module);
    vprintf(message, args);
    printf("\n");
    va_end(args);
}

void forensic_log_individual_lum(const char* info) {
    unified_forensic_log("LUM", "%s", info);
}
