/**
 * @file test_memory_tracker.c
 * @brief Tests unitaires Memory Tracker RAM+GPU
 * 
 * PHASE 15W: Validation complète du tracker mémoire
 * 
 * @author LumVorax Team
 * @date 2026-05-11
 */

#define _GNU_SOURCE
#include "btc_memory_tracker_gpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * TESTS UNITAIRES
 * ============================================================================ */

/**
 * @brief Test 1: Création/destruction tracker
 */
static bool test_create_destroy(void) {
    printf("\n[TEST 1] Création/destruction tracker...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        printf("❌ ÉCHEC: Création tracker\n");
        return false;
    }
    
    btc_mem_tracker_destroy(tracker);
    
    printf("✅ SUCCÈS: Création/destruction\n");
    return true;
}

/**
 * @brief Test 2: Allocation/libération RAM simple
 */
static bool test_ram_alloc_free(void) {
    printf("\n[TEST 2] Allocation/libération RAM...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    void* ptr = malloc(1024);
    if (!ptr) {
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    bool ok = btc_mem_track_alloc_ram(tracker, ptr, 1024, BTC_MEM_TYPE_RAM_MALLOC, __FILE__, __LINE__);
    if (!ok) {
        printf("❌ ÉCHEC: Track alloc RAM\n");
        free(ptr);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_stats_t stats;
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_alloc_count != 1 || stats.ram_current_bytes != 1024) {
        printf("❌ ÉCHEC: Stats incorrectes (alloc=%lu, bytes=%lu)\n",
               stats.ram_alloc_count, stats.ram_current_bytes);
        free(ptr);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    ok = btc_mem_track_free_ram(tracker, ptr, __FILE__, __LINE__);
    if (!ok) {
        printf("❌ ÉCHEC: Track free RAM\n");
        free(ptr);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    free(ptr);
    
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_free_count != 1 || stats.ram_current_bytes != 0) {
        printf("❌ ÉCHEC: Stats après free incorrectes (free=%lu, bytes=%lu)\n",
               stats.ram_free_count, stats.ram_current_bytes);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_tracker_destroy(tracker);
    
    printf("✅ SUCCÈS: Allocation/libération RAM\n");
    return true;
}

/**
 * @brief Test 3: Macros BTC_MALLOC/BTC_FREE
 */
static bool test_macros(void) {
    printf("\n[TEST 3] Macros BTC_MALLOC/BTC_FREE...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    void* ptr = BTC_MALLOC(tracker, 2048);
    if (!ptr) {
        printf("❌ ÉCHEC: BTC_MALLOC\n");
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_stats_t stats;
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_alloc_count != 1 || stats.ram_current_bytes != 2048) {
        printf("❌ ÉCHEC: Stats après BTC_MALLOC incorrectes\n");
        free(ptr);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    BTC_FREE(tracker, ptr);
    
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_free_count != 1 || stats.ram_current_bytes != 0) {
        printf("❌ ÉCHEC: Stats après BTC_FREE incorrectes\n");
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    if (ptr != NULL) {
        printf("❌ ÉCHEC: BTC_FREE n'a pas mis ptr à NULL\n");
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_tracker_destroy(tracker);
    
    printf("✅ SUCCÈS: Macros BTC_MALLOC/BTC_FREE\n");
    return true;
}

/**
 * @brief Test 4: Allocation/libération GPU (GEM)
 */
static bool test_gpu_alloc_free(void) {
    printf("\n[TEST 4] Allocation/libération GPU...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    uint32_t gem_handle = 42;
    size_t size = 4096;
    
    bool ok = btc_mem_track_alloc_gpu(tracker, gem_handle, size, __FILE__, __LINE__);
    if (!ok) {
        printf("❌ ÉCHEC: Track alloc GPU\n");
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_stats_t stats;
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.gpu_alloc_count != 1 || stats.gpu_current_bytes != size) {
        printf("❌ ÉCHEC: Stats GPU incorrectes (alloc=%lu, bytes=%lu)\n",
               stats.gpu_alloc_count, stats.gpu_current_bytes);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    ok = btc_mem_track_free_gpu(tracker, gem_handle, __FILE__, __LINE__);
    if (!ok) {
        printf("❌ ÉCHEC: Track free GPU\n");
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.gpu_free_count != 1 || stats.gpu_current_bytes != 0) {
        printf("❌ ÉCHEC: Stats GPU après free incorrectes\n");
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_tracker_destroy(tracker);
    
    printf("✅ SUCCÈS: Allocation/libération GPU\n");
    return true;
}

/**
 * @brief Test 5: Détection double free RAM
 */
static bool test_double_free_ram(void) {
    printf("\n[TEST 5] Détection double free RAM...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    void* ptr = malloc(512);
    if (!ptr) {
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_track_alloc_ram(tracker, ptr, 512, BTC_MEM_TYPE_RAM_MALLOC, __FILE__, __LINE__);
    btc_mem_track_free_ram(tracker, ptr, __FILE__, __LINE__);
    
    bool ok = btc_mem_track_free_ram(tracker, ptr, __FILE__, __LINE__);
    
    free(ptr);
    btc_mem_tracker_destroy(tracker);
    
    if (ok) {
        printf("❌ ÉCHEC: Double free non détecté\n");
        return false;
    }
    
    printf("✅ SUCCÈS: Double free détecté\n");
    return true;
}

/**
 * @brief Test 6: Détection double free GPU
 */
static bool test_double_free_gpu(void) {
    printf("\n[TEST 6] Détection double free GPU...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    uint32_t gem_handle = 123;
    
    btc_mem_track_alloc_gpu(tracker, gem_handle, 8192, __FILE__, __LINE__);
    btc_mem_track_free_gpu(tracker, gem_handle, __FILE__, __LINE__);
    
    bool ok = btc_mem_track_free_gpu(tracker, gem_handle, __FILE__, __LINE__);
    
    btc_mem_tracker_destroy(tracker);
    
    if (ok) {
        printf("❌ ÉCHEC: Double free GPU non détecté\n");
        return false;
    }
    
    printf("✅ SUCCÈS: Double free GPU détecté\n");
    return true;
}

/**
 * @brief Test 7: Détection fuites mémoire
 */
static bool test_leak_detection(void) {
    printf("\n[TEST 7] Détection fuites mémoire...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    void* ptr1 = malloc(1024);
    void* ptr2 = malloc(2048);
    
    btc_mem_track_alloc_ram(tracker, ptr1, 1024, BTC_MEM_TYPE_RAM_MALLOC, __FILE__, __LINE__);
    btc_mem_track_alloc_ram(tracker, ptr2, 2048, BTC_MEM_TYPE_RAM_MALLOC, __FILE__, __LINE__);
    
    btc_mem_track_alloc_gpu(tracker, 100, 4096, __FILE__, __LINE__);
    btc_mem_track_alloc_gpu(tracker, 200, 8192, __FILE__, __LINE__);
    
    btc_mem_track_free_ram(tracker, ptr1, __FILE__, __LINE__);
    free(ptr1);
    
    btc_mem_track_free_gpu(tracker, 100, __FILE__, __LINE__);
    
    uint64_t leaks = btc_mem_detect_leaks(tracker);
    
    if (leaks != 2) {
        printf("❌ ÉCHEC: Nombre fuites incorrect (attendu=2, obtenu=%lu)\n", leaks);
        free(ptr2);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    btc_mem_stats_t stats;
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_leaks_count != 1 || stats.ram_leaks_bytes != 2048) {
        printf("❌ ÉCHEC: Fuites RAM incorrectes (count=%lu, bytes=%lu)\n",
               stats.ram_leaks_count, stats.ram_leaks_bytes);
        free(ptr2);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    if (stats.gpu_leaks_count != 1 || stats.gpu_leaks_bytes != 8192) {
        printf("❌ ÉCHEC: Fuites GPU incorrectes (count=%lu, bytes=%lu)\n",
               stats.gpu_leaks_count, stats.gpu_leaks_bytes);
        free(ptr2);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    free(ptr2);
    btc_mem_tracker_destroy(tracker);
    
    printf("✅ SUCCÈS: Détection fuites mémoire\n");
    return true;
}

/**
 * @brief Test 8: Statistiques pic mémoire
 */
static bool test_peak_memory(void) {
    printf("\n[TEST 8] Statistiques pic mémoire...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    void* ptr1 = BTC_MALLOC(tracker, 1024);
    void* ptr2 = BTC_MALLOC(tracker, 2048);
    void* ptr3 = BTC_MALLOC(tracker, 4096);
    
    btc_mem_stats_t stats;
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_peak_bytes != 7168) {
        printf("❌ ÉCHEC: Pic RAM incorrect (attendu=7168, obtenu=%lu)\n",
               stats.ram_peak_bytes);
        BTC_FREE(tracker, ptr1);
        BTC_FREE(tracker, ptr2);
        BTC_FREE(tracker, ptr3);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    BTC_FREE(tracker, ptr2);
    
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_current_bytes != 5120) {
        printf("❌ ÉCHEC: RAM actuelle incorrecte après free (attendu=5120, obtenu=%lu)\n",
               stats.ram_current_bytes);
        BTC_FREE(tracker, ptr1);
        BTC_FREE(tracker, ptr3);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    if (stats.ram_peak_bytes != 7168) {
        printf("❌ ÉCHEC: Pic RAM modifié après free\n");
        BTC_FREE(tracker, ptr1);
        BTC_FREE(tracker, ptr3);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    BTC_FREE(tracker, ptr1);
    BTC_FREE(tracker, ptr3);
    
    btc_mem_tracker_destroy(tracker);
    
    printf("✅ SUCCÈS: Statistiques pic mémoire\n");
    return true;
}

/**
 * @brief Test 9: Scénario complet RAM+GPU
 */
static bool test_full_scenario(void) {
    printf("\n[TEST 9] Scénario complet RAM+GPU...\n");
    
    btc_mem_tracker_t* tracker = btc_mem_tracker_create(1000);
    if (!tracker) {
        return false;
    }
    
    void* ram1 = BTC_MALLOC(tracker, 1024);
    void* ram2 = BTC_CALLOC(tracker, 10, 100);
    
    btc_mem_track_alloc_gpu(tracker, 1, 4096, __FILE__, __LINE__);
    btc_mem_track_alloc_gpu(tracker, 2, 8192, __FILE__, __LINE__);
    
    void* mmap1 = (void*)0x7f0000000000;
    btc_mem_track_mmap_gpu(tracker, mmap1, 1, 4096, __FILE__, __LINE__);
    
    BTC_FREE(tracker, ram1);
    btc_mem_track_free_gpu(tracker, 1, __FILE__, __LINE__);
    btc_mem_track_munmap_gpu(tracker, mmap1, __FILE__, __LINE__);
    
    btc_mem_stats_t stats;
    btc_mem_get_stats(tracker, &stats);
    
    if (stats.ram_alloc_count != 2 || stats.ram_free_count != 1) {
        printf("❌ ÉCHEC: Stats RAM incorrectes\n");
        BTC_FREE(tracker, ram2);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    if (stats.gpu_alloc_count != 2 || stats.gpu_free_count != 1) {
        printf("❌ ÉCHEC: Stats GPU incorrectes\n");
        BTC_FREE(tracker, ram2);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    uint64_t leaks = btc_mem_detect_leaks(tracker);
    
    if (leaks != 2) {
        printf("❌ ÉCHEC: Nombre fuites incorrect (attendu=2, obtenu=%lu)\n", leaks);
        BTC_FREE(tracker, ram2);
        btc_mem_tracker_destroy(tracker);
        return false;
    }
    
    BTC_FREE(tracker, ram2);
    btc_mem_tracker_destroy(tracker);
    
    printf("✅ SUCCÈS: Scénario complet RAM+GPU\n");
    return true;
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  TESTS MEMORY TRACKER RAM+GPU — Phase 15W\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    int passed = 0;
    int total = 9;
    
    if (test_create_destroy()) passed++;
    if (test_ram_alloc_free()) passed++;
    if (test_macros()) passed++;
    if (test_gpu_alloc_free()) passed++;
    if (test_double_free_ram()) passed++;
    if (test_double_free_gpu()) passed++;
    if (test_leak_detection()) passed++;
    if (test_peak_memory()) passed++;
    if (test_full_scenario()) passed++;
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTATS: %d/%d tests réussis (%.1f%%)\n", 
           passed, total, (passed * 100.0) / total);
    printf("═══════════════════════════════════════════════════════════\n");
    
    return (passed == total) ? 0 : 1;
}

// Made with Bob
