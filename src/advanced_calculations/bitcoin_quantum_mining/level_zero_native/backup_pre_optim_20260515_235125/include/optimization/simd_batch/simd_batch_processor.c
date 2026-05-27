#include "simd_batch_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef __AVX2__
#include <immintrin.h>
#define SIMD_AVX2_AVAILABLE 1
#else
#define SIMD_AVX2_AVAILABLE 0
#endif

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

simd_batch_processor_t* simd_batch_processor_create(void) {
    simd_batch_processor_t* proc = (simd_batch_processor_t*)calloc(1, sizeof(simd_batch_processor_t));
    if (!proc) return NULL;
    
    proc->task_capacity = 64;
    proc->tasks = (simd_batch_task_t*)calloc(proc->task_capacity, sizeof(simd_batch_task_t));
    if (!proc->tasks) {
        free(proc);
        return NULL;
    }
    
    proc->avx2_enabled = SIMD_AVX2_AVAILABLE;
    proc->avx512_enabled = false;
    
    return proc;
}

void simd_batch_processor_destroy(simd_batch_processor_t* proc) {
    if (proc) {
        free(proc->tasks);
        free(proc);
    }
}

simd_batch_buffer_t* simd_batch_buffer_create(size_t count) {
    simd_batch_buffer_t* buffer = (simd_batch_buffer_t*)calloc(1, sizeof(simd_batch_buffer_t));
    if (!buffer) return NULL;
    
    size_t aligned_count = ((count + SIMD_VECTOR_WIDTH - 1) / SIMD_VECTOR_WIDTH) * SIMD_VECTOR_WIDTH;
    
    if (posix_memalign((void**)&buffer->data, 32, aligned_count * sizeof(float)) != 0) {
        free(buffer);
        return NULL;
    }
    
    memset(buffer->data, 0, aligned_count * sizeof(float));
    buffer->count = count;
    buffer->capacity = aligned_count;
    buffer->stride = 1;
    
    return buffer;
}

void simd_batch_buffer_destroy(simd_batch_buffer_t* buffer) {
    if (buffer) {
        free(buffer->data);
        free(buffer);
    }
}

void simd_batch_buffer_fill(simd_batch_buffer_t* buffer, float value) {
    if (!buffer) return;
    
#if SIMD_AVX2_AVAILABLE
    __m256 val = _mm256_set1_ps(value);
    size_t i = 0;
    for (; i + 8 <= buffer->capacity; i += 8) {
        _mm256_store_ps(&buffer->data[i], val);
    }
    for (; i < buffer->count; i++) {
        buffer->data[i] = value;
    }
#else
    for (size_t i = 0; i < buffer->count; i++) {
        buffer->data[i] = value;
    }
#endif
}

void simd_batch_buffer_copy(simd_batch_buffer_t* dst, const simd_batch_buffer_t* src) {
    if (!dst || !src) return;
    size_t count = (dst->count < src->count) ? dst->count : src->count;
    memcpy(dst->data, src->data, count * sizeof(float));
}

int simd_batch_add(simd_batch_processor_t* proc, simd_batch_buffer_t* a,
                   simd_batch_buffer_t* b, simd_batch_buffer_t* out) {
    if (!proc || !a || !b || !out) return -1;
    if (a->count != b->count || a->count != out->count) return -2;
    
    uint64_t start = get_time_ns();
    
#if SIMD_AVX2_AVAILABLE
    size_t i = 0;
    for (; i + 8 <= a->count; i += 8) {
        __m256 va = _mm256_load_ps(&a->data[i]);
        __m256 vb = _mm256_load_ps(&b->data[i]);
        __m256 vr = _mm256_add_ps(va, vb);
        _mm256_store_ps(&out->data[i], vr);
    }
    for (; i < a->count; i++) {
        out->data[i] = a->data[i] + b->data[i];
    }
#else
    for (size_t i = 0; i < a->count; i++) {
        out->data[i] = a->data[i] + b->data[i];
    }
#endif
    
    uint64_t elapsed = get_time_ns() - start;
    proc->total_operations += a->count;
    proc->total_time_ns += elapsed;
    
    return 0;
}

int simd_batch_mul(simd_batch_processor_t* proc, simd_batch_buffer_t* a,
                   simd_batch_buffer_t* b, simd_batch_buffer_t* out) {
    if (!proc || !a || !b || !out) return -1;
    if (a->count != b->count || a->count != out->count) return -2;
    
    uint64_t start = get_time_ns();
    
#if SIMD_AVX2_AVAILABLE
    size_t i = 0;
    for (; i + 8 <= a->count; i += 8) {
        __m256 va = _mm256_load_ps(&a->data[i]);
        __m256 vb = _mm256_load_ps(&b->data[i]);
        __m256 vr = _mm256_mul_ps(va, vb);
        _mm256_store_ps(&out->data[i], vr);
    }
    for (; i < a->count; i++) {
        out->data[i] = a->data[i] * b->data[i];
    }
#else
    for (size_t i = 0; i < a->count; i++) {
        out->data[i] = a->data[i] * b->data[i];
    }
#endif
    
    uint64_t elapsed = get_time_ns() - start;
    proc->total_operations += a->count;
    proc->total_time_ns += elapsed;
    
    return 0;
}

int simd_batch_fma(simd_batch_processor_t* proc, simd_batch_buffer_t* a,
                   simd_batch_buffer_t* b, simd_batch_buffer_t* c, simd_batch_buffer_t* out) {
    if (!proc || !a || !b || !c || !out) return -1;
    
    uint64_t start = get_time_ns();
    
#if SIMD_AVX2_AVAILABLE
    size_t i = 0;
    for (; i + 8 <= a->count; i += 8) {
        __m256 va = _mm256_load_ps(&a->data[i]);
        __m256 vb = _mm256_load_ps(&b->data[i]);
        __m256 vc = _mm256_load_ps(&c->data[i]);
        __m256 vr = _mm256_fmadd_ps(va, vb, vc);
        _mm256_store_ps(&out->data[i], vr);
    }
    for (; i < a->count; i++) {
        out->data[i] = a->data[i] * b->data[i] + c->data[i];
    }
#else
    for (size_t i = 0; i < a->count; i++) {
        out->data[i] = a->data[i] * b->data[i] + c->data[i];
    }
#endif
    
    uint64_t elapsed = get_time_ns() - start;
    proc->total_operations += a->count * 2;
    proc->total_time_ns += elapsed;
    
    return 0;
}

float simd_batch_dot(simd_batch_processor_t* proc, simd_batch_buffer_t* a, simd_batch_buffer_t* b) {
    if (!proc || !a || !b || a->count != b->count) return 0.0f;
    
    uint64_t start = get_time_ns();
    float result = 0.0f;
    
#if SIMD_AVX2_AVAILABLE
    __m256 sum = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= a->count; i += 8) {
        __m256 va = _mm256_load_ps(&a->data[i]);
        __m256 vb = _mm256_load_ps(&b->data[i]);
        sum = _mm256_fmadd_ps(va, vb, sum);
    }
    
    float temp[8] __attribute__((aligned(32)));
    _mm256_store_ps(temp, sum);
    result = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
    
    for (; i < a->count; i++) {
        result += a->data[i] * b->data[i];
    }
#else
    for (size_t i = 0; i < a->count; i++) {
        result += a->data[i] * b->data[i];
    }
#endif
    
    uint64_t elapsed = get_time_ns() - start;
    proc->total_operations += a->count * 2;
    proc->total_time_ns += elapsed;
    
    return result;
}

float simd_batch_reduce_sum(simd_batch_processor_t* proc, simd_batch_buffer_t* buffer) {
    if (!proc || !buffer) return 0.0f;
    
    uint64_t start = get_time_ns();
    float result = 0.0f;
    
#if SIMD_AVX2_AVAILABLE
    __m256 sum = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= buffer->count; i += 8) {
        __m256 v = _mm256_load_ps(&buffer->data[i]);
        sum = _mm256_add_ps(sum, v);
    }
    
    float temp[8] __attribute__((aligned(32)));
    _mm256_store_ps(temp, sum);
    result = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
    
    for (; i < buffer->count; i++) {
        result += buffer->data[i];
    }
#else
    for (size_t i = 0; i < buffer->count; i++) {
        result += buffer->data[i];
    }
#endif
    
    uint64_t elapsed = get_time_ns() - start;
    proc->total_operations += buffer->count;
    proc->total_time_ns += elapsed;
    
    return result;
}

int simd_batch_execute_all(simd_batch_processor_t* proc) {
    if (!proc) return -1;
    
    for (size_t i = 0; i < proc->task_count; i++) {
        simd_batch_task_t* task = &proc->tasks[i];
        
        switch (task->operation) {
            case SIMD_OP_ADD:
                simd_batch_add(proc, task->input_a, task->input_b, task->output);
                break;
            case SIMD_OP_MUL:
                simd_batch_mul(proc, task->input_a, task->input_b, task->output);
                break;
            default:
                break;
        }
    }
    
    proc->task_count = 0;
    return 0;
}

double simd_batch_get_throughput(simd_batch_processor_t* proc) {
    if (!proc || proc->total_time_ns == 0) return 0.0;
    
    double ops_per_second = (double)proc->total_operations / ((double)proc->total_time_ns / 1e9);
    proc->throughput_gflops = ops_per_second / 1e9;
    return proc->throughput_gflops;
}
