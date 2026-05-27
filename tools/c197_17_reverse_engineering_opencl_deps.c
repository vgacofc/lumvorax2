// CYCLE C197.17 - REVERSE ENGINEERING COMPLET DÉPENDANCES OPENCL
// Objectif : Analyser libigdrcl.so + libigdgmm.so avec logging LumVorax bit-level
// Approche : dlopen + dlsym + hook malloc/mmap + trace TOUTES opérations

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <CL/cl.h>

// ═══════════════════════════════════════════════════════════════════════════
// LUMVORAX BIT-LEVEL LOGGING + MEMORY TRACKER
// ═══════════════════════════════════════════════════════════════════════════

#define MAX_EVENTS 100000
#define MAX_MEMORY_ALLOCS 10000

typedef struct {
    uint64_t timestamp_ns;
    const char* phase;
    const char* event;
    uint64_t value;
    void* ptr;
    size_t size;
} lumvorax_event_t;

typedef struct {
    void* ptr;
    size_t size;
    uint64_t timestamp_alloc;
    uint64_t timestamp_free;
    const char* source;  // "malloc", "mmap", "dlopen"
    int active;
} lumvorax_memory_t;

static lumvorax_event_t g_events[MAX_EVENTS];
static size_t g_event_count = 0;

static lumvorax_memory_t g_memory[MAX_MEMORY_ALLOCS];
static size_t g_memory_count = 0;

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_EVENT(phase_name, event_name, val, ptr_val, size_val) do { \
    if (g_event_count < MAX_EVENTS) { \
        g_events[g_event_count].timestamp_ns = get_timestamp_ns(); \
        g_events[g_event_count].phase = (phase_name); \
        g_events[g_event_count].event = (event_name); \
        g_events[g_event_count].value = (uint64_t)(val); \
        g_events[g_event_count].ptr = (ptr_val); \
        g_events[g_event_count].size = (size_val); \
        g_event_count++; \
    } \
} while(0)

static void track_memory_alloc(void* ptr, size_t size, const char* source) {
    if (g_memory_count < MAX_MEMORY_ALLOCS) {
        g_memory[g_memory_count].ptr = ptr;
        g_memory[g_memory_count].size = size;
        g_memory[g_memory_count].timestamp_alloc = get_timestamp_ns();
        g_memory[g_memory_count].timestamp_free = 0;
        g_memory[g_memory_count].source = source;
        g_memory[g_memory_count].active = 1;
        g_memory_count++;
    }
    LOG_EVENT("MEMORY", "alloc", size, ptr, size);
}

static void track_memory_free(void* ptr, const char* source) {
    for (size_t i = 0; i < g_memory_count; i++) {
        if (g_memory[i].ptr == ptr && g_memory[i].active) {
            g_memory[i].timestamp_free = get_timestamp_ns();
            g_memory[i].active = 0;
            LOG_EVENT("MEMORY", "free", g_memory[i].size, ptr, 0);
            return;
        }
    }
    LOG_EVENT("MEMORY", "free_unknown", 0, ptr, 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// MALLOC/MMAP HOOKS (LD_PRELOAD style)
// ═══════════════════════════════════════════════════════════════════════════

static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static void* (*real_mmap)(void*, size_t, int, int, int, off_t) = NULL;
static int (*real_munmap)(void*, size_t) = NULL;

static void init_hooks(void) {
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
        real_free = dlsym(RTLD_NEXT, "free");
        real_mmap = dlsym(RTLD_NEXT, "mmap");
        real_munmap = dlsym(RTLD_NEXT, "munmap");
    }
}

void* malloc(size_t size) {
    init_hooks();
    void* ptr = real_malloc(size);
    if (ptr) {
        track_memory_alloc(ptr, size, "malloc");
    }
    return ptr;
}

void free(void* ptr) {
    init_hooks();
    if (ptr) {
        track_memory_free(ptr, "free");
    }
    real_free(ptr);
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    init_hooks();
    void* ptr = real_mmap(addr, length, prot, flags, fd, offset);
    if (ptr != MAP_FAILED) {
        track_memory_alloc(ptr, length, "mmap");
    }
    return ptr;
}

int munmap(void* addr, size_t length) {
    init_hooks();
    track_memory_free(addr, "munmap");
    return real_munmap(addr, length);
}

// ═══════════════════════════════════════════════════════════════════════════
// REVERSE ENGINEERING LIBIGDRCL.SO
// ═══════════════════════════════════════════════════════════════════════════

static void analyze_libigdrcl(void) {
    printf("\n[PHASE 1] Analyzing libigdrcl.so (Intel Compute Runtime)...\n");
    LOG_EVENT("REVERSE", "libigdrcl_start", 0, NULL, 0);
    
    // Load library
    void* handle = dlopen("/usr/lib/x86_64-linux-gnu/intel-opencl/libigdrcl.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "[ERROR] Cannot load libigdrcl.so: %s\n", dlerror());
        return;
    }
    
    LOG_EVENT("REVERSE", "libigdrcl_loaded", (uint64_t)handle, handle, 0);
    printf("[SUCCESS] libigdrcl.so loaded at %p\n", handle);
    
    // Extract key symbols (OpenCL entry points)
    const char* symbols[] = {
        "clGetPlatformIDs",
        "clGetDeviceIDs",
        "clCreateContext",
        "clCreateCommandQueue",
        "clCreateProgramWithSource",
        "clBuildProgram",
        "clCreateKernel",
        "clCreateBuffer",
        "clSetKernelArg",
        "clEnqueueNDRangeKernel",
        "clFinish",
        "clEnqueueReadBuffer"
    };
    
    printf("\n[SYMBOLS] Extracting OpenCL entry points:\n");
    for (size_t i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
        void* sym = dlsym(handle, symbols[i]);
        if (sym) {
            printf("  ✅ %s @ %p\n", symbols[i], sym);
            LOG_EVENT("REVERSE", "symbol_found", (uint64_t)sym, sym, i);
        } else {
            printf("  ❌ %s NOT FOUND\n", symbols[i]);
            LOG_EVENT("REVERSE", "symbol_missing", 0, NULL, i);
        }
    }
    
    // Keep library loaded for OpenCL usage
    // dlclose(handle);  // Don't close, needed for OpenCL
    LOG_EVENT("REVERSE", "libigdrcl_analyzed", g_event_count, NULL, 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// REVERSE ENGINEERING LIBIGDGMM.SO
// ═══════════════════════════════════════════════════════════════════════════

static void analyze_libigdgmm(void) {
    printf("\n[PHASE 2] Analyzing libigdgmm.so (Intel Graphics Memory Manager)...\n");
    LOG_EVENT("REVERSE", "libigdgmm_start", 0, NULL, 0);
    
    // Load library
    void* handle = dlopen("/lib/x86_64-linux-gnu/libigdgmm.so.12", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "[ERROR] Cannot load libigdgmm.so: %s\n", dlerror());
        return;
    }
    
    LOG_EVENT("REVERSE", "libigdgmm_loaded", (uint64_t)handle, handle, 0);
    printf("[SUCCESS] libigdgmm.so loaded at %p\n", handle);
    
    // Extract key GMM symbols (memory management)
    const char* symbols[] = {
        "GmmInitGlobalContext",
        "GmmDestroyGlobalContext",
        "GmmCreateSingletonContext",
        "GmmDestroySingletonContext"
    };
    
    printf("\n[SYMBOLS] Extracting GMM entry points:\n");
    for (size_t i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
        void* sym = dlsym(handle, symbols[i]);
        if (sym) {
            printf("  ✅ %s @ %p\n", symbols[i], sym);
            LOG_EVENT("REVERSE", "gmm_symbol_found", (uint64_t)sym, sym, i);
        } else {
            printf("  ❌ %s NOT FOUND\n", symbols[i]);
            LOG_EVENT("REVERSE", "gmm_symbol_missing", 0, NULL, i);
        }
    }
    
    // Keep library loaded
    LOG_EVENT("REVERSE", "libigdgmm_analyzed", g_event_count, NULL, 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST OPENCL AVEC LOGGING LUMVORAX
// ═══════════════════════════════════════════════════════════════════════════

const char* kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"}\n";

static void test_opencl_with_logging(void) {
    printf("\n[PHASE 3] Testing OpenCL with LumVorax logging...\n");
    LOG_EVENT("OPENCL", "test_start", 0, NULL, 0);
    
    // Get platform
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    LOG_EVENT("OPENCL", "platform_id", (uint64_t)platform, platform, 0);
    
    // Get device
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    LOG_EVENT("OPENCL", "device_id", (uint64_t)device, device, 0);
    
    // Create context
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    LOG_EVENT("OPENCL", "context_created", (uint64_t)context, context, 0);
    
    // Create queue
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
    LOG_EVENT("OPENCL", "queue_created", (uint64_t)queue, queue, 0);
    
    // Create program
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, NULL);
    LOG_EVENT("OPENCL", "program_created", (uint64_t)program, program, 0);
    
    // Build program
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    LOG_EVENT("OPENCL", "program_built", 0, NULL, 0);
    
    // Create kernel
    cl_kernel kernel = clCreateKernel(program, "test_write", NULL);
    LOG_EVENT("OPENCL", "kernel_created", (uint64_t)kernel, kernel, 0);
    
    // Create buffer
    cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, NULL);
    LOG_EVENT("OPENCL", "buffer_created", (uint64_t)output_buffer, output_buffer, 4096);
    
    // Set argument
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    LOG_EVENT("OPENCL", "arg_set", 0, NULL, 0);
    
    // Enqueue kernel
    size_t global_work_size = 1;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    LOG_EVENT("OPENCL", "kernel_enqueued", 0, NULL, 0);
    
    // Wait
    clFinish(queue);
    LOG_EVENT("OPENCL", "kernel_finished", 0, NULL, 0);
    
    // Read result
    uint32_t result[1];
    clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(uint32_t), result, 0, NULL, NULL);
    LOG_EVENT("OPENCL", "result_read", result[0], NULL, 0);
    
    printf("[RESULT] output[0] = 0x%08X\n", result[0]);
    
    if (result[0] == 0xDEADBEEF) {
        printf("✅ VERIFICATION SUCCESS\n");
        LOG_EVENT("VERIFY", "success", 1, NULL, 0);
    } else {
        printf("❌ VERIFICATION FAILED\n");
        LOG_EVENT("VERIFY", "success", 0, NULL, 0);
    }
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    LOG_EVENT("OPENCL", "test_end", 0, NULL, 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN - REVERSE ENGINEERING COMPLET
// ═══════════════════════════════════════════════════════════════════════════

int main(void) {
    LOG_EVENT("INIT", "reverse_engineering_start", 0, NULL, 0);
    
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  CYCLE C197.17 - REVERSE ENGINEERING OPENCL DEPENDENCIES\n");
    printf("  LumVorax Bit-Level Logging + Memory Tracker ACTIF\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    // Phase 1: Analyze libigdrcl.so
    analyze_libigdrcl();
    
    // Phase 2: Analyze libigdgmm.so
    analyze_libigdgmm();
    
    // Phase 3: Test OpenCL with logging
    test_opencl_with_logging();
    
    uint64_t total_time = get_timestamp_ns() - g_events[0].timestamp_ns;
    LOG_EVENT("INIT", "reverse_engineering_end", total_time, NULL, 0);
    
    // Save forensic logs
    FILE* log_file = fopen("c197_17_forensic_reverse_engineering.csv", "w");
    fprintf(log_file, "timestamp_ns,phase,event,value,ptr,size\n");
    for (size_t i = 0; i < g_event_count; i++) {
        fprintf(log_file, "%lu,%s,%s,%lu,%p,%zu\n",
                g_events[i].timestamp_ns,
                g_events[i].phase,
                g_events[i].event,
                g_events[i].value,
                g_events[i].ptr,
                g_events[i].size);
    }
    fclose(log_file);
    
    // Save memory tracker report
    FILE* mem_file = fopen("c197_17_memory_tracker.csv", "w");
    fprintf(mem_file, "ptr,size,timestamp_alloc,timestamp_free,lifetime_ns,source,active\n");
    for (size_t i = 0; i < g_memory_count; i++) {
        uint64_t lifetime = g_memory[i].active ? 
            (get_timestamp_ns() - g_memory[i].timestamp_alloc) :
            (g_memory[i].timestamp_free - g_memory[i].timestamp_alloc);
        fprintf(mem_file, "%p,%zu,%lu,%lu,%lu,%s,%d\n",
                g_memory[i].ptr,
                g_memory[i].size,
                g_memory[i].timestamp_alloc,
                g_memory[i].timestamp_free,
                lifetime,
                g_memory[i].source,
                g_memory[i].active);
    }
    fclose(mem_file);
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("  FORENSIC REPORT C197.17\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Total execution time: %.3f ms\n", total_time / 1000000.0);
    printf("Total events captured: %zu\n", g_event_count);
    printf("Total memory allocations: %zu\n", g_memory_count);
    printf("\nLogs saved:\n");
    printf("  - c197_17_forensic_reverse_engineering.csv\n");
    printf("  - c197_17_memory_tracker.csv\n");
    
    return 0;
}

// Made with Bob - LumVorax Bit-Level Logging + Memory Tracker