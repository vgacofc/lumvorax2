
#ifndef LOGGING_SYSTEM_H_INCLUDED
#define LOGGING_SYSTEM_H_INCLUDED

#include "../common/common_types.h"
#include <time.h>

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI
// Aucune utilisation d'emoji dans le code source ou dans les fichiers de log. 
// Toute inclusion d'emoji sera considérée comme une violation des standards de codage.

// Fonction de logging sécurisée
void create_log(const char* filepath, const char* message);
void log_module_action(const char* module_name, const char* action);
void log_error_safe(const char* module_name, const char* error_msg);

// Macro pour logging avec validation de taille
#define SAFE_LOG(module, msg) do { \
    char safe_msg[MAX_ERROR_MESSAGE_LENGTH]; \
    int result = snprintf(safe_msg, sizeof(safe_msg), "%s", msg); \
    if (result >= 0 && result < (int)sizeof(safe_msg)) { \
        log_module_action(module, safe_msg); \
    } else { \
        log_error_safe(module, "Message trop long pour le logging"); \
    } \
} while(0)

#endif // LOGGING_SYSTEM_H_INCLUDED
