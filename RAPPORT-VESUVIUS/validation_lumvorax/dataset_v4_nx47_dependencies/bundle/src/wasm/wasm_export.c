#include "wasm_export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t WASM_MAGIC[] = {0x00, 0x61, 0x73, 0x6D};
static const uint8_t WASM_VERSION[] = {0x01, 0x00, 0x00, 0x00};

wasm_module_t* wasm_module_create(void) {
    wasm_module_t* module = (wasm_module_t*)calloc(1, sizeof(wasm_module_t));
    if (!module) return NULL;
    
    module->export_capacity = 32;
    module->exports = (wasm_export_func_t*)calloc(module->export_capacity, sizeof(wasm_export_func_t));
    if (!module->exports) {
        free(module);
        return NULL;
    }
    
    module->memory = (wasm_memory_t*)calloc(1, sizeof(wasm_memory_t));
    if (!module->memory) {
        free(module->exports);
        free(module);
        return NULL;
    }
    
    module->memory->pages = 1;
    module->memory->max_pages = WASM_MAX_PAGES;
    module->memory->memory = (uint8_t*)calloc(module->memory->pages, WASM_PAGE_SIZE);
    if (!module->memory->memory) {
        free(module->memory);
        free(module->exports);
        free(module);
        return NULL;
    }
    
    return module;
}

void wasm_module_destroy(wasm_module_t* module) {
    if (!module) return;
    
    for (size_t i = 0; i < module->export_count; i++) {
        free((void*)module->exports[i].name);
        free(module->exports[i].param_types);
        free(module->exports[i].result_types);
    }
    
    free(module->exports);
    
    if (module->memory) {
        free(module->memory->memory);
        free(module->memory);
    }
    
    free(module->module_bytes);
    free(module);
}

int wasm_module_add_export(wasm_module_t* module, const char* name, void* func_ptr,
                           wasm_value_type_t* params, size_t param_count,
                           wasm_value_type_t* results, size_t result_count) {
    if (!module || !name || !func_ptr) return -1;
    
    if (module->export_count >= module->export_capacity) {
        size_t new_cap = module->export_capacity * 2;
        wasm_export_func_t* new_exports = (wasm_export_func_t*)realloc(
            module->exports, new_cap * sizeof(wasm_export_func_t));
        if (!new_exports) return -2;
        module->exports = new_exports;
        module->export_capacity = new_cap;
    }
    
    wasm_export_func_t* exp = &module->exports[module->export_count];
    
    exp->name = strdup(name);
    if (!exp->name) return -3;
    
    exp->func_ptr = func_ptr;
    exp->param_count = param_count;
    exp->result_count = result_count;
    
    if (param_count > 0 && params) {
        exp->param_types = (wasm_value_type_t*)malloc(param_count * sizeof(wasm_value_type_t));
        if (!exp->param_types) {
            free((void*)exp->name);
            return -4;
        }
        memcpy(exp->param_types, params, param_count * sizeof(wasm_value_type_t));
    }
    
    if (result_count > 0 && results) {
        exp->result_types = (wasm_value_type_t*)malloc(result_count * sizeof(wasm_value_type_t));
        if (!exp->result_types) {
            free((void*)exp->name);
            free(exp->param_types);
            return -5;
        }
        memcpy(exp->result_types, results, result_count * sizeof(wasm_value_type_t));
    }
    
    module->export_count++;
    return 0;
}

static size_t encode_leb128(uint8_t* buffer, uint32_t value) {
    size_t count = 0;
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) byte |= 0x80;
        buffer[count++] = byte;
    } while (value != 0);
    return count;
}

int wasm_module_compile(wasm_module_t* module) {
    if (!module) return -1;
    
    size_t estimated_size = 8 + 1024 * module->export_count;
    module->module_bytes = (uint8_t*)malloc(estimated_size);
    if (!module->module_bytes) return -2;
    
    uint8_t* ptr = module->module_bytes;
    
    memcpy(ptr, WASM_MAGIC, 4);
    ptr += 4;
    memcpy(ptr, WASM_VERSION, 4);
    ptr += 4;
    
    *ptr++ = 0x01;
    uint8_t* type_size_ptr = ptr++;
    size_t type_section_start = ptr - module->module_bytes;
    
    ptr += encode_leb128(ptr, (uint32_t)module->export_count);
    
    for (size_t i = 0; i < module->export_count; i++) {
        wasm_export_func_t* exp = &module->exports[i];
        *ptr++ = 0x60;
        ptr += encode_leb128(ptr, (uint32_t)exp->param_count);
        for (size_t j = 0; j < exp->param_count; j++) {
            *ptr++ = (uint8_t)exp->param_types[j];
        }
        ptr += encode_leb128(ptr, (uint32_t)exp->result_count);
        for (size_t j = 0; j < exp->result_count; j++) {
            *ptr++ = (uint8_t)exp->result_types[j];
        }
    }
    
    *type_size_ptr = (uint8_t)(ptr - module->module_bytes - type_section_start);
    
    *ptr++ = 0x03;
    *ptr++ = (uint8_t)(1 + module->export_count);
    ptr += encode_leb128(ptr, (uint32_t)module->export_count);
    for (size_t i = 0; i < module->export_count; i++) {
        ptr += encode_leb128(ptr, (uint32_t)i);
    }
    
    *ptr++ = 0x05;
    *ptr++ = 0x03;
    *ptr++ = 0x01;
    *ptr++ = 0x00;
    *ptr++ = 0x01;
    
    *ptr++ = 0x07;
    uint8_t* export_size_ptr = ptr++;
    size_t export_section_start = ptr - module->module_bytes;
    
    ptr += encode_leb128(ptr, (uint32_t)module->export_count + 1);
    
    for (size_t i = 0; i < module->export_count; i++) {
        wasm_export_func_t* exp = &module->exports[i];
        size_t name_len = strlen(exp->name);
        ptr += encode_leb128(ptr, (uint32_t)name_len);
        memcpy(ptr, exp->name, name_len);
        ptr += name_len;
        *ptr++ = 0x00;
        ptr += encode_leb128(ptr, (uint32_t)i);
    }
    
    const char* mem_name = "memory";
    size_t mem_name_len = strlen(mem_name);
    ptr += encode_leb128(ptr, (uint32_t)mem_name_len);
    memcpy(ptr, mem_name, mem_name_len);
    ptr += mem_name_len;
    *ptr++ = 0x02;
    *ptr++ = 0x00;
    
    *export_size_ptr = (uint8_t)(ptr - module->module_bytes - export_section_start);
    
    module->module_size = ptr - module->module_bytes;
    module->compiled = true;
    
    return 0;
}

size_t wasm_module_get_bytes(wasm_module_t* module, uint8_t* buffer, size_t buffer_size) {
    if (!module || !module->compiled || !buffer) return 0;
    
    size_t copy_size = (module->module_size < buffer_size) ? module->module_size : buffer_size;
    memcpy(buffer, module->module_bytes, copy_size);
    
    return copy_size;
}

wasm_instance_t* wasm_instance_create(wasm_module_t* module) {
    if (!module || !module->compiled) return NULL;
    
    wasm_instance_t* instance = (wasm_instance_t*)calloc(1, sizeof(wasm_instance_t));
    if (!instance) return NULL;
    
    instance->module = module;
    
    instance->memory = (wasm_memory_t*)calloc(1, sizeof(wasm_memory_t));
    if (!instance->memory) {
        free(instance);
        return NULL;
    }
    
    instance->memory->pages = module->memory->pages;
    instance->memory->max_pages = module->memory->max_pages;
    instance->memory->memory = (uint8_t*)calloc(instance->memory->pages, WASM_PAGE_SIZE);
    if (!instance->memory->memory) {
        free(instance->memory);
        free(instance);
        return NULL;
    }
    
    instance->stack = (uint8_t*)malloc(WASM_STACK_SIZE);
    if (!instance->stack) {
        free(instance->memory->memory);
        free(instance->memory);
        free(instance);
        return NULL;
    }
    instance->stack_size = WASM_STACK_SIZE;
    
    instance->initialized = true;
    
    return instance;
}

void wasm_instance_destroy(wasm_instance_t* instance) {
    if (!instance) return;
    
    if (instance->memory) {
        free(instance->memory->memory);
        free(instance->memory);
    }
    
    free(instance->stack);
    free(instance->globals);
    free(instance);
}

int wasm_instance_call(wasm_instance_t* instance, const char* func_name,
                       wasm_value_t* args, size_t arg_count,
                       wasm_value_t* results, size_t result_count) {
    if (!instance || !func_name) return -1;
    if (!instance->module) return -5;
    
    wasm_module_t* mod = instance->module;
    if (!mod->exports) return -6;
    
    for (size_t i = 0; i < mod->export_count; i++) {
        if (mod->exports[i].name && strcmp(mod->exports[i].name, func_name) == 0) {
            wasm_export_func_t* func = &mod->exports[i];
            if (func->param_count != arg_count) return -3;
            if (func->result_count != result_count) return -4;
            return 0;
        }
    }
    
    return -2;
}

void* wasm_memory_get_ptr(wasm_instance_t* instance, uint32_t offset) {
    if (!instance || !instance->memory) return NULL;
    
    size_t mem_size = instance->memory->pages * WASM_PAGE_SIZE;
    if (offset >= mem_size) return NULL;
    
    return instance->memory->memory + offset;
}

int wasm_memory_grow(wasm_instance_t* instance, size_t pages) {
    if (!instance || !instance->memory) return -1;
    
    size_t new_pages = instance->memory->pages + pages;
    if (new_pages > instance->memory->max_pages) return -2;
    
    uint8_t* new_mem = (uint8_t*)realloc(instance->memory->memory, new_pages * WASM_PAGE_SIZE);
    if (!new_mem) return -3;
    
    memset(new_mem + instance->memory->pages * WASM_PAGE_SIZE, 0, pages * WASM_PAGE_SIZE);
    
    instance->memory->memory = new_mem;
    instance->memory->pages = new_pages;
    
    return (int)(new_pages - pages);
}

size_t wasm_memory_size(wasm_instance_t* instance) {
    if (!instance || !instance->memory) return 0;
    return instance->memory->pages * WASM_PAGE_SIZE;
}
