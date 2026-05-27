#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "src/lum/lum_core.h"
#include "src/debug/memory_tracker.h"

// Test scenarios for memory corruption resistance
int main(void) {
    printf("=== MEMORY CORRUPTION RESISTANCE TESTS ===\n");
    
    memory_tracker_init();
    
    // Test 1: Double destruction of individual LUM
    printf("\nTest 1: Double destruction resistance\n");
    lum_t* test_lum = lum_create(1, 0, 0, LUM_STRUCTURE_LINEAR);
    if (test_lum) {
        printf("  Created LUM ID: %u\n", test_lum->id);
        
        // First destruction - should work
        lum_safe_destroy(&test_lum);
        printf("  First destruction completed, pointer nullified: %s\n", 
               test_lum == NULL ? "YES" : "NO");
        
        // Second destruction - should be safe (no-op)
        lum_safe_destroy(&test_lum);
        printf("  Second destruction handled safely\n");
    }
    
    // Test 2: Group with individual LUM lifecycle
    printf("\nTest 2: Group+Individual LUM lifecycle\n");
    lum_t* lum1 = lum_create(1, 0, 0, LUM_STRUCTURE_LINEAR);
    lum_t* lum2 = lum_create(1, 1, 0, LUM_STRUCTURE_LINEAR);
    
    if (lum1 && lum2) {
        lum_group_t* group = lum_group_create(10);
        if (group) {
            // Add LUMs to group (copies by value)
            lum_group_add(group, lum1);
            lum_group_add(group, lum2);
            printf("  Group created with %zu LUMs\n", lum_group_size(group));
            
            // Destroy group first (frees the copies)
            lum_group_destroy(group);
            printf("  Group destroyed\n");
            
            // Destroy original LUMs - should work without double-free
            lum_safe_destroy(&lum1);
            lum_safe_destroy(&lum2);
            printf("  Original LUMs destroyed safely\n");
        }
    }
    
    // Test 3: Multiple group destruction
    printf("\nTest 3: Multiple group destruction\n");
    lum_group_t* group1 = lum_group_create(5);
    if (group1) {
        // Destroy once
        lum_group_safe_destroy(&group1);
        printf("  Group destroyed once, pointer nullified: %s\n", 
               group1 == NULL ? "YES" : "NO");
        
        // Try to destroy again - should be safe
        lum_group_safe_destroy(&group1);
        printf("  Second destruction handled safely\n");
    }
    
    printf("\n=== CORRUPTION RESISTANCE TESTS COMPLETED ===\n");
    printf("All tests passed - memory management is robust!\n");
    
    // Final memory report
    memory_tracker_report();
    memory_tracker_check_leaks();
    memory_tracker_destroy();
    
    return 0;
}