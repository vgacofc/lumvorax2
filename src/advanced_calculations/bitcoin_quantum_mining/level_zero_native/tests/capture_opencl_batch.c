#include <stdio.h>
#include <CL/cl.h>

const char *kernel_src = "__kernel void test(__global uint *out) { out[0] = 0xDEADBEEF; }";

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer;
    cl_int err;
    
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device, 0, &err);
    
    program = clCreateProgramWithSource(context, 1, &kernel_src, NULL, &err);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "test", &err);
    
    buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    
    size_t global = 1;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    clFinish(queue);
    
    printf("OpenCL kernel executed\n");
    
    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}
