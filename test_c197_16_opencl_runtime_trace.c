// CYCLE C197.16 - ANALYSE FORENSIQUE OPENCL RUNTIME
// Objectif : Tracer toutes opérations OpenCL runtime pour identifier ce qui manque dans DRM custom
// Approche : Programme minimal OpenCL + strace/ltrace pour capture complète

#define _GNU_SOURCE
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

// OpenCL kernel source (identique C197.13-C197.15)
const char* kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"}\n";

// Forensic logging
#define MAX_EVENTS 200
typedef struct {
    uint64_t timestamp_ns;
    const char* phase;
    const char* event;
    uint64_t value;
} forensic_event_t;

static forensic_event_t g_events[MAX_EVENTS];
static size_t g_event_count = 0;

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_EVENT(phase_name, event_name, val) do { \
    if (g_event_count < MAX_EVENTS) { \
        g_events[g_event_count].timestamp_ns = get_timestamp_ns(); \
        g_events[g_event_count].phase = (phase_name); \
        g_events[g_event_count].event = (event_name); \
        g_events[g_event_count].value = (uint64_t)(val); \
        g_event_count++; \
    } \
} while(0)

int main(void) {
    LOG_EVENT("INIT", "test_start", 0);
    
    printf("=== CYCLE C197.16 - OPENCL RUNTIME TRACE ===\n\n");
    printf("NOTE: Run with strace/ltrace to capture all syscalls/library calls\n\n");
    
    // Phase 1: Get platform
    printf("[PHASE 1] Getting OpenCL platform...\n");
    LOG_EVENT("OPENCL", "get_platform_start", 0);
    
    cl_platform_id platform;
    cl_int err = clGetPlatformIDs(1, &platform, NULL);
    LOG_EVENT("OPENCL", "platform_id", (uint64_t)platform);
    LOG_EVENT("OPENCL", "platform_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    // Get platform info
    char platform_name[256];
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
    LOG_EVENT("OPENCL", "platform_name_len", strlen(platform_name));
    printf("[SUCCESS] Platform: %s\n\n", platform_name);
    
    // Phase 2: Get device
    printf("[PHASE 2] Getting GPU device...\n");
    LOG_EVENT("OPENCL", "get_device_start", 0);
    
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    LOG_EVENT("OPENCL", "device_id", (uint64_t)device);
    LOG_EVENT("OPENCL", "device_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    // Get device info
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    LOG_EVENT("OPENCL", "device_name_len", strlen(device_name));
    printf("[SUCCESS] Device: %s\n\n", device_name);
    
    // Phase 3: Create context
    printf("[PHASE 3] Creating OpenCL context...\n");
    LOG_EVENT("OPENCL", "create_context_start", 0);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    LOG_EVENT("OPENCL", "context_id", (uint64_t)context);
    LOG_EVENT("OPENCL", "context_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateContext failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Context created\n\n");
    
    // Phase 4: Create command queue
    printf("[PHASE 4] Creating command queue...\n");
    LOG_EVENT("OPENCL", "create_queue_start", 0);
    
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    LOG_EVENT("OPENCL", "queue_id", (uint64_t)queue);
    LOG_EVENT("OPENCL", "queue_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateCommandQueue failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Command queue created\n\n");
    
    // Phase 5: Create program
    printf("[PHASE 5] Creating program from source...\n");
    LOG_EVENT("OPENCL", "create_program_start", 0);
    
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    LOG_EVENT("OPENCL", "program_id", (uint64_t)program);
    LOG_EVENT("OPENCL", "program_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Program created\n\n");
    
    // Phase 6: Build program
    printf("[PHASE 6] Building program...\n");
    LOG_EVENT("OPENCL", "build_program_start", 0);
    
    uint64_t build_start = get_timestamp_ns();
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    uint64_t build_time = get_timestamp_ns() - build_start;
    
    LOG_EVENT("OPENCL", "build_error", err);
    LOG_EVENT("OPENCL", "build_time_ns", build_time);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clBuildProgram failed: %d\n", err);
        
        // Get build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    
    printf("[SUCCESS] Program built (%.3f ms)\n\n", build_time / 1000000.0);
    
    // Phase 7: Create kernel
    printf("[PHASE 7] Creating kernel...\n");
    LOG_EVENT("OPENCL", "create_kernel_start", 0);
    
    cl_kernel kernel = clCreateKernel(program, "test_write", &err);
    LOG_EVENT("OPENCL", "kernel_id", (uint64_t)kernel);
    LOG_EVENT("OPENCL", "kernel_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateKernel failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Kernel created\n\n");
    
    // Phase 8: Create output buffer
    printf("[PHASE 8] Creating output buffer...\n");
    LOG_EVENT("OPENCL", "create_buffer_start", 0);
    
    cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, &err);
    LOG_EVENT("OPENCL", "buffer_id", (uint64_t)output_buffer);
    LOG_EVENT("OPENCL", "buffer_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateBuffer failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Output buffer created (4096 bytes)\n\n");
    
    // Phase 9: Set kernel argument
    printf("[PHASE 9] Setting kernel argument...\n");
    LOG_EVENT("OPENCL", "set_arg_start", 0);
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    LOG_EVENT("OPENCL", "set_arg_error", err);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clSetKernelArg failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Kernel argument set\n\n");
    
    // Phase 10: Enqueue kernel
    printf("[PHASE 10] Enqueuing kernel...\n");
    LOG_EVENT("OPENCL", "enqueue_kernel_start", 0);
    
    size_t global_work_size = 1;
    uint64_t enqueue_start = get_timestamp_ns();
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    uint64_t enqueue_time = get_timestamp_ns() - enqueue_start;
    
    LOG_EVENT("OPENCL", "enqueue_error", err);
    LOG_EVENT("OPENCL", "enqueue_time_ns", enqueue_time);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clEnqueueNDRangeKernel failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Kernel enqueued (%.3f ms)\n\n", enqueue_time / 1000000.0);
    
    // Phase 11: Wait for completion
    printf("[PHASE 11] Waiting for completion...\n");
    LOG_EVENT("OPENCL", "finish_start", 0);
    
    uint64_t finish_start = get_timestamp_ns();
    err = clFinish(queue);
    uint64_t finish_time = get_timestamp_ns() - finish_start;
    
    LOG_EVENT("OPENCL", "finish_error", err);
    LOG_EVENT("OPENCL", "finish_time_ns", finish_time);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clFinish failed: %d\n", err);
        return 1;
    }
    
    printf("[SUCCESS] Kernel completed (%.3f ms)\n\n", finish_time / 1000000.0);
    
    // Phase 12: Read output
    printf("[PHASE 12] Reading output buffer...\n");
    LOG_EVENT("OPENCL", "read_buffer_start", 0);
    
    uint32_t result[1];
    uint64_t read_start = get_timestamp_ns();
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(uint32_t), result, 0, NULL, NULL);
    uint64_t read_time = get_timestamp_ns() - read_start;
    
    LOG_EVENT("OPENCL", "read_error", err);
    LOG_EVENT("OPENCL", "read_time_ns", read_time);
    LOG_EVENT("OPENCL", "output_value", result[0]);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clEnqueueReadBuffer failed: %d\n", err);
        return 1;
    }
    
    printf("[RESULT] output[0] = 0x%08X (%.3f ms)\n\n", result[0], read_time / 1000000.0);
    
    // Phase 13: Verification
    printf("[PHASE 13] Verification...\n");
    if (result[0] == 0xDEADBEEF) {
        printf("✅ VERIFICATION SUCCESS: output[0] = 0xDEADBEEF\n\n");
        LOG_EVENT("VERIFY", "success", 1);
    } else {
        printf("❌ VERIFICATION FAILED: output[0] = 0x%08X (expected 0xDEADBEEF)\n\n", result[0]);
        LOG_EVENT("VERIFY", "success", 0);
    }
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    uint64_t total_time = get_timestamp_ns() - g_events[0].timestamp_ns;
    LOG_EVENT("INIT", "test_end", total_time);
    
    // Save forensic log
    FILE* log_file = fopen("c197_16_forensic_opencl_trace.csv", "w");
    fprintf(log_file, "timestamp_ns,phase,event,value\n");
    for (size_t i = 0; i < g_event_count; i++) {
        fprintf(log_file, "%lu,%s,%s,%lu\n",
                g_events[i].timestamp_ns,
                g_events[i].phase,
                g_events[i].event,
                g_events[i].value);
    }
    fclose(log_file);
    
    printf("=== FORENSIC REPORT C197.16 ===\n\n");
    printf("Total execution time: %.3f ms\n", total_time / 1000000.0);
    printf("Total events captured: %zu\n\n", g_event_count);
    printf("Forensic log saved: c197_16_forensic_opencl_trace.csv\n\n");
    printf("NOTE: Analyze strace/ltrace output to identify missing DRM operations\n");
    
    return (result[0] == 0xDEADBEEF) ? 0 : 1;
}

// Made with Bob