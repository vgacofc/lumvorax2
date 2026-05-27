#include <stdlib.h>

void* tracked_malloc(size_t size) {
    return malloc(size);
}

void tracked_free(void* ptr) {
    free(ptr);
}
