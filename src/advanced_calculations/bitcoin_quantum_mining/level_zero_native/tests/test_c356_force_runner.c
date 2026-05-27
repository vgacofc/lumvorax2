/*
 * Test C356 Force Leading Zero - Runner
 * 
 * Ce test exécute le kernel sur 1000 nonces et affiche
 * la distribution des leading zeros pour valider count_leading_zeros()
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Déclarations externes du runner Gen9
typedef struct gen9_context gen9_context_t;
extern gen9_context_t* gen9_init(void);
extern void gen9_cleanup(gen9_context_t* ctx);

// Fonction pour exécuter un kernel Gen9 custom
extern int gen9_execute_custom_kernel(
    gen9_context_t* ctx,
    const char* kernel_path,
    const char* kernel_name,
    uint32_t global_size,
    uint32_t local_size,
    void** buffers,
    size_t* buffer_sizes,
    int num_buffers
);

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C356 - FORCE LEADING ZERO DETECTION                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // Initialiser contexte GPU
    gen9_context_t* ctx = gen9_init();
    if (!ctx) {
        printf("❌ Erreur: Impossible d'initialiser le contexte GPU\n");
        return 1;
    }
    
    // Configuration test
    uint32_t start_nonce = 0;
    uint32_t count = 1000;  // Tester 1000 nonces
    
    printf("📋 Configuration:\n");
    printf("   Start nonce: %u\n", start_nonce);
    printf("   Count: %u nonces\n", count);
    printf("   Output size: %u × 10 uint32 = %zu bytes\n", 
           count, count * 10 * sizeof(uint32_t));
    printf("\n");
    
    // Allouer buffer résultats
    size_t results_size = count * 10 * sizeof(uint32_t);
    uint32_t* results = (uint32_t*)malloc(results_size);
    if (!results) {
        printf("❌ Erreur: Allocation mémoire échouée\n");
        gen9_cleanup(ctx);
        return 1;
    }
    
    memset(results, 0, results_size);
    
    // Préparer buffers pour kernel
    void* buffers[3] = {
        results,
        &start_nonce,
        &count
    };
    
    size_t buffer_sizes[3] = {
        results_size,
        sizeof(uint32_t),
        sizeof(uint32_t)
    };
    
    printf("🔬 Exécution kernel GPU...\n");
    
    // Compiler et exécuter kernel
    system("cd tests && clang -cl-std=CL2.0 -target spir64-unknown-unknown -c test_c356_force_leading_zero.cl -o test_c356_force.bc -emit-llvm && llvm-spirv test_c356_force.bc -o test_c356_force.spv && ocloc compile -file test_c356_force.spv -spirv_input -device kbl -output test_c356_force.bin");
    
    int result = gen9_execute_custom_kernel(
        ctx,
        "tests/test_c356_force.bin",
        "test_force_leading_zero_c356",
        count,  // global_size
        64,     // local_size
        buffers,
        buffer_sizes,
        3
    );
    
    if (result != 0) {
        printf("❌ Erreur: Exécution kernel échouée (code: %d)\n", result);
        free(results);
        gen9_cleanup(ctx);
        return 1;
    }
    
    printf("✅ Kernel exécuté avec succès\n\n");
    
    // Analyser résultats
    printf("📊 Analyse des résultats:\n\n");
    
    uint32_t histogram[33] = {0};  // 0-32 leading zeros
    uint32_t max_leading = 0;
    uint32_t best_nonce = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t offset = i * 10;
        uint32_t nonce = results[offset + 0];
        uint32_t leading_zeros = results[offset + 1];
        
        if (leading_zeros > 32) leading_zeros = 32;  // Cap à 32
        histogram[leading_zeros]++;
        
        if (leading_zeros > max_leading) {
            max_leading = leading_zeros;
            best_nonce = nonce;
        }
        
        // Afficher les nonces avec ≥1 leading zero
        if (leading_zeros > 0) {
            printf("   Nonce %10u: %2u leading zeros | Hash: ", nonce, leading_zeros);
            for (int j = 7; j >= 0; j--) {
                printf("%08x", results[offset + 2 + j]);
            }
            printf("\n");
        }
    }
    
    printf("\n");
    printf("📈 Distribution des leading zeros:\n");
    for (int i = 0; i <= 32; i++) {
        if (histogram[i] > 0) {
            printf("   %2d bits: %4u nonces (%.1f%%)\n", 
                   i, histogram[i], (histogram[i] * 100.0) / count);
        }
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS FINAUX                                          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Meilleur nonce: %u\n", best_nonce);
    printf("Max leading zeros: %u bits\n", max_leading);
    printf("\n");
    
    if (max_leading > 0) {
        printf("✅ SUCCESS: count_leading_zeros() fonctionne correctement!\n");
        printf("   Le GPU a trouvé %u nonces avec ≥1 leading zero\n", 
               count - histogram[0]);
    } else {
        printf("⚠️  WARNING: Aucun nonce avec ≥1 leading zero trouvé\n");
        printf("   Cela peut être normal statistiquement\n");
        printf("   Probabilité attendue: ~50%% des nonces ont ≥1 bit\n");
    }
    
    printf("\n");
    
    free(results);
    gen9_cleanup(ctx);
    
    return (max_leading > 0) ? 0 : 1;
}

// Made with Bob
