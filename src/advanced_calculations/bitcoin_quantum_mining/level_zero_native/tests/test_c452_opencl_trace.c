#include <stdio.h>
#include <CL/cl.h>

int main() {
    const char *source = 
        "__kernel void write_test(__global uint* output) {\n"
        "    int gid = get_global_id(0);\n"
        "    output[gid] = 0xAABBCCDD;\n"
        "}\n";
    
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(ctx, device, 0, NULL);
    
    cl_program prog = clCreateProgramWithSource(ctx, 1, &source, NULL, NULL);
    clBuildProgram(prog, 1, &device, NULL, NULL, NULL);
    
    cl_kernel kernel = clCreateKernel(prog, "write_test", NULL);
    
    cl_mem output = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, 1028, NULL, NULL);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output);
    
    size_t global = 256;
    printf("🚀 Enqueuing kernel...\n");
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    clFinish(queue);
    printf("✅ Kernel executed\n");
    
    uint32_t result[257];
    clEnqueueReadBuffer(queue, output, CL_TRUE, 0, 1028, result, 0, NULL, NULL);
    
    printf("📊 output[0] = 0x%08X\n", result[0]);
    printf("📊 output[255] = 0x%08X\n", result[255]);
    
    clReleaseMemObject(output);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
    
    return 0;
}
