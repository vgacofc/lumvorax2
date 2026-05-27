#!/bin/bash
# C197.22 - Extract Gen9 ISA from OpenCL kernel
# This script compiles a minimal OpenCL kernel and extracts the Gen9 ISA binary

set -e

echo "=== C197.22 - Extract Gen9 ISA from OpenCL ==="
echo ""

# Create minimal OpenCL kernel
cat > /tmp/kernel_write_magic.cl << 'EOF'
__kernel void write_magic(__global uint* output) {
    output[0] = 0x12345678;
}
EOF

echo "Step 1: Created minimal OpenCL kernel"
cat /tmp/kernel_write_magic.cl
echo ""

# Check if ocloc is available
if ! command -v ocloc &> /dev/null; then
    echo "⚠️ ocloc not found. Trying alternative method with clinfo..."
    
    # Alternative: Use existing OpenCL runtime to compile and dump
    cat > /tmp/extract_isa.c << 'EOFC'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    cl_int err;
    
    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get platform\n");
        return 1;
    }
    
    // Get device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get device\n");
        return 1;
    }
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create context\n");
        return 1;
    }
    
    // Read kernel source
    FILE* f = fopen("/tmp/kernel_write_magic.cl", "r");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    // Create program
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &size, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create program\n");
        return 1;
    }
    
    // Build program
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to build program\n");
        
        // Get build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    
    // Get binary size
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get binary size\n");
        return 1;
    }
    
    printf("Binary size: %zu bytes\n", binary_size);
    
    // Get binary
    unsigned char* binary = malloc(binary_size);
    unsigned char* binaries[] = {binary};
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get binary\n");
        return 1;
    }
    
    // Write binary to file
    FILE* out = fopen("/tmp/kernel_write_magic.bin", "wb");
    fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    printf("✅ Binary written to /tmp/kernel_write_magic.bin\n");
    
    // Dump hexadecimal
    FILE* hex = fopen("/tmp/kernel_write_magic_hex.txt", "w");
    for (size_t i = 0; i < binary_size; i++) {
        fprintf(hex, "%02x", binary[i]);
        if ((i + 1) % 16 == 0) fprintf(hex, "\n");
        else if ((i + 1) % 4 == 0) fprintf(hex, " ");
    }
    fclose(hex);
    
    printf("✅ Hexadecimal dump written to /tmp/kernel_write_magic_hex.txt\n");
    
    // Cleanup
    free(binary);
    free(source);
    clReleaseProgram(program);
    clReleaseContext(context);
    
    return 0;
}
EOFC

    echo "Step 2: Compiling ISA extractor..."
    gcc -o /tmp/extract_isa /tmp/extract_isa.c -lOpenCL
    
    echo "Step 3: Extracting ISA binary..."
    /tmp/extract_isa
    
    if [ -f /tmp/kernel_write_magic.bin ]; then
        echo ""
        echo "Step 4: Analyzing binary..."
        ls -lh /tmp/kernel_write_magic.bin
        echo ""
        echo "First 256 bytes (hexdump):"
        hexdump -C /tmp/kernel_write_magic.bin | head -20
        echo ""
        echo "✅ ISA extraction complete!"
        echo "Binary: /tmp/kernel_write_magic.bin"
        echo "Hex dump: /tmp/kernel_write_magic_hex.txt"
    else
        echo "❌ Failed to extract ISA binary"
        exit 1
    fi
else
    echo "Step 2: Compiling with ocloc..."
    ocloc compile -file /tmp/kernel_write_magic.cl -device skl -output /tmp/kernel_write_magic
    
    echo ""
    echo "Step 3: Analyzing generated files..."
    ls -lh /tmp/kernel_write_magic*
    
    echo ""
    echo "✅ ISA compilation complete!"
fi

echo ""
echo "=== Next: Integrate ISA into C197.22 DRM code ==="

# Made with Bob
