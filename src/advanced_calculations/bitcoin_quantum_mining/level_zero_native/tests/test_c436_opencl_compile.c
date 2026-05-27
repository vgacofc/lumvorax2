#include <stdio.h>
#include <CL/cl.h>

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    cl_int err;
    
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    
    const char* source = 
        "__kernel void test_minimal_write(__global unsigned int* output) {\n"
        "    if (get_global_id(0) == 0) {\n"
        "        output[0] = 0x12345678;\n"
        "    }\n"
        "}\n";
    
    size_t source_size = strlen(source);
    program = clCreateProgramWithSource(context, 1, &source, &source_size, &err);
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        printf("Build failed:\n%s\n", log);
        return 1;
    }
    
    size_t binary_size;
    clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    
    unsigned char* binary = malloc(binary_size);
    unsigned char* binaries[1] = { binary };
    clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    
    FILE* out = fopen("tests/test_c436_opencl.bin", "wb");
    fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    printf("✅ Kernel compiled: %zu bytes\n", binary_size);
    printf("✅ Saved to: tests/test_c436_opencl.bin\n");
    
    free(binary);
    clReleaseProgram(program);
    clReleaseContext(context);
    return 0;
}
