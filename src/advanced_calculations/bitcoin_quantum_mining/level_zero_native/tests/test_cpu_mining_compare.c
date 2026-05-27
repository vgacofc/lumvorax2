// Test CPU Mining - Comparaison avec GPU
// Utilise le MÊME block header que le test GPU pour comparaison directe

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

// Structure block header Bitcoin (80 bytes)
typedef struct {
    uint32_t version;
    uint8_t prev_block[32];
    uint8_t merkle_root[32];
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
} __attribute__((packed)) btc_block_header_t;

// Fonction SHA256 double (Bitcoin)
void double_sha256(const uint8_t *data, size_t len, uint8_t *hash) {
    uint8_t temp[32];
    SHA256(data, len, temp);
    SHA256(temp, 32, hash);
}

// Compte les leading zeros dans un hash
int count_leading_zeros(const uint8_t *hash) {
    int zeros = 0;
    for (int i = 0; i < 32; i++) {
        if (hash[i] == 0) {
            zeros += 8;
        } else {
            // Compte les bits à 0 dans le premier byte non-nul
            uint8_t byte = hash[i];
            while ((byte & 0x80) == 0 && zeros < 256) {
                zeros++;
                byte <<= 1;
            }
            break;
        }
    }
    return zeros;
}

int main() {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  TEST CPU MINING - Comparaison avec GPU\n");
    printf("  MÊME block header que test GPU\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    // MÊME block header que le test GPU
    btc_block_header_t header = {
        .version = 0x20000000,
        .timestamp = (uint32_t)time(NULL),
        .bits = 0x1d00ffff,  // Difficulté Bitcoin testnet
        .nonce = 0
    };
    
    // Remplir prev_block et merkle_root avec des valeurs de test
    memset(header.prev_block, 0xAA, 32);
    memset(header.merkle_root, 0xBB, 32);

    printf("[CPU] Block header configuré:\n");
    printf("  version=0x%08x\n", header.version);
    printf("  timestamp=%u\n", header.timestamp);
    printf("  bits=0x%08x\n", header.bits);
    printf("\n");

    // Test sur 100M nonces (comme le GPU)
    uint32_t start_nonce = 0;
    uint32_t count = 10000000;
    
    uint32_t best_nonce = 0;
    int max_leading_zeros = 0;
    int total_near_miss = 0;
    
    printf("[CPU] Mining %u nonces (0 → %u)...\n", count, count-1);
    
    clock_t start_time = clock();
    
    for (uint32_t nonce = start_nonce; nonce < start_nonce + count; nonce++) {
        header.nonce = nonce;
        
        uint8_t hash[32];
        double_sha256((uint8_t*)&header, sizeof(header), hash);
        
        int leading_zeros = count_leading_zeros(hash);
        
        if (leading_zeros > max_leading_zeros) {
            max_leading_zeros = leading_zeros;
            best_nonce = nonce;
            
            printf("[CPU] Near-miss trouvé! nonce=%u leading_zeros=%d\n", nonce, leading_zeros);
            printf("      Hash: ");
            for (int i = 0; i < 32; i++) {
                printf("%02x", hash[i]);
            }
            printf("\n");
            
            total_near_miss++;
        }
        
        // Afficher progression tous les 10M nonces
        if ((nonce - start_nonce) % 10000000 == 0 && nonce > start_nonce) {
            printf("[CPU] Progression: %u/%u nonces (%.1f%%)\n", 
                   nonce - start_nonce, count, 
                   100.0 * (nonce - start_nonce) / count);
        }
    }
    
    clock_t end_time = clock();
    double elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    double hashrate = count / elapsed / 1000000.0;  // MH/s
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS CPU MINING                                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("Nonces testés: %u\n", count);
    printf("Temps: %.2f secondes\n", elapsed);
    printf("Hashrate: %.2f MH/s\n", hashrate);
    printf("Best nonce: %u\n", best_nonce);
    printf("Max leading zeros: %d\n", max_leading_zeros);
    printf("Total near-miss: %d\n", total_near_miss);
    printf("\n");
    
    if (max_leading_zeros == 0) {
        printf("⚠️  AUCUN near-miss trouvé (leading_zeros=0)\n");
        printf("    Cela signifie que ce block header ne produit AUCUN\n");
        printf("    hash avec des leading zeros sur 100M nonces.\n");
        printf("    C'est NORMAL et explique pourquoi le GPU ne trouve rien.\n");
    } else {
        printf("✅ Near-miss trouvés! Le CPU a trouvé %d nonces avec leading zeros.\n", total_near_miss);
        printf("   Si le GPU ne trouve rien, c'est un BUG GPU.\n");
    }
    
    return 0;
}

// Made with Bob
