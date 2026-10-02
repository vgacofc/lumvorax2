// Feature test macros for POSIX functions
// _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L

#include "transaction_wal_extension.h"
#include <inttypes.h> /* C2-FIX: PRIu64 pour uint64_t portable macOS/Linux */
#include "../debug/memory_tracker.h"
#include <unistd.h>     // Pour fileno()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "../debug/forensic_logger.h"
#include <fcntl.h>
#include <dirent.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

#define WAL_MAGIC_SIGNATURE 0x57414C58  // "WALX"
#define WAL_VERSION 1
#define WAL_CONTEXT_MAGIC 0x57434F4E    // "WCON"
#define WAL_RESULT_MAGIC 0x57524553     // "WRES"
#define WAL_EXTENSION_MAGIC 0x57455854  // "WEXT"
#define TRANSACTION_WAL_MAGIC 0x54584E57  // "TXNW"

// Table CRC32 standard IEEE 802.3
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b93148, 0x84be01de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x140c9c6f, 0x630ba,
    0xfc02f02c, 0x8b05c0ba, 0x1f6f1345, 0x686823d3, 0xf1613269, 0x866602ff,
    0x1809575c, 0x6f0c67ca, 0xf6053670, 0x810206e6, 0x1e67595f, 0x696069c9,
    0xf0693873, 0x876e08e5, 0x1011f474, 0x6716c4e2, 0xfe1f9558, 0x8918a5ce,
    0x1773f42d, 0x6074c4bb, 0xf97dd501, 0x8e7ae597, 0xb40bbe37, 0xc30c8ea1,
    0x5a05df1b, 0x2d02ef8d
};

uint32_t wal_extension_calculate_crc32(const void* data, size_t length) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

bool wal_extension_verify_crc32(const void* data, size_t length, uint32_t expected_crc) {
    return wal_extension_calculate_crc32(data, length) == expected_crc;
}

// === FONCTIONS RECOVERY MANAGER SUPPORT ===

bool wal_extension_replay_from_existing_persistence(wal_extension_context_t* wal_ctx, 
                                                   void* persistence_ctx) {
    if (!wal_ctx || wal_ctx->magic_number != WAL_EXTENSION_MAGIC) return false;
    if (!persistence_ctx) return false;

    forensic_log(FORENSIC_LEVEL_INFO, "wal_extension_replay_from_existing_persistence",
                "Debut replay WAL depuis persistance existante");

    // Parcourir toutes les transactions du WAL
    // Note: L'implémentation actuelle du WAL ne stocke pas les transactions en mémoire de manière persistante
    // Cette boucle devrait lire depuis un fichier ou une structure de données persistante.
    // Pour l'instant, on simule un succès si le contexte est valide.
    if (wal_ctx->transactions) { // Si une liste de transactions est gérée en mémoire
        for (size_t i = 0; i < wal_ctx->transaction_count; i++) {
            transaction_wal_extended_t* txn = &wal_ctx->transactions[i];

            if (txn->magic_number != TRANSACTION_WAL_MAGIC) {
                forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_replay_from_existing_persistence",
                            "Transaction corrompue detectee");
                return false;
            }

            // Vérifier CRC32 de l'en-tête WAL étendu
            if (!wal_extension_verify_crc32(&txn->base_record, sizeof(transaction_record_t), txn->data_integrity_crc32)) {
                forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_replay_from_existing_persistence",
                            "Echec verification CRC32 pour transaction");
                return false;
            }
             // Vérifier CRC32 de l'enregistrement de base seulement
            if (!wal_extension_verify_crc32(&txn->base_record, sizeof(transaction_record_t), txn->data_integrity_crc32)) {
                forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_replay_from_existing_persistence",
                            "Echec verification CRC32 pour transaction");
                return false;
            }

            char msg[256];
            snprintf(msg, sizeof(msg), "Replay transaction %zu: operation=%u, lum_count=%zu", 
                    i, txn->base_record.operation_type, txn->lum_count);
            forensic_log(FORENSIC_LEVEL_DEBUG, "wal_extension_replay_from_existing_persistence", msg);
        }
    } else {
        // Si pas de transactions stockées en mémoire, on ne peut rien vérifier.
        // Dans un vrai scénario, on lirait le fichier WAL ici.
        forensic_log(FORENSIC_LEVEL_WARNING, "wal_extension_replay_from_existing_persistence",
                    "Aucune transaction WAL trouvee en memoire pour verification.");
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Replay WAL termine (simulation): %zu transactions", wal_ctx->transaction_count);
    forensic_log(FORENSIC_LEVEL_INFO, "wal_extension_replay_from_existing_persistence", msg);
    return true;
}


bool wal_extension_create_checkpoint_with_existing(wal_extension_context_t* wal_ctx,
                                                   void* persistence_ctx) {
    if (!wal_ctx || wal_ctx->magic_number != WAL_EXTENSION_MAGIC) return false;
    if (!persistence_ctx) return false;

    forensic_log(FORENSIC_LEVEL_INFO, "wal_extension_create_checkpoint_with_existing",
                "Creation checkpoint avec persistance existante");

    // Dans une implémentation réelle, cela impliquerait de sauvegarder l'état actuel
    // du WAL (par exemple, la position de lecture/écriture, les transactions non validées, etc.)
    // dans la structure de persistance. Ici, on simule en mettant à jour des compteurs.
    wal_ctx->last_checkpoint_transaction = wal_ctx->transaction_count; // Simule la dernière transaction traitée
    wal_ctx->checkpoint_count++;

    char msg[256];
    snprintf(msg, sizeof(msg), "Checkpoint cree: transaction=%zu, total_checkpoints=%zu", 
            wal_ctx->last_checkpoint_transaction, (size_t)wal_ctx->checkpoint_count);
    forensic_log(FORENSIC_LEVEL_INFO, "wal_extension_create_checkpoint_with_existing", msg);

    return true;
}

// === FIN NOUVELLES FONCTIONS EXTENSION ===


wal_extension_context_t* wal_extension_context_create(const char* wal_filename) {
    if (!wal_filename) return NULL;

    wal_extension_context_t* ctx = TRACKED_MALLOC(sizeof(wal_extension_context_t));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(wal_extension_context_t));
    ctx->magic_number = WAL_EXTENSION_MAGIC; // Utilisation du magic number défini pour le contexte WAL

    // Initialisation contexte base avec modules existants
    ctx->base_context = persistence_context_create("wal_data");
    if (!ctx->base_context) {
        TRACKED_FREE(ctx);
        return NULL;
    }

    strncpy(ctx->wal_extension_filename, wal_filename, sizeof(ctx->wal_extension_filename) - 1);

    // Ouverture fichier WAL en mode append
    ctx->wal_extension_file = fopen(wal_filename, "a+b");
    if (!ctx->wal_extension_file) {
        persistence_context_destroy(ctx->base_context);
        TRACKED_FREE(ctx);
        return NULL;
    }

    // Initialisation compteurs atomiques
    atomic_store(&ctx->sequence_counter_atomic, 1);
    atomic_store(&ctx->transaction_counter_atomic, 1);

    // Initialisation mutex
    if (pthread_mutex_init(&ctx->wal_extension_mutex, NULL) != 0) {
        fclose(ctx->wal_extension_file);
        persistence_context_destroy(ctx->base_context);
        TRACKED_FREE(ctx);
        return NULL;
    }

    ctx->auto_fsync_enabled = true;
    ctx->recovery_mode_active = false;
    ctx->transaction_count = 0; // Initialisation du compteur de transactions
    ctx->checkpoint_count = 0; // Initialisation du compteur de checkpoints
    ctx->last_checkpoint_transaction = 0; // Initialisation du dernier checkpoint
    ctx->transactions = NULL; // Initialisation du pointeur vers les transactions

    return ctx;
}

void wal_extension_context_destroy(wal_extension_context_t* ctx) {
    if (!ctx || ctx->magic_number != WAL_EXTENSION_MAGIC) return;

    pthread_mutex_lock(&ctx->wal_extension_mutex);

    if (ctx->wal_extension_file) {
        fflush(ctx->wal_extension_file);
        fclose(ctx->wal_extension_file);
        ctx->wal_extension_file = NULL;
    }

    if (ctx->base_context) {
        persistence_context_destroy(ctx->base_context);
        ctx->base_context = NULL;
    }

    // Libérer la mémoire allouée pour les transactions si elle est utilisée
    if (ctx->transactions) {
        TRACKED_FREE(ctx->transactions);
        ctx->transactions = NULL;
    }

    pthread_mutex_unlock(&ctx->wal_extension_mutex);
    pthread_mutex_destroy(&ctx->wal_extension_mutex);

    ctx->magic_number = 0; // Protection double-free
    TRACKED_FREE(ctx);
}

wal_extension_result_t* wal_extension_begin_transaction(wal_extension_context_t* ctx) {
    if (!ctx || ctx->magic_number != WAL_EXTENSION_MAGIC) return NULL;

    wal_extension_result_t* result = TRACKED_MALLOC(sizeof(wal_extension_result_t));
    if (!result) return NULL;

    memset(result, 0, sizeof(wal_extension_result_t));
    result->magic_number = WAL_RESULT_MAGIC;

    pthread_mutex_lock(&ctx->wal_extension_mutex);

    // Créer enregistrement WAL
    transaction_wal_extended_t wal_record = {0};
    wal_record.magic_number = TRANSACTION_WAL_MAGIC;
    wal_record.wal_magic_signature = WAL_MAGIC_SIGNATURE;
    wal_record.wal_version = WAL_VERSION;
    wal_record.sequence_number_global = atomic_fetch_add(&ctx->sequence_counter_atomic, 1);
    wal_record.nanosecond_timestamp = time(NULL) * 1000000000UL;

    // Créer transaction de base simulée
    uint64_t current_transaction_id = atomic_fetch_add(&ctx->transaction_counter_atomic, 1);

    wal_record.base_record.transaction_id = current_transaction_id;
    wal_record.base_record.operation_type = TRANSACTION_BEGIN;
    wal_record.base_record.timestamp = time(NULL);

    result->base_result = TRACKED_MALLOC(sizeof(storage_result_t));
    if (result->base_result) {
        result->base_result->success = true;
        result->base_result->transaction_ref = NULL; // Simplifié pour éviter les erreurs
    }

    // Calculer checksums
    wal_record.data_integrity_crc32 = wal_extension_calculate_crc32(&wal_record.base_record, sizeof(transaction_record_t));
    wal_record.header_integrity_crc32 = wal_extension_calculate_crc32(&wal_record, sizeof(transaction_wal_extended_t) - sizeof(uint32_t));

    // Écriture dans fichier WAL
    size_t written = fwrite(&wal_record, sizeof(transaction_wal_extended_t), 1, ctx->wal_extension_file);
    if (written == 1) {
        if (ctx->auto_fsync_enabled) {
            fflush(ctx->wal_extension_file);
            fsync(fileno(ctx->wal_extension_file));
        }

        result->wal_sequence_assigned = wal_record.sequence_number_global;
        result->wal_transaction_id = current_transaction_id; // Utiliser l'ID généré
        result->wal_durability_confirmed = true;

        snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                "Transaction %" PRIu64 " began successfully", result->wal_transaction_id);
    } else {
        snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                "Failed to write WAL record: %s", strerror(errno));
        result->wal_durability_confirmed = false;
        TRACKED_FREE(result->base_result); // Libérer si l'écriture a échoué
        result->base_result = NULL;
    }

    pthread_mutex_unlock(&ctx->wal_extension_mutex);
    return result;
}

wal_extension_result_t* wal_extension_commit_transaction(wal_extension_context_t* ctx, uint64_t transaction_id) {
    if (!ctx || ctx->magic_number != WAL_EXTENSION_MAGIC) return NULL;

    wal_extension_result_t* result = TRACKED_MALLOC(sizeof(wal_extension_result_t));
    if (!result) return NULL;

    memset(result, 0, sizeof(wal_extension_result_t));
    result->magic_number = WAL_RESULT_MAGIC;
    result->wal_transaction_id = transaction_id;

    pthread_mutex_lock(&ctx->wal_extension_mutex);

    // Créer enregistrement COMMIT
    transaction_wal_extended_t wal_record = {0};
    wal_record.magic_number = TRANSACTION_WAL_MAGIC;
    wal_record.wal_magic_signature = WAL_MAGIC_SIGNATURE;
    wal_record.wal_version = WAL_VERSION;
    wal_record.sequence_number_global = atomic_fetch_add(&ctx->sequence_counter_atomic, 1);
    wal_record.nanosecond_timestamp = time(NULL) * 1000000000UL;

    wal_record.base_record.transaction_id = transaction_id;
    wal_record.base_record.operation_type = TRANSACTION_COMMIT;
    wal_record.base_record.timestamp = time(NULL);

    // Calculer checksums
    wal_record.data_integrity_crc32 = wal_extension_calculate_crc32(&wal_record.base_record, sizeof(transaction_record_t));
    wal_record.header_integrity_crc32 = wal_extension_calculate_crc32(&wal_record, sizeof(transaction_wal_extended_t) - sizeof(uint32_t));

    // Écriture WAL
    size_t written = fwrite(&wal_record, sizeof(transaction_wal_extended_t), 1, ctx->wal_extension_file);
    if (written == 1) {
        if (ctx->auto_fsync_enabled) {
            fflush(ctx->wal_extension_file);
            fsync(fileno(ctx->wal_extension_file));
        }

        result->wal_sequence_assigned = wal_record.sequence_number_global;
        result->wal_durability_confirmed = true;

        snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                "Transaction %" PRIu64 " committed successfully", transaction_id);
    } else {
        snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                "Failed to commit transaction %" PRIu64 ": %s", transaction_id, strerror(errno));
        result->wal_durability_confirmed = false;
    }

    pthread_mutex_unlock(&ctx->wal_extension_mutex);
    return result;
}

wal_extension_result_t* wal_extension_rollback_transaction(wal_extension_context_t* ctx, uint64_t transaction_id) {
    if (!ctx || ctx->magic_number != WAL_EXTENSION_MAGIC) return NULL;

    wal_extension_result_t* result = TRACKED_MALLOC(sizeof(wal_extension_result_t));
    if (!result) return NULL;

    memset(result, 0, sizeof(wal_extension_result_t));
    result->magic_number = WAL_RESULT_MAGIC;
    result->wal_transaction_id = transaction_id;

    pthread_mutex_lock(&ctx->wal_extension_mutex);

    // Créer enregistrement ROLLBACK
    transaction_wal_extended_t wal_record = {0};
    wal_record.magic_number = TRANSACTION_WAL_MAGIC;
    wal_record.wal_magic_signature = WAL_MAGIC_SIGNATURE;
    wal_record.wal_version = WAL_VERSION;
    wal_record.sequence_number_global = atomic_fetch_add(&ctx->sequence_counter_atomic, 1);
    wal_record.nanosecond_timestamp = time(NULL) * 1000000000UL;

    wal_record.base_record.transaction_id = transaction_id;
    wal_record.base_record.operation_type = TRANSACTION_ROLLBACK;
    wal_record.base_record.timestamp = time(NULL);

    // Calculer checksums
    wal_record.data_integrity_crc32 = wal_extension_calculate_crc32(&wal_record.base_record, sizeof(transaction_record_t));
    wal_record.header_integrity_crc32 = wal_extension_calculate_crc32(&wal_record, sizeof(transaction_wal_extended_t) - sizeof(uint32_t));

    // Écriture WAL
    size_t written = fwrite(&wal_record, sizeof(transaction_wal_extended_t), 1, ctx->wal_extension_file);
    if (written == 1) {
        if (ctx->auto_fsync_enabled) {
            fflush(ctx->wal_extension_file);
            fsync(fileno(ctx->wal_extension_file));
        }

        result->wal_sequence_assigned = wal_record.sequence_number_global;
        result->wal_durability_confirmed = true;

        snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                "Transaction %" PRIu64 " rolled back successfully", transaction_id);
    } else {
        snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                "Failed to rollback transaction %" PRIu64 ": %s", transaction_id, strerror(errno));
        result->wal_durability_confirmed = false;
    }

    pthread_mutex_unlock(&ctx->wal_extension_mutex);
    return result;
}

wal_extension_result_t* wal_extension_log_lum_operation(wal_extension_context_t* ctx, 
                                                       uint64_t transaction_id,
                                                       const lum_t* lum) {
    if (!ctx || ctx->magic_number != WAL_EXTENSION_MAGIC || !lum) return NULL;

    wal_extension_result_t* result = TRACKED_MALLOC(sizeof(wal_extension_result_t));
    if (!result) return NULL;

    memset(result, 0, sizeof(wal_extension_result_t));
    result->magic_number = WAL_RESULT_MAGIC;
    result->wal_transaction_id = transaction_id;

    pthread_mutex_lock(&ctx->wal_extension_mutex);

    // Créer enregistrement pour opération LUM
    transaction_wal_extended_t wal_record = {0};
    wal_record.magic_number = TRANSACTION_WAL_MAGIC;
    wal_record.wal_magic_signature = WAL_MAGIC_SIGNATURE;
    wal_record.wal_version = WAL_VERSION;
    wal_record.sequence_number_global = atomic_fetch_add(&ctx->sequence_counter_atomic, 1);
    wal_record.nanosecond_timestamp = time(NULL) * 1000000000UL;

    wal_record.base_record.transaction_id = transaction_id;
    wal_record.base_record.operation_type = TRANSACTION_WRITE; // ou autre selon le contexte
    wal_record.base_record.timestamp = time(NULL);
    wal_record.base_record.lum_id = lum->id;

    // Calculer checksums incluant les données LUM
    wal_record.data_integrity_crc32 = wal_extension_calculate_crc32(lum, sizeof(lum_t));
    wal_record.header_integrity_crc32 = wal_extension_calculate_crc32(&wal_record, sizeof(transaction_wal_extended_t) - sizeof(uint32_t));

    // Écriture enregistrement WAL
    size_t written = fwrite(&wal_record, sizeof(transaction_wal_extended_t), 1, ctx->wal_extension_file);
    if (written == 1) {
        // Écriture données LUM après l'enregistrement
        written = fwrite(lum, sizeof(lum_t), 1, ctx->wal_extension_file);
        if (written == 1) {
            if (ctx->auto_fsync_enabled) {
                fflush(ctx->wal_extension_file);
                fsync(fileno(ctx->wal_extension_file));
            }

            result->wal_sequence_assigned = wal_record.sequence_number_global;
            result->wal_durability_confirmed = true;

            snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                    "LUM operation logged for transaction %" PRIu64 "", transaction_id);
        } else {
            snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                    "Failed to write LUM data: %s", strerror(errno));
            result->wal_durability_confirmed = false;
        }
    } else {
        snprintf(result->wal_error_details, sizeof(result->wal_error_details),
                "Failed to write WAL header: %s", strerror(errno));
        result->wal_durability_confirmed = false;
    }

    pthread_mutex_unlock(&ctx->wal_extension_mutex);
    return result;
}

bool wal_extension_verify_integrity_complete(wal_extension_context_t* ctx) {
    if (!ctx || ctx->magic_number != WAL_EXTENSION_MAGIC) return false;

    pthread_mutex_lock(&ctx->wal_extension_mutex);

    // Rembobiner fichier WAL
    if (fseek(ctx->wal_extension_file, 0, SEEK_SET) != 0) {
        forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete", "Failed to seek WAL file");
        pthread_mutex_unlock(&ctx->wal_extension_mutex);
        return false;
    }


    transaction_wal_extended_t wal_record;
    size_t records_read = 0;
    size_t integrity_errors = 0;

    while (fread(&wal_record, sizeof(transaction_wal_extended_t), 1, ctx->wal_extension_file) == 1) {
        records_read++;

        // Vérifier signature magique WALX
        if (wal_record.wal_magic_signature != WAL_MAGIC_SIGNATURE) {
            forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete",
                        "Invalid WAL magic signature at record %zu", records_read);
            integrity_errors++;
            continue; 
        }

        // Vérifier magic number de la transaction
        if (wal_record.magic_number != TRANSACTION_WAL_MAGIC) {
             forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete",
                        "Invalid transaction magic number at record %zu", records_read);
            integrity_errors++;
            continue;
        }

        // Vérifier checksum header
        uint32_t calculated_header_crc = wal_extension_calculate_crc32(&wal_record, sizeof(transaction_wal_extended_t) - sizeof(uint32_t));
        if (wal_record.header_integrity_crc32 != calculated_header_crc) {
            forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete",
                        "Header CRC32 mismatch at record %zu", records_read);
            integrity_errors++;
        }

        // Vérifier checksum data (base_record)
        uint32_t calculated_data_crc = wal_extension_calculate_crc32(&wal_record.base_record, sizeof(transaction_record_t));
        if (wal_record.data_integrity_crc32 != calculated_data_crc) {
            forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete",
                        "Data CRC32 mismatch for base_record at record %zu", records_read);
            integrity_errors++;
        }

        // Si l'enregistrement contient des données LUM, les vérifier aussi
        if (wal_record.base_record.operation_type == TRANSACTION_WRITE) {
            lum_t lum_data;
            // On doit lire les données LUM juste après l'en-tête WAL étendu
            if (fread(&lum_data, sizeof(lum_t), 1, ctx->wal_extension_file) == 1) {
                // Le data_integrity_crc32 est censé couvrir les données LUM dans ce cas
                uint32_t lum_crc = wal_extension_calculate_crc32(&lum_data, sizeof(lum_t));
                if (lum_crc != wal_record.data_integrity_crc32) {
                     forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete",
                                "LUM data CRC32 mismatch at record %zu", records_read);
                    integrity_errors++;
                }
            } else {
                 forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete",
                            "Failed to read LUM data at record %zu", records_read);
                integrity_errors++;
                // Si on ne peut pas lire les données LUM, on ne peut pas vérifier l'intégrité
                // On peut choisir de continuer ou d'arrêter ici. Continuons pour l'instant.
            }
        }
    }

    // Vérifier si la fin du fichier a été atteinte normalement
    if (!feof(ctx->wal_extension_file)) {
        forensic_log(FORENSIC_LEVEL_ERROR, "wal_extension_verify_integrity_complete",
                    "Error reading WAL file or unexpected end of file.");
        integrity_errors++;
    }


    pthread_mutex_unlock(&ctx->wal_extension_mutex);

    printf("🔍 WAL Integrity Check: %zu records read, %zu errors found.\n", records_read, integrity_errors);
    return integrity_errors == 0;
}

void wal_extension_result_destroy(wal_extension_result_t* result) {
    if (!result || result->magic_number != WAL_RESULT_MAGIC) return;

    if (result->base_result) {
        TRACKED_FREE(result->base_result);
        result->base_result = NULL;
    }

    result->magic_number = 0; // Protection double-free
    TRACKED_FREE(result);
}