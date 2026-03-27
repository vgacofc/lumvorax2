
#include "logging_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI
// Aucune utilisation d'emoji dans le code source ou dans les fichiers de log. 
// Toute inclusion d'emoji sera considérée comme une violation des standards de codage.

void create_log(const char* filepath, const char* message) {
    if (!filepath || !message) return;
    
    // Créer le répertoire si nécessaire
    char dir_path[MAX_STORAGE_PATH_LENGTH];
    int result = snprintf(dir_path, sizeof(dir_path), "logs");
    if (result >= (int)sizeof(dir_path) || result < 0) {
        unified_forensic_log(FORENSIC_LEVEL_ERROR, "create_log", "Path trop long pour répertoire logs");
        return; // Path trop long
    }
    
    mkdir(dir_path, 0755);
    
    FILE *file = fopen(filepath, "a");
    if (file != NULL) {
        // Ajouter timestamp
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(file, "[%s] %s\n", timestamp, message);
        fclose(file);
    }
}

void log_module_action(const char* module_name, const char* action) {
    if (!module_name || !action) return;
    
    char log_filepath[MAX_STORAGE_PATH_LENGTH];
    int result = snprintf(log_filepath, sizeof(log_filepath), "logs/%s.log", module_name);
    
    if (result >= 0 && result < (int)sizeof(log_filepath)) {
        create_log(log_filepath, action);
    }
}

void log_error_safe(const char* module_name, const char* error_msg) {
    if (!module_name || !error_msg) return;
    
    char log_filepath[MAX_STORAGE_PATH_LENGTH];
    char full_msg[MAX_ERROR_MESSAGE_LENGTH];
    
    int path_result = snprintf(log_filepath, sizeof(log_filepath), "logs/%s_errors.log", module_name);
    int msg_result = snprintf(full_msg, sizeof(full_msg), "ERROR: %s", error_msg);
    
    if (path_result >= 0 && path_result < (int)sizeof(log_filepath) &&
        msg_result >= 0 && msg_result < (int)sizeof(full_msg)) {
        create_log(log_filepath, full_msg);
    }
}
