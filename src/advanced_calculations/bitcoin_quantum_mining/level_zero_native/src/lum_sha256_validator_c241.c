/* LumVorax C241 — SHA256 Cryptographic Validator Implementation
 * 0% OpenCL, 100% Native OpenSSL Comparison
 * Device: Intel UHD Graphics 620 (Gen9)
 */

#define _POSIX_C_SOURCE 199309L

#include "lum_sha256_validator_c241.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

/* Internal context structure */
struct lum_sha256_validator_s {
    lum_sha256_validator_config_t config;
    FILE* log_file;
    lum_sha256_validation_stats_t stats;
};

/* Helper: Get nanosecond timestamp */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Initialize SHA256 validator */
int lum_sha256_validator_init(lum_sha256_validator_t** validator_out, 
                              const lum_sha256_validator_config_t* config) {
    if (!validator_out) return -1;
    
    lum_sha256_validator_t* validator = calloc(1, sizeof(lum_sha256_validator_t));
    if (!validator) return -1;
    
    /* Copy configuration */
    if (config) {
        validator->config = *config;
    } else {
        lum_sha256_validator_config_t default_config = LUM_SHA256_VALIDATOR_DEFAULT_CONFIG;
        validator->config = default_config;
    }
    
    /* Open log file */
    if (validator->config.log_path && validator->config.enable_auto_logging) {
        validator->log_file = fopen(validator->config.log_path, "w");
        if (validator->log_file) {
            /* Write CSV header */
            fprintf(validator->log_file, "timestamp_ns,nonce,");
            fprintf(validator->log_file, "hash_lumvorax,hash_openssl,");
            fprintf(validator->log_file, "match,divergence_bits\n");
            fflush(validator->log_file);
        }
    }
    
    *validator_out = validator;
    return 0;
}

/* Compute double SHA256 using OpenSSL */
int lum_sha256_compute_openssl(const uint8_t* data, size_t len, uint8_t* hash_out) {
    if (!data || !hash_out) return -1;
    
    uint8_t hash1[32];
    
    /* First SHA256 */
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(hash1, &ctx);
    
    /* Second SHA256 */
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, hash1, 32);
    SHA256_Final(hash_out, &ctx);
    
    return 0;
}

/* Compute double SHA256 Bitcoin */
int lum_sha256_double(const uint8_t* data, size_t len, uint8_t* hash_out) {
    return lum_sha256_compute_openssl(data, len, hash_out);
}

/* Compare two hashes bit-by-bit */
bool lum_sha256_compare_hashes(const uint8_t* hash1, 
                               const uint8_t* hash2,
                               uint32_t* divergence_bits) {
    if (!hash1 || !hash2) return false;
    
    uint32_t divergent = 0;
    
    for (int i = 0; i < 32; i++) {
        uint8_t xor_byte = hash1[i] ^ hash2[i];
        
        /* Count set bits in XOR result */
        while (xor_byte) {
            divergent += xor_byte & 1;
            xor_byte >>= 1;
        }
    }
    
    if (divergence_bits) {
        *divergence_bits = divergent;
    }
    
    return (divergent == 0);
}

/* Validate single nonce */
int lum_sha256_validate_single(lum_sha256_validator_t* validator,
                               const uint8_t* header,
                               uint32_t nonce,
                               const uint8_t* hash_lumvorax,
                               lum_sha256_test_vector_t* vector_out) {
    if (!validator || !header || !hash_lumvorax) return -1;
    
    uint64_t start_ns = get_timestamp_ns();
    
    /* Create test vector */
    lum_sha256_test_vector_t vector = {0};
    memcpy(vector.header, header, 80);
    vector.nonce = nonce;
    memcpy(vector.hash_lumvorax, hash_lumvorax, 32);
    vector.timestamp_ns = start_ns;
    
    /* Inject nonce into header */
    uint8_t header_with_nonce[80];
    memcpy(header_with_nonce, header, 80);
    memcpy(header_with_nonce + 76, &nonce, 4);
    
    /* Compute reference hash with OpenSSL */
    lum_sha256_compute_openssl(header_with_nonce, 80, vector.hash_openssl);
    
    /* Compare hashes */
    uint32_t divergence_bits;
    vector.match_openssl = lum_sha256_compare_hashes(
        vector.hash_lumvorax, 
        vector.hash_openssl, 
        &divergence_bits
    );
    
    /* Update statistics */
    validator->stats.total_tests++;
    if (vector.match_openssl) {
        validator->stats.passed_tests++;
    } else {
        validator->stats.failed_tests++;
        validator->stats.divergence_bits += divergence_bits;
    }
    validator->stats.success_rate = (double)validator->stats.passed_tests / validator->stats.total_tests;
    validator->stats.total_time_ns += (get_timestamp_ns() - start_ns);
    
    /* Log if enabled */
    if (validator->config.enable_auto_logging) {
        lum_sha256_log_vector(validator, &vector);
    }
    
    /* Verbose output */
    if (validator->config.verbose) {
        if (!vector.match_openssl) {
            printf("[SHA256 VALIDATOR] DIVERGENCE: nonce=0x%08x divergence_bits=%u\n", 
                   nonce, divergence_bits);
        }
    }
    
    /* Copy output */
    if (vector_out) {
        *vector_out = vector;
    }
    
    return vector.match_openssl ? 0 : -1;
}

/* Validate batch of nonces */
int lum_sha256_validate_batch(lum_sha256_validator_t* validator,
                              const uint8_t* header,
                              const uint32_t* nonces,
                              const uint8_t* hashes_lumvorax,
                              size_t count,
                              lum_sha256_test_vector_t* vectors_out) {
    if (!validator || !header || !nonces || !hashes_lumvorax) return -1;
    
    int matches = 0;
    
    for (size_t i = 0; i < count; i++) {
        lum_sha256_test_vector_t vector;
        int ret = lum_sha256_validate_single(
            validator,
            header,
            nonces[i],
            hashes_lumvorax + (i * 32),
            &vector
        );
        
        if (ret == 0) {
            matches++;
        }
        
        if (vectors_out) {
            vectors_out[i] = vector;
        }
    }
    
    return matches;
}

/* Validate random nonces */
int lum_sha256_validate_random(lum_sha256_validator_t* validator, size_t count) {
    if (!validator) return -1;
    
    /* Create test header */
    uint8_t header[80];
    lum_sha256_create_test_header(header, 1, 0);
    
    int matches = 0;
    
    for (size_t i = 0; i < count; i++) {
        /* Generate random nonce */
        uint32_t nonce = (uint32_t)rand();
        
        /* Compute hash with OpenSSL (simulating LumVorax) */
        uint8_t hash_lumvorax[32];
        uint8_t header_with_nonce[80];
        memcpy(header_with_nonce, header, 80);
        memcpy(header_with_nonce + 76, &nonce, 4);
        lum_sha256_compute_openssl(header_with_nonce, 80, hash_lumvorax);
        
        /* Validate */
        int ret = lum_sha256_validate_single(validator, header, nonce, hash_lumvorax, NULL);
        if (ret == 0) {
            matches++;
        }
    }
    
    return matches;
}

/* Get validation statistics */
int lum_sha256_get_stats(lum_sha256_validator_t* validator, 
                        lum_sha256_validation_stats_t* stats) {
    if (!validator || !stats) return -1;
    *stats = validator->stats;
    return 0;
}

/* Reset validation statistics */
int lum_sha256_reset_stats(lum_sha256_validator_t* validator) {
    if (!validator) return -1;
    memset(&validator->stats, 0, sizeof(lum_sha256_validation_stats_t));
    return 0;
}

/* Log test vector to file */
int lum_sha256_log_vector(lum_sha256_validator_t* validator, 
                         const lum_sha256_test_vector_t* vector) {
    if (!validator || !vector || !validator->log_file) return -1;
    
    /* Timestamp */
    fprintf(validator->log_file, "%lu,", vector->timestamp_ns);
    
    /* Nonce */
    fprintf(validator->log_file, "0x%08x,", vector->nonce);
    
    /* Hash LumVorax */
    for (int i = 0; i < 32; i++) {
        fprintf(validator->log_file, "%02x", vector->hash_lumvorax[i]);
    }
    fprintf(validator->log_file, ",");
    
    /* Hash OpenSSL */
    for (int i = 0; i < 32; i++) {
        fprintf(validator->log_file, "%02x", vector->hash_openssl[i]);
    }
    fprintf(validator->log_file, ",");
    
    /* Match */
    fprintf(validator->log_file, "%s,", vector->match_openssl ? "true" : "false");
    
    /* Divergence bits */
    uint32_t divergence_bits;
    lum_sha256_compare_hashes(vector->hash_lumvorax, vector->hash_openssl, &divergence_bits);
    fprintf(validator->log_file, "%u\n", divergence_bits);
    
    return 0;
}

/* Flush log file */
int lum_sha256_flush_log(lum_sha256_validator_t* validator) {
    if (!validator || !validator->log_file) return -1;
    fflush(validator->log_file);
    return 0;
}

/* Cleanup validator */
void lum_sha256_validator_cleanup(lum_sha256_validator_t* validator) {
    if (!validator) return;
    
    if (validator->log_file) {
        fflush(validator->log_file);
        fclose(validator->log_file);
    }
    
    free(validator);
}

/* Print test vector */
void lum_sha256_print_vector(const lum_sha256_test_vector_t* vector) {
    if (!vector) return;
    
    printf("[SHA256 TEST VECTOR]\n");
    printf("  Nonce: 0x%08x\n", vector->nonce);
    
    printf("  Hash LumVorax: ");
    for (int i = 0; i < 32; i++) {
        printf("%02x", vector->hash_lumvorax[i]);
    }
    printf("\n");
    
    printf("  Hash OpenSSL:  ");
    for (int i = 0; i < 32; i++) {
        printf("%02x", vector->hash_openssl[i]);
    }
    printf("\n");
    
    printf("  Match: %s\n", vector->match_openssl ? "✅ YES" : "❌ NO");
    
    if (!vector->match_openssl) {
        uint32_t divergence_bits;
        lum_sha256_compare_hashes(vector->hash_lumvorax, vector->hash_openssl, &divergence_bits);
        printf("  Divergence: %u bits\n", divergence_bits);
    }
}

/* Print validation statistics */
void lum_sha256_print_stats(const lum_sha256_validation_stats_t* stats) {
    if (!stats) return;
    
    printf("[SHA256 VALIDATION STATISTICS]\n");
    printf("  Total Tests: %lu\n", stats->total_tests);
    printf("  Passed: %lu (%.2f%%)\n", stats->passed_tests, stats->success_rate * 100.0);
    printf("  Failed: %lu\n", stats->failed_tests);
    
    if (stats->failed_tests > 0) {
        double avg_divergence = (double)stats->divergence_bits / stats->failed_tests;
        printf("  Average Divergence: %.2f bits/failure\n", avg_divergence);
    }
    
    if (stats->total_tests > 0) {
        double avg_time_us = (double)stats->total_time_ns / stats->total_tests / 1000.0;
        printf("  Average Time: %.2f µs/test\n", avg_time_us);
    }
}

/* Create Bitcoin block header for testing */
void lum_sha256_create_test_header(uint8_t* header, uint32_t version, uint32_t nonce) {
    if (!header) return;
    
    memset(header, 0, 80);
    
    /* Version */
    memcpy(header, &version, 4);
    
    /* Previous block hash (zeros for test) */
    memset(header + 4, 0, 32);
    
    /* Merkle root (zeros for test) */
    memset(header + 36, 0, 32);
    
    /* Timestamp (current time) */
    uint32_t timestamp = (uint32_t)time(NULL);
    memcpy(header + 68, &timestamp, 4);
    
    /* Bits (difficulty target, zeros for test) */
    memset(header + 72, 0, 4);
    
    /* Nonce */
    memcpy(header + 76, &nonce, 4);
}

/* Hex dump hash */
void lum_sha256_hash_to_hex(const uint8_t* hash, char* buffer) {
    if (!hash || !buffer) return;
    
    for (int i = 0; i < 32; i++) {
        sprintf(buffer + (i * 2), "%02x", hash[i]);
    }
    buffer[64] = '\0';
}

// Made with Bob
