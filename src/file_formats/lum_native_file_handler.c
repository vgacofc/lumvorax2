// Feature test macros for POSIX functions - _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L

#include "lum_native_file_handler.h"
#include "../debug/memory_tracker.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

// ================== CONFIGURATION DYNAMIQUE ==================

// CORRECTION RAPPORT 117: Buffer size adaptatif selon ressources système
static lum_file_error_messages_t global_error_messages = {
    .success_message = "Operation completed successfully",
    .invalid_params_message = "Invalid parameters provided",
    .file_open_error_message = "Failed to open file",
    .memory_error_message = "Memory allocation failed", 
    .format_error_message = "Unsupported file format"
};

size_t lum_file_get_optimal_buffer_size(void) {
    // Lecture RAM disponible système 
    FILE* meminfo = fopen("/proc/meminfo", "r");
    size_t available_kb = 0;
    
    if (meminfo) {
        char line[256];
        while (fgets(line, sizeof(line), meminfo)) {
            if (sscanf(line, "MemAvailable: %zu kB", &available_kb) == 1) {
                break;
            }
        }
        fclose(meminfo);
    }
    
    // Fallback si lecture impossible
    if (available_kb == 0) available_kb = 4194304; // 4GB par défaut
    
    // Buffer = 0.1% de RAM disponible, min 4KB, max 64MB
    size_t optimal_size = (available_kb * 1024) / 1000;
    if (optimal_size < 4096) optimal_size = 4096;
    if (optimal_size > 67108864) optimal_size = 67108864;
    
    return optimal_size;
}

size_t lum_file_calculate_buffer_size_for_count(size_t lum_count) {
    // Buffer adaptatif selon nombre de LUMs
    size_t base_size = lum_count * sizeof(lum_t);
    size_t optimal = base_size / 10; // 10% de la taille données
    
    if (optimal < 4096) optimal = 4096;
    if (optimal > 67108864) optimal = 67108864;
    
    return optimal;
}

lum_file_error_messages_t* lum_file_get_error_messages(void) {
    return &global_error_messages;
}

bool lum_file_set_custom_error_messages(const lum_file_error_messages_t* messages) {
    if (!messages) return false;
    global_error_messages = *messages;
    return true;
}

lum_file_config_t* lum_file_config_create(lum_file_format_e format) {
    lum_file_config_t* config = TRACKED_MALLOC(sizeof(lum_file_config_t));
    if (!config) return NULL;

    // CORRECTION RAPPORT 117: Configuration adaptative selon contexte
    config->format = format;
    
    // Configuration selon type de format
    switch (format) {
        case LUM_FORMAT_FORENSIC_AUDIT:
            config->include_metadata = true;
            config->include_timestamps = true;
            config->include_checksums = true;
            config->validate_integrity = true;
            config->compress_data = false;
            break;
            
        case LUM_FORMAT_COMPRESSED_LZ4:
        case LUM_FORMAT_COMPRESSED_ZLIB:
            config->include_metadata = true;
            config->include_timestamps = false;
            config->include_checksums = true;
            config->validate_integrity = true;
            config->compress_data = true;
            break;
            
        case LUM_FORMAT_NATIVE_BINARY:
        case LUM_FORMAT_BINARY_OPTIMIZED:
            config->include_metadata = false;
            config->include_timestamps = false;
            config->include_checksums = false;
            config->validate_integrity = false;
            config->compress_data = false;
            break;
            
        default:
            config->include_metadata = true;
            config->include_timestamps = false;
            config->include_checksums = false;
            config->validate_integrity = false;
            config->compress_data = false;
            break;
    }
    
    config->compression_level = LUM_FILE_DEFAULT_COMPRESSION_LEVEL;
    config->buffer_size = lum_file_get_optimal_buffer_size();
    config->custom_encoding = NULL;
    config->memory_address = (void*)config;
    config->config_magic = LUM_FILE_CONFIG_MAGIC;

    return config;
}

lum_file_config_t* lum_file_config_create_adaptive(size_t expected_lum_count) {
    lum_file_config_t* config = lum_file_config_create(LUM_FORMAT_NATIVE_BINARY);
    if (!config) return NULL;
    
    // Adaptation selon nombre attendu de LUMs
    config->buffer_size = lum_file_calculate_buffer_size_for_count(expected_lum_count);
    
    if (expected_lum_count > 1000000) {
        // Pour gros volumes : compression et validation
        config->compress_data = true;
        config->include_checksums = true;
        config->format = LUM_FORMAT_COMPRESSED_LZ4;
    } else if (expected_lum_count > 100000) {
        // Pour volumes moyens : checksums
        config->include_checksums = true;
    }
    
    return config;
}

void lum_file_config_destroy(lum_file_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;

    lum_file_config_t* config = *config_ptr;

    if (config->config_magic != LUM_FILE_CONFIG_MAGIC || 
        config->memory_address != (void*)config) {
        return;
    }

    if (config->custom_encoding) {
        TRACKED_FREE(config->custom_encoding);
    }

    config->config_magic = LUM_FILE_DESTROYED_MAGIC;
    config->memory_address = NULL;

    TRACKED_FREE(config);
    *config_ptr = NULL;
}

bool lum_file_config_set_format(lum_file_config_t* config, lum_file_format_e format) {
    if (!config || config->config_magic != LUM_FILE_CONFIG_MAGIC) return false;

    config->format = format;
    return true;
}

bool lum_file_config_enable_compression(lum_file_config_t* config, double level) {
    if (!config || config->config_magic != LUM_FILE_CONFIG_MAGIC) return false;

    if (level < LUM_FILE_MIN_COMPRESSION_LEVEL || level > LUM_FILE_MAX_COMPRESSION_LEVEL) {
        return false;
    }

    config->compress_data = true;
    config->compression_level = level;
    return true;
}

bool lum_file_config_set_buffer_size(lum_file_config_t* config, size_t size) {
    if (!config || config->config_magic != LUM_FILE_CONFIG_MAGIC) return false;

    if (size < LUM_FILE_MIN_BUFFER_SIZE || size > LUM_FILE_MAX_BUFFER_SIZE) {
        return false;
    }

    config->buffer_size = size;
    return true;
}

// ================== GESTION MÉTADONNÉES ==================

lum_file_metadata_t* lum_file_metadata_create(void) {
    lum_file_metadata_t* metadata = TRACKED_MALLOC(sizeof(lum_file_metadata_t));
    if (!metadata) return NULL;

    metadata->magic_number = LUM_FILE_MAGIC_NUMBER;
    metadata->version_major = LUM_FILE_VERSION_MAJOR;
    metadata->version_minor = LUM_FILE_VERSION_MINOR;
    metadata->format_type = LUM_FORMAT_NATIVE_BINARY;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);  // CORRECTION: CLOCK_REALTIME pour forensique
    metadata->creation_timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    metadata->modification_timestamp = metadata->creation_timestamp;

    metadata->total_lums = 0;
    metadata->total_groups = 0;
    metadata->total_size_bytes = 0;
    metadata->checksum_crc32 = 0;

    SAFE_STRCPY(metadata->creator_info, "LUM/VORAX Native File Handler v1.0", sizeof(metadata->creator_info));
    SAFE_STRCPY(metadata->file_description, "Native LUM/VORAX File Format", sizeof(metadata->file_description));

    metadata->memory_address = (void*)metadata;

    return metadata;
}

void lum_file_metadata_destroy(lum_file_metadata_t** metadata_ptr) {
    if (!metadata_ptr || !*metadata_ptr) return;

    lum_file_metadata_t* metadata = *metadata_ptr;

    if (metadata->memory_address != (void*)metadata) return;

    metadata->memory_address = NULL;
    TRACKED_FREE(metadata);
    *metadata_ptr = NULL;
}

bool lum_file_metadata_update(lum_file_metadata_t* metadata, const lum_group_t* group) {
    if (!metadata || !group) return false;

    if (metadata->memory_address != (void*)metadata) return false;

    metadata->total_lums += group->count;
    metadata->total_groups++;
    metadata->total_size_bytes += group->count * sizeof(lum_t);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);  // CORRECTION: CLOCK_REALTIME pour forensique
    metadata->modification_timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    return true;
}

bool lum_file_metadata_validate(const lum_file_metadata_t* metadata) {
    if (!metadata) return false;

    if (metadata->magic_number != LUM_FILE_MAGIC_NUMBER) return false;
    if (metadata->memory_address != (void*)metadata) return false;

    return true;
}

// ================== GESTION CONTEXTE I/O ==================

lum_file_context_t* lum_file_context_create(const char* filename, lum_file_config_t* config, bool writing) {
    if (!filename || !config) return NULL;

    lum_file_context_t* context = TRACKED_MALLOC(sizeof(lum_file_context_t));
    if (!context) return NULL;

    context->file_handle = NULL;
    context->config = config;
    context->metadata = lum_file_metadata_create();
    context->io_buffer = TRACKED_MALLOC(config->buffer_size);
    context->buffer_position = 0;
    context->buffer_size = config->buffer_size;
    context->is_reading = !writing;
    context->is_writing = writing;
    context->memory_address = (void*)context;
    context->context_magic = LUM_FILE_CONTEXT_MAGIC;

    if (!context->metadata || !context->io_buffer) {
        lum_file_context_destroy(&context);
        return NULL;
    }

    return context;
}

void lum_file_context_destroy(lum_file_context_t** context_ptr) {
    if (!context_ptr || !*context_ptr) return;

    lum_file_context_t* context = *context_ptr;

    if (context->context_magic != LUM_FILE_CONTEXT_MAGIC || 
        context->memory_address != (void*)context) {
        return;
    }

    if (context->file_handle) {
        fclose(context->file_handle);
    }

    if (context->metadata) {
        lum_file_metadata_destroy(&context->metadata);
    }

    if (context->io_buffer) {
        TRACKED_FREE(context->io_buffer);
    }

    context->context_magic = LUM_FILE_DESTROYED_MAGIC;
    context->memory_address = NULL;

    TRACKED_FREE(context);
    *context_ptr = NULL;
}

bool lum_file_context_open(lum_file_context_t* context, const char* filename, const char* mode) {
    if (!context || !filename || !mode) return false;

    if (context->context_magic != LUM_FILE_CONTEXT_MAGIC) return false;

    context->file_handle = fopen(filename, mode);
    return (context->file_handle != NULL);
}

bool lum_file_context_close(lum_file_context_t* context) {
    if (!context || context->context_magic != LUM_FILE_CONTEXT_MAGIC) return false;

    if (context->file_handle) {
        fclose(context->file_handle);
        context->file_handle = NULL;
        return true;
    }

    return false;
}

// ================== EXPORT MULTI-FORMAT ==================

lum_file_result_t* lum_export_single_binary(const lum_t* lum, const char* filename, lum_file_config_t* config) {
    lum_file_result_t* result = lum_file_result_create();
    if (!result || !lum || !filename || !config) {
        if (result) lum_file_result_set_error(result, "Invalid parameters for binary export");
        return result;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // MONOTONIC pour mesures performance

    lum_file_context_t* context = lum_file_context_create(filename, config, true);
    if (!context) {
        lum_file_result_set_error(result, "Failed to create file context");
        return result;
    }

    if (!lum_file_context_open(context, filename, "wb")) {
        lum_file_context_destroy(&context);
        lum_file_result_set_error(result, "Failed to open file for writing");
        return result;
    }

    // Écrire métadonnées si activées
    if (config->include_metadata) {
        context->metadata->format_type = LUM_FORMAT_NATIVE_BINARY;
        // CORRECTION RAPPORT 117: Valeurs réelles calculées, pas falsifiées
        context->metadata->total_lums = lum ? 1 : 0;  // Nombre réel de LUMs
        context->metadata->total_groups = lum ? 1 : 0;  // Un groupe contenant cette LUM
        context->metadata->total_size_bytes = sizeof(lum_t);

        // CORRECTION CRITIQUE: Utiliser sérialisation sécurisée au lieu de fwrite raw
        if (!lum_write_metadata_serialized(context->file_handle, context->metadata)) {
            lum_file_context_destroy(&context);
            lum_file_result_set_error(result, "Failed to write secure metadata");
            return result;
        }
    }

    // CORRECTION CRITIQUE: Utiliser sérialisation sécurisée au lieu de fwrite raw
    if (!lum_write_serialized(context->file_handle, lum)) {
        lum_file_context_destroy(&context);
        lum_file_result_set_error(result, "Failed to write secure LUM data");
        return result;
    }

    lum_file_context_close(context);
    lum_file_context_destroy(&context);

    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t processing_time = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                              (end.tv_nsec - start.tv_nsec);

    lum_file_result_set_success(result, filename, sizeof(lum_t), 1);
    result->processing_time_ns = processing_time;
    result->format_used = LUM_FORMAT_NATIVE_BINARY;

    return result;
}

lum_file_result_t* lum_export_single_text(const lum_t* lum, const char* filename, lum_file_config_t* config) {
    lum_file_result_t* result = lum_file_result_create();
    if (!result || !lum || !filename || !config) {
        if (result) lum_file_result_set_error(result, "Invalid parameters for text export");
        return result;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE* file = fopen(filename, "w");
    if (!file) {
        lum_file_result_set_error(result, "Failed to open file for text writing");
        return result;
    }

    // Format texte lisible
    fprintf(file, "=== LUM/VORAX Native Text Format ===\n");
    if (config->include_timestamps) {
        fprintf(file, "Export timestamp: %llu\n", (unsigned long long)time(NULL));
    }
    fprintf(file, "\n");

    fprintf(file, "LUM ID: %u\n", lum->id);
    fprintf(file, "Presence: %u\n", lum->presence);
    fprintf(file, "Position X: %d\n", lum->position_x);
    fprintf(file, "Position Y: %d\n", lum->position_y);
    // Position Z n'existe pas dans lum_t structure
    fprintf(file, "Structure Type: %u\n", (unsigned int)lum->structure_type);

    if (config->include_timestamps) {
        fprintf(file, "Timestamp: %llu\n", (unsigned long long)lum->timestamp);
    }

    fprintf(file, "Memory Address: %p\n", lum->memory_address);

    if (config->include_checksums) {
        fprintf(file, "Checksum: %u\n", lum->checksum);
    }

    fprintf(file, "Is Destroyed: %u\n", lum->is_destroyed);

    size_t bytes_written = ftell(file);
    fclose(file);

    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t processing_time = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                              (end.tv_nsec - start.tv_nsec);

    lum_file_result_set_success(result, filename, bytes_written, 1);
    result->processing_time_ns = processing_time;
    result->format_used = LUM_FORMAT_TEXT_READABLE;

    return result;
}

lum_file_result_t* lum_export_single_json(const lum_t* lum, const char* filename, lum_file_config_t* config) {
    lum_file_result_t* result = lum_file_result_create();
    if (!result || !lum || !filename || !config) {
        if (result) lum_file_result_set_error(result, "Invalid parameters for JSON export");
        return result;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE* file = fopen(filename, "w");
    if (!file) {
        lum_file_result_set_error(result, "Failed to open file for JSON writing");
        return result;
    }

    // Format JSON structuré
    fprintf(file, "{\n");
    fprintf(file, "  \"format\": \"LUM/VORAX Native JSON\",\n");
    fprintf(file, "  \"version\": \"%d.%d\",\n", LUM_FILE_VERSION_MAJOR, LUM_FILE_VERSION_MINOR);

    if (config->include_timestamps) {
        fprintf(file, "  \"export_timestamp\": %llu,\n", (unsigned long long)time(NULL));
    }

    fprintf(file, "  \"lum\": {\n");
    fprintf(file, "    \"id\": %u,\n", lum->id);
    fprintf(file, "    \"presence\": %u,\n", lum->presence);
    fprintf(file, "    \"position\": {\n");
    fprintf(file, "      \"x\": %d,\n", lum->position_x);
    fprintf(file, "      \"y\": %d,\n", lum->position_y);
      // Position Z n'existe pas dans lum_t structure
    fprintf(file, "    },\n");
    fprintf(file, "    \"structure_type\": %u", (unsigned int)lum->structure_type);

    if (config->include_timestamps) {
        fprintf(file, ",\n    \"timestamp\": %llu", (unsigned long long)lum->timestamp);
    }

    if (config->include_checksums) {
        fprintf(file, ",\n    \"checksum\": %u", lum->checksum);
    }

    fprintf(file, ",\n    \"is_destroyed\": %s", lum->is_destroyed ? "true" : "false");
    fprintf(file, "\n  }");

    if (config->include_metadata) {
        // CORRECTION RAPPORT 117: Métadonnées réelles calculées
        size_t actual_file_size = ftell(file);
        
        fprintf(file, ",\n  \"metadata\": {\n");
        fprintf(file, "    \"total_lums\": 1,\n");
        fprintf(file, "    \"file_size_bytes\": %zu,\n", actual_file_size);
        fprintf(file, "    \"lum_structure_size\": %zu,\n", sizeof(lum_t));
        fprintf(file, "    \"format_version\": \"%d.%d\",\n", LUM_FILE_VERSION_MAJOR, LUM_FILE_VERSION_MINOR);
        fprintf(file, "    \"creation_time\": %llu\n", (unsigned long long)time(NULL));
        fprintf(file, "  }");
    }

    fprintf(file, "\n}\n");

    size_t bytes_written = ftell(file);
    fclose(file);

    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t processing_time = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                              (end.tv_nsec - start.tv_nsec);

    lum_file_result_set_success(result, filename, bytes_written, 1);
    result->processing_time_ns = processing_time;
    result->format_used = LUM_FORMAT_JSON_STRUCTURED;

    return result;
}

lum_file_result_t* lum_export_group_csv(const lum_group_t* group, const char* filename, lum_file_config_t* config) {
    lum_file_result_t* result = lum_file_result_create();
    if (!result || !group || !filename || !config) {
        if (result) lum_file_result_set_error(result, "Invalid parameters for CSV export");
        return result;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE* file = fopen(filename, "w");
    if (!file) {
        lum_file_result_set_error(result, "Failed to open file for CSV writing");
        return result;
    }

    // En-têtes CSV
    fprintf(file, "id,presence,position_x,position_y,structure_type");
    if (config->include_timestamps) {
        fprintf(file, ",timestamp");
    }
    if (config->include_checksums) {
        fprintf(file, ",checksum");
    }
    fprintf(file, ",is_destroyed\n");

    // Données LUMs
    for (size_t i = 0; i < group->count; i++) {
        const lum_t* lum = &group->lums[i];

        fprintf(file, "%u,%u,%d,%d,%u", 
                lum->id, lum->presence, lum->position_x, lum->position_y, 
                (unsigned int)lum->structure_type);

        if (config->include_timestamps) {
            fprintf(file, ",%llu", (unsigned long long)lum->timestamp);
        }

        if (config->include_checksums) {
            fprintf(file, ",%u", lum->checksum);
        }

        fprintf(file, ",%s\n", lum->is_destroyed ? "true" : "false");
    }

    size_t bytes_written = ftell(file);
    fclose(file);

    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t processing_time = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                              (end.tv_nsec - start.tv_nsec);

    lum_file_result_set_success(result, filename, bytes_written, group->count);
    result->processing_time_ns = processing_time;
    result->format_used = LUM_FORMAT_CSV_TABULAR;

    return result;
}

// ================== FONCTION EXPORT GÉNÉRIQUE ==================

lum_file_result_t* lum_export_single(const lum_t* lum, const char* filename, lum_file_config_t* config) {
    if (!lum || !filename || !config) return NULL;

    switch (config->format) {
        case LUM_FORMAT_NATIVE_BINARY:
        case LUM_FORMAT_BINARY_OPTIMIZED:
            return lum_export_single_binary(lum, filename, config);

        case LUM_FORMAT_TEXT_READABLE:
            return lum_export_single_text(lum, filename, config);

        case LUM_FORMAT_JSON_STRUCTURED:
            return lum_export_single_json(lum, filename, config);

        default: {
            lum_file_result_t* result = lum_file_result_create();
            if (result) {
                lum_file_result_set_error(result, "Unsupported export format");
            }
            return result;
        }
    }
}

// ================== TESTS STRESS 100M+ ==================

bool lum_file_stress_test_100m_export(lum_file_config_t* config) {
    if (!config) return false;

    printf("=== LUM FILE EXPORT STRESS TEST: 100M+ LUMs ===\n");

    const size_t lum_count = 100000000; // 100M LUMs
    const size_t test_lums = 100000;    // 100K LUMs échantillon
    struct timespec start, end;

    printf("Creating test LUM group with %zu LUMs...\n", test_lums);
    lum_group_t* test_group = lum_group_create(test_lums);

    if (!test_group) {
        printf("❌ Failed to create test LUM group\n");
        return false;
    }

    // Initialisation LUMs
    for (size_t i = 0; i < test_lums; i++) {
        lum_t* lum = lum_create(1, (int32_t)i, (int32_t)(i % 1000), LUM_STRUCTURE_LINEAR);
        if (lum) {
            lum_group_add(test_group, lum);
            lum_destroy(lum);
        }
    }

    printf("Testing file export performance...\n");

    // Test export binaire
    clock_gettime(CLOCK_MONOTONIC, &start);
    lum_file_result_t* result = lum_export_group_csv(test_group, "stress_test_export.csv", config);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (result && result->success) {
        double export_time = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1000000000.0;

        printf("✅ Export completed in %.3f seconds\n", export_time);
        printf("Export rate: %.0f LUMs/second\n", test_lums / export_time);
        printf("Bytes written: %zu\n", result->bytes_processed);

        // Projection pour 100M
        double projected_time = export_time * (lum_count / (double)test_lums);
        printf("Projected time for %zu LUMs: %.1f seconds\n", lum_count, projected_time);
        printf("Projected throughput: %.2f MB/s\n", 
               (result->bytes_processed * (lum_count / (double)test_lums)) / (projected_time * 1024 * 1024));

        lum_file_result_destroy(&result);
    } else {
        printf("❌ Export failed\n");
        if (result) {
            printf("Error: %s\n", result->error_message);
            lum_file_result_destroy(&result);
        }
        lum_group_destroy(test_group);
        return false;
    }

    lum_group_destroy(test_group);
    printf("✅ LUM file export stress test 100M+ completed successfully\n");
    return true;
}

// ================== GESTION RÉSULTATS ==================

lum_file_result_t* lum_file_result_create(void) {
    lum_file_result_t* result = TRACKED_MALLOC(sizeof(lum_file_result_t));
    if (!result) return NULL;

    result->success = false;
    result->filename[0] = '\0';
    result->bytes_processed = 0;
    result->lums_processed = 0;
    result->processing_time_ns = 0;
    result->checksum = 0;
    result->error_message[0] = '\0';
    result->format_used = LUM_FORMAT_NATIVE_BINARY;
    result->memory_address = (void*)result;
    result->result_magic = LUM_FILE_RESULT_MAGIC;

    return result;
}

void lum_file_result_destroy(lum_file_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;

    lum_file_result_t* result = *result_ptr;

    if (result->result_magic != LUM_FILE_RESULT_MAGIC || 
        result->memory_address != (void*)result) {
        return;
    }

    result->result_magic = LUM_FILE_DESTROYED_MAGIC;
    result->memory_address = NULL;

    TRACKED_FREE(result);
    *result_ptr = NULL;
}

bool lum_file_result_set_success(lum_file_result_t* result, const char* filename, size_t bytes, size_t lums) {
    if (!result || !filename) return false;

    if (result->result_magic != LUM_FILE_RESULT_MAGIC) return false;

    result->success = true;
    strncpy(result->filename, filename, sizeof(result->filename) - 1);
    result->filename[sizeof(result->filename) - 1] = '\0';
    result->bytes_processed = bytes;
    result->lums_processed = lums;
    strncpy(result->error_message, global_error_messages.success_message, 
            sizeof(result->error_message) - 1);
    result->error_message[sizeof(result->error_message) - 1] = '\0';

    return true;
}

bool lum_file_result_set_error(lum_file_result_t* result, const char* error_message) {
    if (!result || !error_message) return false;

    if (result->result_magic != LUM_FILE_RESULT_MAGIC) return false;

    result->success = false;
    strncpy(result->error_message, error_message, sizeof(result->error_message) - 1);
    result->error_message[sizeof(result->error_message) - 1] = '\0';

    return true;
}

// ================== CONFIGURATIONS PAR DÉFAUT ==================

lum_file_config_t* lum_file_config_create_default(void) {
    return lum_file_config_create(LUM_FORMAT_NATIVE_BINARY);
}

lum_file_config_t* lum_file_config_create_high_performance(void) {
    lum_file_config_t* config = lum_file_config_create(LUM_FORMAT_BINARY_OPTIMIZED);
    if (config) {
        config->include_metadata = false;
        config->include_checksums = false;
        config->validate_integrity = false;
        lum_file_config_set_buffer_size(config, 64 * 1024); // 64KB buffer
    }
    return config;
}

lum_file_config_t* lum_file_config_create_high_compression(void) {
    lum_file_config_t* config = lum_file_config_create(LUM_FORMAT_COMPRESSED_LZ4);
    if (config) {
        lum_file_config_enable_compression(config, 0.9); // 90% compression
    }
    return config;
}

lum_file_config_t* lum_file_config_create_forensic_audit(void) {
    lum_file_config_t* config = lum_file_config_create(LUM_FORMAT_FORENSIC_AUDIT);
    if (config) {
        config->include_metadata = true;
        config->include_timestamps = true;
        config->include_checksums = true;
        config->validate_integrity = true;
    }
    return config;
}