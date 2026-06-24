/**
 * @file lum_poh_simd.c
 * @brief Implémentation optimisations SIMD pour PoH
 * 
 * PHASE 3 - Cycle C199.23
 * Objectif: Récupérer 15% TPS perdu via vectorisation AVX2/SSE4.2
 */

#include "lum_poh_simd.h"
#include "../debug/ultra_forensic_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Intrinsics SIMD
#ifdef __AVX2__
#include <immintrin.h>
#endif
#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static lum_simd_stats_t g_simd_stats = {0};

/* ============================================================================
 * DÉTECTION CAPACITÉS CPU
 * ============================================================================ */

bool lum_simd_detect_capabilities(lum_simd_capabilities_t* caps) {
    if (!caps) return false;
    
    // Initialiser structure
    memset(caps, 0, sizeof(lum_simd_capabilities_t));
    
    // Ouvrir /proc/cpuinfo
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        FORENSIC_LOG_ANOMALY("SIMD", "cpuinfo_open_failed", 0);
        return false;
    }
    
    char line[512];
    bool flags_found = false;
    
    while (fgets(line, sizeof(line), fp)) {
        // Détecter modèle CPU
        if (strncmp(line, "model name", 10) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                colon += 2; // Sauter ": "
                size_t len = strlen(colon);
                if (len > 0 && colon[len-1] == '\n') {
                    colon[len-1] = '\0'; // Retirer \n
                }
                strncpy(caps->cpu_model, colon, sizeof(caps->cpu_model) - 1);
            }
        }
        
        // Détecter nombre cœurs physiques
        if (strncmp(line, "cpu cores", 9) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                caps->cpu_cores = atoi(colon + 1);
            }
        }
        
        // Détecter nombre threads logiques
        if (strncmp(line, "siblings", 8) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                caps->cpu_threads = atoi(colon + 1);
            }
        }
        
        // Détecter flags SIMD
        if (strncmp(line, "flags", 5) == 0) {
            flags_found = true;
            
            // Chercher AVX2
            if (strstr(line, "avx2")) {
                caps->avx2_available = true;
            }
            
            // Chercher SSE4.2
            if (strstr(line, "sse4_2")) {
                caps->sse42_available = true;
            }
            
            // Chercher AES-NI
            if (strstr(line, "aes")) {
                caps->aes_ni_available = true;
            }
            
            break; // Flags trouvés, sortir
        }
    }
    
    fclose(fp);
    
    if (!flags_found) {
        FORENSIC_LOG_ANOMALY("SIMD", "cpu_flags_not_found", 0);
        return false;
    }
    
    // Logger détection dans forensic
    FORENSIC_LOG_NANO("SIMD", "avx2_detected", caps->avx2_available ? 1.0 : 0.0);
    FORENSIC_LOG_NANO("SIMD", "sse42_detected", caps->sse42_available ? 1.0 : 0.0);
    FORENSIC_LOG_NANO("SIMD", "aes_ni_detected", caps->aes_ni_available ? 1.0 : 0.0);
    FORENSIC_LOG_NANO("SIMD", "cpu_cores", (double)caps->cpu_cores);
    FORENSIC_LOG_NANO("SIMD", "cpu_threads", (double)caps->cpu_threads);
    
    return true;
}

void lum_simd_print_capabilities(const lum_simd_capabilities_t* caps) {
    if (!caps) return;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           CAPACITÉS SIMD DÉTECTÉES (Runtime)                  ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ CPU Model    : %-47s║\n", caps->cpu_model);
    printf("║ Cores/Threads: %d / %d                                          ║\n", 
           caps->cpu_cores, caps->cpu_threads);
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ AVX2         : %s                                            ║\n",
           caps->avx2_available ? "✅ DISPONIBLE (256-bit)" : "❌ NON DISPONIBLE");
    printf("║ SSE4.2       : %s                                            ║\n",
           caps->sse42_available ? "✅ DISPONIBLE (128-bit)" : "❌ NON DISPONIBLE");
    printf("║ AES-NI       : %s                                            ║\n",
           caps->aes_ni_available ? "✅ DISPONIBLE (Crypto HW)" : "❌ NON DISPONIBLE");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

/* ============================================================================
 * OPTIMISATIONS MEMCPY VECTORISÉES
 * ============================================================================ */

void lum_simd_copy_hashes(uint8_t* dst, const uint8_t* src, int count,
                          const lum_simd_capabilities_t* caps) {
    if (!dst || !src || count <= 0 || !caps) return;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    const int HASH_SIZE = 32; // SHA-256 = 32 bytes
    
#ifdef __AVX2__
    // AVX2: Copier 4× hashes (128 bytes) par itération
    if (caps->avx2_available && count >= 4) {
        int i = 0;
        for (; i <= count - 4; i += 4) {
            // Charger 4× hashes (128 bytes) dans registres AVX2
            __m256i hash0 = _mm256_loadu_si256((__m256i*)(src + i * HASH_SIZE));
            __m256i hash1 = _mm256_loadu_si256((__m256i*)(src + (i+1) * HASH_SIZE));
            __m256i hash2 = _mm256_loadu_si256((__m256i*)(src + (i+2) * HASH_SIZE));
            __m256i hash3 = _mm256_loadu_si256((__m256i*)(src + (i+3) * HASH_SIZE));
            
            // Stocker dans destination
            _mm256_storeu_si256((__m256i*)(dst + i * HASH_SIZE), hash0);
            _mm256_storeu_si256((__m256i*)(dst + (i+1) * HASH_SIZE), hash1);
            _mm256_storeu_si256((__m256i*)(dst + (i+2) * HASH_SIZE), hash2);
            _mm256_storeu_si256((__m256i*)(dst + (i+3) * HASH_SIZE), hash3);
        }
        
        // Copier hashes restants (< 4)
        for (; i < count; i++) {
            memcpy(dst + i * HASH_SIZE, src + i * HASH_SIZE, HASH_SIZE);
        }
        
        g_simd_stats.avx2_copies++;
        g_simd_stats.total_bytes += count * HASH_SIZE;
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        g_simd_stats.avg_copy_time_ns = (g_simd_stats.avg_copy_time_ns * (g_simd_stats.avx2_copies - 1) + elapsed_ns) / g_simd_stats.avx2_copies;
        
        return;
    }
#endif

#ifdef __SSE4_2__
    // SSE4.2: Copier 2× hashes (64 bytes) par itération
    if (caps->sse42_available && count >= 2) {
        int i = 0;
        for (; i <= count - 2; i += 2) {
            // Charger 2× hashes (64 bytes) dans registres SSE
            __m128i hash0_lo = _mm_loadu_si128((__m128i*)(src + i * HASH_SIZE));
            __m128i hash0_hi = _mm_loadu_si128((__m128i*)(src + i * HASH_SIZE + 16));
            __m128i hash1_lo = _mm_loadu_si128((__m128i*)(src + (i+1) * HASH_SIZE));
            __m128i hash1_hi = _mm_loadu_si128((__m128i*)(src + (i+1) * HASH_SIZE + 16));
            
            // Stocker dans destination
            _mm_storeu_si128((__m128i*)(dst + i * HASH_SIZE), hash0_lo);
            _mm_storeu_si128((__m128i*)(dst + i * HASH_SIZE + 16), hash0_hi);
            _mm_storeu_si128((__m128i*)(dst + (i+1) * HASH_SIZE), hash1_lo);
            _mm_storeu_si128((__m128i*)(dst + (i+1) * HASH_SIZE + 16), hash1_hi);
        }
        
        // Copier hashes restants (< 2)
        for (; i < count; i++) {
            memcpy(dst + i * HASH_SIZE, src + i * HASH_SIZE, HASH_SIZE);
        }
        
        g_simd_stats.sse42_copies++;
        g_simd_stats.total_bytes += count * HASH_SIZE;
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        g_simd_stats.avg_copy_time_ns = (g_simd_stats.avg_copy_time_ns * (g_simd_stats.sse42_copies - 1) + elapsed_ns) / g_simd_stats.sse42_copies;
        
        return;
    }
#endif

    // Fallback: memcpy() standard
    for (int i = 0; i < count; i++) {
        memcpy(dst + i * HASH_SIZE, src + i * HASH_SIZE, HASH_SIZE);
    }
    
    g_simd_stats.fallback_copies++;
    g_simd_stats.total_bytes += count * HASH_SIZE;
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    g_simd_stats.avg_copy_time_ns = (g_simd_stats.avg_copy_time_ns * (g_simd_stats.fallback_copies - 1) + elapsed_ns) / g_simd_stats.fallback_copies;
}

void lum_simd_copy_sequences(uint64_t* dst, const uint64_t* src, int count,
                             const lum_simd_capabilities_t* caps) {
    if (!dst || !src || count <= 0 || !caps) return;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
#ifdef __AVX2__
    // AVX2: Copier 4× sequences (32 bytes) par itération
    if (caps->avx2_available && count >= 4) {
        int i = 0;
        for (; i <= count - 4; i += 4) {
            // Charger 4× uint64_t (32 bytes) dans registre AVX2
            __m256i seqs = _mm256_loadu_si256((__m256i*)(src + i));
            
            // Stocker dans destination
            _mm256_storeu_si256((__m256i*)(dst + i), seqs);
        }
        
        // Copier sequences restantes (< 4)
        for (; i < count; i++) {
            dst[i] = src[i];
        }
        
        g_simd_stats.avx2_copies++;
        g_simd_stats.total_bytes += count * sizeof(uint64_t);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        g_simd_stats.avg_copy_time_ns = (g_simd_stats.avg_copy_time_ns * (g_simd_stats.avx2_copies - 1) + elapsed_ns) / g_simd_stats.avx2_copies;
        
        return;
    }
#endif

#ifdef __SSE4_2__
    // SSE4.2: Copier 2× sequences (16 bytes) par itération
    if (caps->sse42_available && count >= 2) {
        int i = 0;
        for (; i <= count - 2; i += 2) {
            // Charger 2× uint64_t (16 bytes) dans registre SSE
            __m128i seqs = _mm_loadu_si128((__m128i*)(src + i));
            
            // Stocker dans destination
            _mm_storeu_si128((__m128i*)(dst + i), seqs);
        }
        
        // Copier sequences restantes (< 2)
        for (; i < count; i++) {
            dst[i] = src[i];
        }
        
        g_simd_stats.sse42_copies++;
        g_simd_stats.total_bytes += count * sizeof(uint64_t);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        g_simd_stats.avg_copy_time_ns = (g_simd_stats.avg_copy_time_ns * (g_simd_stats.sse42_copies - 1) + elapsed_ns) / g_simd_stats.sse42_copies;
        
        return;
    }
#endif

    // Fallback: copie standard
    for (int i = 0; i < count; i++) {
        dst[i] = src[i];
    }
    
    g_simd_stats.fallback_copies++;
    g_simd_stats.total_bytes += count * sizeof(uint64_t);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    g_simd_stats.avg_copy_time_ns = (g_simd_stats.avg_copy_time_ns * (g_simd_stats.fallback_copies - 1) + elapsed_ns) / g_simd_stats.fallback_copies;
}

/* ============================================================================
 * MÉTRIQUES FORENSIQUES
 * ============================================================================ */

lum_simd_stats_t lum_simd_get_stats(void) {
    return g_simd_stats;
}

void lum_simd_reset_stats(void) {
    memset(&g_simd_stats, 0, sizeof(lum_simd_stats_t));
}

// Made with Bob
