/**
 * @file btc_hybrid_opencl_levelzero.h
 * @brief Module hybride OpenCL→Level Zero DYNAMIQUE avec traçabilité bit-level
 * 
 * OBJECTIF : Compiler OpenCL C → Gen9 ISA à la volée, puis charger dans Level Zero
 *            SANS dépendre de binaires précompilés.
 * 
 * ARCHITECTURE :
 *   OpenCL C Source (runtime)
 *        ↓
 *   OpenCL Compiler (clBuildProgram)
 *        ↓
 *   Gen9 ISA Binary (cache SHA-256)
 *        ↓
 *   Level Zero Runtime (contrôle direct GPU)
 *        ↓
 *   GPU Execution + Forensic Logging + Memory Tracker
 * 
 * TRAÇABILITÉ BIT-LEVEL :
 *   - Logging nanoseconde toutes opérations
 *   - Memory tracker allocations/libérations
 *   - Timestamps GPU natifs (events Level Zero)
 *   - Forensic logging complet
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10D)
 * DATE : 2026-05-09
 */

#ifndef BTC_HYBRID_OPENCL_LEVELZERO_H
#define BTC_HYBRID_OPENCL_LEVELZERO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

/**
 * @brief Statistiques cache binaires
 */
typedef struct {
    uint64_t total_compilations;  /**< Total compilations */
    uint64_t cache_hits;           /**< Cache hits */
    uint64_t cache_misses;         /**< Cache misses */
    uint64_t total_compile_time_ns; /**< Temps total compilation (ns) */
    uint64_t total_cache_load_time_ns; /**< Temps total chargement cache (ns) */
} btc_hybrid_stats_t;

/**
 * @brief Contexte hybride OpenCL→Level Zero
 */
typedef struct {
    // OpenCL (compilation)
    void* cl_platform;
    void* cl_device;
    void* cl_context;
    
    // Level Zero (exécution)
    void* ze_driver;
    void* ze_device;
    void* ze_context;
    void* ze_queue;
    void* ze_cmd_list;
    
    // Handles dynamiques
    void* gmmlib_handle;
    void* ze_driver_handle;
    
    // Pointeurs fonctions Level Zero
    void* ze_Init;
    void* ze_DriverGet;
    void* ze_DeviceGet;
    void* ze_ContextCreate;
    void* ze_CommandQueueCreate;
    void* ze_CommandListCreate;
    void* ze_MemAllocDevice;
    void* ze_MemAllocShared;
    void* ze_CommandListAppendMemoryCopy;
    void* ze_CommandListClose;
    void* ze_CommandQueueExecuteCommandLists;
    void* ze_CommandQueueSynchronize;
    void* ze_ModuleCreate;
    void* ze_KernelCreate;
    void* ze_KernelSetArgumentValue;
    void* ze_KernelSetGroupSize;
    void* ze_CommandListAppendLaunchKernel;
    void* ze_MemFree;
    void* ze_KernelDestroy;
    void* ze_ModuleDestroy;
    void* ze_CommandListDestroy;
    void* ze_CommandQueueDestroy;
    void* ze_ContextDestroy;
    void* ze_EventPoolCreate;
    void* ze_EventCreate;
    void* ze_EventHostSynchronize;
    void* ze_EventQueryStatus;
    void* ze_EventDestroy;
    void* ze_EventPoolDestroy;
    
    // Configuration
    char cache_dir[512];
    bool enable_cache;
    bool enable_forensic;
    bool dma_warmed_up;
    
    // Forensique
    FILE* forensic_log;
    
    // Statistiques
    btc_hybrid_stats_t stats;
} btc_hybrid_ctx_t;

/**
 * @brief Kernel compilé hybride
 */
typedef struct {
    void* ze_module;        /**< Module Level Zero */
    void* ze_kernel;        /**< Kernel Level Zero */
    uint8_t* isa_binary;    /**< Binaire Gen9 ISA */
    size_t isa_size;        /**< Taille binaire */
    uint32_t crc32;         /**< Checksum */
    char kernel_name[256];  /**< Nom kernel */
    uint64_t compile_time_ns; /**< Temps compilation */
    bool from_cache;        /**< Chargé depuis cache ? */
} btc_hybrid_kernel_t;

/**
 * @brief Buffer GPU
 */
typedef struct {
    void* ze_memory;        /**< Pointeur GPU */
    size_t size;            /**< Taille */
    bool is_device;         /**< Device ou shared ? */
    uint64_t alloc_time_ns; /**< Timestamp allocation */
} btc_hybrid_buffer_t;

/**
 * @brief Options compilation
 */
typedef struct {
    const char* build_options;  /**< Options clBuildProgram */
    bool optimize;              /**< -O3 */
    bool fast_math;             /**< -cl-fast-relaxed-math */
    bool warnings;              /**< Afficher warnings */
} btc_hybrid_compile_opts_t;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

/**
 * @brief Initialise contexte hybride OpenCL→Level Zero
 * 
 * @param cache_dir Répertoire cache (NULL = ./cache_gen9_isa)
 * @param forensic_log Fichier log forensique (NULL = stdout)
 * @return Contexte hybride (NULL si erreur)
 */
btc_hybrid_ctx_t* btc_hybrid_init(
    const char* cache_dir,
    FILE* forensic_log
);

/**
 * @brief Compile kernel OpenCL C → Gen9 ISA → Level Zero
 * 
 * ÉTAPES :
 * 1. Calculer hash SHA-256 du source
 * 2. Vérifier cache (si activé)
 * 3. Compiler via OpenCL (si cache miss)
 * 4. Extraire binaire Gen9 ISA
 * 5. Charger dans Level Zero
 * 6. Sauvegarder cache (si activé)
 * 7. Logger forensique (si activé)
 * 
 * @param ctx Contexte hybride
 * @param source Code OpenCL C
 * @param kernel_name Nom du kernel
 * @param options Options compilation (NULL = défaut)
 * @return Kernel compilé (NULL si erreur)
 */
btc_hybrid_kernel_t* btc_hybrid_compile_kernel(
    btc_hybrid_ctx_t* ctx,
    const char* source,
    const char* kernel_name,
    const btc_hybrid_compile_opts_t* options
);

/**
 * @brief Alloue buffer GPU device
 * 
 * @param ctx Contexte hybride
 * @param size Taille en bytes
 * @return Buffer GPU (NULL si erreur)
 */
btc_hybrid_buffer_t* btc_hybrid_alloc_device(
    btc_hybrid_ctx_t* ctx,
    size_t size
);

/**
 * @brief Alloue buffer GPU shared (UMA)
 * 
 * @param ctx Contexte hybride
 * @param size Taille en bytes
 * @return Buffer GPU (NULL si erreur)
 */
btc_hybrid_buffer_t* btc_hybrid_alloc_shared(
    btc_hybrid_ctx_t* ctx,
    size_t size
);

/**
 * @brief Upload données vers GPU
 * 
 * @param ctx Contexte hybride
 * @param buffer Buffer GPU
 * @param host_data Données host
 * @param size Taille
 * @return true si succès
 */
bool btc_hybrid_upload(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_buffer_t* buffer,
    const void* host_data,
    size_t size
);

/**
 * @brief Download données depuis GPU
 * 
 * @param ctx Contexte hybride
 * @param buffer Buffer GPU
 * @param host_data Buffer host
 * @param size Taille
 * @return true si succès
 */
bool btc_hybrid_download(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_buffer_t* buffer,
    void* host_data,
    size_t size
);

/**
 * @brief Configure argument kernel
 * 
 * @param ctx Contexte hybride
 * @param kernel Kernel compilé
 * @param index Index argument
 * @param size Taille argument
 * @param value Pointeur valeur
 * @return true si succès
 */
bool btc_hybrid_set_kernel_arg(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_kernel_t* kernel,
    uint32_t index,
    size_t size,
    const void* value
);

/**
 * @brief Configure taille groupe de travail
 * 
 * @param ctx Contexte hybride
 * @param kernel Kernel compilé
 * @param group_size_x Threads par groupe (X)
 * @param group_size_y Threads par groupe (Y)
 * @param group_size_z Threads par groupe (Z)
 * @return true si succès
 */
bool btc_hybrid_set_group_size(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_kernel_t* kernel,
    uint32_t group_size_x,
    uint32_t group_size_y,
    uint32_t group_size_z
);

/**
 * @brief Exécute kernel sur GPU
 * 
 * @param ctx Contexte hybride
 * @param kernel Kernel compilé
 * @param global_x Threads globaux (X)
 * @param global_y Threads globaux (Y)
 * @param global_z Threads globaux (Z)
 * @return true si succès
 */
bool btc_hybrid_execute(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_kernel_t* kernel,
    uint32_t global_x,
    uint32_t global_y,
    uint32_t global_z
);

/**
 * @brief Warmup DMA GPU (résout upload 338× plus lent cold start)
 * 
 * @param ctx Contexte hybride
 * @return true si succès
 */
bool btc_hybrid_warmup_dma(btc_hybrid_ctx_t* ctx);

/**
 * @brief Libère buffer GPU
 * 
 * @param ctx Contexte hybride
 * @param buffer Buffer à libérer
 */
void btc_hybrid_free_buffer(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_buffer_t* buffer
);

/**
 * @brief Libère kernel compilé
 * 
 * @param ctx Contexte hybride
 * @param kernel Kernel à libérer
 */
void btc_hybrid_free_kernel(
    btc_hybrid_ctx_t* ctx,
    btc_hybrid_kernel_t* kernel
);

/**
 * @brief Libère contexte hybride
 * 
 * @param ctx Contexte à libérer
 */
void btc_hybrid_destroy(btc_hybrid_ctx_t* ctx);

/**
 * @brief Obtient statistiques cache
 * 
 * @param ctx Contexte hybride
 * @param hits [OUT] Cache hits
 * @param misses [OUT] Cache misses
 * @param total [OUT] Total compilations
 */
void btc_hybrid_get_cache_stats(
    btc_hybrid_ctx_t* ctx,
    uint64_t* hits,
    uint64_t* misses,
    uint64_t* total
);

#ifdef __cplusplus
}
#endif

#endif /* BTC_HYBRID_OPENCL_LEVELZERO_H */

// Made with Bob
