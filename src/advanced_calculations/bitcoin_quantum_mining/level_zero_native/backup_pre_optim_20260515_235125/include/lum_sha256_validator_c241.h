/* LumVorax C241 — SHA256 Cryptographic Validator
 * 0% OpenCL, 100% Native OpenSSL Comparison
 * Device: Intel UHD Graphics 620 (Gen9)
 */

#ifndef LUM_SHA256_VALIDATOR_C241_H
#define LUM_SHA256_VALIDATOR_C241_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bitcoin block header structure (80 bytes) */
typedef struct {
    uint32_t version;           /* Block version */
    uint8_t prev_block[32];     /* Previous block hash */
    uint8_t merkle_root[32];    /* Merkle root */
    uint32_t timestamp;         /* Timestamp */
    uint32_t bits;              /* Difficulty target */
    uint32_t nonce;             /* Nonce */
} btc_block_header_t;

/* SHA256 test vector */
typedef struct {
    uint8_t header[80];         /* Block header */
    uint32_t nonce;             /* Nonce to test */
    uint8_t hash_expected[32];  /* Expected hash (if known) */
    uint8_t hash_lumvorax[32];  /* Hash computed by LumVorax */
    uint8_t hash_openssl[32];   /* Hash computed by OpenSSL */
    bool match_openssl;         /* LumVorax matches OpenSSL */
    bool match_expected;        /* LumVorax matches expected */
    uint64_t timestamp_ns;      /* Timestamp of validation */
} lum_sha256_test_vector_t;

/* Validation statistics */
typedef struct {
    uint64_t total_tests;       /* Total tests performed */
    uint64_t passed_tests;      /* Tests passed */
    uint64_t failed_tests;      /* Tests failed */
    uint64_t divergence_bits;   /* Total bits diverged */
    double success_rate;        /* Success rate (0.0-1.0) */
    uint64_t total_time_ns;     /* Total validation time */
} lum_sha256_validation_stats_t;

/* Validator context */
typedef struct lum_sha256_validator_s lum_sha256_validator_t;

/* Configuration */
typedef struct {
    const char* log_path;       /* Log file path (NULL = no logging) */
    bool enable_auto_logging;   /* Enable automatic logging */
    bool verbose;               /* Verbose output */
} lum_sha256_validator_config_t;

/* Default configuration */
#define LUM_SHA256_VALIDATOR_DEFAULT_CONFIG { \
    .log_path = "logs/forensic/sha256_validation_c241.csv", \
    .enable_auto_logging = true, \
    .verbose = false \
}

/* API Functions */

/**
 * Initialize SHA256 validator
 * @param validator_out Output validator context
 * @param config Configuration (NULL = default)
 * @return 0 on success, -1 on error
 */
int lum_sha256_validator_init(lum_sha256_validator_t** validator_out, 
                              const lum_sha256_validator_config_t* config);

/**
 * Compute double SHA256 using OpenSSL (reference)
 * @param data Input data
 * @param len Input length
 * @param hash_out Output hash (32 bytes)
 * @return 0 on success, -1 on error
 */
int lum_sha256_compute_openssl(const uint8_t* data, size_t len, uint8_t* hash_out);

/**
 * Compute double SHA256 Bitcoin (SHA256(SHA256(data)))
 * @param data Input data
 * @param len Input length
 * @param hash_out Output hash (32 bytes)
 * @return 0 on success, -1 on error
 */
int lum_sha256_double(const uint8_t* data, size_t len, uint8_t* hash_out);

/**
 * Validate single nonce
 * @param validator Validator context
 * @param header Block header (80 bytes)
 * @param nonce Nonce to test
 * @param hash_lumvorax Hash computed by LumVorax (32 bytes)
 * @param vector_out Output test vector (optional)
 * @return 0 if match, -1 if divergence
 */
int lum_sha256_validate_single(lum_sha256_validator_t* validator,
                               const uint8_t* header,
                               uint32_t nonce,
                               const uint8_t* hash_lumvorax,
                               lum_sha256_test_vector_t* vector_out);

/**
 * Validate batch of nonces
 * @param validator Validator context
 * @param header Block header (80 bytes)
 * @param nonces Array of nonces
 * @param hashes_lumvorax Array of hashes computed by LumVorax
 * @param count Number of nonces
 * @param vectors_out Output test vectors (optional)
 * @return Number of matches, -1 on error
 */
int lum_sha256_validate_batch(lum_sha256_validator_t* validator,
                              const uint8_t* header,
                              const uint32_t* nonces,
                              const uint8_t* hashes_lumvorax,
                              size_t count,
                              lum_sha256_test_vector_t* vectors_out);

/**
 * Validate random nonces
 * @param validator Validator context
 * @param count Number of random nonces to test
 * @return Number of matches, -1 on error
 */
int lum_sha256_validate_random(lum_sha256_validator_t* validator, size_t count);

/**
 * Compare two hashes bit-by-bit
 * @param hash1 First hash (32 bytes)
 * @param hash2 Second hash (32 bytes)
 * @param divergence_bits Output number of divergent bits
 * @return true if exact match, false otherwise
 */
bool lum_sha256_compare_hashes(const uint8_t* hash1, 
                               const uint8_t* hash2,
                               uint32_t* divergence_bits);

/**
 * Get validation statistics
 * @param validator Validator context
 * @param stats Output statistics
 * @return 0 on success, -1 on error
 */
int lum_sha256_get_stats(lum_sha256_validator_t* validator, 
                        lum_sha256_validation_stats_t* stats);

/**
 * Reset validation statistics
 * @param validator Validator context
 * @return 0 on success, -1 on error
 */
int lum_sha256_reset_stats(lum_sha256_validator_t* validator);

/**
 * Log test vector to file
 * @param validator Validator context
 * @param vector Test vector to log
 * @return 0 on success, -1 on error
 */
int lum_sha256_log_vector(lum_sha256_validator_t* validator, 
                         const lum_sha256_test_vector_t* vector);

/**
 * Flush log file
 * @param validator Validator context
 * @return 0 on success, -1 on error
 */
int lum_sha256_flush_log(lum_sha256_validator_t* validator);

/**
 * Cleanup validator
 * @param validator Validator context
 */
void lum_sha256_validator_cleanup(lum_sha256_validator_t* validator);

/**
 * Print test vector (human-readable)
 * @param vector Test vector to print
 */
void lum_sha256_print_vector(const lum_sha256_test_vector_t* vector);

/**
 * Print validation statistics (human-readable)
 * @param stats Statistics to print
 */
void lum_sha256_print_stats(const lum_sha256_validation_stats_t* stats);

/**
 * Create Bitcoin block header for testing
 * @param header Output header (80 bytes)
 * @param version Block version
 * @param nonce Nonce
 */
void lum_sha256_create_test_header(uint8_t* header, uint32_t version, uint32_t nonce);

/**
 * Hex dump hash
 * @param hash Hash to dump (32 bytes)
 * @param buffer Output buffer (65 bytes minimum)
 */
void lum_sha256_hash_to_hex(const uint8_t* hash, char* buffer);

#ifdef __cplusplus
}
#endif

#endif /* LUM_SHA256_VALIDATOR_C241_H */

// Made with Bob
