/*
 * BTC NEO WRAPPER - Header
 * Phase 15Y-F26 : Projet A
 */

#ifndef BTC_NEO_WRAPPER_H
#define BTC_NEO_WRAPPER_H

#include <stdint.h>
#include <stddef.h>
#include <CL/cl.h>

// Structure opaque
typedef struct btc_neo_wrapper_t btc_neo_wrapper_t;

// Métriques wrapper
typedef struct {
    uint64_t dispatch_count;
    uint64_t total_time_ns;
    uint64_t bytes_transferred;
    uint64_t kernel_executions;
    uint64_t errors;
} btc_neo_metrics_t;

// API publique

/**
 * Initialiser le wrapper NEO
 * @param wrapper_out Pointeur vers le wrapper créé
 * @return 0 si succès, -1 si erreur
 */
int btc_neo_wrapper_init(btc_neo_wrapper_t **wrapper_out);

/**
 * Charger un kernel OpenCL
 * @param wrapper Le wrapper
 * @param kernel_source Code source du kernel
 * @param kernel_name Nom du kernel
 * @return 0 si succès, -1 si erreur
 */
int btc_neo_wrapper_load_kernel(btc_neo_wrapper_t *wrapper,
                                const char *kernel_source,
                                const char *kernel_name);

/**
 * Dispatcher un kernel
 * @param wrapper Le wrapper
 * @param global_work_size Taille globale de travail
 * @param local_work_size Taille locale de travail
 * @param buffers Buffers OpenCL
 * @param num_buffers Nombre de buffers
 * @return 0 si succès, -1 si erreur
 */
int btc_neo_wrapper_dispatch(btc_neo_wrapper_t *wrapper,
                             size_t global_work_size,
                             size_t local_work_size,
                             cl_mem *buffers,
                             size_t num_buffers);

/**
 * Nettoyer le wrapper
 * @param wrapper Le wrapper à nettoyer
 */
void btc_neo_wrapper_cleanup(btc_neo_wrapper_t *wrapper);

/**
 * Obtenir les métriques du wrapper
 * @param wrapper Le wrapper
 * @param metrics_out Pointeur vers structure métriques
 * @return 0 si succès, -1 si erreur
 */
int btc_neo_wrapper_get_metrics(btc_neo_wrapper_t *wrapper, btc_neo_metrics_t *metrics_out);

/**
 * Obtenir le contexte OpenCL
 * @param wrapper Le wrapper
 * @return Contexte OpenCL ou NULL
 */
cl_context btc_neo_wrapper_get_context(btc_neo_wrapper_t *wrapper);

/**
 * Obtenir la queue OpenCL
 * @param wrapper Le wrapper
 * @return Queue OpenCL ou NULL
 */
cl_command_queue btc_neo_wrapper_get_queue(btc_neo_wrapper_t *wrapper);

/**
 * Obtenir le kernel OpenCL
 * @param wrapper Le wrapper
 * @return Kernel OpenCL ou NULL
 */
cl_kernel btc_neo_wrapper_get_kernel(btc_neo_wrapper_t *wrapper);

/**
 * Créer ou réutiliser un buffer persistant (Optimisation C198 #1)
 * Réutilise automatiquement les buffers existants pour éviter allocation GPU coûteuse
 * Gain mesuré : 16× speedup (240ms → 15ms pour 192MB)
 * @param wrapper Le wrapper
 * @param size Taille du buffer en bytes
 * @param index Index du buffer (0-7)
 * @param flags Flags OpenCL (CL_MEM_READ_WRITE, etc.)
 * @return Buffer OpenCL ou NULL si erreur
 */
cl_mem btc_neo_wrapper_create_persistent_buffer(btc_neo_wrapper_t *wrapper,
                                                 size_t size,
                                                 int index,
                                                 cl_mem_flags flags);

#endif /* BTC_NEO_WRAPPER_H */

// Made with Bob
