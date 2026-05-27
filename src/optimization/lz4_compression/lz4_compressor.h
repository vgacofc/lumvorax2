#ifndef LZ4_COMPRESSOR_H
#define LZ4_COMPRESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define LZ4_VERSION_MAJOR 1
#define LZ4_VERSION_MINOR 0
#define LZ4_VERSION_PATCH 0

#define LZ4_BLOCK_SIZE 65536
#define LZ4_HASH_LOG 12
#define LZ4_HASH_SIZE (1 << LZ4_HASH_LOG)
#define LZ4_MIN_MATCH 4
#define LZ4_MAX_DISTANCE 65535
#define LZ4_ML_BITS 4
#define LZ4_ML_MASK ((1U << LZ4_ML_BITS) - 1)
#define LZ4_RUN_BITS (8 - LZ4_ML_BITS)
#define LZ4_RUN_MASK ((1U << LZ4_RUN_BITS) - 1)

typedef struct {
    uint32_t hash_table[LZ4_HASH_SIZE];
    size_t dict_size;
    const uint8_t* dict;
} lz4_stream_t;

typedef struct {
    uint8_t* prefix;
    size_t prefix_size;
    size_t external_dict_size;
    const uint8_t* external_dict;
} lz4_decode_stream_t;

typedef struct {
    uint64_t compressed_bytes;
    uint64_t uncompressed_bytes;
    double compression_ratio;
    uint64_t compression_time_ns;
    uint64_t decompression_time_ns;
    uint64_t blocks_processed;
} lz4_stats_t;

typedef struct {
    lz4_stream_t* encode_stream;
    lz4_decode_stream_t* decode_stream;
    lz4_stats_t stats;
    int acceleration;
    bool initialized;
} lz4_context_t;

lz4_context_t* lz4_context_create(void);
void lz4_context_destroy(lz4_context_t* ctx);
int lz4_compress(lz4_context_t* ctx, const void* src, size_t src_size,
                 void* dst, size_t dst_capacity);
int lz4_decompress(lz4_context_t* ctx, const void* src, size_t src_size,
                   void* dst, size_t dst_capacity);
size_t lz4_compress_bound(size_t input_size);
int lz4_compress_fast(lz4_context_t* ctx, const void* src, size_t src_size,
                      void* dst, size_t dst_capacity, int acceleration);
void lz4_context_reset(lz4_context_t* ctx);
void lz4_get_stats(lz4_context_t* ctx, lz4_stats_t* stats);
double lz4_get_ratio(lz4_context_t* ctx);

#endif
