#ifndef AUDIT_HARDENING_H
#define AUDIT_HARDENING_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint64_t total_audits; uint64_t vulnerabilities_found; bool hardened; } audit_system_t;
audit_system_t* audit_system_create(void);
void audit_system_run(audit_system_t* sys);
void audit_system_destroy(audit_system_t* sys);
#endif