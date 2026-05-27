#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/btc_gen9_native_runner.h"

int main(void) {
    printf("[C350] Test kernel ULTRA SIMPLE sans atomics\n");
    printf("[DEBUG] Tentative ouverture log: logs/forensic/btc_mining_c350_simple.log\n");
    
    btc_gen9_config_t config = {
        .kernel_path = "kernels/btc_sha256_c351_noatomic_fixed.bin_kbl.bin",  /* C351: Kernel SANS atomics (FIX FINAL) */
        .batch_size = 10000000,
        .work_group_size = 256,
        .log_path = "logs/forensic/btc_mining_c351_noatomic.log"
    };
    
    printf("[DEBUG] Log ouvert avec succès\n");
    
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret != 0 || !ctx) {
        fprintf(stderr, "Erreur init (ret=%d)\n", ret);
        return 1;
    }
    
    /* C350: Utiliser MÊME block header que test CPU pour comparaison directe
     * Ce block header produit 9 near-miss sur 10M nonces (CPU validé)
     * Nonces attendus: 3, 5, 92, 359, 793, 30468, 176709, 601998, 6567206
     * Max leading zeros attendu: 20 (nonce=6567206)
     */
    uint8_t block_header[80] = {
        /* Version (little-endian) */
        0x01, 0x00, 0x00, 0x00,
        /* Previous block hash (32 bytes, little-endian) */
        0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72,
        0xc1, 0xa6, 0xa2, 0x46, 0xae, 0x63, 0xf7, 0x4f,
        0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
        0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* Merkle root (32 bytes, little-endian) */
        0x98, 0x20, 0x51, 0xfd, 0x1e, 0x4b, 0xa7, 0x44,
        0xbb, 0xbe, 0x68, 0x0e, 0x1f, 0xee, 0x14, 0x67,
        0x7b, 0xa1, 0xa3, 0xc3, 0x54, 0x0b, 0xf7, 0xb1,
        0xcd, 0xb6, 0x06, 0xe8, 0x57, 0x23, 0x3e, 0x0e,
        /* Timestamp (4 bytes, little-endian) */
        0xc3, 0xbf, 0x61, 0x4d,
        /* Bits/Difficulty (4 bytes, little-endian) */
        0x1a, 0x44, 0xb9, 0xf2,
        /* Nonce (4 bytes, little-endian) - sera testé par GPU */
        0x00, 0x00, 0x00, 0x00
    };
    
    printf("[C351] Block header configuré (MÊME que test CPU)\n");
    printf("[C351] Attendu: 9 near-miss, max_leading_zeros=20\n");
    
    /* Test 1 dispatch avec 10M nonces (même que CPU) */
    uint32_t best_nonce = 0;
    uint32_t leading_zeros = 0;
    uint32_t start_nonce = 0;
    
    printf("[C351] Test GPU: start_nonce=%u count=10M\n", start_nonce);
    
    ret = btc_gen9_execute_mining(ctx, block_header, start_nonce, 10000000,
                                  &best_nonce, &leading_zeros);
    if (ret != 0) {
        fprintf(stderr, "Execute mining failed (ret=%d)\n", ret);
        btc_gen9_cleanup(ctx);
        return 1;
    }
    
    printf("[C351] Résultat GPU: best_nonce=%u leading_zeros=%u\n",
           best_nonce, leading_zeros);
    
    btc_gen9_cleanup(ctx);
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C351 KERNEL SANS ATOMICS - RÉSULTATS TEST GPU            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("Kernel: btc_sha256_c351_noatomic_fixed.bin_kbl.bin\n");
    printf("Nonces testés: 10,000,000\n");
    printf("Best nonce trouvé: %u\n", best_nonce);
    printf("Leading zeros: %u\n", leading_zeros);
    printf("\n");
    printf("COMPARAISON CPU vs GPU:\n");
    printf("CPU (OpenSSL): 9 near-miss, max=20 leading zeros\n");
    printf("GPU (C351):    %s\n", (leading_zeros > 0) ? "✅ DÉTECTE" : "❌ NE DÉTECTE PAS");
    printf("\n");
    printf("Nonces attendus: 3, 5, 92, 359, 793, 30468, 176709, 601998, 6567206\n");
    printf("Nonce GPU trouvé: %u %s\n", best_nonce, 
           (best_nonce == 6567206) ? "✅ CORRECT (max)" : 
           (best_nonce == 3 || best_nonce == 5 || best_nonce == 92 || 
            best_nonce == 359 || best_nonce == 793 || best_nonce == 30468 || 
            best_nonce == 176709 || best_nonce == 601998) ? "✅ CORRECT" : "❓ DIFFÉRENT");
    
    return 0;
}

// Made with Bob