// Test C542 - Replay batch OpenCL exact avec notre output buffer
// ROOT CAUSE #53: GPU HANG détecté - batch malformé
// Solution: Utiliser batch OpenCL fonctionnel comme référence

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <i915_drm.h>

const char *kernel_source = 
"__kernel void test_write(__global uint *output) {\n"
"    output[0] = 0x12345678;\n"
"}\n";

int main() {
    printf("=== TEST C542 - REPLAY BATCH OPENCL ===\n");
    printf("Objectif: Capturer batch OpenCL et le rejouer avec i915 natif\n\n");
    
    // 1. Exécuter OpenCL pour obtenir un batch fonctionnel
    printf("Phase 1: Exécution OpenCL de référence...\n");
    
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
    
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, NULL);
    
    uint32_t init_data[1024];
    memset(init_data, 0xFF, 4096);
    clEnqueueWriteBuffer(queue, output_buf, CL_TRUE, 0, 4096, init_data, 0, NULL, NULL);
    
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, NULL);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    cl_kernel kernel = clCreateKernel(program, "test_write", NULL);
    
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    
    size_t global_size = 1;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    clFinish(queue);
    
    uint32_t result[2];
    clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, 8, result, 0, NULL, NULL);
    
    printf("✓ OpenCL: output[0] = 0x%08x\n", result[0]);
    
    if (result[0] != 0x12345678) {
        printf("❌ OpenCL a échoué!\n");
        return 1;
    }
    
    // Cleanup OpenCL
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(output_buf);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n=== ANALYSE ===\n");
    printf("OpenCL fonctionne correctement.\n");
    printf("Pour capturer le batch exact, utiliser:\n");
    printf("  strace -e ioctl -s 65536 ./test_c542_replay_opencl_batch 2>&1 | grep EXECBUFFER2\n");
    printf("\nProchaine étape C543:\n");
    printf("1. Capturer batch OpenCL avec strace\n");
    printf("2. Extraire les données binaires du batch\n");
    printf("3. Rejouer avec i915 natif\n");
    
    return 0;
}

// Made with Bob
