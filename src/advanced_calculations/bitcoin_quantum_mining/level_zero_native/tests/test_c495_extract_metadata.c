/*
 * TEST C495: EXTRACTION METADATA KERNEL OPENCL
 * 
 * Objectif: Extraire toutes les informations de configuration du kernel
 * pour comprendre pourquoi il fonctionne avec OpenCL mais HANG avec i915
 */

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) \
    if (err != CL_SUCCESS) { \
        fprintf(stderr, "❌ %s: error %d\n", msg, err); \
        exit(1); \
    }

int main(void) {
    cl_int err;
    
    printf("=== C495: EXTRACTION METADATA KERNEL OPENCL ===\n\n");
    
    // 1. Initialisation OpenCL
    printf("[1/6] Initialisation OpenCL...\n");
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    printf("  ✓ Device GPU trouvé\n");
    
    // 2. Création contexte
    printf("\n[2/6] Création contexte...\n");
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    printf("  ✓ Contexte créé\n");
    
    // 3. Lecture kernel source
    printf("\n[3/6] Lecture kernel source...\n");
    FILE *f = fopen("tests/test_c495_kernel_minimal.cl", "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t source_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *source = malloc(source_size + 1);
    fread(source, 1, source_size, f);
    source[source_size] = '\0';
    fclose(f);
    printf("  ✓ Kernel source lu: %zu bytes\n", source_size);
    
    // 4. Compilation kernel
    printf("\n[4/6] Compilation kernel...\n");
    cl_program program = clCreateProgramWithSource(context, 1, 
        (const char**)&source, &source_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        CHECK_CL(err, "clBuildProgram");
    }
    printf("  ✓ Kernel compilé\n");
    
    // 5. Création kernel
    printf("\n[5/6] Création kernel...\n");
    cl_kernel kernel = clCreateKernel(program, "minimal_write", &err);
    CHECK_CL(err, "clCreateKernel");
    printf("  ✓ Kernel créé\n");
    
    // 6. Extraction metadata
    printf("\n[6/6] Extraction metadata kernel...\n\n");
    
    printf("=== METADATA KERNEL OPENCL ===\n\n");
    
    // Work Group Size
    size_t work_group_size;
    err = clGetKernelWorkGroupInfo(kernel, device, 
        CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &work_group_size, NULL);
    CHECK_CL(err, "CL_KERNEL_WORK_GROUP_SIZE");
    printf("CL_KERNEL_WORK_GROUP_SIZE: %zu\n", work_group_size);
    
    // Preferred Work Group Size Multiple
    size_t preferred_multiple;
    err = clGetKernelWorkGroupInfo(kernel, device,
        CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, 
        sizeof(size_t), &preferred_multiple, NULL);
    CHECK_CL(err, "CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE");
    printf("CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: %zu\n", preferred_multiple);
    
    // Local Memory Size
    cl_ulong local_mem_size;
    err = clGetKernelWorkGroupInfo(kernel, device,
        CL_KERNEL_LOCAL_MEM_SIZE, sizeof(cl_ulong), &local_mem_size, NULL);
    CHECK_CL(err, "CL_KERNEL_LOCAL_MEM_SIZE");
    printf("CL_KERNEL_LOCAL_MEM_SIZE: %llu bytes\n", 
        (unsigned long long)local_mem_size);
    
    // Private Memory Size
    cl_ulong private_mem_size;
    err = clGetKernelWorkGroupInfo(kernel, device,
        CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &private_mem_size, NULL);
    CHECK_CL(err, "CL_KERNEL_PRIVATE_MEM_SIZE");
    printf("CL_KERNEL_PRIVATE_MEM_SIZE: %llu bytes\n", 
        (unsigned long long)private_mem_size);
    
    // Compile Work Group Size
    size_t compile_work_group_size[3];
    err = clGetKernelWorkGroupInfo(kernel, device,
        CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 
        sizeof(compile_work_group_size), compile_work_group_size, NULL);
    CHECK_CL(err, "CL_KERNEL_COMPILE_WORK_GROUP_SIZE");
    printf("CL_KERNEL_COMPILE_WORK_GROUP_SIZE: [%zu, %zu, %zu]\n",
        compile_work_group_size[0], compile_work_group_size[1], 
        compile_work_group_size[2]);
    
    // Kernel Name
    size_t kernel_name_size;
    err = clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, 
        0, NULL, &kernel_name_size);
    CHECK_CL(err, "CL_KERNEL_FUNCTION_NAME size");
    char *kernel_name = malloc(kernel_name_size);
    err = clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME,
        kernel_name_size, kernel_name, NULL);
    CHECK_CL(err, "CL_KERNEL_FUNCTION_NAME");
    printf("CL_KERNEL_FUNCTION_NAME: %s\n", kernel_name);
    free(kernel_name);
    
    // Num Args
    cl_uint num_args;
    err = clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS,
        sizeof(cl_uint), &num_args, NULL);
    CHECK_CL(err, "CL_KERNEL_NUM_ARGS");
    printf("CL_KERNEL_NUM_ARGS: %u\n", num_args);
    
    printf("\n=== INFORMATIONS DEVICE ===\n\n");
    
    // Max Compute Units
    cl_uint max_compute_units;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS,
        sizeof(cl_uint), &max_compute_units, NULL);
    CHECK_CL(err, "CL_DEVICE_MAX_COMPUTE_UNITS");
    printf("CL_DEVICE_MAX_COMPUTE_UNITS: %u\n", max_compute_units);
    
    // Max Work Group Size
    size_t max_work_group_size;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
        sizeof(size_t), &max_work_group_size, NULL);
    CHECK_CL(err, "CL_DEVICE_MAX_WORK_GROUP_SIZE");
    printf("CL_DEVICE_MAX_WORK_GROUP_SIZE: %zu\n", max_work_group_size);
    
    // Max Work Item Dimensions
    cl_uint max_work_item_dimensions;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
        sizeof(cl_uint), &max_work_item_dimensions, NULL);
    CHECK_CL(err, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS");
    printf("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: %u\n", max_work_item_dimensions);
    
    // Max Work Item Sizes
    size_t max_work_item_sizes[3];
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
        sizeof(max_work_item_sizes), max_work_item_sizes, NULL);
    CHECK_CL(err, "CL_DEVICE_MAX_WORK_ITEM_SIZES");
    printf("CL_DEVICE_MAX_WORK_ITEM_SIZES: [%zu, %zu, %zu]\n",
        max_work_item_sizes[0], max_work_item_sizes[1], max_work_item_sizes[2]);
    
    // Preferred Vector Width
    cl_uint preferred_vector_width_int;
    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,
        sizeof(cl_uint), &preferred_vector_width_int, NULL);
    CHECK_CL(err, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT");
    printf("CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: %u\n", preferred_vector_width_int);
    
    printf("\n=== ANALYSE CONFIGURATION ===\n\n");
    
    printf("Configuration recommandée pour i915 natif:\n");
    printf("  - Work Group Size: %zu (ou multiple de %zu)\n", 
        work_group_size, preferred_multiple);
    printf("  - SIMD Size: SIMD%u (basé sur preferred vector width)\n",
        preferred_vector_width_int * 4);
    printf("  - Thread Count: %zu threads\n", work_group_size);
    printf("  - Max Threads: %zu (device limit)\n", max_work_group_size);
    
    printf("\n=== CONFIGURATION GPGPU_WALKER ===\n\n");
    printf("GPGPU_WALKER.DW4 devrait être:\n");
    if (preferred_vector_width_int == 1) {
        printf("  DW4 = (0 << 30) | %zu  // SIMD8\n", work_group_size - 1);
    } else if (preferred_vector_width_int == 2) {
        printf("  DW4 = (1 << 30) | %zu  // SIMD16\n", work_group_size - 1);
    } else {
        printf("  DW4 = (2 << 30) | %zu  // SIMD32\n", work_group_size - 1);
    }
    
    printf("\nInterface Descriptor.dw6 devrait être:\n");
    printf("  dw6 = (%zu << 24)  // Number of threads\n", work_group_size);
    
    printf("\n=== RÉSUMÉ C495 ===\n");
    printf("✓ Metadata extraites avec succès\n");
    printf("✓ Configuration pour i915 natif identifiée\n");
    printf("✓ Prochaine étape: Appliquer configuration en C496\n");
    
    // Cleanup
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
    free(source);
    
    return 0;
}

// Made with Bob
