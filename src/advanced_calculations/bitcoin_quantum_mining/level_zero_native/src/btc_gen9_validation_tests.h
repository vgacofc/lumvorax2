/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_validation_tests.h — Tests de Validation Préliminaires
 *
 * CYCLE C369 — Intégration Tests Automatiques au Démarrage
 * 
 * OBJECTIF: Valider TOUTES les couches avant minage
 * - Test écriture GPU (pattern simple)
 * - Test output buffer accessible
 * - Test GPGPU_WALKER dispatch
 * - Test ISA Gen9 valide
 *
 * PRINCIPE: Exécuter AVANT chaque session de minage
 * ÉCHEC: Arrêter immédiatement avec diagnostic précis
 */

#ifndef BTC_GEN9_VALIDATION_TESTS_H
#define BTC_GEN9_VALIDATION_TESTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Inclure le header principal pour avoir la définition de btc_gen9_context_t */
#include "../include/btc_gen9_native_runner.h"

/**
 * C369 TEST 1: Validation écriture GPU
 * Kernel simple qui écrit pattern 0xCAFE0000 + gid
 * Vérifie que GPU peut écrire dans output buffer
 */
int btc_gen9_test_gpu_write(btc_gen9_context_t* ctx);

/**
 * C369 TEST 2: Validation output buffer
 * Vérifie que output buffer est accessible en lecture/écriture
 * Teste mapping CPU → GPU → CPU
 */
int btc_gen9_test_output_buffer(btc_gen9_context_t* ctx);

/**
 * C369 TEST 3: Validation GPGPU_WALKER
 * Vérifie que threads GPU sont correctement dispatchés
 * Teste que tous les work-items s'exécutent
 */
int btc_gen9_test_gpgpu_walker(btc_gen9_context_t* ctx);

/**
 * C369 TEST 4: Validation ISA Gen9
 * Compare ISA généré avec ISA attendu
 * Vérifie que kernel est correctement compilé
 */
int btc_gen9_test_isa_validation(btc_gen9_context_t* ctx);

/**
 * C369 SUITE COMPLÈTE: Exécute tous les tests
 * Retourne 0 si tous les tests passent, -1 sinon
 * Log détaillé de chaque test
 */
int btc_gen9_run_validation_suite(btc_gen9_context_t* ctx);

#endif /* BTC_GEN9_VALIDATION_TESTS_H */

// Made with Bob
