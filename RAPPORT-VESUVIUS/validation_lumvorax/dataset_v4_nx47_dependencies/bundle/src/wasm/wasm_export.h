#ifndef WASM_EXPORT_H
#define WASM_EXPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define WASM_VERSION_MAJOR 1
#define WASM_VERSION_MINOR 0
#define WASM_VERSION_PATCH 0

#define WASM_PAGE_SIZE 65536
#define WASM_MAX_PAGES 32768
#define WASM_STACK_SIZE 65536

typedef enum {
    WASM_TYPE_I32 = 0x7F,
    WASM_TYPE_I64 = 0x7E,
    WASM_TYPE_F32 = 0x7D,
    WASM_TYPE_F64 = 0x7C,
    WASM_TYPE_VOID = 0x40
} wasm_value_type_t;

typedef struct {
    wasm_value_type_t type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    } value;
} wasm_value_t;

typedef struct {
    const char* name;
    wasm_value_type_t* param_types;
    size_t param_count;
    wasm_value_type_t* result_types;
    size_t result_count;
    void* func_ptr;
} wasm_export_func_t;

typedef struct {
    uint8_t* memory;
    size_t pages;
    size_t max_pages;
    bool is_shared;
} wasm_memory_t;

typedef struct {
    wasm_export_func_t* exports;
    size_t export_count;
    size_t export_capacity;
    wasm_memory_t* memory;
    uint8_t* module_bytes;
    size_t module_size;
    bool compiled;
} wasm_module_t;

typedef struct {
    wasm_module_t* module;
    wasm_memory_t* memory;
    wasm_value_t* globals;
    size_t global_count;
    uint8_t* stack;
    size_t stack_size;
    bool initialized;
} wasm_instance_t;

wasm_module_t* wasm_module_create(void);
void wasm_module_destroy(wasm_module_t* module);
int wasm_module_add_export(wasm_module_t* module, const char* name, void* func_ptr,
                           wasm_value_type_t* params, size_t param_count,
                           wasm_value_type_t* results, size_t result_count);
int wasm_module_compile(wasm_module_t* module);
size_t wasm_module_get_bytes(wasm_module_t* module, uint8_t* buffer, size_t buffer_size);
wasm_instance_t* wasm_instance_create(wasm_module_t* module);
void wasm_instance_destroy(wasm_instance_t* instance);
int wasm_instance_call(wasm_instance_t* instance, const char* func_name,
                       wasm_value_t* args, size_t arg_count,
                       wasm_value_t* results, size_t result_count);
void* wasm_memory_get_ptr(wasm_instance_t* instance, uint32_t offset);
int wasm_memory_grow(wasm_instance_t* instance, size_t pages);
size_t wasm_memory_size(wasm_instance_t* instance);

#endif
