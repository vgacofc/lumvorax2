#include "api_contract.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

api_contract_t* api_contract_create(const char* module_name) {
    api_contract_t* contract = (api_contract_t*)calloc(1, sizeof(api_contract_t));
    if (!contract) return NULL;
    
    contract->entry_capacity = 128;
    contract->entries = (api_entry_t*)calloc(contract->entry_capacity, sizeof(api_entry_t));
    if (!contract->entries) {
        free(contract);
        return NULL;
    }
    
    contract->module_name = module_name;
    contract->api_version_major = API_VERSION_MAJOR;
    contract->api_version_minor = API_VERSION_MINOR;
    
    return contract;
}

void api_contract_destroy(api_contract_t* contract) {
    if (contract) {
        free(contract->entries);
        free(contract);
    }
}

static int api_contract_add_entry(api_contract_t* contract, const api_entry_t* entry) {
    if (!contract || !entry) return -1;
    
    if (contract->entry_count >= contract->entry_capacity) {
        size_t new_cap = contract->entry_capacity * 2;
        api_entry_t* new_entries = (api_entry_t*)realloc(
            contract->entries, new_cap * sizeof(api_entry_t));
        if (!new_entries) return -2;
        contract->entries = new_entries;
        contract->entry_capacity = new_cap;
    }
    
    contract->entries[contract->entry_count++] = *entry;
    return 0;
}

int api_contract_add_function(api_contract_t* contract, const char* name,
                               const char* signature, const char* description,
                               bool is_stable, const char* since) {
    api_entry_t entry = {
        .name = name,
        .description = description,
        .type = API_TYPE_FUNCTION,
        .is_public = true,
        .is_stable = is_stable,
        .deprecated = API_DEPRECATED_NO,
        .since_version = since,
        .signature = signature
    };
    return api_contract_add_entry(contract, &entry);
}

int api_contract_add_struct(api_contract_t* contract, const char* name,
                             const char* description, bool is_stable, const char* since) {
    api_entry_t entry = {
        .name = name,
        .description = description,
        .type = API_TYPE_STRUCT,
        .is_public = true,
        .is_stable = is_stable,
        .deprecated = API_DEPRECATED_NO,
        .since_version = since
    };
    return api_contract_add_entry(contract, &entry);
}

int api_contract_deprecate(api_contract_t* contract, const char* name,
                            const char* since, const char* replacement) {
    api_entry_t* entry = api_contract_find(contract, name);
    if (!entry) return -1;
    
    entry->deprecated = API_DEPRECATED_WARN;
    entry->deprecated_since = since;
    entry->replacement = replacement;
    
    return 0;
}

api_entry_t* api_contract_find(api_contract_t* contract, const char* name) {
    if (!contract || !name) return NULL;
    
    for (size_t i = 0; i < contract->entry_count; i++) {
        if (strcmp(contract->entries[i].name, name) == 0) {
            return &contract->entries[i];
        }
    }
    
    return NULL;
}

bool api_contract_is_stable(api_contract_t* contract, const char* name) {
    api_entry_t* entry = api_contract_find(contract, name);
    return entry ? entry->is_stable : false;
}

static const char* api_type_to_string(api_type_t type) {
    switch (type) {
        case API_TYPE_FUNCTION: return "function";
        case API_TYPE_STRUCT: return "struct";
        case API_TYPE_ENUM: return "enum";
        case API_TYPE_TYPEDEF: return "typedef";
        case API_TYPE_MACRO: return "macro";
        case API_TYPE_VARIABLE: return "variable";
        default: return "unknown";
    }
}

int api_contract_export_json(api_contract_t* contract, char* buffer, size_t buffer_size) {
    if (!contract || !buffer || buffer_size == 0) return -1;
    
    char* ptr = buffer;
    size_t remaining = buffer_size;
    int written;
    
    written = snprintf(ptr, remaining, "{\n  \"module\": \"%s\",\n  \"version\": \"%d.%d\",\n  \"entries\": [\n",
                       contract->module_name ? contract->module_name : "unknown",
                       contract->api_version_major, contract->api_version_minor);
    if (written < 0 || (size_t)written >= remaining) return -2;
    ptr += written;
    remaining -= written;
    
    for (size_t i = 0; i < contract->entry_count; i++) {
        api_entry_t* entry = &contract->entries[i];
        
        written = snprintf(ptr, remaining, 
                           "    {\n"
                           "      \"name\": \"%s\",\n"
                           "      \"type\": \"%s\",\n"
                           "      \"stable\": %s,\n"
                           "      \"deprecated\": %s\n"
                           "    }%s\n",
                           entry->name ? entry->name : "",
                           api_type_to_string(entry->type),
                           entry->is_stable ? "true" : "false",
                           entry->deprecated != API_DEPRECATED_NO ? "true" : "false",
                           (i < contract->entry_count - 1) ? "," : "");
        if (written < 0 || (size_t)written >= remaining) return -3;
        ptr += written;
        remaining -= written;
    }
    
    written = snprintf(ptr, remaining, "  ]\n}\n");
    if (written < 0 || (size_t)written >= remaining) return -4;
    
    return (int)(buffer_size - remaining + written);
}

int api_contract_validate(api_contract_t* contract) {
    if (!contract) return -1;
    
    int errors = 0;
    
    for (size_t i = 0; i < contract->entry_count; i++) {
        api_entry_t* entry = &contract->entries[i];
        
        if (!entry->name || entry->name[0] == '\0') {
            errors++;
        }
        
        if (entry->type == API_TYPE_FUNCTION && !entry->signature) {
            errors++;
        }
        
        if (entry->deprecated != API_DEPRECATED_NO && !entry->replacement) {
            errors++;
        }
    }
    
    return errors;
}
