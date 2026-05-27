#ifndef VERSION_MANAGER_H
#define VERSION_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define LUM_VORAX_VERSION_MAJOR 32
#define LUM_VORAX_VERSION_MINOR 0
#define LUM_VORAX_VERSION_PATCH 0
#define LUM_VORAX_VERSION_STRING "32.0.0"
#define LUM_VORAX_BUILD_DATE __DATE__
#define LUM_VORAX_BUILD_TIME __TIME__

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    char prerelease[32];
    char build_metadata[64];
} semver_t;

typedef struct {
    const char* module_name;
    semver_t version;
    const char* author;
    const char* description;
    const char* license;
    const char** dependencies;
    size_t dependency_count;
} module_info_t;

typedef struct {
    semver_t version;
    const char* release_date;
    const char** changes;
    size_t change_count;
    bool breaking_changes;
} changelog_entry_t;

typedef struct {
    module_info_t* modules;
    size_t module_count;
    size_t module_capacity;
    changelog_entry_t* changelog;
    size_t changelog_count;
    semver_t current_version;
} version_registry_t;

version_registry_t* version_registry_create(void);
void version_registry_destroy(version_registry_t* registry);
int version_registry_add_module(version_registry_t* registry, const module_info_t* info);
module_info_t* version_registry_get_module(version_registry_t* registry, const char* name);
int semver_parse(const char* str, semver_t* version);
int semver_compare(const semver_t* a, const semver_t* b);
char* semver_to_string(const semver_t* version, char* buffer, size_t buffer_size);
bool semver_satisfies(const semver_t* version, const char* constraint);
const char* version_get_string(void);
void version_get_semver(semver_t* version);
const char* version_get_build_info(void);

#endif
