#include <stdio.h>
#include <stdlib.h>
#include "src/debug/memory_tracker.h"
#include "src/lum/lum_core.h"
#include "src/vorax/vorax_operations.h"
#include "src/parser/vorax_parser.h"
#include "src/logger/lum_logger.h"

int main() {
    printf("=== Testing parser memory corruption issue ===\n");
    
    // Initialize memory tracking
    memory_tracker_init();
    lum_logger_init("test_parser.log", LUM_LOG_DEBUG);
    
    // Test the parser code that causes crashes
    const char* vorax_code =
        "zone A, B, C;\n"
        "mem buf;\n"
        "emit A += 3•;\n"
        "split A -> [B, C];\n"
        "move B -> C, 1•;\n";

    printf("Parsing VORAX code:\n%s\n", vorax_code);

    vorax_ast_node_t* ast = vorax_parse(vorax_code);
    if (ast) {
        printf("✓ Parsing successful, AST created\n");
        
        // Test execution context creation and destruction
        vorax_execution_context_t* ctx = vorax_execution_context_create();
        if (ctx) {
            printf("✓ Execution context created\n");
            
            bool exec_result = vorax_execute(ctx, ast);
            printf("✓ Execution: %s\n", exec_result ? "Success" : "Failed");
            printf("Zones created: %zu\n", ctx->zone_count);
            printf("Memories created: %zu\n", ctx->memory_count);

            printf("Destroying execution context...\n");
            vorax_execution_context_destroy(ctx);
            printf("✓ Execution context destroyed\n");
        }

        printf("Destroying AST...\n");
        vorax_ast_destroy(ast);
        printf("✓ AST destroyed\n");
    } else {
        printf("✗ Parsing failed\n");
    }
    
    // Check for memory leaks
    printf("=== Memory Check ===\n");
    memory_tracker_report();
    memory_tracker_check_leaks();
    
    printf("=== Test completed ===\n");
    return 0;
}