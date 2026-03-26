#include "shf_v2.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct shf_v2_context {
    uint8_t state[128];
    uint64_t counter;
    bool initialized;
};

shf_v2_context_t* shf_v2_init(void) {
    shf_v2_context_t* ctx = (shf_v2_context_t*)malloc(sizeof(shf_v2_context_t));
    if (ctx) {
        memset(ctx->state, 0, 128);
        ctx->counter = 0;
        ctx->initialized = true;
    }
    return ctx;
}

void shf_v2_destroy(shf_v2_context_t* ctx) {
    if (ctx) {
        memset(ctx, 0, sizeof(shf_v2_context_t));
        free(ctx);
    }
}

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int shf_v2_compute(shf_v2_context_t* ctx, const uint8_t* data, size_t len, shf_v2_result_t* result) {
    if (!ctx || !data || !result) return -1;
    
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < len; i++) {
        ctx->state[i % 128] ^= data[i];
        ctx->state[(i + 31) % 128] += data[i] ^ (ctx->counter & 0xFF);
        ctx->counter++;
    }
    
    memcpy(result->hash, ctx->state, 64);
    result->computation_time_ns = get_time_ns() - start;
    result->verified = true;
    
    return 0;
}
