#ifndef SHF_V2_H
#define SHF_V2_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct shf_v2_context shf_v2_context_t;

typedef struct {
    uint8_t hash[64];
    uint64_t computation_time_ns;
    bool verified;
} shf_v2_result_t;

shf_v2_context_t* shf_v2_init(void);
void shf_v2_destroy(shf_v2_context_t* ctx);
int shf_v2_compute(shf_v2_context_t* ctx, const uint8_t* data, size_t len, shf_v2_result_t* result);

#endif
