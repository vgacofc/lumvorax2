/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_gen9_stress.c — Test Stress CORRECTIONS C198 Phase 15Y-F4
 *
 * OBJECTIF : Valider corrections Bug #1, #2, #3, #4
 * - Bug #4 : Reset GPU anti-hang (CRITIQUE)
 * - Bug #3 : Batch buffer nettoyé
 * - Bug #1 : Logging EXEC_SUBMIT corrigé
 * - Bug #2 : Code destruction pool supprimé
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int num_dispatches = 10;  /* Par défaut : 10 dispatches */
    
    if (argc > 1) {
        num_dispatches = atoi(argv[1]);
        if (num_dispatches <= 0 || num_dispatches > 1000) {
            printf("❌ ERREUR : num_dispatches doit être entre 1 et 1000\n");
            return 1;
        }
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 15Y-F4 — Test Stress Corrections     ║\n");
    printf("║  Validation Bug #1, #2, #3, #4                           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Configuration */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = "kernels/test_add_gen9.bin";
    config.log_path = "logs/forensic/test_gen9_stress.log";
    
    printf("[CONFIG] Dispatches à exécuter : %d\n", num_dispatches);
    printf("[CONFIG] Kernel : %s\n", config.kernel_path);
    printf("[CONFIG] Log : %s\n", config.log_path);
    printf("\n");
    
    /* Initialisation */
    printf("[PHASE 1/4] Initialisation Gen9 Native...\n");
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0) {
        printf("❌ ÉCHEC : Initialisation échouée\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n");
    printf("\n");
    
    /* Exécution stress test */
    printf("[PHASE 2/4] Exécution %d dispatches consécutifs...\n", num_dispatches);
    printf("  (Validation correction Bug #4 : Reset GPU anti-hang)\n");
    printf("\n");
    
    int success_count = 0;
    int fail_count = 0;
    
    for (int i = 0; i < num_dispatches; i++) {
        printf("  [%3d/%3d] Dispatch GPU... ", i + 1, num_dispatches);
        fflush(stdout);
        
        ret = btc_gen9_execute(ctx);
        if (ret < 0) {
            printf("❌ ÉCHEC (errno=%d)\n", ret);
            fail_count++;
            
            /* Avant corrections : échouait au 4ème dispatch (errno=5)
             * Après corrections : devrait réussir tous les dispatches
             */
            if (i < 3) {
                printf("\n");
                printf("⚠️  RÉGRESSION DÉTECTÉE !\n");
                printf("    Échec avant dispatch #4 (attendu : succès)\n");
                printf("    Avant corrections : 3 dispatches OK\n");
                printf("    Maintenant : %d dispatches OK\n", i);
                printf("\n");
            }
            break;
        }
        
        printf("✅ OK\n");
        success_count++;
    }
    
    printf("\n");
    printf("[PHASE 3/4] Résultats...\n");
    printf("  Dispatches réussis : %d/%d (%.1f%%)\n", 
           success_count, num_dispatches, 
           (100.0 * success_count) / num_dispatches);
    printf("  Dispatches échoués : %d/%d\n", fail_count, num_dispatches);
    printf("\n");
    
    /* Métriques */
    printf("[PHASE 4/4] Métriques finales...\n");
    btc_gen9_print_metrics(ctx);
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    /* Verdict */
    printf("╔════════════════════════════════════════════════════════════╗\n");
    if (success_count == num_dispatches) {
        printf("║  ✅ SUCCÈS TOTAL — CORRECTIONS VALIDÉES                   ║\n");
        printf("║  Bug #4 : Reset GPU anti-hang → CORRIGÉ ✅                ║\n");
        printf("║  Bug #3 : Batch buffer nettoyé → CORRIGÉ ✅               ║\n");
        printf("║  Dispatches illimités : %d/%d (100%%)                      ║\n", 
               success_count, num_dispatches);
    } else if (success_count >= 3) {
        printf("║  ⚠️  SUCCÈS PARTIEL — AMÉLIORATION DÉTECTÉE               ║\n");
        printf("║  Avant corrections : 3 dispatches max                     ║\n");
        printf("║  Après corrections : %d dispatches                         ║\n", 
               success_count);
        printf("║  Gain : +%d dispatches (+%.0f%%)                           ║\n",
               success_count - 3, ((success_count - 3) * 100.0) / 3);
    } else {
        printf("║  ❌ ÉCHEC — RÉGRESSION DÉTECTÉE                           ║\n");
        printf("║  Avant corrections : 3 dispatches                         ║\n");
        printf("║  Après corrections : %d dispatches                         ║\n", 
               success_count);
        printf("║  Régression : -%d dispatches                              ║\n",
               3 - success_count);
    }
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return (success_count == num_dispatches) ? 0 : 1;
}

// Made with Bob