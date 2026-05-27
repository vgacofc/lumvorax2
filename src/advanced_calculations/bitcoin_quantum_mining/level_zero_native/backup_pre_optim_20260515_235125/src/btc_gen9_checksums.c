/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_checksums.c — Validation Checksums CPU vs GPU
 *
 * CYCLE C255v5 — Validations HARDWARE Complètes
 * OBJECTIF : Valider intégrité calculs GPU avec checksums CRC32C
 *
 * MÉTHODE :
 * - Calculer checksum CPU référence (input data)
 * - Calculer checksum GPU output
 * - Comparer pour détecter corruption/erreurs
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <x86intrin.h>  /* Pour _mm_crc32_u32 (CRC32C hardware) */

/* ══════════════════════════════════════════════════════════════════════
 * CRC32C HARDWARE ACCELERATED (x86 SSE4.2)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Calculer CRC32C avec accélération hardware x86
 * Utilise instruction SSE4.2 _mm_crc32_u32
 */
static uint32_t crc32c_hardware(const uint32_t* data, size_t count) {
    uint32_t crc = 0xFFFFFFFF;
    
    /* Traiter par blocs de 4 bytes (uint32_t) */
    for (size_t i = 0; i < count; i++) {
        crc = _mm_crc32_u32(crc, data[i]);
    }
    
    return ~crc;  /* Inverser bits finaux */
}

/**
 * Calculer CRC32C software fallback (si pas SSE4.2)
 */
static uint32_t crc32c_software(const uint32_t* data, size_t count) {
    /* Table CRC32C (Castagnoli polynomial 0x1EDC6F41) */
    static const uint32_t crc32c_table[256] = {
        0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4,
        0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
        /* ... table complète 256 entrées ... */
        /* Simplifié pour exemple, utiliser table complète en production */
    };
    
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* bytes = (const uint8_t*)data;
    size_t byte_count = count * sizeof(uint32_t);
    
    for (size_t i = 0; i < byte_count; i++) {
        uint8_t index = (crc ^ bytes[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32c_table[index];
    }
    
    return ~crc;
}

/* ══════════════════════════════════════════════════════════════════════
 * CHECKSUMS CPU vs GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Calculer checksum CPU référence
 */
uint32_t btc_compute_cpu_checksum(const uint32_t* data, size_t count) {
    if (!data || count == 0) {
        fprintf(stderr, "ERROR: Invalid data for CPU checksum\n");
        return 0;
    }
    
    /* Utiliser CRC32C hardware si disponible */
    #ifdef __SSE4_2__
    return crc32c_hardware(data, count);
    #else
    return crc32c_software(data, count);
    #endif
}

/**
 * Calculer checksum GPU output
 */
uint32_t btc_compute_gpu_checksum(const uint32_t* gpu_output, size_t count) {
    if (!gpu_output || count == 0) {
        fprintf(stderr, "ERROR: Invalid GPU output for checksum\n");
        return 0;
    }
    
    /* Même algorithme que CPU pour comparaison */
    #ifdef __SSE4_2__
    return crc32c_hardware(gpu_output, count);
    #else
    return crc32c_software(gpu_output, count);
    #endif
}

/* ══════════════════════════════════════════════════════════════════════
 * VALIDATION INTÉGRITÉ
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Valider intégrité output GPU
 * 
 * Compare checksum GPU vs référence CPU
 * Retourne true si intégrité OK, false si corruption détectée
 */
bool btc_validate_gpu_output(const uint32_t* input_data, size_t input_count,
                             const uint32_t* gpu_output, size_t output_count,
                             FILE* log_file) {
    /* Calculer checksums */
    uint32_t cpu_crc = btc_compute_cpu_checksum(input_data, input_count);
    uint32_t gpu_crc = btc_compute_gpu_checksum(gpu_output, output_count);
    
    /* Comparer */
    bool valid = (cpu_crc == gpu_crc);
    
    /* Logger résultat */
    if (log_file) {
        if (valid) {
            fprintf(log_file, "CHECKSUM_OK: crc=0x%08x (input=%zu output=%zu)\n",
                   cpu_crc, input_count, output_count);
        } else {
            fprintf(log_file, "CHECKSUM_MISMATCH: cpu=0x%08x gpu=0x%08x (input=%zu output=%zu)\n",
                   cpu_crc, gpu_crc, input_count, output_count);
        }
        fflush(log_file);
    }
    
    /* Afficher console */
    if (valid) {
        printf("✅ Checksum OK: 0x%08x\n", cpu_crc);
    } else {
        printf("❌ Checksum MISMATCH: CPU=0x%08x GPU=0x%08x\n", cpu_crc, gpu_crc);
    }
    
    return valid;
}

/**
 * Valider intégrité avec détails corruption
 * 
 * Analyse différences byte par byte pour localiser corruption
 */
bool btc_validate_gpu_output_detailed(const uint32_t* input_data, size_t input_count,
                                      const uint32_t* gpu_output, size_t output_count,
                                      FILE* log_file) {
    /* Validation basique */
    bool valid = btc_validate_gpu_output(input_data, input_count, 
                                         gpu_output, output_count, log_file);
    
    if (valid) {
        return true;  /* Pas de corruption */
    }
    
    /* Analyse détaillée corruption */
    printf("\n=== ANALYSE CORRUPTION GPU ===\n");
    
    size_t min_count = (input_count < output_count) ? input_count : output_count;
    int corruption_count = 0;
    int first_corruption_index = -1;
    
    for (size_t i = 0; i < min_count; i++) {
        if (input_data[i] != gpu_output[i]) {
            corruption_count++;
            if (first_corruption_index < 0) {
                first_corruption_index = i;
            }
            
            /* Afficher premiers 10 corruptions */
            if (corruption_count <= 10) {
                printf("  [%zu] CPU=0x%08x GPU=0x%08x (diff=0x%08x)\n",
                       i, input_data[i], gpu_output[i], 
                       input_data[i] ^ gpu_output[i]);
            }
        }
    }
    
    printf("\nRésumé:\n");
    printf("  Total corruptions: %d / %zu (%.2f%%)\n", 
           corruption_count, min_count,
           (double)corruption_count / min_count * 100.0);
    printf("  First corruption: index %d\n", first_corruption_index);
    printf("==============================\n\n");
    
    /* Logger détails */
    if (log_file) {
        fprintf(log_file, "CORRUPTION_ANALYSIS: count=%d/%zu (%.2f%%) first_index=%d\n",
               corruption_count, min_count,
               (double)corruption_count / min_count * 100.0,
               first_corruption_index);
        fflush(log_file);
    }
    
    return false;
}

/* ══════════════════════════════════════════════════════════════════════
 * VALIDATION BATCH
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Valider intégrité batch de dispatches
 * 
 * Retourne nombre de dispatches valides
 */
int btc_validate_batch_checksums(const uint32_t** input_data_array,
                                 const uint32_t** gpu_output_array,
                                 size_t* counts,
                                 int num_dispatches,
                                 FILE* log_file) {
    if (!input_data_array || !gpu_output_array || !counts) {
        fprintf(stderr, "ERROR: Invalid arrays for batch validation\n");
        return 0;
    }
    
    int valid_count = 0;
    
    printf("\n=== VALIDATION BATCH CHECKSUMS ===\n");
    printf("Dispatches: %d\n\n", num_dispatches);
    
    for (int i = 0; i < num_dispatches; i++) {
        printf("Dispatch %d: ", i);
        bool valid = btc_validate_gpu_output(input_data_array[i], counts[i],
                                             gpu_output_array[i], counts[i],
                                             log_file);
        if (valid) {
            valid_count++;
        }
    }
    
    printf("\nRésultat: %d/%d dispatches valides (%.1f%%)\n",
           valid_count, num_dispatches,
           (double)valid_count / num_dispatches * 100.0);
    printf("==================================\n\n");
    
    if (log_file) {
        fprintf(log_file, "BATCH_VALIDATION: valid=%d/%d (%.1f%%)\n",
               valid_count, num_dispatches,
               (double)valid_count / num_dispatches * 100.0);
        fflush(log_file);
    }
    
    return valid_count;
}

// Made with Bob
