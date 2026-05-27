#!/usr/bin/env python3
"""
LUMVORAX C491 - Génération header C depuis ISA Gen9 extrait
"""

import sys

def generate_header(bin_file, output_file):
    with open(bin_file, 'rb') as f:
        data = f.read()
    
    with open(output_file, 'w') as f:
        f.write("/*\n")
        f.write(" * LUMVORAX C491 - Kernel ISA Gen9 extrait depuis OpenCL\n")
        f.write(" * \n")
        f.write(" * Source: OpenCL runtime Intel\n")
        f.write(f" * Taille: {len(data)} bytes\n")
        f.write(" * Format: ISA Gen9 pur (sans header ELF)\n")
        f.write(" * \n")
        f.write(" * Kernel source:\n")
        f.write(" * __kernel void minimal_write(__global uint *output) {\n")
        f.write(" *     output[0] = 0xDEADBEEF;\n")
        f.write(" *     output[1] = 0xCAFEBABE;\n")
        f.write(" *     output[2] = 0x12345678;\n")
        f.write(" *     output[3] = 0xABCDEF00;\n")
        f.write(" * }\n")
        f.write(" */\n\n")
        f.write("#ifndef TEST_C491_OPENCL_ISA_GEN9_H\n")
        f.write("#define TEST_C491_OPENCL_ISA_GEN9_H\n\n")
        f.write("#include <stddef.h>\n\n")
        f.write("// ISA Gen9 extrait depuis OpenCL runtime\n")
        f.write("const unsigned char kernel_isa_opencl_gen9[] = {\n")
        
        for i in range(0, len(data), 16):
            f.write("    ")
            for j in range(16):
                if i + j < len(data):
                    f.write(f"0x{data[i+j]:02x}")
                    if i + j < len(data) - 1:
                        f.write(", ")
            f.write("\n")
        
        f.write("};\n\n")
        f.write(f"const size_t kernel_isa_opencl_gen9_size = {len(data)};\n\n")
        f.write("#endif // TEST_C491_OPENCL_ISA_GEN9_H\n")
    
    print(f"✓ Header généré: {output_file} ({len(data)} bytes)")

if __name__ == "__main__":
    generate_header(
        "tests/test_c491_isa_gen9_pure.bin",
        "tests/test_c491_opencl_isa_gen9.h"
    )

# Made with Bob
