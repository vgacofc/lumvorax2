/**
 * TEST MINIMALISTE zeKernelCreate — Isoler crash Gen9
 * 
 * Objectif: Déterminer exactement où crash le backend compute Gen9
 * 
 * Stratégie:
 * 1. PAS de dispatch
 * 2. PAS d'arguments
 * 3. PAS de memory copy
 * 4. PAS de synchronize
 * 5. PAS de cleanup complexe
 * 
 * Seulement: zeModuleCreate → zeKernelCreate → printf → _exit(0)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <level_zero/ze_api.h>

// Prototypes fonctions Level Zero
typedef ze_result_t (*zeInit_fn)(ze_init_flags_t);
typedef ze_result_t (*zeDriverGet_fn)(uint32_t*, ze_driver_handle_t*);
typedef ze_result_t (*zeDeviceGet_fn)(ze_driver_handle_t, uint32_t*, ze_device_handle_t*);
typedef ze_result_t (*zeContextCreate_fn)(ze_driver_handle_t, const ze_context_desc_t*, ze_context_handle_t*);
typedef ze_result_t (*zeModuleCreate_fn)(ze_context_handle_t, ze_device_handle_t, const ze_module_desc_t*, ze_module_handle_t*, ze_module_build_log_handle_t*);
typedef ze_result_t (*zeKernelCreate_fn)(ze_module_handle_t, const ze_kernel_desc_t*, ze_kernel_handle_t*);

// Globals
static void* g_ze_lib = NULL;
static zeInit_fn zeInit_ptr = NULL;
static zeDriverGet_fn zeDriverGet_ptr = NULL;
static zeDeviceGet_fn zeDeviceGet_ptr = NULL;
static zeContextCreate_fn zeContextCreate_ptr = NULL;
static zeModuleCreate_fn zeModuleCreate_ptr = NULL;
static zeKernelCreate_fn zeKernelCreate_ptr = NULL;

// Kernel OpenCL C ultra minimal (addition vide)
static const char* KERNEL_EMPTY = 
"__kernel void empty_kernel() {\n"
"    // Kernel vide - aucune opération\n"
"}\n";

static const char* KERNEL_NOP = 
"__kernel void nop_kernel() {\n"
"    int x = 0;\n"
"    x = x + 1;\n"
"}\n";

static const char* KERNEL_SINGLE_THREAD = 
"__kernel void single_thread() {\n"
"    if (get_global_id(0) == 0) {\n"
"        int x = 42;\n"
"    }\n"
"}\n";

/**
 * Charger Level Zero dynamiquement
 */
int load_level_zero() {
    printf("[LOAD] Chargement libze_loader.so...\n");
    
    g_ze_lib = dlopen("libze_loader.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!g_ze_lib) {
        fprintf(stderr, "❌ Erreur dlopen: %s\n", dlerror());
        return -1;
    }
    
    zeInit_ptr = (zeInit_fn)dlsym(g_ze_lib, "zeInit");
    zeDriverGet_ptr = (zeDriverGet_fn)dlsym(g_ze_lib, "zeDriverGet");
    zeDeviceGet_ptr = (zeDeviceGet_fn)dlsym(g_ze_lib, "zeDeviceGet");
    zeContextCreate_ptr = (zeContextCreate_fn)dlsym(g_ze_lib, "zeContextCreate");
    zeModuleCreate_ptr = (zeModuleCreate_fn)dlsym(g_ze_lib, "zeModuleCreate");
    zeKernelCreate_ptr = (zeKernelCreate_fn)dlsym(g_ze_lib, "zeKernelCreate");
    
    if (!zeInit_ptr || !zeDriverGet_ptr || !zeDeviceGet_ptr || 
        !zeContextCreate_ptr || !zeModuleCreate_ptr || !zeKernelCreate_ptr) {
        fprintf(stderr, "❌ Erreur dlsym\n");
        return -1;
    }
    
    printf("✅ Level Zero chargé\n");
    return 0;
}

/**
 * Compiler kernel OpenCL C → SPIR-V via ocloc
 */
int compile_kernel_spirv(const char* source, const char* output_path) {
    printf("[COMPILE] Compilation OpenCL C → SPIR-V...\n");
    
    // Écrire source temporaire
    FILE* f = fopen("/tmp/test_kernel.cl", "w");
    if (!f) {
        fprintf(stderr, "❌ Erreur création fichier source\n");
        return -1;
    }
    fprintf(f, "%s", source);
    fclose(f);
    
    // Compiler avec ocloc
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "ocloc compile -file /tmp/test_kernel.cl -device skl "
        "-spirv_input -output %s -output_no_suffix 2>&1",
        output_path);
    
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "❌ Compilation échouée (code=%d)\n", ret);
        return -1;
    }
    
    printf("✅ SPIR-V généré: %s\n", output_path);
    return 0;
}

/**
 * Lire binaire SPIR-V
 */
uint8_t* read_spirv(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "❌ Erreur lecture %s\n", path);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* data = malloc(*size);
    if (!data) {
        fclose(f);
        return NULL;
    }
    
    fread(data, 1, *size, f);
    fclose(f);
    
    printf("✅ SPIR-V lu: %zu bytes\n", *size);
    return data;
}

/**
 * TEST 1: Module Create seul
 */
int test_module_create_only(ze_context_handle_t ctx, ze_device_handle_t dev, 
                             const uint8_t* spirv, size_t spirv_size) {
    printf("\n[TEST 1] zeModuleCreate seul...\n");
    
    ze_module_desc_t module_desc = {0};
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    module_desc.inputSize = spirv_size;
    module_desc.pInputModule = spirv;
    
    ze_module_handle_t module = NULL;
    ze_module_build_log_handle_t build_log = NULL;
    
    ze_result_t result = zeModuleCreate_ptr(ctx, dev, &module_desc, &module, &build_log);
    
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeModuleCreate échoué: 0x%x\n", result);
        return -1;
    }
    
    printf("✅ zeModuleCreate OK (module=%p)\n", (void*)module);
    return 0;
}

/**
 * TEST 2: Module Create + Kernel Create
 */
int test_kernel_create(ze_context_handle_t ctx, ze_device_handle_t dev,
                       const uint8_t* spirv, size_t spirv_size,
                       const char* kernel_name) {
    printf("\n[TEST 2] zeModuleCreate + zeKernelCreate...\n");
    
    // Module
    ze_module_desc_t module_desc = {0};
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    module_desc.inputSize = spirv_size;
    module_desc.pInputModule = spirv;
    
    ze_module_handle_t module = NULL;
    ze_module_build_log_handle_t build_log = NULL;
    
    ze_result_t result = zeModuleCreate_ptr(ctx, dev, &module_desc, &module, &build_log);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeModuleCreate échoué: 0x%x\n", result);
        return -1;
    }
    printf("✅ zeModuleCreate OK (module=%p)\n", (void*)module);
    
    // Kernel
    printf("[CRITICAL] Appel zeKernelCreate (kernel_name='%s')...\n", kernel_name);
    fflush(stdout);
    
    ze_kernel_desc_t kernel_desc = {0};
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pKernelName = kernel_name;
    
    ze_kernel_handle_t kernel = NULL;
    
    printf("[CRITICAL] Avant zeKernelCreate...\n");
    fflush(stdout);
    
    result = zeKernelCreate_ptr(module, &kernel_desc, &kernel);
    
    printf("[CRITICAL] Après zeKernelCreate (result=0x%x)\n", result);
    fflush(stdout);
    
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeKernelCreate échoué: 0x%x\n", result);
        return -1;
    }
    
    printf("✅ zeKernelCreate OK (kernel=%p)\n", (void*)kernel);
    return 0;
}

int main(int argc, char** argv) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  TEST MINIMALISTE zeKernelCreate — Isoler Crash Gen9\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    // Charger Level Zero
    if (load_level_zero() != 0) {
        return 1;
    }
    
    // Init Level Zero
    printf("[INIT] zeInit...\n");
    ze_result_t result = zeInit_ptr(0);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeInit échoué: 0x%x\n", result);
        return 1;
    }
    printf("✅ zeInit OK\n");
    
    // Driver
    printf("[INIT] zeDriverGet...\n");
    uint32_t driver_count = 0;
    result = zeDriverGet_ptr(&driver_count, NULL);
    if (result != ZE_RESULT_SUCCESS || driver_count == 0) {
        fprintf(stderr, "❌ Aucun driver Level Zero\n");
        return 1;
    }
    
    ze_driver_handle_t driver = NULL;
    driver_count = 1;
    result = zeDriverGet_ptr(&driver_count, &driver);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeDriverGet échoué\n");
        return 1;
    }
    printf("✅ Driver Level Zero: %p\n", (void*)driver);
    
    // Device
    printf("[INIT] zeDeviceGet...\n");
    uint32_t device_count = 0;
    result = zeDeviceGet_ptr(driver, &device_count, NULL);
    if (result != ZE_RESULT_SUCCESS || device_count == 0) {
        fprintf(stderr, "❌ Aucun device\n");
        return 1;
    }
    
    ze_device_handle_t device = NULL;
    device_count = 1;
    result = zeDeviceGet_ptr(driver, &device_count, &device);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeDeviceGet échoué\n");
        return 1;
    }
    printf("✅ Device Level Zero: %p\n", (void*)device);
    
    // Context
    printf("[INIT] zeContextCreate...\n");
    ze_context_desc_t ctx_desc = {0};
    ctx_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
    
    ze_context_handle_t context = NULL;
    result = zeContextCreate_ptr(driver, &ctx_desc, &context);
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeContextCreate échoué: 0x%x\n", result);
        return 1;
    }
    printf("✅ Context Level Zero: %p\n", (void*)context);
    
    // Compiler kernel vide
    const char* spirv_path = "/tmp/test_empty.spv";
    if (compile_kernel_spirv(KERNEL_EMPTY, spirv_path) != 0) {
        return 1;
    }
    
    // Lire SPIR-V
    size_t spirv_size = 0;
    uint8_t* spirv_data = read_spirv(spirv_path, &spirv_size);
    if (!spirv_data) {
        return 1;
    }
    
    // TEST 1: Module seul
    if (test_module_create_only(context, device, spirv_data, spirv_size) != 0) {
        free(spirv_data);
        return 1;
    }
    
    // TEST 2: Module + Kernel
    if (test_kernel_create(context, device, spirv_data, spirv_size, "empty_kernel") != 0) {
        free(spirv_data);
        return 1;
    }
    
    free(spirv_data);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✅ TOUS LES TESTS RÉUSSIS — PAS DE CRASH\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    // Exit propre sans cleanup (éviter double-free)
    _exit(0);
    return 0;
}

// Made with Bob
