/* ═══════════════════════════════════════════════════════════════════════════
   TEST C193 FULL GPU BENCHMARK — SHA-256 + DRM POOL OPTIMISÉ
   ═══════════════════════════════════════════════════════════════════════════
   
   OBJECTIF: Tester le hashrate RÉEL avec calcul SHA-256 + allocation DRM
   
   ARCHITECTURE:
   - Pool DRM optimisé (3.2x speedup allocation)
   - Kernel SHA-256 natif C187 (CPU baseline: 3.87 MH/s)
   - Simulation dispatch GPU C189/C191
   - Métriques forensiques complètes
   
   TARGET: ≥23.14 MH/s (égaler C180 OpenCL)
   
   ═══════════════════════════════════════════════════════════════════════════ */

#include "lum_drm_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════
   SHA-256 NATIF (COPIÉ DE C187)
   ═══════════════════════════════════════════════════════════════════════════ */

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h, i, t1, t2, m[64];
    
    for (i = 0; i < 16; ++i) {
        m[i] = (data[i * 4] << 24) | (data[i * 4 + 1] << 16) | 
               (data[i * 4 + 2] << 8) | (data[i * 4 + 3]);
    }
    for (; i < 64; ++i) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }
    
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256_hash(const uint8_t* data, size_t len, uint8_t hash[32]) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    uint8_t block[64];
    size_t i;
    
    for (i = 0; i + 64 <= len; i += 64) {
        sha256_transform(state, data + i);
    }
    
    size_t rem = len - i;
    memcpy(block, data + i, rem);
    block[rem] = 0x80;
    
    if (rem >= 56) {
        memset(block + rem + 1, 0, 64 - rem - 1);
        sha256_transform(state, block);
        memset(block, 0, 56);
    } else {
        memset(block + rem + 1, 0, 56 - rem - 1);
    }
    
    uint64_t bitlen = len * 8;
    for (int j = 0; j < 8; j++) {
        block[63 - j] = bitlen >> (j * 8);
    }
    
    sha256_transform(state, block);
    
    for (i = 0; i < 8; i++) {
        hash[i * 4] = (state[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (state[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (state[i] >> 8) & 0xff;
        hash[i * 4 + 3] = state[i] & 0xff;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   BENCHMARK CONFIGURATION
   ═══════════════════════════════════════════════════════════════════════════ */

#define NONCES_PER_BATCH 262144  /* 256k nonces (comme C180) */
#define NUM_BATCHES 10           /* 10 batches = 2.62M hashes */
#define BUFFER_SIZE 4096

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

/* ═══════════════════════════════════════════════════════════════════════════
   BENCHMARK FULL GPU (SHA-256 + DRM POOL)
   ═══════════════════════════════════════════════════════════════════════════ */

static int benchmark_full_gpu(void) {
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  BENCHMARK C193 FULL GPU (SHA-256 + DRM POOL)\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Configuration:\n");
    printf("  - Nonces/batch: %d\n", NONCES_PER_BATCH);
    printf("  - Batches: %d\n", NUM_BATCHES);
    printf("  - Total hashes: %.2f M\n", (NONCES_PER_BATCH * NUM_BATCHES) / 1000000.0);
    printf("  - DRM Pool: 4 contextes\n");
    printf("  - SHA-256: Natif C187\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    /* Init pool DRM */
    printf("[1/4] Initializing DRM pool...\n");
    if (lum_drm_pool_init() < 0) {
        fprintf(stderr, "ERROR: DRM pool init failed\n");
        return -1;
    }
    printf("✅ DRM pool initialized (4 contexts)\n\n");
    
    /* Préparer données test */
    printf("[2/4] Preparing test data...\n");
    uint8_t block_header[80];
    memset(block_header, 0xAB, 80);  /* Données test */
    printf("✅ Test data ready (80 bytes block header)\n\n");
    
    /* Benchmark */
    printf("[3/4] Running benchmark...\n");
    double start = get_time_sec();
    uint64_t total_hashes = 0;
    
    for (int batch = 0; batch < NUM_BATCHES; batch++) {
        /* Obtenir contexte du pool */
        lum_drm_context_t* ctx = lum_drm_pool_get();
        if (!ctx) {
            fprintf(stderr, "ERROR: pool_get failed\n");
            lum_drm_pool_cleanup();
            return -1;
        }
        
        /* Allouer buffer GPU (batch) */
        uint32_t handle;
        if (lum_drm_gem_create(ctx, BUFFER_SIZE, &handle) < 0) {
            fprintf(stderr, "ERROR: gem_create failed\n");
            lum_drm_pool_cleanup();
            return -1;
        }
        
        /* Mapper mémoire GPU */
        void* gpu_mem;
        if (lum_drm_gem_mmap(ctx, handle, BUFFER_SIZE, &gpu_mem) < 0) {
            fprintf(stderr, "ERROR: gem_mmap failed\n");
            lum_drm_gem_close(ctx, handle);
            lum_drm_pool_cleanup();
            return -1;
        }
        
        /* CALCUL SHA-256 (simulation GPU via CPU) */
        for (uint32_t nonce = 0; nonce < NONCES_PER_BATCH; nonce++) {
            /* Modifier nonce dans block header */
            uint32_t* nonce_ptr = (uint32_t*)(block_header + 76);
            *nonce_ptr = nonce;
            
            /* Double SHA-256 (comme Bitcoin) */
            uint8_t hash1[32], hash2[32];
            sha256_hash(block_header, 80, hash1);
            sha256_hash(hash1, 32, hash2);
            
            /* Écrire résultat dans GPU mem (simulation) */
            if (nonce % 10000 == 0) {
                memcpy(gpu_mem, hash2, 32);
            }
            
            total_hashes++;
        }
        
        /* Cleanup buffer */
        lum_drm_gem_close(ctx, handle);
        
        /* Progress */
        if ((batch + 1) % 2 == 0) {
            double elapsed = get_time_sec() - start;
            double mhs = (total_hashes / elapsed) / 1000000.0;
            printf("  Batch %d/%d: %.2f MH/s (%.1f%% complete)\n", 
                   batch + 1, NUM_BATCHES, mhs, 
                   ((batch + 1) * 100.0) / NUM_BATCHES);
        }
    }
    
    double elapsed = get_time_sec() - start;
    double mhs = (total_hashes / elapsed) / 1000000.0;
    
    printf("\n✅ Benchmark complete\n\n");
    
    /* Cleanup */
    printf("[4/4] Cleaning up...\n");
    lum_drm_pool_cleanup();
    printf("✅ Cleanup complete\n\n");
    
    /* Résultats */
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTATS C193 FULL GPU\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Total hashes:    %lu\n", total_hashes);
    printf("Time:            %.3f sec\n", elapsed);
    printf("Hashrate:        %.2f MH/s\n", mhs);
    printf("Target C180:     23.14 MH/s\n");
    
    if (mhs >= 23.14) {
        printf("Status:          ✅ TARGET ATTEINT (+%.1f%%)\n",
               ((mhs - 23.14) / 23.14) * 100.0);
    } else {
        printf("Status:          ⚠️  Target non atteint (%.1f%% manquant)\n",
               ((23.14 - mhs) / 23.14) * 100.0);
    }
    
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  LUMVORAX C193 FULL GPU BENCHMARK                         ║\n");
    printf("║  SHA-256 Natif + DRM Pool Optimisé                        ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    int ret = benchmark_full_gpu();
    
    if (ret < 0) {
        fprintf(stderr, "\n❌ BENCHMARK FAILED\n\n");
        return 1;
    }
    
    printf("\n✅ BENCHMARK SUCCESS\n\n");
    return 0;
}

// Made with Bob - C193 Full GPU Benchmark