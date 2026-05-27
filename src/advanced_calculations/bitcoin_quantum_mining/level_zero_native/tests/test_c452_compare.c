#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

int main() {
    const char *source = 
        "__kernel void write_test(__global uint* output)\n"
        "{\n"
        "    int gid = get_global_id(0);\n"
        "    output[gid] = 0xAABBCCDD;\n"
        "}\n";
    
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
    
    cl_program program = clCreateProgramWithSource(context, 1, &source, NULL, NULL);
    clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    
    cl_kernel kernel = clCreateKernel(program, "write_test", NULL);
    
    // Get binary
    size_t bin_size;
    clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &bin_size, NULL);
    
    unsigned char *binary = malloc(bin_size);
    clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), &binary, NULL);
    
    FILE *f = fopen("test_c452_opencl_binary.bin", "wb");
    fwrite(binary, 1, bin_size, f);
    fclose(f);
    
    printf("✅ OpenCL binary saved: %zu bytes\n", bin_size);
    
    free(binary);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}
