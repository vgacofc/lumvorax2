/**
 * LumVorax C198 Phase 15U — Test Bit-Level Tracker
 * 
 * Test unitaire du tracker bit-level format .lum
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "btc_bit_level_tracker.h"

// ═══════════════════════════════════════════════════════════════════════════
// MACROS TEST
// ═══════════════════════════════════════════════════════════════════════════

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "❌ ÉCHEC: %s\n", msg); \
            return 1; \
        } \
        fprintf(stderr, "✅ OK: %s\n", msg); \
    } while(0)

#define TEST_LOG(fmt, ...) \
    fprintf(stderr, "[TEST] " fmt "\n", ##__VA_ARGS__)

// ═══════════════════════════════════════════════════════════════════════════
// TESTS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Test 1 : Création et fermeture tracker
 */
int test_create_close(void) {
    TEST_LOG("Test 1: Création et fermeture tracker");
    
    const char *filepath = "logs/forensic/test_tracker.lum";
    
    // Créer tracker
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    TEST_ASSERT(tracker->is_open, "Tracker ouvert");
    TEST_ASSERT(tracker->fd >= 0, "File descriptor valide");
    TEST_ASSERT(tracker->event_count == 0, "Compteur événements = 0");
    
    // Fermer tracker
    btc_lum_close(tracker);
    
    // Vérifier fichier existe
    struct stat st;
    TEST_ASSERT(stat(filepath, &st) == 0, "Fichier .lum créé");
    TEST_ASSERT(st.st_size >= (off_t)sizeof(btc_lum_header), "Taille fichier >= header");
    
    TEST_LOG("✅ Test 1 réussi\n");
    return 0;
}

/**
 * Test 2 : Logger événements GEM
 */
int test_gem_events(void) {
    TEST_LOG("Test 2: Événements GEM");
    
    const char *filepath = "logs/forensic/test_gem_events.lum";
    
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    
    // Logger GEM_CREATE
    btc_lum_log_gem_create(tracker, 1, 1048576, 0x7f8a4c000000ULL);
    TEST_ASSERT(tracker->event_count == 1, "1 événement loggé");
    
    // Logger GEM_CLOSE
    btc_lum_log_gem_close(tracker, 1);
    TEST_ASSERT(tracker->event_count == 2, "2 événements loggés");
    
    btc_lum_close(tracker);
    
    // Vérifier taille fichier
    struct stat st;
    stat(filepath, &st);
    size_t expected_size = sizeof(btc_lum_header) 
                         + 2 * sizeof(btc_lum_event_header)
                         + sizeof(btc_lum_gem_create_data)
                         + sizeof(btc_lum_gem_close_data);
    TEST_ASSERT(st.st_size == (off_t)expected_size, "Taille fichier correcte");
    
    TEST_LOG("✅ Test 2 réussi\n");
    return 0;
}

/**
 * Test 3 : Logger événements GPU
 */
int test_gpu_events(void) {
    TEST_LOG("Test 3: Événements GPU");
    
    const char *filepath = "logs/forensic/test_gpu_events.lum";
    
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    
    // Logger GPU_MMAP
    btc_lum_log_gpu_mmap(tracker, 1, 0x7f8a4c000000ULL, 1048576);
    
    // Logger EXECBUFFER2
    btc_lum_log_execbuffer2(tracker, 1, 1, 0, 220);
    
    // Logger GPU_WAIT
    btc_lum_log_gpu_wait(tracker, 1, 1000000000ULL, 0);
    
    // Logger GPU_MUNMAP
    btc_lum_log_gpu_munmap(tracker, 0x7f8a4c000000ULL, 1048576);
    
    TEST_ASSERT(tracker->event_count == 4, "4 événements loggés");
    
    btc_lum_close(tracker);
    
    TEST_LOG("✅ Test 3 réussi\n");
    return 0;
}

/**
 * Test 4 : Logger événements BATCH_WRITE
 */
int test_batch_write(void) {
    TEST_LOG("Test 4: Événement BATCH_WRITE");
    
    const char *filepath = "logs/forensic/test_batch_write.lum";
    
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    
    // Données batch simulées (10 bytes)
    uint8_t batch_data[10] = {0x79, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    // Logger BATCH_WRITE
    btc_lum_log_batch_write(tracker, 1, 0, batch_data, sizeof(batch_data));
    TEST_ASSERT(tracker->event_count == 1, "1 événement loggé");
    
    btc_lum_close(tracker);
    
    // Vérifier taille fichier
    struct stat st;
    stat(filepath, &st);
    size_t expected_size = sizeof(btc_lum_header)
                         + sizeof(btc_lum_event_header)
                         + sizeof(btc_lum_batch_write_data)
                         + sizeof(batch_data);
    TEST_ASSERT(st.st_size == (off_t)expected_size, "Taille fichier correcte");
    
    TEST_LOG("✅ Test 4 réussi\n");
    return 0;
}

/**
 * Test 5 : Logger événements RAM
 */
int test_ram_events(void) {
    TEST_LOG("Test 5: Événements RAM");
    
    const char *filepath = "logs/forensic/test_ram_events.lum";
    
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    
    // Logger RAM_ALLOC
    btc_lum_log_ram_alloc(tracker, 0x12345678ULL, 1024, "test.c:42");
    
    // Logger RAM_FREE
    btc_lum_log_ram_free(tracker, 0x12345678ULL);
    
    TEST_ASSERT(tracker->event_count == 2, "2 événements loggés");
    
    btc_lum_close(tracker);
    
    TEST_LOG("✅ Test 5 réussi\n");
    return 0;
}

/**
 * Test 6 : Logger événements IOCTL
 */
int test_ioctl_events(void) {
    TEST_LOG("Test 6: Événements IOCTL");
    
    const char *filepath = "logs/forensic/test_ioctl_events.lum";
    
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    
    // Logger IOCTL_CALL
    btc_lum_log_ioctl_call(tracker, 0x40406469, 3);  // DRM_IOCTL_I915_GEM_EXECBUFFER2
    
    // Logger IOCTL_RESULT (succès)
    btc_lum_log_ioctl_result(tracker, 0x40406469, 0, 0);
    
    // Logger IOCTL_CALL
    btc_lum_log_ioctl_call(tracker, 0x40406469, 3);
    
    // Logger IOCTL_RESULT (échec errno=5)
    btc_lum_log_ioctl_result(tracker, 0x40406469, -1, 5);
    
    TEST_ASSERT(tracker->event_count == 4, "4 événements loggés");
    
    btc_lum_close(tracker);
    
    TEST_LOG("✅ Test 6 réussi\n");
    return 0;
}

/**
 * Test 7 : Logger événement ERROR
 */
int test_error_event(void) {
    TEST_LOG("Test 7: Événement ERROR");
    
    const char *filepath = "logs/forensic/test_error_event.lum";
    
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    
    // Logger ERROR
    btc_lum_log_error(tracker, 5, "EXECBUFFER2 failed: Input/output error");
    TEST_ASSERT(tracker->event_count == 1, "1 événement loggé");
    
    btc_lum_close(tracker);
    
    TEST_LOG("✅ Test 7 réussi\n");
    return 0;
}

/**
 * Test 8 : Scénario complet (simulation dispatch GPU)
 */
int test_full_scenario(void) {
    TEST_LOG("Test 8: Scénario complet (simulation dispatch GPU)");
    
    const char *filepath = "logs/forensic/test_full_scenario.lum";
    
    btc_lum_tracker *tracker = btc_lum_create(filepath);
    TEST_ASSERT(tracker != NULL, "Tracker créé");
    
    // Scénario : 1 dispatch GPU complet
    
    // 1. Créer contexte
    btc_lum_log_ctx_create(tracker, 1);
    
    // 2. Créer buffer GEM
    btc_lum_log_gem_create(tracker, 1, 1048576, 0x7f8a4c000000ULL);
    
    // 3. Mapper buffer
    btc_lum_log_gpu_mmap(tracker, 1, 0x7f8a4c000000ULL, 1048576);
    
    // 4. Écrire batch buffer
    uint8_t batch[10] = {0x79, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    btc_lum_log_batch_write(tracker, 1, 0, batch, sizeof(batch));
    
    // 5. Appeler EXECBUFFER2
    btc_lum_log_ioctl_call(tracker, 0x40406469, 3);
    btc_lum_log_execbuffer2(tracker, 1, 1, 0, 220);
    btc_lum_log_ioctl_result(tracker, 0x40406469, 0, 0);
    
    // 6. Attendre GPU
    btc_lum_log_gpu_wait(tracker, 1, 1000000000ULL, 0);
    
    // 7. Unmapper buffer
    btc_lum_log_gpu_munmap(tracker, 0x7f8a4c000000ULL, 1048576);
    
    // 8. Fermer buffer
    btc_lum_log_gem_close(tracker, 1);
    
    // 9. Détruire contexte
    btc_lum_log_ctx_destroy(tracker, 1);
    
    TEST_ASSERT(tracker->event_count == 11, "11 événements loggés");
    
    btc_lum_close(tracker);
    
    // Vérifier fichier existe et taille > 0
    struct stat st;
    stat(filepath, &st);
    TEST_ASSERT(st.st_size > (off_t)sizeof(btc_lum_header), "Fichier contient données");
    
    TEST_LOG("✅ Test 8 réussi\n");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════

int main(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    fprintf(stderr, "  LumVorax C198 Phase 15U — Test Bit-Level Tracker\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    fprintf(stderr, "\n");
    
    int failed = 0;
    
    failed += test_create_close();
    failed += test_gem_events();
    failed += test_gpu_events();
    failed += test_batch_write();
    failed += test_ram_events();
    failed += test_ioctl_events();
    failed += test_error_event();
    failed += test_full_scenario();
    
    fprintf(stderr, "\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    if (failed == 0) {
        fprintf(stderr, "  ✅ TOUS LES TESTS RÉUSSIS (8/8)\n");
    } else {
        fprintf(stderr, "  ❌ ÉCHECS: %d test(s)\n", failed);
    }
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    fprintf(stderr, "\n");
    
    return failed;
}

// Made with Bob
