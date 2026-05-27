/**
 * @file btc_gen9_native_compiler.h
 * @brief Module natif LumVorax pour compilation Gen9 ISA
 * 
 * OBJECTIF : Contourner limitations Level Zero Gen9 en créant un compilateur
 *            offline OpenCL C → Gen9 ISA pur, avec cache et traçabilité forensique.
 * 
 * ARCHITECTURE :
 *   OpenCL C Source
 *        ↓
 *   OpenCL Runtime (clBuildProgram)
 *        ↓
 *   ELF + SPIR-V + Gen9 ISA (5944 bytes)
 *        ↓
 *   Extracteur ISA Natif LumVorax
 *        ↓
 *   Gen9 ISA Pur (sans wrapper)
 *        ↓
 *   Level Zero zeModuleCreate (ZE_MODULE_FORMAT_NATIVE)
 *        ↓
 *   Exécution GPU
 * 
 * FONCTIONNALITÉS :
 * - Compilation offline OpenCL C → Gen9 ISA
 * - Extraction ISA pur depuis ELF+SPIR-V
 * - Cache kernels compilés (éviter recompilation)
 * - Traçabilité forensique nanoseconde
 * - Validation CRC32 binaires
 * - Support multi-kernels
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10B)
 * DATE : 2026-05-09
 */

#ifndef BTC_GEN9_NATIVE_COMPILER_H
#define BTC_GEN9_NATIVE_COMPILER_H

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
 * @brief Binaire Gen9 ISA natif extrait
 */
typedef struct {
    uint8_t* data;              /**< Données ISA Gen9 pures */
    size_t size;                /**< Taille en bytes */
    uint32_t crc32;             /**< Checksum CRC32 */
    char kernel_name[256];      /**< Nom du kernel */
    uint64_t compile_time_ns;   /**< Temps compilation (ns) */
    bool cached;                /**< Chargé depuis cache ? */
} btc_gen9_isa_binary_t;

/**
 * @brief Contexte compilateur Gen9 natif
 */
typedef struct {
    void* cl_context;           /**< Contexte OpenCL */
    void* cl_device;            /**< Device OpenCL */
    void* cl_queue;             /**< Queue OpenCL */
    char cache_dir[512];        /**< Répertoire cache */
    bool enable_cache;          /**< Activer cache ? */
    bool enable_forensic;       /**< Activer traçabilité ? */
    FILE* forensic_log;         /**< Fichier log forensique */
} btc_gen9_compiler_ctx_t;

/**
 * @brief Options compilation
 */
typedef struct {
    const char* build_options;  /**< Options clBuildProgram */
    bool optimize;              /**< Optimisations (-O3) */
    bool debug;                 /**< Symboles debug (-g) */
    bool warnings;              /**< Afficher warnings */
} btc_gen9_compile_options_t;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

/**
 * @brief Initialise le compilateur Gen9 natif
 * 
 * @param cache_dir Répertoire cache (NULL = ~/.lumvorax/gen9_cache)
 * @param enable_cache Activer cache kernels compilés
 * @param enable_forensic Activer traçabilité forensique
 * @return Contexte compilateur (NULL si erreur)
 */
btc_gen9_compiler_ctx_t* btc_gen9_compiler_init(
    const char* cache_dir,
    bool enable_cache,
    bool enable_forensic
);

/**
 * @brief Compile OpenCL C source → Gen9 ISA natif
 * 
 * ÉTAPES :
 * 1. Vérifier cache (si activé)
 * 2. Compiler via OpenCL runtime (clBuildProgram)
 * 3. Extraire binaire ELF+SPIR-V (clGetProgramInfo)
 * 4. Extraire ISA Gen9 pur (suppression wrapper)
 * 5. Calculer CRC32
 * 6. Sauvegarder dans cache (si activé)
 * 7. Logger forensique (si activé)
 * 
 * @param ctx Contexte compilateur
 * @param source Code OpenCL C
 * @param kernel_name Nom du kernel
 * @param options Options compilation (NULL = défaut)
 * @return Binaire Gen9 ISA natif (NULL si erreur)
 */
btc_gen9_isa_binary_t* btc_gen9_compile_kernel(
    btc_gen9_compiler_ctx_t* ctx,
    const char* source,
    const char* kernel_name,
    const btc_gen9_compile_options_t* options
);

/**
 * @brief Compile depuis fichier OpenCL C
 * 
 * @param ctx Contexte compilateur
 * @param source_file Chemin fichier .cl
 * @param kernel_name Nom du kernel
 * @param options Options compilation (NULL = défaut)
 * @return Binaire Gen9 ISA natif (NULL si erreur)
 */
btc_gen9_isa_binary_t* btc_gen9_compile_file(
    btc_gen9_compiler_ctx_t* ctx,
    const char* source_file,
    const char* kernel_name,
    const btc_gen9_compile_options_t* options
);

/**
 * @brief Extrait ISA Gen9 pur depuis binaire ELF+SPIR-V
 * 
 * ALGORITHME :
 * 1. Parser header ELF64
 * 2. Localiser section .text.intel_gen9
 * 3. Extraire données ISA pures
 * 4. Valider magic bytes Gen9
 * 5. Calculer CRC32
 * 
 * @param elf_binary Binaire ELF+SPIR-V complet
 * @param elf_size Taille binaire ELF
 * @param isa_size [OUT] Taille ISA extrait
 * @return Pointeur ISA Gen9 pur (NULL si erreur)
 */
uint8_t* btc_gen9_extract_pure_isa(
    const uint8_t* elf_binary,
    size_t elf_size,
    size_t* isa_size
);

/**
 * @brief Charge binaire depuis cache
 * 
 * @param ctx Contexte compilateur
 * @param kernel_name Nom du kernel
 * @param source_hash Hash SHA-256 du source (pour validation)
 * @return Binaire Gen9 ISA natif (NULL si non trouvé)
 */
btc_gen9_isa_binary_t* btc_gen9_load_from_cache(
    btc_gen9_compiler_ctx_t* ctx,
    const char* kernel_name,
    const uint8_t source_hash[32]
);

/**
 * @brief Sauvegarde binaire dans cache
 * 
 * @param ctx Contexte compilateur
 * @param binary Binaire Gen9 ISA natif
 * @param source_hash Hash SHA-256 du source
 * @return true si succès
 */
bool btc_gen9_save_to_cache(
    btc_gen9_compiler_ctx_t* ctx,
    const btc_gen9_isa_binary_t* binary,
    const uint8_t source_hash[32]
);

/**
 * @brief Libère binaire Gen9 ISA
 * 
 * @param binary Binaire à libérer
 */
void btc_gen9_free_binary(btc_gen9_isa_binary_t* binary);

/**
 * @brief Libère contexte compilateur
 * 
 * @param ctx Contexte à libérer
 */
void btc_gen9_compiler_cleanup(btc_gen9_compiler_ctx_t* ctx);

/**
 * @brief Obtient log de compilation
 * 
 * @param ctx Contexte compilateur
 * @return Log de compilation (NULL si aucun)
 */
const char* btc_gen9_get_build_log(btc_gen9_compiler_ctx_t* ctx);

/**
 * @brief Valide binaire Gen9 ISA
 * 
 * VÉRIFICATIONS :
 * - Magic bytes Gen9
 * - Taille minimale (>= 64 bytes)
 * - CRC32 valide
 * - Alignement 64 bytes
 * 
 * @param binary Binaire à valider
 * @return true si valide
 */
bool btc_gen9_validate_binary(const btc_gen9_isa_binary_t* binary);

/**
 * @brief Désassemble binaire Gen9 ISA (debug)
 * 
 * @param binary Binaire à désassembler
 * @param output_file Fichier sortie (NULL = stdout)
 * @return true si succès
 */
bool btc_gen9_disassemble(
    const btc_gen9_isa_binary_t* binary,
    const char* output_file
);

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

/**
 * @brief Calcule hash SHA-256 du source OpenCL C
 * 
 * @param source Code source
 * @param hash [OUT] Hash SHA-256 (32 bytes)
 */
void btc_gen9_hash_source(const char* source, uint8_t hash[32]);

/**
 * @brief Calcule CRC32 d'un buffer
 * 
 * @param data Données
 * @param size Taille
 * @return CRC32
 */
uint32_t btc_gen9_crc32(const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* BTC_GEN9_NATIVE_COMPILER_H */

// Made with Bob
