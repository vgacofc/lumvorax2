/**
 * @file test_nx_trace.c
 * @brief Tests unitaires pour le module NX-Trace (Execution Intelligence System)
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 * Méthodologie : TDD (Test-Driven Development)
 * 
 * Tests couverts :
 * - Initialisation / cleanup
 * - Enregistrement événements
 * - Buffer overflow protection
 * - Thread-safety
 * - Replay déterministe
 * - Analyse sémantique
 * - Performance
 */

#include "test_framework.h"
#include "../src/execution_intelligence/nx_trace.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * TESTS INITIALISATION / CLEANUP
 * ============================================================================ */

/**
 * @brief Test initialisation basique
 */
static void test_nx_trace_init_basic(void) {
    nx_trace_context_t* ctx = nx_trace_init(1000);
    
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    ASSERT_EQ(ctx->max_events, 1000, "Max events should be 1000");
    ASSERT_EQ(ctx->event_count, 0, "Event count should be 0");
    ASSERT_TRUE(ctx->events != NULL, "Events buffer should be allocated");
    
    nx_trace_cleanup(ctx);
}

/**
 * @brief Test initialisation avec capacité invalide
 */
static void test_nx_trace_init_invalid_capacity(void) {
    nx_trace_context_t* ctx1 = nx_trace_init(0);
    ASSERT_TRUE(ctx1 == NULL, "Init with 0 capacity should fail");
    
    nx_trace_context_t* ctx2 = nx_trace_init(-1);
    ASSERT_TRUE(ctx2 == NULL, "Init with negative capacity should fail");
}

/**
 * @brief Test cleanup avec contexte NULL
 */
static void test_nx_trace_cleanup_null(void) {
    // Ne doit pas crasher
    nx_trace_cleanup(NULL);
}

/**
 * @brief Test double cleanup
 */
static void test_nx_trace_double_cleanup(void) {
    nx_trace_context_t* ctx = nx_trace_init(100);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    nx_trace_cleanup(ctx);
    // Second cleanup ne doit pas crasher (idempotent)
    nx_trace_cleanup(ctx);
}

/* ============================================================================
 * TESTS ENREGISTREMENT ÉVÉNEMENTS
 * ============================================================================ */

/**
 * @brief Test enregistrement événement simple
 */
static void test_nx_trace_record_event_simple(void) {
    nx_trace_context_t* ctx = nx_trace_init(100);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    int result = nx_trace_record_event(
        ctx,
        NX_TRACE_EVENT_FUNCTION_CALL,
        0x12345678,
        0,
        "test_function"
    );
    
    ASSERT_EQ(result, 0, "Record should succeed");
    ASSERT_EQ(ctx->event_count, 1, "Event count should be 1");
    
    nx_trace_event_t* event = &ctx->events[0];
    ASSERT_EQ(event->type, NX_TRACE_EVENT_FUNCTION_CALL, "Event type should match");
    ASSERT_EQ(event->instruction_ptr, 0x12345678, "Instruction pointer should match");
    ASSERT_STR_EQ(event->semantic_label, "test_function", "Semantic label should match");
    
    nx_trace_cleanup(ctx);
}

/**
 * @brief Test enregistrement événements multiples
 */
static void test_nx_trace_record_multiple_events(void) {
    nx_trace_context_t* ctx = nx_trace_init(10);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    for (int i = 0; i < 5; i++) {
        char label[64];
        snprintf(label, sizeof(label), "event_%d", i);
        
        int result = nx_trace_record_event(
            ctx,
            NX_TRACE_EVENT_MEMORY_READ,
            0x1000 + i,
            0x2000 + i,
            label
        );
        
        ASSERT_EQ(result, 0, "Record should succeed");
    }
    
    ASSERT_EQ(ctx->event_count, 5, "Event count should be 5");
    
    // Vérifier ordre chronologique
    for (int i = 1; i < 5; i++) {
        ASSERT_TRUE(
            ctx->events[i].timestamp_ns >= ctx->events[i-1].timestamp_ns,
            "Events should be in chronological order"
        );
    }
    
    nx_trace_cleanup(ctx);
}

/**
 * @brief Test enregistrement avec contexte NULL
 */
static void test_nx_trace_record_null_context(void) {
    int result = nx_trace_record_event(
        NULL,
        NX_TRACE_EVENT_FUNCTION_CALL,
        0x12345678,
        0,
        "test"
    );
    
    ASSERT_EQ(result, -1, "Record with NULL context should fail");
}

/**
 * @brief Test enregistrement avec label NULL
 */
static void test_nx_trace_record_null_label(void) {
    nx_trace_context_t* ctx = nx_trace_init(100);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    int result = nx_trace_record_event(
        ctx,
        NX_TRACE_EVENT_FUNCTION_CALL,
        0x12345678,
        0,
        NULL
    );
    
    ASSERT_EQ(result, 0, "Record with NULL label should succeed");
    ASSERT_STR_EQ(ctx->events[0].semantic_label, "", "Label should be empty string");
    
    nx_trace_cleanup(ctx);
}

/* ============================================================================
 * TESTS BUFFER OVERFLOW PROTECTION
 * ============================================================================ */

/**
 * @brief Test protection contre buffer overflow
 */
static void test_nx_trace_buffer_overflow(void) {
    nx_trace_context_t* ctx = nx_trace_init(5);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    // Remplir buffer
    for (int i = 0; i < 5; i++) {
        int result = nx_trace_record_event(
            ctx,
            NX_TRACE_EVENT_FUNCTION_CALL,
            0x1000 + i,
            0,
            "test"
        );
        ASSERT_EQ(result, 0, "Record should succeed");
    }
    
    ASSERT_EQ(ctx->event_count, 5, "Buffer should be full");
    
    // Tenter d'ajouter un événement supplémentaire
    int result = nx_trace_record_event(
        ctx,
        NX_TRACE_EVENT_FUNCTION_CALL,
        0x9999,
        0,
        "overflow"
    );
    
    ASSERT_EQ(result, -1, "Record should fail when buffer is full");
    ASSERT_EQ(ctx->event_count, 5, "Event count should remain 5");
    
    nx_trace_cleanup(ctx);
}

/**
 * @brief Test comportement circular buffer (si implémenté)
 */
static void test_nx_trace_circular_buffer(void) {
    nx_trace_context_t* ctx = nx_trace_init(3);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    // Activer mode circular si disponible
    if (ctx->circular_mode_available) {
        nx_trace_enable_circular_mode(ctx);
        
        // Ajouter plus d'événements que la capacité
        for (int i = 0; i < 10; i++) {
            nx_trace_record_event(
                ctx,
                NX_TRACE_EVENT_FUNCTION_CALL,
                0x1000 + i,
                0,
                "test"
            );
        }
        
        // Buffer devrait contenir les 3 derniers événements
        ASSERT_EQ(ctx->event_count, 3, "Buffer should contain 3 events");
        ASSERT_EQ(ctx->events[0].instruction_ptr, 0x1007, "Should have event 7");
        ASSERT_EQ(ctx->events[1].instruction_ptr, 0x1008, "Should have event 8");
        ASSERT_EQ(ctx->events[2].instruction_ptr, 0x1009, "Should have event 9");
    }
    
    nx_trace_cleanup(ctx);
}

/* ============================================================================
 * TESTS THREAD-SAFETY
 * ============================================================================ */

typedef struct {
    nx_trace_context_t* ctx;
    int thread_id;
    int num_events;
} thread_test_data_t;

static void* thread_record_events(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    for (int i = 0; i < data->num_events; i++) {
        char label[64];
        snprintf(label, sizeof(label), "thread_%d_event_%d", data->thread_id, i);
        
        nx_trace_record_event(
            data->ctx,
            NX_TRACE_EVENT_FUNCTION_CALL,
            (uint64_t)(data->thread_id * 1000 + i),
            0,
            label
        );
        
        // Petit délai pour augmenter chances de race conditions
        usleep(1);
    }
    
    return NULL;
}

/**
 * @brief Test thread-safety avec 4 threads concurrents
 */
static void test_nx_trace_thread_safety(void) {
    const int NUM_THREADS = 4;
    const int EVENTS_PER_THREAD = 25;
    
    nx_trace_context_t* ctx = nx_trace_init(NUM_THREADS * EVENTS_PER_THREAD);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    pthread_t threads[NUM_THREADS];
    thread_test_data_t thread_data[NUM_THREADS];
    
    // Lancer threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].ctx = ctx;
        thread_data[i].thread_id = i;
        thread_data[i].num_events = EVENTS_PER_THREAD;
        
        pthread_create(&threads[i], NULL, thread_record_events, &thread_data[i]);
    }
    
    // Attendre threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Vérifier résultats
    ASSERT_EQ(
        ctx->event_count,
        NUM_THREADS * EVENTS_PER_THREAD,
        "All events should be recorded"
    );
    
    // Vérifier pas de corruption mémoire
    for (size_t i = 0; i < ctx->event_count; i++) {
        ASSERT_TRUE(
            ctx->events[i].instruction_ptr < 10000,
            "Instruction pointer should be valid"
        );
    }
    
    nx_trace_cleanup(ctx);
}

/* ============================================================================
 * TESTS REPLAY DÉTERMINISTE
 * ============================================================================ */

/**
 * @brief Test replay basique
 */
static void test_nx_trace_replay_basic(void) {
    nx_trace_context_t* ctx = nx_trace_init(100);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    // Enregistrer séquence d'événements
    nx_trace_record_event(ctx, NX_TRACE_EVENT_FUNCTION_CALL, 0x1000, 0, "func_a");
    nx_trace_record_event(ctx, NX_TRACE_EVENT_MEMORY_WRITE, 0x1010, 0x2000, "write_x");
    nx_trace_record_event(ctx, NX_TRACE_EVENT_FUNCTION_RETURN, 0x1020, 0, "func_a");
    
    // Créer replay context
    nx_trace_replay_context_t* replay = nx_trace_replay_init(ctx);
    ASSERT_TRUE(replay != NULL, "Replay context should be initialized");
    
    // Rejouer événements
    nx_trace_event_t* event;
    int count = 0;
    
    while ((event = nx_trace_replay_next(replay)) != NULL) {
        count++;
        ASSERT_TRUE(event->type >= NX_TRACE_EVENT_FUNCTION_CALL, "Event type should be valid");
    }
    
    ASSERT_EQ(count, 3, "Should replay 3 events");
    
    nx_trace_replay_cleanup(replay);
    nx_trace_cleanup(ctx);
}

/* ============================================================================
 * TESTS ANALYSE SÉMANTIQUE
 * ============================================================================ */

/**
 * @brief Test détection hot paths
 */
static void test_nx_trace_detect_hot_paths(void) {
    nx_trace_context_t* ctx = nx_trace_init(1000);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    // Simuler hot path : fonction appelée 100 fois
    for (int i = 0; i < 100; i++) {
        nx_trace_record_event(ctx, NX_TRACE_EVENT_FUNCTION_CALL, 0x5000, 0, "hot_function");
    }
    
    // Autres fonctions appelées moins souvent
    for (int i = 0; i < 10; i++) {
        nx_trace_record_event(ctx, NX_TRACE_EVENT_FUNCTION_CALL, 0x6000, 0, "cold_function");
    }
    
    // Analyser hot paths
    nx_trace_hot_path_t* hot_paths = NULL;
    size_t num_hot_paths = 0;
    
    int result = nx_trace_analyze_hot_paths(ctx, &hot_paths, &num_hot_paths);
    ASSERT_EQ(result, 0, "Analysis should succeed");
    ASSERT_TRUE(num_hot_paths > 0, "Should detect at least one hot path");
    
    // Vérifier que hot_function est détectée
    bool found_hot = false;
    for (size_t i = 0; i < num_hot_paths; i++) {
        if (strcmp(hot_paths[i].function_name, "hot_function") == 0) {
            found_hot = true;
            ASSERT_TRUE(hot_paths[i].call_count >= 100, "Hot function should have high call count");
        }
    }
    
    ASSERT_TRUE(found_hot, "Should detect hot_function as hot path");
    
    free(hot_paths);
    nx_trace_cleanup(ctx);
}

/* ============================================================================
 * TESTS PERFORMANCE
 * ============================================================================ */

/**
 * @brief Test performance enregistrement
 */
static void test_nx_trace_performance(void) {
    const size_t NUM_EVENTS = 100000;
    
    nx_trace_context_t* ctx = nx_trace_init(NUM_EVENTS);
    ASSERT_TRUE(ctx != NULL, "Context should be initialized");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (size_t i = 0; i < NUM_EVENTS; i++) {
        nx_trace_record_event(
            ctx,
            NX_TRACE_EVENT_FUNCTION_CALL,
            0x1000 + i,
            0,
            "perf_test"
        );
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t duration_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL +
                           (end.tv_nsec - start.tv_nsec);
    
    double ns_per_event = (double)duration_ns / NUM_EVENTS;
    
    // Overhead devrait être < 500ns par événement
    ASSERT_TRUE(
        ns_per_event < 500.0,
        "Performance should be < 500ns per event"
    );
    
    printf("  Performance: %.2f ns/event (%.2f M events/sec)\n",
           ns_per_event,
           1000.0 / ns_per_event);
    
    nx_trace_cleanup(ctx);
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    test_start("NX-Trace Module");
    
    printf("\n=== Tests Initialisation / Cleanup ===\n");
    RUN_TEST(test_nx_trace_init_basic);
    RUN_TEST(test_nx_trace_init_invalid_capacity);
    RUN_TEST(test_nx_trace_cleanup_null);
    RUN_TEST(test_nx_trace_double_cleanup);
    
    printf("\n=== Tests Enregistrement Événements ===\n");
    RUN_TEST(test_nx_trace_record_event_simple);
    RUN_TEST(test_nx_trace_record_multiple_events);
    RUN_TEST(test_nx_trace_record_null_context);
    RUN_TEST(test_nx_trace_record_null_label);
    
    printf("\n=== Tests Buffer Overflow Protection ===\n");
    RUN_TEST(test_nx_trace_buffer_overflow);
    RUN_TEST(test_nx_trace_circular_buffer);
    
    printf("\n=== Tests Thread-Safety ===\n");
    RUN_TEST(test_nx_trace_thread_safety);
    
    printf("\n=== Tests Replay Déterministe ===\n");
    RUN_TEST(test_nx_trace_replay_basic);
    
    printf("\n=== Tests Analyse Sémantique ===\n");
    RUN_TEST(test_nx_trace_detect_hot_paths);
    
    printf("\n=== Tests Performance ===\n");
    RUN_TEST(test_nx_trace_performance);
    
    test_end();
    
    // Générer rapport forensique
    test_generate_forensic_report("RAPPORT_NX_TRACE_TESTS.md");
    
    return test_print_results();
}

// Made with Bob
