/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_kernel_loader_static.c — Chargeur Bytecode Statique (0% OpenCL)
 *
 * CYCLE C198 — Phase 12A — Élimination Totale Dépendance OpenCL
 * STANDARD_NAMES.md v4.5 §M-BTC-KERNEL-LOADER-STATIC-C198
 *
 * OBJECTIF : Charger kernels Gen9 ISA depuis bytecode embarqué
 *            SANS AUCUNE DÉPENDANCE OPENCL RUNTIME
 *
 * ARCHITECTURE :
 * - Bytecode compilé OFFLINE via ocloc
 * - Embarqué dans headers C (.h)
 * - Chargé directement via Level Zero
 * - 0% dépendance OpenCL runtime
 */

#include "btc_levelzero_runner.h"
#include "btc_levelzero_driver_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Inclure tous les kernels embarqués */
#include "embedded/test_add_embedded.h"

/* ══════════════════════════════════════════════════════════════════════
 * STRUCTURES INTERNES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Entrée catalogue kernels embarqués
 */
typedef struct {
    const char* name;
    const uint8_t* isa_data;
    size_t isa_size;
} btc_embedded_kernel_entry_t;

/* ══════════════════════════════════════════════════════════════════════
 * CATALOGUE KERNELS EMBARQUÉS
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Catalogue de tous les kernels embarqués
 * 
 * Ajouter ici chaque nouveau kernel compilé offline
 */
static const btc_embedded_kernel_entry_t g_embedded_kernels[] = {
    {
        .name = "test_add",
        .isa_data = test_add_gen9_isa,
        .isa_size = test_add_gen9_isa_len
    },
    {
        .name = "vector_add",  /* Alias pour test_add */
        .isa_data = test_add_gen9_isa,
        .isa_size = test_add_gen9_isa_len
    },
    /* Ajouter ici les futurs kernels SHA-256, etc. */
};

static const size_t g_embedded_kernels_count = sizeof(g_embedded_kernels) / sizeof(g_embedded_kernels[0]);

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Charge kernel depuis bytecode embarqué (0% OpenCL)
 * 
 * @param ctx Contexte Level Zero
 * @param kernel_name Nom du kernel ("test_add", "sha256_simple", etc.)
 * @return Handle kernel Level Zero (NULL si erreur)
 */
void* btc_load_embedded_kernel(btc_l0_context_t* ctx, const char* kernel_name) {
    if (!ctx || !kernel_name) {
        fprintf(stderr, "[C198-STATIC] ERROR: Invalid parameters\n");
        return NULL;
    }
    
    fprintf(stderr, "[C198-STATIC] Recherche kernel embarqué: %s\n", kernel_name);
    
    /* Rechercher kernel dans catalogue */
    const btc_embedded_kernel_entry_t* entry = NULL;
    for (size_t i = 0; i < g_embedded_kernels_count; i++) {
        if (strcmp(g_embedded_kernels[i].name, kernel_name) == 0) {
            entry = &g_embedded_kernels[i];
            break;
        }
    }
    
    if (!entry) {
        fprintf(stderr, "[C198-STATIC] ERROR: Kernel '%s' non trouvé dans catalogue\n", kernel_name);
        fprintf(stderr, "[C198-STATIC] Kernels disponibles:\n");
        for (size_t i = 0; i < g_embedded_kernels_count; i++) {
            fprintf(stderr, "[C198-STATIC]   - %s (%zu bytes)\n", 
                    g_embedded_kernels[i].name, g_embedded_kernels[i].isa_size);
        }
        return NULL;
    }
    
    fprintf(stderr, "[C198-STATIC] ✅ Kernel trouvé: %s (%zu bytes)\n", 
            entry->name, entry->isa_size);
    
    /* Créer module Level Zero depuis bytecode (0% OpenCL) */
    ze_module_desc_t module_desc = {0};
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.pNext = NULL;
    module_desc.format = ZE_MODULE_FORMAT_NATIVE;  /* Format natif Gen9 ISA */
    module_desc.inputSize = entry->isa_size;
    module_desc.pInputModule = entry->isa_data;
    module_desc.pBuildFlags = NULL;
    module_desc.pConstants = NULL;
    
    ze_module_handle_t module = NULL;
    ze_module_build_log_handle_t build_log = NULL;
    
    fprintf(stderr, "[C198-STATIC] Création module Level Zero...\n");
    
    ze_result_t result = btc_l0_zeModuleCreate(
        ctx->ze_context,
        ctx->ze_device,
        &module_desc,
        &module,
        &build_log
    );
    
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-STATIC] ERROR: zeModuleCreate failed: %d\n", result);
        
        /* Afficher build log si disponible */
        if (build_log) {
            size_t log_size = 0;
            btc_l0_zeModuleBuildLogGetString(build_log, &log_size, NULL);
            if (log_size > 0) {
                char* log_str = (char*)malloc(log_size);
                if (log_str) {
                    btc_l0_zeModuleBuildLogGetString(build_log, &log_size, log_str);
                    fprintf(stderr, "[C198-STATIC] Build log:\n%s\n", log_str);
                    free(log_str);
                }
            }
            btc_l0_zeModuleBuildLogDestroy(build_log);
        }
        return NULL;
    }
    
    fprintf(stderr, "[C198-STATIC] ✅ Module créé avec succès\n");
    
    /* Créer kernel depuis module */
    ze_kernel_desc_t kernel_desc = {0};
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pNext = NULL;
    kernel_desc.flags = 0;
    kernel_desc.pKernelName = kernel_name;
    
    ze_kernel_handle_t kernel = NULL;
    
    fprintf(stderr, "[C198-STATIC] Création kernel '%s'...\n", kernel_name);
    
    result = btc_l0_zeKernelCreate(module, &kernel_desc, &kernel);
    
    if (result != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C198-STATIC] ERROR: zeKernelCreate failed: %d\n", result);
        btc_l0_zeModuleDestroy(module);
        return NULL;
    }
    
    fprintf(stderr, "[C198-STATIC] ✅ Kernel '%s' créé avec succès (0%% OpenCL)\n", kernel_name);
    
    return kernel;
}

/**
 * @brief Liste tous les kernels embarqués disponibles
 * 
 * @return Nombre de kernels disponibles
 */
size_t btc_list_embedded_kernels(void) {
    fprintf(stderr, "[C198-STATIC] Kernels embarqués disponibles (%zu):\n", g_embedded_kernels_count);
    for (size_t i = 0; i < g_embedded_kernels_count; i++) {
        fprintf(stderr, "[C198-STATIC]   %zu. %s (%zu bytes)\n", 
                i + 1, g_embedded_kernels[i].name, g_embedded_kernels[i].isa_size);
    }
    return g_embedded_kernels_count;
}

/**
 * @brief Vérifie si un kernel est disponible
 * 
 * @param kernel_name Nom du kernel
 * @return true si disponible
 */
bool btc_is_kernel_available(const char* kernel_name) {
    if (!kernel_name) {
        return false;
    }
    
    for (size_t i = 0; i < g_embedded_kernels_count; i++) {
        if (strcmp(g_embedded_kernels[i].name, kernel_name) == 0) {
            return true;
        }
    }
    
    return false;
}

/* Made with Bob - LumVorax C198 Phase 12A - 0% OpenCL Runtime */

// Made with Bob
