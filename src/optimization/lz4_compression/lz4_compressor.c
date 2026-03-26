#include "lz4_compressor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline uint32_t lz4_hash(uint32_t value) {
    return ((value * 2654435761U) >> (32 - LZ4_HASH_LOG)) & (LZ4_HASH_SIZE - 1);
}

static inline uint32_t lz4_read32(const void* ptr) {
    uint32_t val;
    memcpy(&val, ptr, sizeof(val));
    return val;
}

static inline void lz4_write16(void* ptr, uint16_t val) {
    memcpy(ptr, &val, sizeof(val));
}

lz4_context_t* lz4_context_create(void) {
    lz4_context_t* ctx = (lz4_context_t*)calloc(1, sizeof(lz4_context_t));
    if (!ctx) return NULL;
    
    ctx->encode_stream = (lz4_stream_t*)calloc(1, sizeof(lz4_stream_t));
    if (!ctx->encode_stream) {
        free(ctx);
        return NULL;
    }
    
    ctx->decode_stream = (lz4_decode_stream_t*)calloc(1, sizeof(lz4_decode_stream_t));
    if (!ctx->decode_stream) {
        free(ctx->encode_stream);
        free(ctx);
        return NULL;
    }
    
    ctx->acceleration = 1;
    ctx->initialized = true;
    
    return ctx;
}

void lz4_context_destroy(lz4_context_t* ctx) {
    if (ctx) {
        free(ctx->encode_stream);
        free(ctx->decode_stream);
        free(ctx);
    }
}

size_t lz4_compress_bound(size_t input_size) {
    return input_size + (input_size / 255) + 16;
}

int lz4_compress(lz4_context_t* ctx, const void* src, size_t src_size,
                 void* dst, size_t dst_capacity) {
    if (!ctx || !src || !dst || src_size == 0) return -1;
    if (dst_capacity < lz4_compress_bound(src_size)) return -2;
    
    uint64_t start = get_time_ns();
    
    const uint8_t* ip = (const uint8_t*)src;
    const uint8_t* ip_end = ip + src_size;
    const uint8_t* ip_limit = ip_end - 12;
    const uint8_t* anchor = ip;
    
    uint8_t* op = (uint8_t*)dst;
    uint8_t* op_limit = op + dst_capacity;
    
    uint32_t* hash_table = ctx->encode_stream->hash_table;
    memset(hash_table, 0, sizeof(ctx->encode_stream->hash_table));
    
    if (src_size < 13) {
        size_t run = src_size;
        if (run >= 15) {
            *op++ = 0xF0;
            run -= 15;
            while (run >= 255) {
                *op++ = 255;
                run -= 255;
            }
            *op++ = (uint8_t)run;
        } else {
            *op++ = (uint8_t)(run << 4);
        }
        memcpy(op, src, src_size);
        op += src_size;
        
        ctx->stats.compressed_bytes += (op - (uint8_t*)dst);
        ctx->stats.uncompressed_bytes += src_size;
        ctx->stats.compression_time_ns += get_time_ns() - start;
        ctx->stats.blocks_processed++;
        
        return (int)(op - (uint8_t*)dst);
    }
    
    ip++;
    
    while (ip < ip_limit) {
        uint32_t h = lz4_hash(lz4_read32(ip));
        const uint8_t* ref = (const uint8_t*)src + hash_table[h];
        hash_table[h] = (uint32_t)(ip - (const uint8_t*)src);
        
        if (ref >= (const uint8_t*)src && ref < ip && 
            ip - ref <= LZ4_MAX_DISTANCE &&
            lz4_read32(ref) == lz4_read32(ip)) {
            
            size_t literal_len = ip - anchor;
            size_t match_len = LZ4_MIN_MATCH;
            
            while (ip + match_len < ip_end && ref[match_len] == ip[match_len]) {
                match_len++;
            }
            
            uint8_t* token = op++;
            
            if (literal_len >= 15) {
                *token = 0xF0;
                size_t rem = literal_len - 15;
                while (rem >= 255) {
                    *op++ = 255;
                    rem -= 255;
                }
                *op++ = (uint8_t)rem;
            } else {
                *token = (uint8_t)(literal_len << 4);
            }
            
            memcpy(op, anchor, literal_len);
            op += literal_len;
            
            uint16_t offset = (uint16_t)(ip - ref);
            lz4_write16(op, offset);
            op += 2;
            
            size_t ml = match_len - LZ4_MIN_MATCH;
            if (ml >= 15) {
                *token |= 0x0F;
                ml -= 15;
                while (ml >= 255) {
                    *op++ = 255;
                    ml -= 255;
                }
                *op++ = (uint8_t)ml;
            } else {
                *token |= (uint8_t)ml;
            }
            
            ip += match_len;
            anchor = ip;
        } else {
            ip++;
        }
    }
    
    size_t last_literal = ip_end - anchor;
    if (last_literal > 0) {
        uint8_t* token = op++;
        if (last_literal >= 15) {
            *token = 0xF0;
            size_t rem = last_literal - 15;
            while (rem >= 255) {
                *op++ = 255;
                rem -= 255;
            }
            *op++ = (uint8_t)rem;
        } else {
            *token = (uint8_t)(last_literal << 4);
        }
        memcpy(op, anchor, last_literal);
        op += last_literal;
    }
    
    int compressed_size = (int)(op - (uint8_t*)dst);
    
    ctx->stats.compressed_bytes += compressed_size;
    ctx->stats.uncompressed_bytes += src_size;
    ctx->stats.compression_time_ns += get_time_ns() - start;
    ctx->stats.blocks_processed++;
    
    if (ctx->stats.compressed_bytes > 0) {
        ctx->stats.compression_ratio = (double)ctx->stats.uncompressed_bytes / (double)ctx->stats.compressed_bytes;
    }
    
    return compressed_size;
}

int lz4_decompress(lz4_context_t* ctx, const void* src, size_t src_size,
                   void* dst, size_t dst_capacity) {
    if (!ctx || !src || !dst || src_size == 0) return -1;
    
    const uint8_t* ip = (const uint8_t*)src;
    const uint8_t* ip_end = ip + src_size;
    uint8_t* op = (uint8_t*)dst;
    uint8_t* op_end = op + dst_capacity;
    
    while (ip < ip_end) {
        uint8_t token = *ip++;
        size_t literal_len = (token >> 4) & 0x0F;
        if (literal_len == 15) {
            uint8_t s;
            do {
                if (ip >= ip_end) return -2;
                s = *ip++;
                literal_len += s;
            } while (s == 255);
        }
        
        if (literal_len > 0) {
            if (ip + literal_len > ip_end || op + literal_len > op_end) return -3;
            for (size_t i = 0; i < literal_len; i++) op[i] = ip[i];
            ip += literal_len;
            op += literal_len;
        }
        
        if (ip >= ip_end) break;
        if (ip + 2 > ip_end) return -5;
        // Extraction sécurisée bit-à-bit pour éviter l'alignement mémoire
        uint16_t offset = (uint16_t)ip[0] | ((uint16_t)ip[1] << 8);
        ip += 2;
        
        if (offset == 0 || (size_t)(op - (uint8_t*)dst) < (size_t)offset) return -6;
        
        size_t match_len = (token & 0x0F) + 4;
        if ((token & 0x0F) == 15) {
            uint8_t s;
            do {
                if (ip >= ip_end) return -8;
                s = *ip++;
                match_len += s;
            } while (s == 255);
        }
        
        if (op + match_len > op_end) return -9;
        const uint8_t* match = op - offset;
        // Boucle de copie explicite pour gérer l'overlapping sans memcpy
        for (size_t i = 0; i < match_len; i++) {
            op[i] = match[i];
        }
        op += match_len;
    }
    return (int)(op - (uint8_t*)dst);
}

int lz4_compress_fast(lz4_context_t* ctx, const void* src, size_t src_size,
                      void* dst, size_t dst_capacity, int acceleration) {
    if (ctx) ctx->acceleration = acceleration;
    return lz4_compress(ctx, src, src_size, dst, dst_capacity);
}

void lz4_context_reset(lz4_context_t* ctx) {
    if (!ctx) return;
    
    memset(ctx->encode_stream->hash_table, 0, sizeof(ctx->encode_stream->hash_table));
    memset(&ctx->stats, 0, sizeof(ctx->stats));
}

void lz4_get_stats(lz4_context_t* ctx, lz4_stats_t* stats) {
    if (ctx && stats) {
        *stats = ctx->stats;
    }
}

double lz4_get_ratio(lz4_context_t* ctx) {
    if (!ctx || ctx->stats.compressed_bytes == 0) return 0.0;
    return (double)ctx->stats.uncompressed_bytes / (double)ctx->stats.compressed_bytes;
}
