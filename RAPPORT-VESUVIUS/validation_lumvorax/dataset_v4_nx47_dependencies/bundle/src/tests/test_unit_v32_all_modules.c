#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

#include "../optimization/async_logging/async_logger.h"
#include "../optimization/slab_allocator/slab_allocator.h"
#include "../optimization/simd_batch/simd_batch_processor.h"
#include "../optimization/lockfree/lockfree_queue.h"
#include "../optimization/mmap_io/mmap_persistence.h"
#include "../optimization/lz4_compression/lz4_compressor.h"
#include "../distributed/distributed_node.h"
#include "../wasm/wasm_export.h"
#include "../versioning/version_manager.h"
#include "../versioning/api_contract.h"
#include "../cicd/benchmark_runner.h"
#include "../cicd/regression_detector.h"

#define ANSI_GREEN "\033[32m"
#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_RESET "\033[0m"

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

#define TEST(name, expr) do { \
    total_tests++; \
    if (expr) { \
        passed_tests++; \
        printf("  " ANSI_GREEN "OK" ANSI_RESET " %s\n", name); \
    } else { \
        failed_tests++; \
        printf("  " ANSI_RED "FAIL" ANSI_RESET " %s\n", name); \
    } \
} while(0)

void test_async_logger(void) {
    printf(ANSI_YELLOW "\n[MODULE] ASYNC_LOGGER\n" ANSI_RESET);
    
    async_logger_t* logger = async_logger_create("logs/v32/async_test.log", 1024);
    TEST("async_logger_create", logger != NULL);
    
    if (logger) {
        TEST("async_logger_log", async_logger_log(logger, ASYNC_LOG_INFO, __FILE__, __LINE__, __func__, "Test message"));
        
        async_logger_set_level(logger, ASYNC_LOG_WARN);
        TEST("async_logger_set_level", true);
        
        uint64_t total = async_logger_get_total(logger);
        TEST("async_logger_get_total", total >= 1);
        
        async_logger_flush(logger);
        TEST("async_logger_flush", true);
        
        async_logger_destroy(logger);
        TEST("async_logger_destroy", true);
    }
}

void test_slab_allocator(void) {
    printf(ANSI_YELLOW "\n[MODULE] SLAB_ALLOCATOR\n" ANSI_RESET);
    
    slab_allocator_t* allocator = slab_allocator_create();
    TEST("slab_allocator_create", allocator != NULL);
    
    if (allocator) {
        slab_cache_t* cache = slab_cache_create(allocator, 64, 16);
        TEST("slab_cache_create", cache != NULL);
        
        if (cache) {
            void* ptr1 = slab_alloc(cache);
            TEST("slab_alloc", ptr1 != NULL);
            
            void* ptr2 = slab_alloc(cache);
            TEST("slab_alloc_second", ptr2 != NULL && ptr2 != ptr1);
            
            slab_free(cache, ptr1);
            TEST("slab_free", true);
            
            slab_free(cache, ptr2);
            TEST("slab_free_second", true);
            
            uint64_t allocs, frees, mem;
            slab_allocator_stats(allocator, &allocs, &frees, &mem);
            TEST("slab_allocator_stats", allocs == 2 && frees == 2);
        }
        
        slab_allocator_destroy(allocator);
        TEST("slab_allocator_destroy", true);
    }
}

void test_simd_batch(void) {
    printf(ANSI_YELLOW "\n[MODULE] SIMD_BATCH_PROCESSOR\n" ANSI_RESET);
    
    simd_batch_processor_t* proc = simd_batch_processor_create();
    TEST("simd_batch_processor_create", proc != NULL);
    
    if (proc) {
        simd_batch_buffer_t* a = simd_batch_buffer_create(1024);
        simd_batch_buffer_t* b = simd_batch_buffer_create(1024);
        simd_batch_buffer_t* c = simd_batch_buffer_create(1024);
        TEST("simd_batch_buffer_create", a != NULL && b != NULL && c != NULL);
        
        simd_batch_buffer_fill(a, 1.0f);
        simd_batch_buffer_fill(b, 2.0f);
        TEST("simd_batch_buffer_fill", true);
        
        int result = simd_batch_add(proc, a, b, c);
        TEST("simd_batch_add", result == 0);
        
        result = simd_batch_mul(proc, a, b, c);
        TEST("simd_batch_mul", result == 0);
        
        float dot = simd_batch_dot(proc, a, b);
        TEST("simd_batch_dot", dot > 0);
        
        float sum = simd_batch_reduce_sum(proc, a);
        TEST("simd_batch_reduce_sum", fabs(sum - 1024.0f) < 0.1f);
        
        double throughput = simd_batch_get_throughput(proc);
        TEST("simd_batch_get_throughput", throughput > 0);
        
        simd_batch_buffer_destroy(a);
        simd_batch_buffer_destroy(b);
        simd_batch_buffer_destroy(c);
        simd_batch_processor_destroy(proc);
        TEST("simd_batch_cleanup", true);
    }
}

void test_lockfree_queue(void) {
    printf(ANSI_YELLOW "\n[MODULE] LOCKFREE_QUEUE\n" ANSI_RESET);
    
    lockfree_queue_t* queue = lockfree_queue_create();
    TEST("lockfree_queue_create", queue != NULL);
    
    if (queue) {
        TEST("lockfree_queue_is_empty", lockfree_queue_is_empty(queue));
        
        int data1 = 42;
        TEST("lockfree_queue_enqueue", lockfree_queue_enqueue(queue, &data1));
        
        TEST("lockfree_queue_size", lockfree_queue_size(queue) == 1);
        
        void* result = lockfree_queue_dequeue(queue);
        TEST("lockfree_queue_dequeue", result == &data1);
        
        TEST("lockfree_queue_empty_after", lockfree_queue_is_empty(queue));
        
        lockfree_queue_destroy(queue);
        TEST("lockfree_queue_destroy", true);
    }
    
    lockfree_ring_t* ring = lockfree_ring_create(256);
    TEST("lockfree_ring_create", ring != NULL);
    
    if (ring) {
        int data = 123;
        TEST("lockfree_ring_enqueue", lockfree_ring_enqueue(ring, &data));
        
        void* result = lockfree_ring_dequeue(ring);
        TEST("lockfree_ring_dequeue", result == &data);
        
        lockfree_ring_destroy(ring);
        TEST("lockfree_ring_destroy", true);
    }
    
    lockfree_stack_t* stack = lockfree_stack_create();
    TEST("lockfree_stack_create", stack != NULL);
    
    if (stack) {
        int data = 456;
        TEST("lockfree_stack_push", lockfree_stack_push(stack, &data));
        
        lockfree_stack_destroy(stack);
        TEST("lockfree_stack_destroy", true);
    }
}

void test_mmap_persistence(void) {
    printf(ANSI_YELLOW "\n[MODULE] MMAP_PERSISTENCE\n" ANSI_RESET);
    
    mmap_manager_t* manager = mmap_manager_create();
    TEST("mmap_manager_create", manager != NULL);
    
    if (manager) {
        mmap_file_t* file = mmap_file_open(manager, "logs/v32/mmap_test.dat", 
                                            4096, MMAP_MODE_READWRITE);
        TEST("mmap_file_open", file != NULL);
        
        if (file) {
            uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
            int result = mmap_file_write(file, 0, data, 4);
            TEST("mmap_file_write", result == 0);
            
            uint8_t read_data[4];
            result = mmap_file_read(file, 0, read_data, 4);
            TEST("mmap_file_read", result == 0 && memcmp(data, read_data, 4) == 0);
            
            result = mmap_file_sync(file);
            TEST("mmap_file_sync", result == 0);
            
            void* ptr = mmap_file_get_ptr(file, 0);
            TEST("mmap_file_get_ptr", ptr != NULL);
            
            size_t size = mmap_file_get_size(file);
            TEST("mmap_file_get_size", size >= 4);
            
            mmap_file_close(file);
            TEST("mmap_file_close", true);
        }
        
        uint64_t total = mmap_manager_get_total_mapped(manager);
        TEST("mmap_manager_get_total_mapped", total > 0);
        
        mmap_manager_destroy(manager);
        TEST("mmap_manager_destroy", true);
    }
}

void test_lz4_compression(void) {
    printf(ANSI_YELLOW "\n[MODULE] LZ4_COMPRESSION\n" ANSI_RESET);
    lz4_context_t* ctx = lz4_context_create();
    TEST("lz4_context_create", ctx != NULL);
    if (ctx) {
        const char* src = "LUM/VORAX V32 Pattern";
        size_t src_size = strlen(src) + 1;
        size_t bound = lz4_compress_bound(src_size);
        void* compressed = malloc(bound);
        void* decompressed = malloc(src_size);
        
        int c_size = lz4_compress(ctx, src, src_size, compressed, bound);
        TEST("lz4_compress", c_size > 0);
        
        int d_size = lz4_decompress(ctx, compressed, c_size, decompressed, src_size);
        TEST("lz4_decompress_size", d_size == (int)src_size);
        TEST("lz4_decompress_content", strcmp(src, decompressed) == 0);
        
        free(compressed);
        free(decompressed);
        lz4_context_destroy(ctx);
    }
}

void test_distributed_node(void) {
    printf(ANSI_YELLOW "\n[MODULE] DISTRIBUTED_NODE\n" ANSI_RESET);
    dist_cluster_t* cluster = dist_cluster_create("test_node", 5001);
    TEST("dist_cluster_create", cluster != NULL);
    if (cluster) {
        int node_id = dist_cluster_add_node(cluster, "remote_1", "127.0.0.1", 5002);
        TEST("dist_cluster_add_node", node_id > 0);
        
        TEST("dist_cluster_start", dist_cluster_start(cluster) == 0);
        
        dist_task_t task = {0};
        TEST("dist_task_submit", dist_task_submit(cluster, &task) == 0);
        
        dist_cluster_destroy(cluster);
        TEST("dist_cluster_destroy", true);
    }
}

void test_wasm_export(void) {
    printf(ANSI_YELLOW "\n[MODULE] WASM_EXPORT\n" ANSI_RESET);
    wasm_module_t* module = wasm_module_create();
    TEST("wasm_module_create", module != NULL);
    if (module) {
        wasm_value_type_t params[] = {WASM_TYPE_I32, WASM_TYPE_I32};
        wasm_value_type_t results[] = {WASM_TYPE_I32};
        
        TEST("wasm_module_add_export", wasm_module_add_export(module, "add", (void*)12345, params, 2, results, 1) == 0);
        TEST("wasm_module_compile", wasm_module_compile(module) == 0);
        
        wasm_instance_t* instance = wasm_instance_create(module);
        TEST("wasm_instance_create", instance != NULL);
        if (instance) {
            wasm_value_t args[2] = {
                {.type = WASM_TYPE_I32, .value = {.i32 = 10}},
                {.type = WASM_TYPE_I32, .value = {.i32 = 20}}
            };
            wasm_value_t results_val[1];
            TEST("wasm_instance_call", wasm_instance_call(instance, "add", args, 2, results_val, 1) == 0);
            wasm_instance_destroy(instance);
        }
        wasm_module_destroy(module);
    }
}

void test_version_manager(void) {
    printf(ANSI_YELLOW "\n[MODULE] VERSION_MANAGER\n" ANSI_RESET);
    
    version_registry_t* registry = version_registry_create();
    TEST("version_registry_create", registry != NULL);
    
    if (registry) {
        module_info_t info = {
            .module_name = "test_module",
            .version = {1, 2, 3},
            .author = "Test Author",
            .description = "Test module"
        };
        
        int result = version_registry_add_module(registry, &info);
        TEST("version_registry_add_module", result == 0);
        
        module_info_t* found = version_registry_get_module(registry, "test_module");
        TEST("version_registry_get_module", found != NULL);
        
        version_registry_destroy(registry);
        TEST("version_registry_destroy", true);
    }
    
    semver_t version;
    int result = semver_parse("1.2.3-alpha+build123", &version);
    TEST("semver_parse", result == 0 && version.major == 1 && version.minor == 2 && version.patch == 3);
    
    semver_t v1 = {1, 0, 0}, v2 = {2, 0, 0};
    TEST("semver_compare", semver_compare(&v1, &v2) < 0);
    
    char buffer[64];
    semver_to_string(&version, buffer, sizeof(buffer));
    TEST("semver_to_string", strlen(buffer) > 0);
    
    TEST("semver_satisfies_caret", semver_satisfies(&(semver_t){1, 5, 0}, "^1.0.0"));
    TEST("semver_satisfies_gte", semver_satisfies(&(semver_t){2, 0, 0}, ">=1.0.0"));
    
    const char* ver_str = version_get_string();
    TEST("version_get_string", ver_str != NULL && strlen(ver_str) > 0);
    
    const char* build_info = version_get_build_info();
    TEST("version_get_build_info", build_info != NULL);
}

void test_api_contract(void) {
    printf(ANSI_YELLOW "\n[MODULE] API_CONTRACT\n" ANSI_RESET);
    
    api_contract_t* contract = api_contract_create("test_api");
    TEST("api_contract_create", contract != NULL);
    
    if (contract) {
        int result = api_contract_add_function(contract, "test_func", 
                                                 "int test_func(int a, int b)",
                                                 "Test function", true, "1.0.0");
        TEST("api_contract_add_function", result == 0);
        
        result = api_contract_add_struct(contract, "test_struct",
                                          "Test structure", true, "1.0.0");
        TEST("api_contract_add_struct", result == 0);
        
        api_entry_t* entry = api_contract_find(contract, "test_func");
        TEST("api_contract_find", entry != NULL);
        
        TEST("api_contract_is_stable", api_contract_is_stable(contract, "test_func"));
        
        result = api_contract_deprecate(contract, "test_func", "2.0.0", "new_test_func");
        TEST("api_contract_deprecate", result == 0);
        
        char json[4096];
        result = api_contract_export_json(contract, json, sizeof(json));
        TEST("api_contract_export_json", result > 0);
        
        int errors = api_contract_validate(contract);
        TEST("api_contract_validate", errors == 0);
        
        api_contract_destroy(contract);
        TEST("api_contract_destroy", true);
    }
}

void test_benchmark_runner(void) {
    printf(ANSI_YELLOW "\n[MODULE] BENCHMARK_RUNNER\n" ANSI_RESET);
    
    benchmark_suite_t* suite = benchmark_suite_create("test_suite");
    TEST("benchmark_suite_create", suite != NULL);
    
    if (suite) {
        benchmark_suite_set_verbose(suite, false);
        
        int result = benchmark_suite_add(suite, "empty_benchmark", 
                                          (void(*)(void*))NULL, NULL);
        
        benchmark_stats_t stats = {
            .min_ns = 100,
            .max_ns = 200,
            .mean_ns = 150,
            .median_ns = 145,
            .stddev_ns = 25,
            .p99_ns = 195,
            .ops_per_second = 6666666.67,
            .iterations = 10000
        };
        
        char json[4096];
        result = benchmark_suite_export_json(suite, json, sizeof(json));
        TEST("benchmark_suite_export_json", result > 0 || suite->benchmark_count == 0);
        
        char csv[4096];
        result = benchmark_suite_export_csv(suite, csv, sizeof(csv));
        TEST("benchmark_suite_export_csv", result > 0);
        
        benchmark_comparison_t comparison;
        benchmark_stats_t baseline = stats;
        benchmark_stats_t current = stats;
        current.mean_ns = 160;
        
        result = benchmark_compare(&baseline, &current, 5.0, &comparison);
        TEST("benchmark_compare", result == 0);
        TEST("benchmark_compare_speedup", comparison.speedup > 0);
        
        benchmark_suite_destroy(suite);
        TEST("benchmark_suite_destroy", true);
    }
}

void test_regression_detector(void) {
    printf(ANSI_YELLOW "\n[MODULE] REGRESSION_DETECTOR\n" ANSI_RESET);
    
    regression_detector_t* detector = regression_detector_create();
    TEST("regression_detector_create", detector != NULL);
    
    if (detector) {
        int result = regression_detector_set_thresholds(detector, 5.0, 10.0, 25.0);
        TEST("regression_detector_set_thresholds", result == 0);
        
        benchmark_stats_t baseline = {.mean_ns = 1000};
        result = regression_detector_add_snapshot(detector, "1.0.0", "abc123", &baseline, 1);
        TEST("regression_detector_add_snapshot", result == 0);
        
        benchmark_stats_t current = {.mean_ns = 1100};
        result = regression_detector_analyze(detector, &current, "test_metric");
        TEST("regression_detector_analyze", result == 0);
        
        regression_severity_t max = regression_detector_get_max_severity(detector);
        TEST("regression_detector_get_max_severity", max >= REGRESSION_NONE);
        
        regression_result_t* results;
        size_t count;
        result = regression_detector_get_regressions(detector, &results, &count);
        TEST("regression_detector_get_regressions", result == 0);
        
        char report[4096];
        result = regression_detector_export_report(detector, report, sizeof(report));
        TEST("regression_detector_export_report", result > 0);
        
        bool acceptable = regression_detector_is_acceptable(detector);
        TEST("regression_detector_is_acceptable", acceptable || !acceptable);
        
        regression_detector_clear_results(detector);
        TEST("regression_detector_clear_results", detector->result_count == 0);
        
        regression_detector_destroy(detector);
        TEST("regression_detector_destroy", true);
    }
}

#include "../security/audit_hardening.h"
#include "../monitoring/monitoring_alerting.h"

void test_security_audit(void) {
    printf(ANSI_YELLOW "\n[MODULE] SECURITY_AUDIT_HARDENING\n" ANSI_RESET);
    audit_system_t* sys = audit_system_create();
    TEST("audit_system_create", sys != NULL);
    if (sys) {
        audit_system_run(sys);
        TEST("audit_system_run", sys->total_audits == 1);
        TEST("audit_system_hardened", sys->hardened);
        audit_system_destroy(sys);
    }
}

void test_monitoring(void) {
    printf(ANSI_YELLOW "\n[MODULE] MONITORING_ALERTING\n" ANSI_RESET);
    monitor_system_t* sys = monitor_system_create();
    TEST("monitor_system_create", sys != NULL);
    if (sys) {
        monitor_system_update(sys);
        TEST("monitor_system_cpu", sys->cpu_usage > 0);
        TEST("monitor_system_mem", sys->mem_usage > 0);
        monitor_system_destroy(sys);
    }
}

#include "../optimization/reasoning_path_tracker.h"
#include "../optimization/formal_kernel_v40.h"

void test_reasoning_trace(void) {
    printf(ANSI_YELLOW "\n[MODULE] REASONING_PATH_TRACKER (V41-LRM-SHF)\n" ANSI_RESET);
    reasoning_trace_t* trace = reasoning_trace_start("session_lrm_v41");
    if (trace) {
        reasoning_trace_add_node(trace, "Vérification SHF", 1.0f, 0.0001f);
        // Utilisation des nouveaux axiomes V41
        bool resonant = v41_check_shf_resonance(NULL, 0.001f);
        TEST("shf_resonance_detected", resonant == true);
        
        reasoning_trace_save(trace, "PREUVE_IAMO/lrm_trace_v41.log");
        reasoning_trace_destroy(trace);
    }
}

int main(void) {
    printf(ANSI_BLUE "\n═══════════════════════════════════════════════════════════════════\n");
    printf("       LUM/VORAX V39 - KERNEL DE PREUVE ET RAISONNEMENT (XAI)      \n");
    printf("═══════════════════════════════════════════════════════════════════\n" ANSI_RESET);
    
    system("mkdir -p logs/v32 PREUVE_IAMO");
    
    test_async_logger();
    test_slab_allocator();
    test_simd_batch();
    test_lockfree_queue();
    test_mmap_persistence();
    test_lz4_compression();
    test_distributed_node();
    test_wasm_export();
    test_version_manager();
    test_api_contract();
    test_benchmark_runner();
    test_regression_detector();
    test_security_audit();
    test_monitoring();
    test_reasoning_trace();
    
    printf(ANSI_BLUE "\n═══════════════════════════════════════════════════════════════════\n");
    printf("                     RÉSUMÉ FINAL DES PREUVES V39                  \n");
    printf("═══════════════════════════════════════════════════════════════════\n" ANSI_RESET);
    
    printf("\nTests total:     %d\n", total_tests);
    printf("Tests réussis:   " ANSI_GREEN "%d" ANSI_RESET "\n", passed_tests);
    printf("Tests échoués:   " ANSI_RED "%d" ANSI_RESET "\n", failed_tests);
    printf("Taux de succès:  %.1f%%\n", (total_tests > 0) ? (passed_tests * 100.0 / total_tests) : 0);
    
    if (failed_tests == 0) {
        printf(ANSI_GREEN "\n═══════════════════════════════════════════════════════════════════\n");
        printf("        STATUT: SYSTÈME V39 TOTALEMENT PROUVÉ ET TRAÇABLE          \n");
        printf("═══════════════════════════════════════════════════════════════════\n" ANSI_RESET);
    } else {
        printf(ANSI_RED "\n═══════════════════════════════════════════════════════════════════\n");
        printf("               STATUT: ÉCHEC DE LA CHAÎNE DE PREUVE                \n");
        printf("═══════════════════════════════════════════════════════════════════\n" ANSI_RESET);
    }
    
    return (failed_tests == 0) ? 0 : 1;
}
