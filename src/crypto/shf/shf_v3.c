#include "shf_v3.h"
#include <stdlib.h>
#include <string.h>

struct shf_v3_context {
    uint8_t state[256];
    shf_v3_config_t config;
    bool initialized;
};

shf_v3_context_t* shf_v3_init(void) {
    shf_v3_context_t* ctx = (shf_v3_context_t*)malloc(sizeof(shf_v3_context_t));
    if (ctx) {
        memset(ctx->state, 0, 256);
        ctx->config = shf_v3_default_config();
        ctx->initialized = true;
    }
    return ctx;
}

void shf_v3_destroy(shf_v3_context_t* ctx) {
    if (ctx) {
        memset(ctx, 0, sizeof(shf_v3_context_t));
        free(ctx);
    }
}

shf_v3_config_t shf_v3_default_config(void) {
    shf_v3_config_t config = {
        .rounds = 64,
        .block_size = 256,
        .use_simd = true
    };
    return config;
}

int shf_v3_configure(shf_v3_context_t* ctx, const shf_v3_config_t* config) {
    if (!ctx || !config) return -1;
    ctx->config = *config;
    return 0;
}
