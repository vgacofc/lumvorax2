#ifndef MONITORING_ALERTING_H
#define MONITORING_ALERTING_H
typedef struct { float cpu_usage; float mem_usage; int alerts_active; } monitor_system_t;
monitor_system_t* monitor_system_create(void);
void monitor_system_update(monitor_system_t* sys);
void monitor_system_destroy(monitor_system_t* sys);
#endif