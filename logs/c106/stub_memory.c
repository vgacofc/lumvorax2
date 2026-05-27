#include <stdlib.h>
void tracked_free(void* p, const char* file, int line, const char* func) {
    (void)file; (void)line; (void)func; free(p);
}
void* tracked_malloc(size_t s, const char* file, int line, const char* func) {
    (void)file; (void)line; (void)func; return malloc(s);
}
void* tracked_calloc(size_t n, size_t s, const char* file, int line, const char* func) {
    (void)file; (void)line; (void)func; return calloc(n, s);
}
#include <stdio.h>
#include <stdarg.h>
void unified_forensic_log(int level, const char* mod, const char* fmt, ...) {
    (void)level; (void)mod;
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap); fputc('\n', stderr);
    va_end(ap);
}
