#include "version_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char build_info[256] = {0};

version_registry_t* version_registry_create(void) {
    version_registry_t* registry = (version_registry_t*)calloc(1, sizeof(version_registry_t));
    if (!registry) return NULL;
    
    registry->module_capacity = 64;
    registry->modules = (module_info_t*)calloc(registry->module_capacity, sizeof(module_info_t));
    if (!registry->modules) {
        free(registry);
        return NULL;
    }
    
    registry->current_version.major = LUM_VORAX_VERSION_MAJOR;
    registry->current_version.minor = LUM_VORAX_VERSION_MINOR;
    registry->current_version.patch = LUM_VORAX_VERSION_PATCH;
    
    return registry;
}

void version_registry_destroy(version_registry_t* registry) {
    if (!registry) return;
    
    for (size_t i = 0; i < registry->module_count; i++) {
        free((void*)registry->modules[i].dependencies);
    }
    
    free(registry->modules);
    free(registry->changelog);
    free(registry);
}

int version_registry_add_module(version_registry_t* registry, const module_info_t* info) {
    if (!registry || !info) return -1;
    
    if (registry->module_count >= registry->module_capacity) {
        size_t new_cap = registry->module_capacity * 2;
        module_info_t* new_modules = (module_info_t*)realloc(
            registry->modules, new_cap * sizeof(module_info_t));
        if (!new_modules) return -2;
        registry->modules = new_modules;
        registry->module_capacity = new_cap;
    }
    
    registry->modules[registry->module_count++] = *info;
    return 0;
}

module_info_t* version_registry_get_module(version_registry_t* registry, const char* name) {
    if (!registry || !name) return NULL;
    
    for (size_t i = 0; i < registry->module_count; i++) {
        if (strcmp(registry->modules[i].module_name, name) == 0) {
            return &registry->modules[i];
        }
    }
    
    return NULL;
}

int semver_parse(const char* str, semver_t* version) {
    if (!str || !version) return -1;
    
    memset(version, 0, sizeof(semver_t));
    
    int major = 0, minor = 0, patch = 0;
    int count = sscanf(str, "%d.%d.%d", &major, &minor, &patch);
    
    if (count < 1) return -2;
    
    version->major = (uint8_t)major;
    version->minor = (uint8_t)minor;
    version->patch = (uint8_t)patch;
    
    const char* pre = strchr(str, '-');
    if (pre) {
        const char* build = strchr(pre, '+');
        size_t pre_len = build ? (size_t)(build - pre - 1) : strlen(pre + 1);
        if (pre_len >= sizeof(version->prerelease)) {
            pre_len = sizeof(version->prerelease) - 1;
        }
        strncpy(version->prerelease, pre + 1, pre_len);
    }
    
    const char* build = strchr(str, '+');
    if (build) {
        strncpy(version->build_metadata, build + 1, sizeof(version->build_metadata) - 1);
    }
    
    return 0;
}

int semver_compare(const semver_t* a, const semver_t* b) {
    if (!a || !b) return 0;
    
    if (a->major != b->major) return (a->major > b->major) ? 1 : -1;
    if (a->minor != b->minor) return (a->minor > b->minor) ? 1 : -1;
    if (a->patch != b->patch) return (a->patch > b->patch) ? 1 : -1;
    
    if (a->prerelease[0] && !b->prerelease[0]) return -1;
    if (!a->prerelease[0] && b->prerelease[0]) return 1;
    if (a->prerelease[0] && b->prerelease[0]) {
        return strcmp(a->prerelease, b->prerelease);
    }
    
    return 0;
}

char* semver_to_string(const semver_t* version, char* buffer, size_t buffer_size) {
    if (!version || !buffer || buffer_size == 0) return NULL;
    
    int written;
    if (version->prerelease[0] && version->build_metadata[0]) {
        written = snprintf(buffer, buffer_size, "%d.%d.%d-%s+%s",
                           version->major, version->minor, version->patch,
                           version->prerelease, version->build_metadata);
    } else if (version->prerelease[0]) {
        written = snprintf(buffer, buffer_size, "%d.%d.%d-%s",
                           version->major, version->minor, version->patch,
                           version->prerelease);
    } else if (version->build_metadata[0]) {
        written = snprintf(buffer, buffer_size, "%d.%d.%d+%s",
                           version->major, version->minor, version->patch,
                           version->build_metadata);
    } else {
        written = snprintf(buffer, buffer_size, "%d.%d.%d",
                           version->major, version->minor, version->patch);
    }
    
    return (written > 0 && (size_t)written < buffer_size) ? buffer : NULL;
}

bool semver_satisfies(const semver_t* version, const char* constraint) {
    if (!version || !constraint) return false;
    
    char op[3] = {0};
    semver_t required;
    
    if (constraint[0] == '^') {
        if (semver_parse(constraint + 1, &required) != 0) return false;
        return (version->major == required.major) && 
               (semver_compare(version, &required) >= 0);
    } else if (constraint[0] == '~') {
        if (semver_parse(constraint + 1, &required) != 0) return false;
        return (version->major == required.major) && 
               (version->minor == required.minor) &&
               (semver_compare(version, &required) >= 0);
    } else if (strncmp(constraint, ">=", 2) == 0) {
        if (semver_parse(constraint + 2, &required) != 0) return false;
        return semver_compare(version, &required) >= 0;
    } else if (strncmp(constraint, "<=", 2) == 0) {
        if (semver_parse(constraint + 2, &required) != 0) return false;
        return semver_compare(version, &required) <= 0;
    } else if (constraint[0] == '>') {
        if (semver_parse(constraint + 1, &required) != 0) return false;
        return semver_compare(version, &required) > 0;
    } else if (constraint[0] == '<') {
        if (semver_parse(constraint + 1, &required) != 0) return false;
        return semver_compare(version, &required) < 0;
    } else if (constraint[0] == '=') {
        if (semver_parse(constraint + 1, &required) != 0) return false;
        return semver_compare(version, &required) == 0;
    } else {
        if (semver_parse(constraint, &required) != 0) return false;
        return semver_compare(version, &required) == 0;
    }
}

const char* version_get_string(void) {
    return LUM_VORAX_VERSION_STRING;
}

void version_get_semver(semver_t* version) {
    if (!version) return;
    version->major = LUM_VORAX_VERSION_MAJOR;
    version->minor = LUM_VORAX_VERSION_MINOR;
    version->patch = LUM_VORAX_VERSION_PATCH;
    version->prerelease[0] = '\0';
    version->build_metadata[0] = '\0';
}

const char* version_get_build_info(void) {
    if (build_info[0] == '\0') {
        snprintf(build_info, sizeof(build_info), 
                 "LUM/VORAX V%s built on %s at %s",
                 LUM_VORAX_VERSION_STRING, LUM_VORAX_BUILD_DATE, LUM_VORAX_BUILD_TIME);
    }
    return build_info;
}
