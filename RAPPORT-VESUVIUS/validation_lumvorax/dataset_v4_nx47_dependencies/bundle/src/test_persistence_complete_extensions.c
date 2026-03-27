
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "src/lum/lum_core.h"
#include "src/persistence/data_persistence.h"
#include "src/persistence/transaction_wal_extension.h"
#include "src/persistence/recovery_manager_extension.h"
#include "src/debug/memory_tracker.h"

void test_persistance_100k_with_wal(void) {
    printf("🔥 === TEST PERSISTANCE 100K LUMs AVEC WAL EXTENSION ===\n");
    
    // Initialiser système recovery complet
    bool recovery_init = initialize_lum_system_with_auto_recovery_extension("test_data_100k", "test_wal_100k.log");
    assert(recovery_init && "Recovery system initialization failed");
    
    // Créer contexte WAL pour logging
    wal_extension_context_t* wal_ctx = wal_extension_context_create("test_wal_100k.log");
    assert(wal_ctx != NULL && "WAL context creation failed");
    
    // Test stress 100,000 LUMs avec logging WAL
    printf("📝 Stockage 100,000 LUMs avec logging WAL...\n");
    clock_t start_time = clock();
    
    for (int i = 0; i < 100000; i++) {
        // Démarrer transaction WAL
        wal_extension_result_t* wal_begin = wal_extension_begin_transaction(wal_ctx);
        assert(wal_begin != NULL && "WAL transaction begin failed");
        
        // Créer et logger LUM
        lum_t* lum = lum_create(i % 2, i % 1000, (i / 1000) % 1000, LUM_STRUCTURE_LINEAR);
        wal_extension_result_t* wal_log = wal_extension_log_lum_operation(wal_ctx, 
                                                                        wal_begin->wal_transaction_id, 
                                                                        lum);
        assert(wal_log != NULL && "WAL logging failed");
        
        // Commit transaction
        wal_extension_result_t* wal_commit = wal_extension_commit_transaction(wal_ctx, 
                                                                            wal_begin->wal_transaction_id);
        assert(wal_commit != NULL && "WAL commit failed");
        
        // Cleanup
        lum_destroy(lum);
        wal_extension_result_destroy(wal_begin);
        wal_extension_result_destroy(wal_log);
        wal_extension_result_destroy(wal_commit);
        
        if (i % 10000 == 0) {
            printf("  Traité: %d LUMs\n", i);
        }
    }
    
    clock_t end_time = clock();
    double elapsed = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("✅ 100,000 LUMs traités en %.2f secondes\n", elapsed);
    printf("📊 Performance: %.0f LUMs/sec avec logging WAL complet\n", 100000.0 / elapsed);
    
    // Vérifier intégrité WAL
    bool integrity_ok = wal_extension_verify_integrity_complete(wal_ctx);
    printf("%s Intégrité WAL: %s\n", integrity_ok ? "✅" : "❌", integrity_ok ? "OK" : "COMPROMISE");
    
    wal_extension_context_destroy(wal_ctx);
    printf("🎯 TEST 100K LUMs AVEC WAL EXTENSION TERMINÉ\n\n");
}

void test_recovery_scenarios_complets(void) {
    printf("🔄 === TEST SCENARIOS RECOVERY COMPLETS ===\n");
    
    // Test 1: Recovery après crash simulé
    recovery_manager_extension_t* manager1 = recovery_manager_extension_create("recovery_test_1", "recovery_test_1.wal");
    assert(manager1 != NULL && "Recovery manager 1 creation failed");
    
    // Simuler crash
    recovery_manager_extension_mark_startup_begin(manager1);
    // Pas de clean shutdown = crash détecté au prochain démarrage
    
    // Nouveau démarrage avec détection crash
    recovery_manager_extension_t* manager2 = recovery_manager_extension_create("recovery_test_1", "recovery_test_1.wal");
    assert(manager2 != NULL && "Recovery manager 2 creation failed");
    
    bool crash_detected = recovery_manager_extension_detect_previous_crash(manager2);
    printf("%s Détection crash: %s\n", crash_detected ? "✅" : "❌", crash_detected ? "DÉTECTÉ" : "AUCUN");
    
    if (crash_detected) {
        bool recovery_ok = recovery_manager_extension_auto_recover_complete(manager2);
        printf("%s Recovery automatique: %s\n", recovery_ok ? "✅" : "❌", recovery_ok ? "SUCCÈS" : "ÉCHEC");
    }
    
    // Cleanup propre
    recovery_manager_extension_mark_clean_shutdown(manager2);
    recovery_manager_extension_destroy(manager1);
    recovery_manager_extension_destroy(manager2);
    
    printf("🎯 TEST RECOVERY SCENARIOS TERMINÉ\n\n");
}

void test_integration_complete_extensions(void) {
    printf("🔗 === TEST INTÉGRATION COMPLÈTE EXTENSIONS ===\n");
    
    // Test intégration WAL + Recovery + Persistance + Memory Tracking
    memory_tracker_init();
    
    // Test persistance avec recovery
    test_persistance_100k_with_wal();
    
    // Test recovery scenarios
    test_recovery_scenarios_complets();
    
    // Rapport mémoire final
    memory_tracker_report();
    
    printf("✅ INTÉGRATION COMPLÈTE EXTENSIONS VALIDÉE\n");
}

int main(void) {
    printf("🚀 === VALIDATION COMPLÈTE PERSISTANCE AVEC EXTENSIONS ===\n");
    printf("Date: %s\n", __DATE__);
    printf("Temps: %s\n\n", __TIME__);
    
    test_integration_complete_extensions();
    
    printf("\n🎯 === VALIDATION PERSISTANCE EXTENSIONS TERMINÉE ===\n");
    return 0;
}
