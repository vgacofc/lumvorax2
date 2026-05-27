
#ifndef MODULE_DEPENDENCY_MANAGER_H
#define MODULE_DEPENDENCY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// Types de dépendances entre modules
typedef enum {
    DEPENDENCY_HARD,    // Dépendance critique
    DEPENDENCY_SOFT,    // Dépendance optionnelle
    DEPENDENCY_CIRCULAR // Dépendance circulaire détectée
} dependency_type_e;

// Structure dépendance module
typedef struct {
    char module_name[64];
    char depends_on[64];
    dependency_type_e type;
    uint32_t version_required;
    uint32_t version_current;
    bool is_compatible;
} module_dependency_t;

// Gestionnaire dépendances global
typedef struct {
    module_dependency_t* dependencies;
    size_t count;
    size_t capacity;
    bool auto_sync_enabled;
    uint64_t last_sync_timestamp;
    void* memory_address;
} dependency_manager_t;

// Fonctions principales
dependency_manager_t* dependency_manager_create(void);
void dependency_manager_destroy(dependency_manager_t** manager);

bool dependency_register_module(dependency_manager_t* manager, 
                               const char* module_name, uint32_t version);
bool dependency_add_dependency(dependency_manager_t* manager,
                              const char* module, const char* depends_on);

bool dependency_check_all_compatible(dependency_manager_t* manager);
bool dependency_auto_sync_modules(dependency_manager_t* manager);

// Validation avant modification
bool dependency_can_modify_module(dependency_manager_t* manager, const char* module_name);
void dependency_notify_module_updated(dependency_manager_t* manager, const char* module_name);

#endif // MODULE_DEPENDENCY_MANAGER_H
