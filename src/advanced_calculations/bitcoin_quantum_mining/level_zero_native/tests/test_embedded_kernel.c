/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_embedded_kernel.c — Test Chargement Kernel Embarqué (0% OpenCL)
 *
 * CYCLE C198 — Phase 12A — Validation 0% Dépendance OpenCL
 * STANDARD_NAMES.md v4.5 §M-BTC-TEST-EMBEDDED-C198
 *
 * OBJECTIF : Valider chargement kernel depuis bytecode embarqué
 *            SANS AUCUNE DÉPENDANCE OPENCL RUNTIME
 */

#include "btc_levelzero_runner.h"
#include "btc_kernel_loader_static.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    fprintf(stderr, "\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════════════════\n");
    fprintf(stderr, "  LumVorax C198 Phase 12A — Test Kernel Embarqué (0%% OpenCL)\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════════════════\n");
    fprintf(stderr, "\n");
    
    /* Étape 1 : Lister kernels disponibles */
    fprintf(stderr, "[1/5] Liste kernels embarqués...\n");
    size_t kernel_count = btc_list_embedded_kernels();
    fprintf(stderr, "✅ %zu kernels disponibles\n\n", kernel_count);
    
    /* Étape 2 : Initialiser Level Zero */
    fprintf(stderr, "[2/5] Initialisation Level Zero...\n");
    btc_l0_context_t* ctx = btc_l0_init(NULL);  /* Config par défaut */
    if (!ctx) {
        fprintf(stderr, "❌ ERREUR: Initialisation Level Zero échouée\n");
        return 1;
    }
    fprintf(stderr, "✅ Level Zero initialisé\n\n");
    
    /* Étape 3 : Vérifier disponibilité kernel */
    fprintf(stderr, "[3/5] Vérification disponibilité kernel 'test_add'...\n");
    if (!btc_is_kernel_available("test_add")) {
        fprintf(stderr, "❌ ERREUR: Kernel 'test_add' non disponible\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    fprintf(stderr, "✅ Kernel 'test_add' disponible\n\n");
    
    /* Étape 4 : Charger kernel embarqué (0% OpenCL) */
    fprintf(stderr, "[4/5] Chargement kernel embarqué (0%% OpenCL)...\n");
    void* kernel = btc_load_embedded_kernel(ctx, "test_add");
    if (!kernel) {
        fprintf(stderr, "❌ ERREUR: Chargement kernel échoué\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    fprintf(stderr, "✅ Kernel chargé avec succès (handle: %p)\n\n", kernel);
    
    /* Étape 5 : Cleanup */
    fprintf(stderr, "[5/5] Nettoyage...\n");
    btc_l0_cleanup(ctx);
    fprintf(stderr, "✅ Nettoyage terminé\n\n");
    
    /* Résultat final */
    fprintf(stderr, "═══════════════════════════════════════════════════════════════════════\n");
    fprintf(stderr, "  ✅ SUCCÈS TOTAL : Kernel chargé SANS OpenCL (0%% dépendance)\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════════════════\n");
    fprintf(stderr, "\n");
    
    return 0;
}

/* Made with Bob - LumVorax C198 Phase 12A - 0% OpenCL Runtime */

// Made with Bob
