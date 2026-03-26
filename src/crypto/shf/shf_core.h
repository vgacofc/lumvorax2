#ifndef SHF_CORE_H
#define SHF_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct shf_context shf_context_t;

shf_context_t* shf_core_init(void);
void shf_core_destroy(shf_context_t* ctx);
int shf_core_compute(shf_context_t* ctx, const uint8_t* data, size_t len, uint8_t* hash);
int shf_core_verify(shf_context_t* ctx, const uint8_t* data, size_t len, const uint8_t* expected);

#endif
