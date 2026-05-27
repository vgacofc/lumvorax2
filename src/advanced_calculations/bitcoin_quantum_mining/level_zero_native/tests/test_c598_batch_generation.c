/**
 * TEST C598 - Validation Génération MEDIA_INTERFACE_DESCRIPTOR_LOAD + GPGPU_WALKER
 * 
 * Objectif: Valider la génération automatique des commandes:
 * - MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWords)
 * - GPGPU_WALKER (15 DWords)
 * 
 * Basé sur reverse engineering OpenCL C545:
 * - MEDIA_IDL @ 0x00EC (3 DWords)
 * - GPGPU_WALKER @ 0x0110 (15 DWords)
 * 
 * Progression batch: 28 DWords (C597) → 46 DWords (C598) = 57.5%
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <i915_drm.h>
#include <xf86drm.h>

#include "../src/batch_generator_auto.h"

// Couleurs pour output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

// Logging forensique nanoseconde
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_FORENSIC(fmt, ...) do { \
    uint64_t ts = get_timestamp_ns(); \
    printf("[%lu.%09lu] " fmt "\n", ts / 1000000000ULL, ts % 1000000000ULL, ##__VA_ARGS__); \
} while(0)

// Test 1: Validation génération MEDIA_INTERFACE_DESCRIPTOR_LOAD
static int test_media_interface_descriptor_load(batch_generator_t *gen) {
    LOG_FORENSIC(COLOR_BLUE "=== TEST 1: MEDIA_INTERFACE_DESCRIPTOR_LOAD ===" COLOR_RESET);
    
    uint32_t batch[1024] = {0};
    uint32_t idx = 0;
    
    uint64_t start_ns = get_timestamp_ns();
    int ret = generate_media_interface_descriptor_load(gen, batch, &idx);
    uint64_t end_ns = get_timestamp_ns();
    
    if (ret != 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: generate_media_interface_descriptor_load retourné %d" COLOR_RESET, ret);
        return -1;
    }
    
    LOG_FORENSIC("Génération MEDIA_IDL: %lu ns", end_ns - start_ns);
    LOG_FORENSIC("DWords générés: %u (attendu: 3)", idx);
    
    if (idx != 3) {
        LOG_FORENSIC(COLOR_RED "ERREUR: Nombre DWords incorrect (attendu 3, obtenu %u)" COLOR_RESET, idx);
        return -1;
    }
    
    // Validation structure (basé sur OpenCL C545 @ 0x00EC)
    LOG_FORENSIC("Validation structure MEDIA_IDL:");
    
    // DW0: Opcode
    uint32_t dw0 = batch[0];
    LOG_FORENSIC("  DW0: 0x%08x (Opcode)", dw0);
    if (dw0 != 0x61020001) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: Opcode incorrect (attendu 0x61020001)" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ Opcode correct: MEDIA_INTERFACE_DESCRIPTOR_LOAD, length=3" COLOR_RESET);
    
    // DW1: Interface Descriptor Total Length (adresse GTT IDRT)
    uint32_t dw1 = batch[1];
    uint64_t idrt_gtt = gen->buffers[BUFFER_TYPE_IDRT].offset;
    LOG_FORENSIC("  DW1: 0x%08x (IDRT GTT LOW)", dw1);
    LOG_FORENSIC("  IDRT GTT attendu: 0x%016lx", idrt_gtt);
    if (dw1 != (uint32_t)(idrt_gtt & 0xFFFFFFFF)) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: IDRT GTT LOW incorrect" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ IDRT GTT LOW correct" COLOR_RESET);
    
    // DW2: Interface Descriptor Data Start Address
    uint32_t dw2 = batch[2];
    LOG_FORENSIC("  DW2: 0x%08x (Start Address)", dw2);
    if (dw2 != 0x00000000) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: Start Address incorrect (attendu 0x00000000)" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ Start Address correct" COLOR_RESET);
    
    LOG_FORENSIC(COLOR_GREEN "TEST 1 RÉUSSI: MEDIA_IDL généré correctement" COLOR_RESET);
    return 0;
}

// Test 2: Validation génération GPGPU_WALKER
static int test_gpgpu_walker(batch_generator_t *gen) {
    LOG_FORENSIC(COLOR_BLUE "=== TEST 2: GPGPU_WALKER ===" COLOR_RESET);
    
    uint32_t batch[1024] = {0};
    uint32_t idx = 0;
    
    // Configuration: 1 thread group dans chaque dimension (comme OpenCL C545)
    uint32_t thread_x = 1;
    uint32_t thread_y = 1;
    uint32_t thread_z = 1;
    
    uint64_t start_ns = get_timestamp_ns();
    int ret = generate_gpgpu_walker(gen, batch, &idx, thread_x, thread_y, thread_z);
    uint64_t end_ns = get_timestamp_ns();
    
    if (ret != 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: generate_gpgpu_walker retourné %d" COLOR_RESET, ret);
        return -1;
    }
    
    LOG_FORENSIC("Génération GPGPU_WALKER: %lu ns", end_ns - start_ns);
    LOG_FORENSIC("DWords générés: %u (attendu: 15)", idx);
    
    if (idx != 15) {
        LOG_FORENSIC(COLOR_RED "ERREUR: Nombre DWords incorrect (attendu 15, obtenu %u)" COLOR_RESET, idx);
        return -1;
    }
    
    // Validation structure (basé sur OpenCL C545 @ 0x0110)
    LOG_FORENSIC("Validation structure GPGPU_WALKER:");
    
    // DW0: Opcode
    uint32_t dw0 = batch[0];
    LOG_FORENSIC("  DW0: 0x%08x (Opcode)", dw0);
    if (dw0 != 0x18800101) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: Opcode incorrect (attendu 0x18800101)" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ Opcode correct: GPGPU_WALKER, length=15" COLOR_RESET);
    
    // DW1-2: Interface Descriptor Offset (adresse GTT IDRT)
    uint32_t dw1 = batch[1];
    uint32_t dw2 = batch[2];
    uint64_t idrt_gtt = gen->buffers[BUFFER_TYPE_IDRT].offset;
    uint64_t idrt_reconstructed = ((uint64_t)dw2 << 32) | dw1;
    LOG_FORENSIC("  DW1-2: 0x%08x%08x (IDRT GTT)", dw2, dw1);
    LOG_FORENSIC("  IDRT GTT attendu: 0x%016lx", idrt_gtt);
    if (idrt_reconstructed != idrt_gtt) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: IDRT GTT incorrect" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ IDRT GTT correct" COLOR_RESET);
    
    // DW3: Indirect Data Length
    uint32_t dw3 = batch[3];
    LOG_FORENSIC("  DW3: 0x%08x (Indirect Data Length)", dw3);
    if (dw3 != 0x00000000) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: Indirect Data Length incorrect" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ Indirect Data Length correct" COLOR_RESET);
    
    // DW4-6: Thread Group ID Starting X/Y/Z
    LOG_FORENSIC("  DW4-6: 0x%08x 0x%08x 0x%08x (Starting X/Y/Z)", batch[4], batch[5], batch[6]);
    if (batch[4] != 0 || batch[5] != 0 || batch[6] != 0) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: Starting X/Y/Z incorrect" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ Starting X/Y/Z correct (0,0,0)" COLOR_RESET);
    
    // DW8-10: Thread Group Dimensions
    LOG_FORENSIC("  DW8-10: 0x%08x 0x%08x 0x%08x (Dimensions X/Y/Z)", batch[8], batch[9], batch[10]);
    // Note: 0 signifie 1 thread group (encodé comme dimension - 1)
    if (batch[8] != 0 || batch[9] != 0 || batch[10] != 0) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: Dimensions X/Y/Z incorrectes" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ Dimensions X/Y/Z correctes (1,1,1 thread groups)" COLOR_RESET);
    
    // DW11-12: Execution Masks
    LOG_FORENSIC("  DW11-12: 0x%08x 0x%08x (Execution Masks)", batch[11], batch[12]);
    if (batch[11] != 0 || batch[12] != 0) {
        LOG_FORENSIC(COLOR_RED "  ERREUR: Execution Masks incorrects" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC(COLOR_GREEN "  ✓ Execution Masks corrects (tous threads actifs)" COLOR_RESET);
    
    LOG_FORENSIC(COLOR_GREEN "TEST 2 RÉUSSI: GPGPU_WALKER généré correctement" COLOR_RESET);
    return 0;
}

// Test 3: Validation batch complet C598 (46 DWords)
static int test_batch_complet_c598(batch_generator_t *gen) {
    LOG_FORENSIC(COLOR_BLUE "=== TEST 3: BATCH COMPLET C598 ===" COLOR_RESET);
    
    uint32_t batch[1024] = {0};
    uint32_t idx = 0;
    
    // Générer toutes les commandes implémentées jusqu'à C598
    uint64_t start_ns = get_timestamp_ns();
    
    // C597: STATE_BASE_ADDRESS (19 DWords)
    if (generate_state_base_address(gen, batch, &idx) != 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: generate_state_base_address échoué" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC("STATE_BASE_ADDRESS: %u DWords", idx);
    
    // C597: MEDIA_VFE_STATE (9 DWords)
    if (generate_media_vfe_state(gen, batch, &idx) != 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: generate_media_vfe_state échoué" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC("MEDIA_VFE_STATE: %u DWords (total: %u)", 9, idx);
    
    // C598: MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWords)
    if (generate_media_interface_descriptor_load(gen, batch, &idx) != 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: generate_media_interface_descriptor_load échoué" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC("MEDIA_IDL: %u DWords (total: %u)", 3, idx);
    
    // C598: GPGPU_WALKER (15 DWords)
    if (generate_gpgpu_walker(gen, batch, &idx, 1, 1, 1) != 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: generate_gpgpu_walker échoué" COLOR_RESET);
        return -1;
    }
    LOG_FORENSIC("GPGPU_WALKER: %u DWords (total: %u)", 15, idx);
    
    uint64_t end_ns = get_timestamp_ns();
    
    LOG_FORENSIC("Génération batch complet C598: %lu ns", end_ns - start_ns);
    LOG_FORENSIC("Total DWords générés: %u (attendu: 46)", idx);
    
    if (idx != 46) {
        LOG_FORENSIC(COLOR_RED "ERREUR: Nombre total DWords incorrect (attendu 46, obtenu %u)" COLOR_RESET, idx);
        return -1;
    }
    
    // Calcul progression
    uint32_t batch_opencl_total = 80;  // DWords batch OpenCL complet (C545)
    float progression = ((float)idx / (float)batch_opencl_total) * 100.0f;
    
    LOG_FORENSIC(COLOR_GREEN "Progression batch: %u/%u DWords (%.1f%%)" COLOR_RESET, 
                 idx, batch_opencl_total, progression);
    
    LOG_FORENSIC(COLOR_GREEN "TEST 3 RÉUSSI: Batch complet C598 généré correctement" COLOR_RESET);
    return 0;
}

int main(void) {
    LOG_FORENSIC(COLOR_BLUE "========================================" COLOR_RESET);
    LOG_FORENSIC(COLOR_BLUE "TEST C598 - GÉNÉRATION MEDIA_IDL + GPGPU_WALKER" COLOR_RESET);
    LOG_FORENSIC(COLOR_BLUE "========================================" COLOR_RESET);
    
    // Ouvrir device DRM (card1 identifié en C597)
    LOG_FORENSIC("Ouverture device DRM /dev/dri/card1...");
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: Impossible d'ouvrir /dev/dri/card1: %s" COLOR_RESET, strerror(errno));
        return 1;
    }
    LOG_FORENSIC(COLOR_GREEN "Device DRM ouvert: fd=%d" COLOR_RESET, drm_fd);
    
    // Initialiser générateur automatique
    LOG_FORENSIC("Initialisation batch_generator...");
    batch_generator_t gen_storage;
    batch_generator_t *gen = &gen_storage;
    if (batch_generator_init(gen, drm_fd) != 0) {
        LOG_FORENSIC(COLOR_RED "ERREUR: batch_generator_init échoué" COLOR_RESET);
        close(drm_fd);
        return 1;
    }
    LOG_FORENSIC(COLOR_GREEN "Batch generator initialisé" COLOR_RESET);
    
    // Exécuter tests
    int ret = 0;
    
    if (test_media_interface_descriptor_load(gen) != 0) {
        LOG_FORENSIC(COLOR_RED "TEST 1 ÉCHOUÉ" COLOR_RESET);
        ret = 1;
        goto cleanup;
    }
    
    if (test_gpgpu_walker(gen) != 0) {
        LOG_FORENSIC(COLOR_RED "TEST 2 ÉCHOUÉ" COLOR_RESET);
        ret = 1;
        goto cleanup;
    }
    
    if (test_batch_complet_c598(gen) != 0) {
        LOG_FORENSIC(COLOR_RED "TEST 3 ÉCHOUÉ" COLOR_RESET);
        ret = 1;
        goto cleanup;
    }
    
    LOG_FORENSIC(COLOR_GREEN "========================================" COLOR_RESET);
    LOG_FORENSIC(COLOR_GREEN "TOUS LES TESTS C598 RÉUSSIS" COLOR_RESET);
    LOG_FORENSIC(COLOR_GREEN "========================================" COLOR_RESET);
    
cleanup:
    batch_generator_cleanup(gen);
    close(drm_fd);
    
    return ret;
}

// Made with Bob
