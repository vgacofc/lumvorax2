#ifndef SIMD_BATCH_PROCESSOR_H
#define SIMD_BATCH_PROCESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SIMD_BATCH_VERSION_MAJOR 1
#define SIMD_BATCH_VERSION_MINOR 0
#define SIMD_BATCH_VERSION_PATCH 0

#define SIMD_BATCH_SIZE 256
#define SIMD_VECTOR_WIDTH 8
#define SIMD_CACHE_LINE 64

typedef enum {
    SIMD_OP_ADD,
    SIMD_OP_SUB,
    SIMD_OP_MUL,
    SIMD_OP_DIV,
    SIMD_OP_FMA,
    SIMD_OP_DOT,
    SIMD_OP_NORM,
    SIMD_OP_REDUCE_SUM,
    SIMD_OP_REDUCE_MAX,
    SIMD_OP_REDUCE_MIN
} simd_batch_op_t;

typedef struct {
    float* data __attribute__((aligned(32)));
    size_t count;
    size_t capacity;
    size_t stride;
} simd_batch_buffer_t;

typedef struct {
    simd_batch_op_t operation;
    simd_batch_buffer_t* input_a;
    simd_batch_buffer_t* input_b;
    simd_batch_buffer_t* output;
    float scalar;
    bool use_fma;
} simd_batch_task_t;

typedef struct {
    simd_batch_task_t* tasks;
    size_t task_count;
    size_t task_capacity;
    uint64_t total_operations;
    uint64_t total_time_ns;
    double throughput_gflops;
    bool avx2_enabled;
    bool avx512_enabled;
} simd_batch_processor_t;

simd_batch_processor_t* simd_batch_processor_create(void);
void simd_batch_processor_destroy(simd_batch_processor_t* proc);
simd_batch_buffer_t* simd_batch_buffer_create(size_t count);
void simd_batch_buffer_destroy(simd_batch_buffer_t* buffer);
void simd_batch_buffer_fill(simd_batch_buffer_t* buffer, float value);
void simd_batch_buffer_copy(simd_batch_buffer_t* dst, const simd_batch_buffer_t* src);
int simd_batch_add(simd_batch_processor_t* proc, simd_batch_buffer_t* a, 
                   simd_batch_buffer_t* b, simd_batch_buffer_t* out);
int simd_batch_mul(simd_batch_processor_t* proc, simd_batch_buffer_t* a, 
                   simd_batch_buffer_t* b, simd_batch_buffer_t* out);
int simd_batch_fma(simd_batch_processor_t* proc, simd_batch_buffer_t* a, 
                   simd_batch_buffer_t* b, simd_batch_buffer_t* c, simd_batch_buffer_t* out);
float simd_batch_dot(simd_batch_processor_t* proc, simd_batch_buffer_t* a, simd_batch_buffer_t* b);
float simd_batch_reduce_sum(simd_batch_processor_t* proc, simd_batch_buffer_t* buffer);
int simd_batch_execute_all(simd_batch_processor_t* proc);
double simd_batch_get_throughput(simd_batch_processor_t* proc);

#endif
