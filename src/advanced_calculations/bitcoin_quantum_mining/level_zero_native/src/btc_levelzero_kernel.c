/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_levelzero_kernel.c — Gestion Kernels Level Zero (Compilation + Exécution)
 *
 * CYCLE C198 — Phase 10A — Support SPIR-V + OpenCL C
 * STANDARD_NAMES.md v4.5 §M-BTC-LEVELZERO-KERNEL-C198
 *
 * OBJECTIFS :
 * 1. OPTION A: Charger kernel SPIR-V précompilé (.spv) via intel-ocloc
 * 2. OPTION B: Compiler kernel OpenCL C (.cl) via Online Compiler Level Zero
 * 3. Créer module Level Zero (ze_module_handle_t)
 * 4. Extraire kernel (ze_kernel_handle_t)
 * 5. Configurer arguments kernel
 * 6. Dispatcher kernel sur GPU avec traçabilité bit-level
 *
 * ARCHITECTURE :
 * - Support DOUBLE format: SPIR-V binaire + OpenCL C source
 * - Détection automatique extension fichier (.spv vs .cl)
 * - Traçabilité nanoseconde de chaque étape
 * - Logging forensique complet
 */

#include "btc_levelzero_runner.h"
#include "btc_levelzero_driver_loader.h"  /* C198 Phase 11: Wrappers Level Zero */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 1 : LECTURE FICHIER KERNEL
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Lit le contenu d'un fichier kernel (OpenCL C ou SPIR-V binaire)
 * C198 Phase 10A: Support SPIR-V + OpenCL C
 */
static uint8_t* read_kernel_file(const char* filepath, size_t* out_size) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : Impossible d'ouvrir %s : %s\n",
                filepath, strerror(errno));
        return NULL;
    }
    
    /* Obtient taille fichier */
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* Alloue buffer (pas de null terminator pour binaire) */
    uint8_t* data = malloc(size);
    if (!data) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : malloc() échoué\n");
        fclose(fp);
        return NULL;
    }
    
    /* Lit contenu */
    size_t read_size = fread(data, 1, size, fp);
    if (read_size != size) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : Lecture incomplète\n");
        free(data);
        fclose(fp);
        return NULL;
    }
    
    fclose(fp);
    
    if (out_size) *out_size = size;
    
    fprintf(stderr, "[C198-KERNEL] Fichier lu : %s (%zu bytes)\n", filepath, size);
    return data;
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 2 : COMPILATION KERNEL
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Compile un kernel en module Level Zero
 * C198 Phase 10A: Support DOUBLE format
 *
 * OPTION A (.spv): Charge SPIR-V binaire précompilé via intel-ocloc
 * OPTION B (.cl):  Compile OpenCL C via Online Compiler Level Zero
 *
 * Détection automatique basée sur extension fichier:
 * - .spv → ZE_MODULE_FORMAT_IL_SPIRV (SPIR-V binaire)
 * - .cl  → ZE_MODULE_FORMAT_NATIVE (OpenCL C source)
 */
int btc_l0_compile_kernel(
    btc_l0_context_t* ctx,
    const char* kernel_path,
    const char* build_options,
    ze_module_handle_t* out_module
) {
    if (!ctx || !kernel_path || !out_module) {
        return BTC_L0_ERROR_INVALID;
    }
    
    uint64_t start_time = btc_l0_get_timestamp().monotonic;
    
    btc_l0_log_event(ctx, "KERNEL_COMPILE_START", kernel_path, 0, 0, 0);
    
    /* Lit fichier kernel (SPIR-V ou OpenCL C) */
    size_t file_size;
    uint8_t* file_data = read_kernel_file(kernel_path, &file_size);
    if (!file_data) {
        btc_l0_log_event(ctx, "KERNEL_COMPILE_ERROR", "read_kernel_file failed", 0, 0, 0);
        return BTC_L0_ERROR_KERNEL;
    }
    
    /* Détecte format basé sur extension */
    const char* ext = strrchr(kernel_path, '.');
    ze_module_format_t format;
    const char* format_name;
    
    if (ext && strcmp(ext, ".spv") == 0) {
        /* OPTION A: SPIR-V binaire précompilé */
        format = ZE_MODULE_FORMAT_IL_SPIRV;
        format_name = "SPIR-V";
        fprintf(stderr, "[C198-KERNEL] Format détecté : SPIR-V binaire (%zu bytes)\n", file_size);
        
        /* Valide magic SPIR-V (0x07230203) */
        if (file_size >= 4) {
            uint32_t magic = *(uint32_t*)file_data;
            if (magic == 0x07230203) {
                fprintf(stderr, "[C198-KERNEL] ✓ Magic SPIR-V valide : 0x%08X\n", magic);
            } else {
                fprintf(stderr, "[C198-KERNEL] AVERTISSEMENT : Magic SPIR-V invalide : 0x%08X\n", magic);
            }
        }
    } else if (ext && strcmp(ext, ".bin") == 0) {
        /* OPTION B: Binaire Gen9 ISA natif (compilé via OpenCL runtime) */
        format = ZE_MODULE_FORMAT_NATIVE;
        format_name = "Gen9 ISA";
        fprintf(stderr, "[C198-KERNEL] Format détecté : Gen9 ISA natif (%zu bytes)\n", file_size);
        fprintf(stderr, "[C198-KERNEL] ✓ Binaire compilé via OpenCL runtime\n");
    } else {
        /* OPTION C: OpenCL C source */
        format = ZE_MODULE_FORMAT_NATIVE;
        format_name = "OpenCL C";
        fprintf(stderr, "[C198-KERNEL] Format détecté : OpenCL C source (%zu bytes)\n", file_size);
    }
    
    /* Prépare description module */
    ze_module_desc_t module_desc;
    memset(&module_desc, 0, sizeof(module_desc));  /* BUG FIX C198 #15 */
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.pNext = NULL;  /* CRITIQUE : éviter segfault */
    module_desc.format = format;
    module_desc.inputSize = file_size;
    module_desc.pInputModule = file_data;
    module_desc.pBuildFlags = build_options ? build_options : "-cl-std=CL2.0 -cl-kernel-arg-info";
    module_desc.pConstants = NULL;
    
    fprintf(stderr, "[C198-KERNEL] Compilation %s avec options: %s\n",
            format_name, module_desc.pBuildFlags);
    
    /* Compile module */
    ze_module_handle_t module = NULL;
    ze_module_build_log_handle_t build_log = NULL;
    
    ze_result_t ze_ret = btc_l0_zeModuleCreate(  /* BUG FIX C198 #15 : wrapper */
        ctx->ze_context,
        ctx->ze_device,
        &module_desc,
        &module,
        &build_log
    );
    
    /* Récupère log de compilation */
    if (build_log) {
        size_t log_size = 0;
        btc_l0_zeModuleBuildLogGetString(build_log, &log_size, NULL);  /* BUG FIX C198 #16 : wrapper */
        
        if (log_size > 0) {
            char* log_str = malloc(log_size);
            if (log_str) {
                btc_l0_zeModuleBuildLogGetString(build_log, &log_size, log_str);  /* BUG FIX C198 #16 : wrapper */
                fprintf(stderr, "[C198-KERNEL] Build log (%zu bytes):\n%s\n", log_size, log_str);
                free(log_str);
            }
        }
        
        btc_l0_zeModuleBuildLogDestroy(build_log);  /* BUG FIX C198 #17 : wrapper */
    }
    
    free(file_data);
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeModuleCreate() = %d\n", ze_ret);
        btc_l0_log_event(ctx, "KERNEL_COMPILE_ERROR", "zeModuleCreate failed", ze_ret, 0, 0);
        return BTC_L0_ERROR_KERNEL;
    }
    
    uint64_t compile_time = btc_l0_get_timestamp().monotonic - start_time;
    
    *out_module = module;
    
    btc_l0_log_event(ctx, "KERNEL_COMPILE_SUCCESS", kernel_path,
                    file_size, (uint64_t)module, compile_time);
    
    fprintf(stderr, "[C198-KERNEL] ✓ Kernel compilé : %s [%s] (module=%p, size=%zu, time=%lu ns)\n",
            kernel_path, format_name, (void*)module, file_size, compile_time);
    
    return BTC_L0_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 3 : CRÉATION KERNEL
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Crée un kernel depuis un module compilé
 */
int btc_l0_create_kernel(
    btc_l0_context_t* ctx,
    ze_module_handle_t module,
    const char* kernel_name,
    ze_kernel_handle_t* out_kernel
) {
    if (!ctx || !module || !kernel_name || !out_kernel) {
        return BTC_L0_ERROR_INVALID;
    }
    
    uint64_t start_time = btc_l0_get_timestamp().monotonic;
    
    btc_l0_log_event(ctx, "KERNEL_CREATE_START", kernel_name, (uint64_t)module, 0, 0);
    
    /* Prépare description kernel */
    ze_kernel_desc_t kernel_desc;
    memset(&kernel_desc, 0, sizeof(kernel_desc));  /* BUG FIX C198 #18 */
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pNext = NULL;  /* CRITIQUE : éviter segfault */
    kernel_desc.flags = 0;
    kernel_desc.pKernelName = kernel_name;
    
    /* Crée kernel */
    ze_kernel_handle_t kernel = NULL;
    ze_result_t ze_ret = btc_l0_zeKernelCreate(module, &kernel_desc, &kernel);  /* BUG FIX C198 #18 : wrapper */
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeKernelCreate() = %d\n", ze_ret);
        btc_l0_log_event(ctx, "KERNEL_CREATE_ERROR", "zeKernelCreate failed", ze_ret, 0, 0);
        return BTC_L0_ERROR_KERNEL;
    }
    
    uint64_t create_time = btc_l0_get_timestamp().monotonic - start_time;
    
    *out_kernel = kernel;
    
    btc_l0_log_event(ctx, "KERNEL_CREATE_SUCCESS", kernel_name,
                    (uint64_t)kernel, create_time, 0);
    
    fprintf(stderr, "[C198-KERNEL] Kernel créé : %s (handle=%p, time=%lu ns)\n",
            kernel_name, (void*)kernel, create_time);
    
    return BTC_L0_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 4 : CONFIGURATION ARGUMENTS
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Configure un argument kernel (buffer ou scalaire)
 */
int btc_l0_set_kernel_arg(
    btc_l0_context_t* ctx,
    ze_kernel_handle_t kernel,
    uint32_t arg_index,
    size_t arg_size,
    const void* arg_value
) {
    if (!ctx || !kernel) {
        return BTC_L0_ERROR_INVALID;
    }
    
    btc_l0_log_event(ctx, "KERNEL_SET_ARG", "Setting argument",
                    arg_index, arg_size, (uint64_t)arg_value);
    
    ze_result_t ze_ret = btc_l0_zeKernelSetArgumentValue(  /* BUG FIX C198 #19 : wrapper */
        kernel,
        arg_index,
        arg_size,
        arg_value
    );
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeKernelSetArgumentValue(arg=%u) = %d\n",
                arg_index, ze_ret);
        btc_l0_log_event(ctx, "KERNEL_SET_ARG_ERROR", "zeKernelSetArgumentValue failed",
                        arg_index, ze_ret, 0);
        return BTC_L0_ERROR_KERNEL;
    }
    
    fprintf(stderr, "[C198-KERNEL] Argument %u configuré (size=%zu)\n", arg_index, arg_size);
    
    return BTC_L0_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 5 : DISPATCH KERNEL
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Configure la taille des work-groups
 */
int btc_l0_set_group_size(
    btc_l0_context_t* ctx,
    ze_kernel_handle_t kernel,
    uint32_t group_size_x,
    uint32_t group_size_y,
    uint32_t group_size_z
) {
    if (!ctx || !kernel) {
        return BTC_L0_ERROR_INVALID;
    }
    
    btc_l0_log_event(ctx, "KERNEL_SET_GROUP_SIZE", "Setting group size",
                    group_size_x, group_size_y, group_size_z);
    
    ze_result_t ze_ret = btc_l0_zeKernelSetGroupSize(  /* BUG FIX C198 #19 : wrapper */
        kernel,
        group_size_x,
        group_size_y,
        group_size_z
    );
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeKernelSetGroupSize() = %d\n", ze_ret);
        btc_l0_log_event(ctx, "KERNEL_SET_GROUP_SIZE_ERROR", "zeKernelSetGroupSize failed",
                        ze_ret, 0, 0);
        return BTC_L0_ERROR_KERNEL;
    }
    
    fprintf(stderr, "[C198-KERNEL] Group size configuré : %u×%u×%u\n",
            group_size_x, group_size_y, group_size_z);
    
    return BTC_L0_OK;
}

/**
 * Dispatch kernel sur GPU
 */
int btc_l0_execute_kernel(
    btc_l0_context_t* ctx,
    ze_kernel_handle_t kernel,
    uint32_t grid_size_x,
    uint32_t grid_size_y,
    uint32_t grid_size_z
) {
    if (!ctx || !kernel) {
        return BTC_L0_ERROR_INVALID;
    }
    
    uint64_t start_time = btc_l0_get_timestamp().monotonic;
    
    btc_l0_log_event(ctx, "KERNEL_EXECUTE_START", "Dispatching kernel",
                    grid_size_x, grid_size_y, grid_size_z);
    
    /* Prépare dispatch */
    ze_group_count_t dispatch_args = {
        .groupCountX = grid_size_x,
        .groupCountY = grid_size_y,
        .groupCountZ = grid_size_z
    };
    
    /* Append kernel launch à la command list */
    ze_result_t ze_ret = btc_l0_zeCommandListAppendLaunchKernel(  /* BUG FIX C198 #20 : wrapper */
        ctx->ze_command_list,
        kernel,
        &dispatch_args,
        NULL,  /* Pas de signal event */
        0,     /* Pas de wait events */
        NULL
    );
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeCommandListAppendLaunchKernel() = %d\n", ze_ret);
        btc_l0_log_event(ctx, "KERNEL_EXECUTE_ERROR", "zeCommandListAppendLaunchKernel failed",
                        ze_ret, 0, 0);
        return BTC_L0_ERROR_EXEC;
    }
    
    /* Close command list */
    ze_ret = btc_l0_zeCommandListClose(ctx->ze_command_list);  /* BUG FIX C198 #21 : wrapper */
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeCommandListClose() = %d\n", ze_ret);
        btc_l0_log_event(ctx, "KERNEL_EXECUTE_ERROR", "zeCommandListClose failed",
                        ze_ret, 0, 0);
        return BTC_L0_ERROR_EXEC;
    }
    
    /* Execute command list */
    ze_command_list_handle_t cmd_list = (ze_command_list_handle_t)ctx->ze_command_list;
    ze_ret = btc_l0_zeCommandQueueExecuteCommandLists(  /* BUG FIX C198 #22 : wrapper */
        (ze_command_queue_handle_t)ctx->ze_command_queue,
        1,
        &cmd_list,
        NULL
    );
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeCommandQueueExecuteCommandLists() = %d\n", ze_ret);
        btc_l0_log_event(ctx, "KERNEL_EXECUTE_ERROR", "zeCommandQueueExecuteCommandLists failed",
                        ze_ret, 0, 0);
        return BTC_L0_ERROR_EXEC;
    }
    
    /* Synchronise avec timeout (C198 Phase 12H Bug #1 FIX) */
    uint64_t timeout_ns = (uint64_t)ctx->config.timeout_ms * 1000000ULL;  /* ms → ns */
    ze_ret = btc_l0_zeCommandQueueSynchronize(ctx->ze_command_queue, timeout_ns);
    if (ze_ret == ZE_RESULT_NOT_READY) {
        fprintf(stderr, "[C198-KERNEL] ⚠️  TIMEOUT GPU : kernel bloqué > %u ms\n", ctx->config.timeout_ms);
        btc_l0_log_event(ctx, "KERNEL_TIMEOUT", "GPU timeout exceeded", timeout_ns, 0, 0);
        return BTC_L0_ERROR_TIMEOUT;
    }
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] ERREUR : zeCommandQueueSynchronize() = %d\n", ze_ret);
        btc_l0_log_event(ctx, "KERNEL_EXECUTE_ERROR", "zeCommandQueueSynchronize failed",
                        ze_ret, 0, 0);
        return BTC_L0_ERROR_EXEC;
    }
    
    /* Reset command list pour réutilisation */
    ze_ret = btc_l0_zeCommandListReset(ctx->ze_command_list);  /* BUG FIX C198 #24 : wrapper */
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-KERNEL] AVERTISSEMENT : zeCommandListReset() = %d\n", ze_ret);
    }
    
    uint64_t exec_time = btc_l0_get_timestamp().monotonic - start_time;
    
    btc_l0_log_event(ctx, "KERNEL_EXECUTE_SUCCESS", "Kernel executed",
                    grid_size_x * grid_size_y * grid_size_z, exec_time, 0);
    
    fprintf(stderr, "[C198-KERNEL] Kernel exécuté : %u×%u×%u groups (time=%lu ns)\n",
            grid_size_x, grid_size_y, grid_size_z, exec_time);
    
    return BTC_L0_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 6 : CLEANUP
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Détruit un kernel
 */
void btc_l0_destroy_kernel(btc_l0_context_t* ctx, ze_kernel_handle_t kernel) {
    if (!kernel) return;
    
    btc_l0_log_event(ctx, "KERNEL_DESTROY", "Destroying kernel", (uint64_t)kernel, 0, 0);
    
    btc_l0_zeKernelDestroy(&kernel);  /* C198 Phase 12F Bug Fix : Pointeur nullifié */
    
    fprintf(stderr, "[C198-KERNEL] Kernel détruit : %p\n", (void*)kernel);
}

/**
 * Détruit un module
 */
void btc_l0_destroy_module(btc_l0_context_t* ctx, ze_module_handle_t module) {
    if (!module) return;
    
    btc_l0_log_event(ctx, "MODULE_DESTROY", "Destroying module", (uint64_t)module, 0, 0);
    
    btc_l0_zeModuleDestroy(&module);  /* C198 Phase 12F Bug Fix : Pointeur nullifié */
    
    fprintf(stderr, "[C198-KERNEL] Module détruit : %p\n", (void*)module);
}

// Made with Bob
