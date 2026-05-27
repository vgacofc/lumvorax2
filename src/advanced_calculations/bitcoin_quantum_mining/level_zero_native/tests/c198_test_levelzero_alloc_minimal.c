/**
 * LumVorax C198 — Test Minimal Allocations Level Zero
 * 
 * Objectif : Valider zeMemAllocShared et zeMemAllocDevice
 * 
 * Test :
 * 1. Init contexte hybride
 * 2. Vérifier symboles Level Zero chargés
 * 3. Allouer shared memory 4MB
 * 4. Test write/read pattern 0xAA
 * 5. Allouer device memory 4MB
 * 6. Cleanup
 * 
 * STANDARD_NAMES.md v4.5 §T-BTC-L0-ALLOC-C198
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "btc_hybrid_opencl_levelzero.h"

#define TEST_SIZE (4 * 1024 * 1024)  // 4MB
#define PATTERN_BYTE 0xAA

int main(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  C198 — Test Minimal Allocations Level Zero\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    // ── Étape 1 : Init contexte ────────────────────────────────────
    printf("[1/6] Initialisation contexte hybride...\n");
    
    btc_hybrid_ctx_t* ctx = btc_hybrid_init(NULL, NULL);
    if (!ctx) {
        fprintf(stderr, "❌ ERREUR : Échec init contexte\n");
        return 1;
    }
    printf("✅ Contexte initialisé\n\n");

    // ── Étape 2 : Vérifier symboles Level Zero ─────────────────────
    printf("[2/6] Vérification symboles Level Zero...\n");
    
    if (!ctx->ze_MemAllocShared) {
        fprintf(stderr, "❌ ERREUR : zeMemAllocShared non chargé\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ zeMemAllocShared : %p\n", ctx->ze_MemAllocShared);
    
    if (!ctx->ze_MemAllocDevice) {
        fprintf(stderr, "❌ ERREUR : zeMemAllocDevice non chargé\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ zeMemAllocDevice : %p\n", ctx->ze_MemAllocDevice);
    
    if (!ctx->ze_MemFree) {
        fprintf(stderr, "❌ ERREUR : zeMemFree non chargé\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ zeMemFree : %p\n\n", ctx->ze_MemFree);

    // ── Étape 3 : Allocation shared memory ─────────────────────────
    printf("[3/6] Allocation shared memory (%u MB)...\n", TEST_SIZE / (1024*1024));
    
    btc_hybrid_buffer_t* shared_buf = btc_hybrid_alloc_shared(ctx, TEST_SIZE);
    if (!shared_buf) {
        fprintf(stderr, "❌ ERREUR : Échec allocation shared\n");
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ Shared memory allouée : %p\n", shared_buf->ze_memory);
    printf("   Type : %s\n", shared_buf->is_device ? "DEVICE" : "SHARED");
    printf("   Taille : %zu bytes\n\n", shared_buf->size);

    // ── Étape 4 : Test write/read pattern ──────────────────────────
    printf("[4/6] Test write/read pattern 0x%02X...\n", PATTERN_BYTE);
    
    // Write pattern
    memset(shared_buf->ze_memory, PATTERN_BYTE, TEST_SIZE);
    printf("✅ Pattern écrit\n");
    
    // Read et vérifier
    uint8_t* ptr = (uint8_t*)shared_buf->ze_memory;
    bool verify_ok = true;
    for (size_t i = 0; i < TEST_SIZE; i++) {
        if (ptr[i] != PATTERN_BYTE) {
            fprintf(stderr, "❌ ERREUR : Byte %zu = 0x%02X (attendu 0x%02X)\n", 
                    i, ptr[i], PATTERN_BYTE);
            verify_ok = false;
            break;
        }
    }
    
    if (verify_ok) {
        printf("✅ Pattern vérifié (%u MB)\n\n", TEST_SIZE / (1024*1024));
    } else {
        btc_hybrid_free_buffer(ctx, shared_buf);
        btc_hybrid_destroy(ctx);
        return 1;
    }

    // ── Étape 5 : Allocation device memory ─────────────────────────
    printf("[5/6] Allocation device memory (%u MB)...\n", TEST_SIZE / (1024*1024));
    
    btc_hybrid_buffer_t* device_buf = btc_hybrid_alloc_device(ctx, TEST_SIZE);
    if (!device_buf) {
        fprintf(stderr, "❌ ERREUR : Échec allocation device\n");
        btc_hybrid_free_buffer(ctx, shared_buf);
        btc_hybrid_destroy(ctx);
        return 1;
    }
    printf("✅ Device memory allouée : %p\n", device_buf->ze_memory);
    printf("   Type : %s\n", device_buf->is_device ? "DEVICE" : "SHARED");
    printf("   Taille : %zu bytes\n\n", device_buf->size);

    // ── Étape 6 : Cleanup ───────────────────────────────────────────
    printf("[6/6] Cleanup...\n");
    
    btc_hybrid_free_buffer(ctx, shared_buf);
    printf("✅ Shared buffer libéré\n");
    
    btc_hybrid_free_buffer(ctx, device_buf);
    printf("✅ Device buffer libéré\n");
    
    btc_hybrid_destroy(ctx);
    printf("✅ Contexte nettoyé\n\n");

    // ── Résultat final ──────────────────────────────────────────────
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✅ TEST RÉUSSI — Allocations Level Zero validées\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    return 0;
}

// Made with Bob
