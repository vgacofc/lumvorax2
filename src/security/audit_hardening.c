#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "audit_hardening.h"

audit_system_t* audit_system_create(void) {
    audit_system_t* sys = calloc(1, sizeof(audit_system_t));
    if (sys) sys->hardened = true;
    return sys;
}

void audit_system_run(audit_system_t* sys) {
    if (sys) sys->total_audits++;
}

void audit_system_destroy(audit_system_t* sys) {
    free(sys);
}
