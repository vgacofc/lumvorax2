#ifndef API_CONTRACT_H
#define API_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define API_VERSION_MAJOR 1
#define API_VERSION_MINOR 0
#define API_STABILITY_STABLE 1
#define API_STABILITY_EXPERIMENTAL 0

typedef enum {
    API_TYPE_FUNCTION,
    API_TYPE_STRUCT,
    API_TYPE_ENUM,
    API_TYPE_TYPEDEF,
    API_TYPE_MACRO,
    API_TYPE_VARIABLE
} api_type_t;

typedef enum {
    API_DEPRECATED_NO,
    API_DEPRECATED_WARN,
    API_DEPRECATED_ERROR
} api_deprecated_t;

typedef struct {
    const char* name;
    const char* description;
    api_type_t type;
    bool is_public;
    bool is_stable;
    api_deprecated_t deprecated;
    const char* since_version;
    const char* deprecated_since;
    const char* replacement;
    const char* signature;
} api_entry_t;

typedef struct {
    api_entry_t* entries;
    size_t entry_count;
    size_t entry_capacity;
    const char* module_name;
    uint8_t api_version_major;
    uint8_t api_version_minor;
} api_contract_t;

api_contract_t* api_contract_create(const char* module_name);
void api_contract_destroy(api_contract_t* contract);
int api_contract_add_function(api_contract_t* contract, const char* name, 
                               const char* signature, const char* description,
                               bool is_stable, const char* since);
int api_contract_add_struct(api_contract_t* contract, const char* name,
                             const char* description, bool is_stable, const char* since);
int api_contract_deprecate(api_contract_t* contract, const char* name,
                            const char* since, const char* replacement);
api_entry_t* api_contract_find(api_contract_t* contract, const char* name);
bool api_contract_is_stable(api_contract_t* contract, const char* name);
int api_contract_export_json(api_contract_t* contract, char* buffer, size_t buffer_size);
int api_contract_validate(api_contract_t* contract);

#endif
