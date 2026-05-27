#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "src/lum/lum_core.h"
#include "src/vorax/vorax_operations.h"

bool test_conservation_fuse() {
    printf("=== TEST CONSERVATION FUSE ===\n");
    
    // Créer deux groupes avec nombre connu de LUMs
    lum_group_t* group1 = lum_group_create(10);
    lum_group_t* group2 = lum_group_create(15);
    
    // Ajouter LUMs au groupe 1
    for (int i = 0; i < 10; i++) {
        lum_t* lum = lum_create(i % 2, i, 0, LUM_STRUCTURE_LINEAR);
        lum_group_add(group1, lum);
        lum_destroy(lum);
    }
    
    // Ajouter LUMs au groupe 2
    for (int i = 0; i < 15; i++) {
        lum_t* lum = lum_create((i+1) % 2, i, 1, LUM_STRUCTURE_LINEAR);
        lum_group_add(group2, lum);
        lum_destroy(lum);
    }
    
    size_t before_count = group1->count + group2->count;
    printf("Avant FUSE: Groupe1=%zu, Groupe2=%zu, Total=%zu\n", 
           group1->count, group2->count, before_count);
    
    // Effectuer la fusion
    vorax_result_t* result = vorax_fuse(group1, group2);
    
    if (!result || !result->success) {
        printf("❌ ÉCHEC: Fusion a échoué\n");
        return false;
    }
    
    size_t after_count = result->result_group->count;
    printf("Après FUSE: %zu LUMs\n", after_count);
    
    bool conserved = (before_count == after_count);
    printf("%s Conservation: %zu = %zu\n", 
           conserved ? "✅" : "❌", before_count, after_count);
    
    lum_group_destroy(group1);
    lum_group_destroy(group2);
    vorax_result_destroy(result);
    
    return conserved;
}

bool test_conservation_split() {
    printf("\n=== TEST CONSERVATION SPLIT ===\n");
    
    const size_t original_count = 100;
    const size_t split_parts = 4;
    
    // Créer un groupe
    lum_group_t* group = lum_group_create(original_count);
    for (size_t i = 0; i < original_count; i++) {
        lum_t* lum = lum_create(i % 2, (int32_t)i, 0, LUM_STRUCTURE_LINEAR);
        lum_group_add(group, lum);
        lum_destroy(lum);
    }
    
    printf("Avant SPLIT: %zu LUMs en 1 groupe\n", group->count);
    
    // Effectuer la division
    vorax_result_t* result = vorax_split(group, split_parts);
    
    if (!result || !result->success) {
        printf("❌ ÉCHEC: Split a échoué\n");
        return false;
    }
    
    // Compter les LUMs dans tous les groupes résultants
    size_t total_after = 0;
    for (size_t i = 0; i < result->result_count; i++) {
        total_after += result->result_groups[i]->count;
        printf("  Groupe %zu: %zu LUMs\n", i+1, result->result_groups[i]->count);
    }
    
    printf("Après SPLIT: %zu LUMs en %zu groupes\n", total_after, result->result_count);
    
    bool conserved = (original_count == total_after);
    printf("%s Conservation: %zu = %zu\n", 
           conserved ? "✅" : "❌", original_count, total_after);
    
    lum_group_destroy(group);
    vorax_result_destroy(result);
    
    return conserved;
}

bool test_conservation_cycle() {
    printf("\n=== TEST CONSERVATION CYCLE ===\n");
    
    const size_t original_count = 17;
    const size_t modulo = 5;
    
    // Créer un groupe
    lum_group_t* group = lum_group_create(original_count);
    for (size_t i = 0; i < original_count; i++) {
        lum_t* lum = lum_create(i % 2, (int32_t)i, 0, LUM_STRUCTURE_LINEAR);
        lum_group_add(group, lum);
        lum_destroy(lum);
    }
    
    printf("Avant CYCLE: %zu LUMs, modulo %zu\n", group->count, modulo);
    
    // Effectuer le cycle
    vorax_result_t* result = vorax_cycle(group, modulo);
    
    if (!result || !result->success) {
        printf("❌ ÉCHEC: Cycle a échoué\n");
        return false;
    }
    
    size_t expected_count = original_count % modulo;
    if (expected_count == 0 && original_count > 0) {
        expected_count = modulo;
    }
    
    size_t actual_count = result->result_group->count;
    printf("Après CYCLE: %zu LUMs (attendu: %zu)\n", actual_count, expected_count);
    
    bool conserved = (expected_count == actual_count);
    printf("%s Conservation modulo: %zu = %zu\n", 
           conserved ? "✅" : "❌", expected_count, actual_count);
    
    lum_group_destroy(group);
    vorax_result_destroy(result);
    
    return conserved;
}

bool test_presence_invariant() {
    printf("\n=== TEST INVARIANT PRÉSENCE ===\n");
    
    // Test que la présence reste 0 ou 1
    const int test_values[] = {-5, 0, 1, 2, 42, 255, -1};
    const int num_tests = sizeof(test_values) / sizeof(test_values[0]);
    
    bool all_valid = true;
    
    for (int i = 0; i < num_tests; i++) {
        lum_t* lum = lum_create(test_values[i], 0, 0, LUM_STRUCTURE_LINEAR);
        
        bool valid = (lum->presence == 0 || lum->presence == 1);
        printf("Entrée: %d -> Présence: %u %s\n", 
               test_values[i], lum->presence, valid ? "✅" : "❌");
        
        if (!valid) all_valid = false;
        
        lum_destroy(lum);
    }
    
    return all_valid;
}

bool test_id_uniqueness() {
    printf("\n=== TEST UNICITÉ DES IDs ===\n");
    
    const int count = 1000;
    uint32_t* ids = malloc(count * sizeof(uint32_t));
    bool all_unique = true;
    
    // Créer plusieurs LUMs et collecter leurs IDs
    for (int i = 0; i < count; i++) {
        lum_t* lum = lum_create(i % 2, i, i, LUM_STRUCTURE_LINEAR);
        ids[i] = lum->id;
        lum_destroy(lum);
    }
    
    // Vérifier l'unicité
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (ids[i] == ids[j]) {
                printf("❌ ID dupliqué: %u (positions %d et %d)\n", ids[i], i, j);
                all_unique = false;
            }
        }
    }
    
    if (all_unique) {
        printf("✅ Tous les %d IDs sont uniques\n", count);
        printf("   Plage: %u à %u\n", ids[0], ids[count-1]);
    }
    
    free(ids);
    return all_unique;
}

int main() {
    printf("=== TESTS DE CONSERVATION MATHÉMATIQUE LUM/VORAX ===\n");
    printf("Validation des propriétés de conservation du système...\n\n");
    
    bool all_passed = true;
    
    all_passed &= test_conservation_fuse();
    all_passed &= test_conservation_split();
    all_passed &= test_conservation_cycle();
    all_passed &= test_presence_invariant();
    all_passed &= test_id_uniqueness();
    
    printf("\n=== RÉSUMÉ VALIDATION MATHÉMATIQUE ===\n");
    if (all_passed) {
        printf("✅ SUCCÈS: Toutes les propriétés de conservation sont respectées\n");
        printf("✅ Le système LUM/VORAX est mathématiquement cohérent\n");
        printf("✅ Les invariants sont préservés dans toutes les opérations\n");
    } else {
        printf("❌ ÉCHEC: Certaines propriétés de conservation ne sont pas respectées\n");
    }
    
    return all_passed ? 0 : 1;
}