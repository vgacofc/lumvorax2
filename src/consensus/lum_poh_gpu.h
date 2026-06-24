/**
 * @file lum_poh_gpu.h
 * @brief LumVorax PoH GPU Accelerator - OpenCL SHA-256 Verification
 * 
 * Module d'accélération GPU pour vérification parallèle des ticks PoH.
 * Utilise OpenCL pour vérifier 1000+ hashes simultanément sur GPU.
 * 
 * Architecture :
 * - Détection automatique GPU (Intel UHD Graphics 620)
 * - Kernel OpenCL optimisé SHA-256
 * - Batch processing 1000 hashes/batch
 * - Fallback CPU si GPU indisponible
 * 
 * Performance cible :
 * - GPU : 10K-50K vérifications/sec
 * - CPU : 1K-5K vérifications/sec
 * 
 * @version 1.0.0
 * @date 2026-06-11
 * @author Bob (LumVorax Core Team)
 */

#ifndef LUM_POH_GPU_H
#define LUM_POH_GPU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define LUM_POH_GPU_BATCH_SIZE 5000        // 5000 hashes par batch MAXIMAL (optimisé pour 65K+ TPS)
#define LUM_POH_GPU_MAX_DEVICES 8          // Max 8 GPUs
#define LUM_POH_GPU_HASH_SIZE 32           // SHA-256 = 32 bytes
#define LUM_POH_GPU_WORKGROUP_SIZE 256     // Taille workgroup OpenCL

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

/**
 * @brief Information GPU détecté
 */
typedef struct {
    cl_device_id device_id;                // ID device OpenCL
    char name[256];                        // Nom GPU
    char vendor[256];                      // Vendeur (Intel, NVIDIA, AMD)
    uint64_t global_mem_size;              // Mémoire globale (bytes)
    uint32_t compute_units;                // Nombre compute units
    uint32_t max_work_group_size;          // Taille max workgroup
    uint64_t max_clock_frequency;          // Fréquence max (MHz)
    bool supports_sha256;                  // Support SHA-256 natif ?
} lum_poh_gpu_device_info_t;

/**
 * @brief Batch de hashes à vérifier sur GPU
 */
typedef struct {
    uint8_t prev_hashes[LUM_POH_GPU_BATCH_SIZE][LUM_POH_GPU_HASH_SIZE];  // Hashes précédents
    uint8_t curr_hashes[LUM_POH_GPU_BATCH_SIZE][LUM_POH_GPU_HASH_SIZE];  // Hashes courants
    uint8_t results[LUM_POH_GPU_BATCH_SIZE];                              // Résultats (0=invalide, 1=valide)
    uint32_t count;                                                       // Nombre hashes dans batch
    uint64_t batch_id;                                                    // ID batch (forensic)
    uint64_t timestamp_submit_ns;                                         // Timestamp soumission
    uint64_t timestamp_complete_ns;                                       // Timestamp complétion
} lum_poh_gpu_batch_t;

/**
 * @brief Contexte GPU OpenCL
 */
typedef struct {
    // OpenCL objects
    cl_platform_id platform;               // Platform OpenCL
    cl_device_id device;                   // Device GPU sélectionné
    cl_context context;                    // Contexte OpenCL
    cl_command_queue queue;                // Queue commandes
    cl_program program;                    // Programme compilé
    cl_kernel kernel;                      // Kernel SHA-256
    
    // Buffers GPU
    cl_mem buffer_prev_hashes;             // Buffer hashes précédents
    cl_mem buffer_curr_hashes;             // Buffer hashes courants
    cl_mem buffer_results;                 // Buffer résultats
    
    // Configuration
    lum_poh_gpu_device_info_t device_info; // Info GPU
    bool initialized;                      // GPU initialisé ?
    bool fallback_cpu;                     // Fallback CPU si GPU fail ?
    
    // Métriques forensiques
    uint64_t total_batches_processed;      // Total batches traités
    uint64_t total_hashes_verified;        // Total hashes vérifiés
    uint64_t total_errors_detected;        // Total erreurs détectées
    double avg_batch_time_ms;              // Temps moyen batch (ms)
    uint64_t gpu_utilization_percent;      // Utilisation GPU (%)
    
    // Performance
    uint64_t verifications_per_second;     // Vérifications/sec
    double speedup_vs_cpu;                 // Accélération vs CPU
} lum_poh_gpu_context_t;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

/**
 * @brief Détecter GPUs disponibles
 * 
 * @param devices Buffer pour stocker infos devices
 * @param max_devices Taille max buffer
 * @return Nombre de GPUs détectés, -1 si erreur
 */
int lum_poh_gpu_detect_devices(lum_poh_gpu_device_info_t* devices, uint32_t max_devices);

/**
 * @brief Initialiser contexte GPU OpenCL
 * 
 * @param device_index Index GPU à utiliser (0 = premier GPU)
 * @param fallback_cpu Utiliser CPU si GPU fail ?
 * @return Contexte GPU ou NULL si erreur
 */
lum_poh_gpu_context_t* lum_poh_gpu_init(uint32_t device_index, bool fallback_cpu);

/**
 * @brief Libérer contexte GPU
 * 
 * @param ctx Contexte GPU
 */
void lum_poh_gpu_free(lum_poh_gpu_context_t* ctx);

/**
 * @brief Vérifier batch de hashes sur GPU
 * 
 * Vérifie que curr_hashes[i] == SHA256(prev_hashes[i]) pour tous i.
 * 
 * @param ctx Contexte GPU
 * @param batch Batch à vérifier (prev_hashes, curr_hashes, count)
 * @return true si succès, false si erreur
 */
bool lum_poh_gpu_verify_batch(lum_poh_gpu_context_t* ctx, lum_poh_gpu_batch_t* batch);

/**
 * @brief Vérifier batch sur CPU (fallback)
 * 
 * @param batch Batch à vérifier
 * @return true si succès, false si erreur
 */
bool lum_poh_gpu_verify_batch_cpu(lum_poh_gpu_batch_t* batch);

/**
 * @brief Obtenir métriques GPU
 * 
 * @param ctx Contexte GPU
 * @param batches_processed Pointeur pour total batches
 * @param hashes_verified Pointeur pour total hashes
 * @param errors_detected Pointeur pour total erreurs
 * @param avg_batch_time_ms Pointeur pour temps moyen batch
 * @param verifications_per_sec Pointeur pour vérifications/sec
 */
void lum_poh_gpu_get_metrics(lum_poh_gpu_context_t* ctx,
                              uint64_t* batches_processed,
                              uint64_t* hashes_verified,
                              uint64_t* errors_detected,
                              double* avg_batch_time_ms,
                              uint64_t* verifications_per_sec);

/**
 * @brief Afficher info GPU
 * 
 * @param info Info GPU
 */
void lum_poh_gpu_print_device_info(const lum_poh_gpu_device_info_t* info);

/**
 * @brief Générer rapport forensique GPU
 * 
 * @param ctx Contexte GPU
 * @param output_path Chemin fichier rapport
 * @return true si succès, false si erreur
 */
bool lum_poh_gpu_generate_report(lum_poh_gpu_context_t* ctx, const char* output_path);

#ifdef __cplusplus
}
#endif

#endif /* LUM_POH_GPU_H */

// Made with Bob - LumVorax GPU Acceleration Module