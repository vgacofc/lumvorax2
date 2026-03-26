#include "recovery_manager_extension.h"
#include "../debug/memory_tracker.h"
#include <signal.h>
#include <setjmp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define RECOVERY_MANAGER_MAGIC 0x52454356  // "RECV"
#define RECOVERY_INFO_MAGIC 0x52494E46     // "RINF"

static recovery_manager_extension_t* global_recovery_manager = NULL;

// Handler pour signaux de crash
void crash_signal_handler(int sig) {
    (void)sig; // Supprime warning unused parameter
    (void)sig; // Supprime warning unused parameter
    if (global_recovery_manager) {
        recovery_manager_extension_mark_clean_shutdown(global_recovery_manager);
    }
    exit(1);
}

recovery_manager_extension_t* recovery_manager_extension_create(const char* data_directory, 
                                                               const char* wal_filename) {
    if (!data_directory || !wal_filename) return NULL;

    recovery_manager_extension_t* manager = TRACKED_MALLOC(sizeof(recovery_manager_extension_t));
    if (!manager) return NULL;

    memset(manager, 0, sizeof(recovery_manager_extension_t));
    manager->magic_number = RECOVERY_MANAGER_MAGIC;

    // Copier chemins
    strncpy(manager->data_directory_path, data_directory, sizeof(manager->data_directory_path) - 1);
    strncpy(manager->wal_extension_filename, wal_filename, sizeof(manager->wal_extension_filename) - 1);

    // Créer répertoire si nécessaire
    mkdir(data_directory, 0755);

    // Initialiser contexte persistance existant
    manager->base_persistence_ctx = persistence_context_create(data_directory);
    if (!manager->base_persistence_ctx) {
        TRACKED_FREE(manager);
        return NULL;
    }

    // Initialiser extension WAL
    manager->wal_extension_ctx = wal_extension_context_create(wal_filename);
    if (!manager->wal_extension_ctx) {
        persistence_context_destroy(manager->base_persistence_ctx);
        TRACKED_FREE(manager);
        return NULL;
    }

    manager->auto_recovery_enabled = true;
    manager->max_recovery_attempts = 3;

    // Charger informations recovery existantes
    char recovery_info_path[512];
    snprintf(recovery_info_path, sizeof(recovery_info_path), "%s/%s", 
             data_directory, RECOVERY_STATE_EXTENSION_FILE);

    manager->current_recovery_info = recovery_info_extension_load(recovery_info_path);
    if (!manager->current_recovery_info) {
        // Créer nouvelle info de recovery
        manager->current_recovery_info = TRACKED_MALLOC(sizeof(recovery_info_extension_t));
        if (manager->current_recovery_info) {
            memset(manager->current_recovery_info, 0, sizeof(recovery_info_extension_t));
            manager->current_recovery_info->magic_number = RECOVERY_INFO_MAGIC;
            manager->current_recovery_info->state = RECOVERY_STATE_NORMAL_EXTENDED;
            manager->current_recovery_info->auto_recovery_enabled = true;
            strncpy(manager->current_recovery_info->wal_extension_filename, wal_filename, 
                   sizeof(manager->current_recovery_info->wal_extension_filename) - 1);
            strncpy(manager->current_recovery_info->persistence_directory, data_directory,
                   sizeof(manager->current_recovery_info->persistence_directory) - 1);
        }
    }

    // Installer handlers signaux pour détection crash
    signal(SIGINT, crash_signal_handler);
    signal(SIGTERM, crash_signal_handler);
    signal(SIGSEGV, crash_signal_handler);
    signal(SIGABRT, crash_signal_handler);

    global_recovery_manager = manager;

    return manager;
}

void recovery_manager_extension_destroy(recovery_manager_extension_t* manager) {
    if (!manager || manager->magic_number != RECOVERY_MANAGER_MAGIC) return;

    // Marquer arrêt propre
    recovery_manager_extension_mark_clean_shutdown(manager);

    if (manager->current_recovery_info) {
        recovery_info_extension_destroy(manager->current_recovery_info);
    }

    if (manager->wal_extension_ctx) {
        wal_extension_context_destroy(manager->wal_extension_ctx);
    }

    if (manager->base_persistence_ctx) {
        persistence_context_destroy(manager->base_persistence_ctx);
    }

    manager->magic_number = 0; // Protection double-free
    TRACKED_FREE(manager);

    if (global_recovery_manager == manager) {
        global_recovery_manager = NULL;
    }
}

bool recovery_manager_extension_detect_previous_crash(recovery_manager_extension_t* manager) {
    if (!manager || manager->magic_number != RECOVERY_MANAGER_MAGIC) return false;

    char crash_file_path[512];
    snprintf(crash_file_path, sizeof(crash_file_path), "%s/%s", 
             manager->data_directory_path, CRASH_DETECTION_EXTENSION_FILE);

    struct stat crash_stat;
    if (stat(crash_file_path, &crash_stat) == 0) {
        // Fichier crash existe = crash précédent détecté
        if (manager->current_recovery_info) {
            manager->current_recovery_info->state = RECOVERY_STATE_CRASHED_DETECTED;
            manager->current_recovery_info->crash_timestamp_nanoseconds = 
                crash_stat.st_mtime * 1000000000UL;
        }
        return true;
    }

    return false;
}

bool recovery_manager_extension_mark_clean_shutdown(recovery_manager_extension_t* manager) {
    if (!manager || manager->magic_number != RECOVERY_MANAGER_MAGIC) return false;

    char crash_file_path[512];
    snprintf(crash_file_path, sizeof(crash_file_path), "%s/%s", 
             manager->data_directory_path, CRASH_DETECTION_EXTENSION_FILE);

    // Supprimer fichier crash pour indiquer arrêt propre
    unlink(crash_file_path);

    if (manager->current_recovery_info) {
        manager->current_recovery_info->state = RECOVERY_STATE_NORMAL_EXTENDED;

        // Sauvegarder état recovery
        char recovery_info_path[512];
        snprintf(recovery_info_path, sizeof(recovery_info_path), "%s/%s", 
                 manager->data_directory_path, RECOVERY_STATE_EXTENSION_FILE);

        recovery_info_extension_save(manager->current_recovery_info, recovery_info_path);
    }

    return true;
}

bool recovery_manager_extension_mark_startup_begin(recovery_manager_extension_t* manager) {
    if (!manager || manager->magic_number != RECOVERY_MANAGER_MAGIC) return false;

    char crash_file_path[512];
    snprintf(crash_file_path, sizeof(crash_file_path), "%s/%s", 
             manager->data_directory_path, CRASH_DETECTION_EXTENSION_FILE);

    // Créer fichier crash pour détecter crash éventuel
    FILE* crash_file = fopen(crash_file_path, "w");
    if (crash_file) {
        fprintf(crash_file, "LUM_SYSTEM_RUNNING_PID_%d\n", getpid());
        fprintf(crash_file, "STARTUP_TIME_%lu\n", time(NULL));
        fclose(crash_file);
        return true;
    }

    return false;
}

bool recovery_manager_extension_auto_recover_complete(recovery_manager_extension_t* manager) {
    if (!manager || manager->magic_number != RECOVERY_MANAGER_MAGIC) return false;

    if (!manager->current_recovery_info || 
        manager->current_recovery_info->state != RECOVERY_STATE_CRASHED_DETECTED) {
        return true; // Pas de crash à récupérer
    }

    printf("🔄 === DÉMARRAGE RECOVERY AUTOMATIQUE ===\n");

    manager->current_recovery_info->state = RECOVERY_STATE_RECOVERING_ACTIVE;
    manager->current_recovery_info->recovery_timestamp_nanoseconds = time(NULL) * 1000000000UL;
    manager->current_recovery_info->recovery_attempts_count++;

    // Vérifier limite tentatives
    if (manager->current_recovery_info->recovery_attempts_count > manager->max_recovery_attempts) {
        printf("[ERROR] Nombre maximum de tentatives recovery atteint\n");
        manager->current_recovery_info->state = RECOVERY_STATE_FAILED_EXTENDED;
        snprintf(manager->current_recovery_info->error_details_extended,
                sizeof(manager->current_recovery_info->error_details_extended),
                "Max recovery attempts (%u) exceeded", manager->max_recovery_attempts);
        return false;
    }

    // Étape 1: Vérifier intégrité WAL
    printf("[CHECK] Étape 1: Vérification intégrité WAL...\n");
    if (!wal_extension_verify_integrity_complete(manager->wal_extension_ctx)) {
        printf("[ERROR] Intégrité WAL compromise\n");
        manager->current_recovery_info->state = RECOVERY_STATE_FAILED_EXTENDED;
        snprintf(manager->current_recovery_info->error_details_extended,
                sizeof(manager->current_recovery_info->error_details_extended),
                "WAL integrity check failed");
        return false;
    }
    printf("[OK] WAL intègre\n");

    // Étape 2: Vérifier intégrité données persistantes
    printf("[CHECK] Étape 2: Vérification intégrité données...\n");
    if (!recovery_manager_extension_verify_data_integrity_with_existing(manager)) {
        printf("[WARNING] Intégrité données compromise, création backup d'urgence...\n");
        if (!recovery_manager_extension_create_emergency_backup_extended(manager)) {
            printf("[ERROR] Échec création backup d'urgence\n");
            manager->current_recovery_info->state = RECOVERY_STATE_FAILED_EXTENDED;
            return false;
        }
    }
    printf("[OK] Données intègres\n");

    // Étape 3: Replay WAL depuis dernier checkpoint
    printf("🔄 Étape 3: Replay transactions WAL...\n");
    if (!wal_extension_replay_from_existing_persistence(manager->wal_extension_ctx, 
                                                        manager->base_persistence_ctx)) {
        printf("[ERROR] Échec replay WAL\n");
        manager->current_recovery_info->state = RECOVERY_STATE_FAILED_EXTENDED;
        snprintf(manager->current_recovery_info->error_details_extended,
                sizeof(manager->current_recovery_info->error_details_extended),
                "WAL replay failed");
        return false;
    }
    printf("[OK] Replay WAL terminé\n");

    // Étape 4: Créer nouveau checkpoint
    printf("[SAVE] Étape 4: Création checkpoint post-recovery...\n");
    if (!wal_extension_create_checkpoint_with_existing(manager->wal_extension_ctx,
                                                       manager->base_persistence_ctx)) {
        printf("[WARNING] Échec création checkpoint (non fatal)\n");
    } else {
        printf("[OK] Checkpoint créé\n");
    }

    // Recovery réussie
    manager->current_recovery_info->state = RECOVERY_STATE_RECOVERED_SUCCESS;
    snprintf(manager->current_recovery_info->error_details_extended,
            sizeof(manager->current_recovery_info->error_details_extended),
            "Recovery completed successfully after %u attempts", 
            manager->current_recovery_info->recovery_attempts_count);

    // Sauvegarder état recovery
    char recovery_info_path[512];
    snprintf(recovery_info_path, sizeof(recovery_info_path), "%s/%s", 
             manager->data_directory_path, RECOVERY_STATE_EXTENSION_FILE);
    recovery_info_extension_save(manager->current_recovery_info, recovery_info_path);

    printf("[SUCCESS] === RECOVERY AUTOMATIQUE TERMINÉE AVEC SUCCÈS ===\n");
    return true;
}

bool recovery_manager_extension_verify_data_integrity_with_existing(recovery_manager_extension_t* manager) {
    if (!manager || manager->magic_number != RECOVERY_MANAGER_MAGIC) return false;

    // Utiliser les fonctions de vérification existantes du module persistence
    if (!manager->base_persistence_ctx) return false;

    // Lister fichiers dans répertoire données
    char data_path[512];
    snprintf(data_path, sizeof(data_path), "%s", manager->data_directory_path);

    DIR* dir = opendir(data_path);
    if (!dir) return false;

    struct dirent* entry;
    size_t files_verified = 0;
    size_t integrity_errors = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".lum") != NULL) {
            char full_path[768];
            snprintf(full_path, sizeof(full_path), "%s/%s", data_path, entry->d_name);

            // Utiliser fonction vérification existante
            if (!persistence_verify_file_integrity(manager->base_persistence_ctx, full_path)) {
                integrity_errors++;
                printf("[ERROR] Intégrité compromise: %s\n", entry->d_name);
            } else {
                files_verified++;
            }
        }
    }

    closedir(dir);

    printf("[CHECK] Intégrité: %zu fichiers OK, %zu erreurs\n", files_verified, integrity_errors);
    return integrity_errors == 0;
}

bool recovery_manager_extension_create_emergency_backup_extended(recovery_manager_extension_t* manager) {
    if (!manager || manager->magic_number != RECOVERY_MANAGER_MAGIC) return false;

    char backup_dir[512];
    snprintf(backup_dir, sizeof(backup_dir), "%s_emergency_backup_%lu", 
             manager->data_directory_path, time(NULL));

    if (mkdir(backup_dir, 0755) != 0) {
        return false;
    }

    // Copier fichiers données critiques
    char copy_cmd[1024];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp -r %s/* %s/ 2>/dev/null", 
             manager->data_directory_path, backup_dir);

    int result = system(copy_cmd);

    printf("[BACKUP] Backup d'urgence: %s (status: %d)\n", backup_dir, result);
    return result == 0;
}

recovery_info_extension_t* recovery_info_extension_load(const char* filename) {
    if (!filename) return NULL;

    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    recovery_info_extension_t* info = TRACKED_MALLOC(sizeof(recovery_info_extension_t));
    if (!info) {
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(info, sizeof(recovery_info_extension_t), 1, file);
    fclose(file);

    if (read_bytes != 1 || info->magic_number != RECOVERY_INFO_MAGIC) {
        TRACKED_FREE(info);
        return NULL;
    }

    return info;
}

bool recovery_info_extension_save(const recovery_info_extension_t* info, const char* filename) {
    if (!info || !filename || info->magic_number != RECOVERY_INFO_MAGIC) return false;

    FILE* file = fopen(filename, "wb");
    if (!file) return false;

    size_t written = fwrite(info, sizeof(recovery_info_extension_t), 1, file);
    fclose(file);

    return written == 1;
}

void recovery_info_extension_destroy(recovery_info_extension_t* info) {
    if (!info || info->magic_number != RECOVERY_INFO_MAGIC) return;

    info->magic_number = 0; // Protection double-free
    TRACKED_FREE(info);
}

bool initialize_lum_system_with_auto_recovery_extension(const char* data_directory, 
                                                       const char* wal_filename) {
    if (!data_directory || !wal_filename) return false;

    printf("[INIT] === INITIALISATION SYSTÈME LUM/VORAX AVEC AUTO-RECOVERY ===\n");

    // Créer manager recovery
    recovery_manager_extension_t* recovery_manager = 
        recovery_manager_extension_create(data_directory, wal_filename);

    if (!recovery_manager) {
        printf("[ERROR] Échec création recovery manager\n");
        return false;
    }

    // Marquer démarrage
    if (!recovery_manager_extension_mark_startup_begin(recovery_manager)) {
        printf("[WARNING] Impossible de marquer démarrage\n");
    }

    // Détecter crash précédent
    bool crash_detected = recovery_manager_extension_detect_previous_crash(recovery_manager);
    if (crash_detected) {
        printf("[ALERT] CRASH PRÉCÉDENT DÉTECTÉ - Démarrage recovery automatique\n");

        if (!recovery_manager_extension_auto_recover_complete(recovery_manager)) {
            printf("[ERROR] Recovery automatique échouée\n");
            recovery_manager_extension_destroy(recovery_manager);
            return false;
        }

        printf("[SUCCESS] Recovery automatique réussie\n");
    } else {
        printf("[OK] Démarrage normal - pas de crash détecté\n");
    }

    // Système prêt
    printf("[SUCCESS] === SYSTÈME LUM/VORAX INITIALISÉ AVEC AUTO-RECOVERY ===\n");

    // Note: manager reste actif pour toute la durée du programme
    // Il sera détruit automatiquement à l'arrêt par les signal handlers

    return true;
}