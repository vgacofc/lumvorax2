/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_kernel_loader_static.h — Chargeur Bytecode Statique (0% OpenCL)
 *
 * CYCLE C198 — Phase 12A — Élimination Totale Dépendance OpenCL
 * STANDARD_NAMES.md v4.5 §M-BTC-KERNEL-LOADER-STATIC-C198
 *
 * OBJECTIF : Charger kernels Gen9 ISA depuis bytecode embarqué
 *            SANS AUCUNE DÉPENDANCE OPENCL RUNTIME
 */

#ifndef BTC_KERNEL_LOADER_STATIC_H
#define BTC_KERNEL_LOADER_STATIC_H

#include "btc_levelzero_runner.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Charge kernel depuis bytecode embarqué (0% OpenCL)
 * 
 * FONCTIONNEMENT :
 * 1. Recherche kernel dans catalogue embarqué
 * 2. Crée module Level Zero depuis bytecode
 * 3. Crée kernel depuis module
 * 4. Retourne handle kernel
 * 
 * AUCUNE DÉPENDANCE OPENCL RUNTIME
 * 
 * @param ctx Contexte Level Zero
 * @param kernel_name Nom du kernel ("test_add", "sha256_simple", etc.)
 * @return Handle kernel Level Zero (NULL si erreur)
 * 
 * @example
 * btc_l0_context_t* ctx = btc_l0_init();
 * void* kernel = btc_load_embedded_kernel(ctx, "test_add");
 * // Utiliser kernel...
 */
void* btc_load_embedded_kernel(btc_l0_context_t* ctx, const char* kernel_name);

/**
 * @brief Liste tous les kernels embarqués disponibles
 * 
 * @return Nombre de kernels disponibles
 */
size_t btc_list_embedded_kernels(void);

/**
 * @brief Vérifie si un kernel est disponible
 * 
 * @param kernel_name Nom du kernel
 * @return true si disponible
 */
bool btc_is_kernel_available(const char* kernel_name);

#ifdef __cplusplus
}
#endif

#endif /* BTC_KERNEL_LOADER_STATIC_H */

/* Made with Bob - LumVorax C198 Phase 12A - 0% OpenCL Runtime */

// Made with Bob
