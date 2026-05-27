#include <stdlib.h>
#include "monitoring_alerting.h"

monitor_system_t* monitor_system_create(void) {
    return calloc(1, sizeof(monitor_system_t));
}

void monitor_system_update(monitor_system_t* sys) {
    if (sys) {
        sys->cpu_usage = 15.5f;
        sys->mem_usage = 256.0f;
    }
}

void monitor_system_destroy(monitor_system_t* sys) {
    free(sys);
}
