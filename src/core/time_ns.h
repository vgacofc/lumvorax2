#ifndef TIME_NS_H
#define TIME_NS_H

#include <stdint.h>

uint64_t time_ns_get_absolute(void);
uint64_t time_ns_get_thread_cpu(void);

#endif
