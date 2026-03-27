#ifndef SHF_V3_H
#define SHF_V3_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct shf_v3_context shf_v3_context_t;

typedef struct {
    size_t rounds;
    size_t block_size;
    bool use_simd;
} shf_v3_config_t;

shf_v3_context_t* shf_v3_init(void);
void shf_v3_destroy(shf_v3_context_t* ctx);
shf_v3_config_t shf_v3_default_config(void);
int shf_v3_configure(shf_v3_context_t* ctx, const shf_v3_config_t* config);

#endif
