#include "shf_core.h"
#include <stdlib.h>
#include <string.h>

struct shf_context {
    uint8_t state[64];
    uint64_t processed_bytes;
    bool initialized;
};

shf_context_t* shf_core_init(void) {
    shf_context_t* ctx = (shf_context_t*)malloc(sizeof(shf_context_t));
    if (ctx) {
        memset(ctx->state, 0, 64);
        ctx->processed_bytes = 0;
        ctx->initialized = true;
    }
    return ctx;
}

void shf_core_destroy(shf_context_t* ctx) {
    if (ctx) {
        memset(ctx, 0, sizeof(shf_context_t));
        free(ctx);
    }
}

int shf_core_compute(shf_context_t* ctx, const uint8_t* data, size_t len, uint8_t* hash) {
    if (!ctx || !data || !hash) return -1;
    
    for (size_t i = 0; i < len; i++) {
        ctx->state[i % 64] ^= data[i];
        ctx->state[(i + 17) % 64] += data[i];
    }
    
    memcpy(hash, ctx->state, 32);
    ctx->processed_bytes += len;
    return 0;
}

int shf_core_verify(shf_context_t* ctx, const uint8_t* data, size_t len, const uint8_t* expected) {
    uint8_t computed[32];
    if (shf_core_compute(ctx, data, len, computed) != 0) return -1;
    return memcmp(computed, expected, 32) == 0 ? 0 : 1;
}
