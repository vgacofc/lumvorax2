#ifndef LOG_WRITER_H
#define LOG_WRITER_H

#include <stdint.h>
#include <stddef.h>

void log_init_session();
void log_writer_entry(const char* module, const char* event, uint64_t value);
uint32_t checksum_generate(const char* data, size_t len);

#endif
