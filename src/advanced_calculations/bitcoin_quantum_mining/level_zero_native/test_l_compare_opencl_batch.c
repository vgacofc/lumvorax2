/*
 * TEST L — COMPARAISON BATCH OPENCL VS NOTRE BATCH
 * 
 * Objectif : Capturer et comparer TOUS les états GPU programmés par OpenCL
 *            vs notre implémentation minimale pour identifier ce qui manque
 * 
 * Méthodologie :
 * 1. Capturer batch OpenCL réel avec intel_aubdump
 * 2. Décoder toutes les commandes GPU
 * 3. Comparer avec notre batch actuel
 * 4. Identifier structures manquantes (PIPE_CONTROL, STATE_BASE_ADDRESS, etc.)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>

// DRM/i915 headers
#include <drm/i915_drm.h>
#include <xf86drm.h>

// Gen9 command opcodes
#define MI_BATCH_BUFFER_END     0x0A000000
#define MI_NOOP                 0x00000000
#define PIPE_CONTROL            0x7A000000
#define STATE_BASE_ADDRESS      0x61010000
#define MEDIA_VFE_STATE         0x70000000
#define MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020000
#define GPGPU_WALKER            0x71050000
#define MI_LOAD_REGISTER_IMM    0x11000000

// Structure pour analyser batch
typedef struct {
    uint32_t *commands;
    size_t size;
    size_t count;
} batch_analysis_t;

// Fonction pour décoder commande GPU
const char* decode_command(uint32_t cmd) {
    uint32_t opcode = cmd & 0xFF000000;
    
    switch (opcode) {
        case 0x0A000000: return "MI_BATCH_BUFFER_END";
        case 0x00000000: return "MI_NOOP";
        case 0x7A000000: return "PIPE_CONTROL";
        case 0x61000000: return "STATE_BASE_ADDRESS";
        case 0x70000000: return "MEDIA_VFE_STATE";
        case 0x70020000: return "MEDIA_INTERFACE_DESCRIPTOR_LOAD";
        case 0x71000000: return "GPGPU_WALKER";
        case 0x11000000: return "MI_LOAD_REGISTER_IMM";
        case 0x24000000: return "MI_STORE_DATA_IMM";
        case 0x18000000: return "MI_FLUSH_DW";
        default: return "UNKNOWN";
    }
}

// Analyser notre batch actuel
void analyze_our_batch(const char *filename) {
    printf("\n=== ANALYSE NOTRE BATCH ===\n");
    
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Erreur: Impossible d'ouvrir %s\n", filename);
        return;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint32_t *batch = malloc(size);
    fread(batch, 1, size, f);
    fclose(f);
    
    printf("Taille batch : %zu bytes (%zu dwords)\n", size, size/4);
    printf("\nCommandes détectées :\n");
    
    size_t cmd_count = 0;
    for (size_t i = 0; i < size/4; i++) {
        const char *name = decode_command(batch[i]);
        if (strcmp(name, "MI_NOOP") != 0 && strcmp(name, "UNKNOWN") != 0) {
            printf("  [%04zx] 0x%08x : %s\n", i*4, batch[i], name);
            cmd_count++;
        }
    }
    
    printf("\nTotal commandes significatives : %zu\n", cmd_count);
    free(batch);
}

// Créer kernel OpenCL minimal pour capture
void create_opencl_test_kernel() {
    printf("\n=== CRÉATION KERNEL OPENCL TEST ===\n");
    
    FILE *f = fopen("test_opencl_minimal.cl", "w");
    fprintf(f, 
        "__kernel void minimal_test(__global uint *output) {\n"
        "    int gid = get_global_id(0);\n"
        "    output[gid] = gid * 2;\n"
        "}\n"
    );
    fclose(f);
    
    printf("Kernel OpenCL créé : test_opencl_minimal.cl\n");
}

// Créer programme C++ pour exécuter kernel OpenCL
void create_opencl_capture_program() {
    printf("\n=== CRÉATION PROGRAMME CAPTURE OPENCL ===\n");
    
    FILE *f = fopen("capture_opencl_batch.cpp", "w");
    fprintf(f,
        "#include <CL/cl.h>\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "\n"
        "int main() {\n"
        "    cl_platform_id platform;\n"
        "    cl_device_id device;\n"
        "    cl_context context;\n"
        "    cl_command_queue queue;\n"
        "    cl_program program;\n"
        "    cl_kernel kernel;\n"
        "    cl_mem buffer;\n"
        "    \n"
        "    // Init OpenCL\n"
        "    clGetPlatformIDs(1, &platform, NULL);\n"
        "    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);\n"
        "    context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);\n"
        "    queue = clCreateCommandQueue(context, device, 0, NULL);\n"
        "    \n"
        "    // Charger kernel\n"
        "    FILE *fp = fopen(\"test_opencl_minimal.cl\", \"r\");\n"
        "    fseek(fp, 0, SEEK_END);\n"
        "    size_t size = ftell(fp);\n"
        "    fseek(fp, 0, SEEK_SET);\n"
        "    char *source = (char*)malloc(size + 1);\n"
        "    fread(source, 1, size, fp);\n"
        "    source[size] = 0;\n"
        "    fclose(fp);\n"
        "    \n"
        "    program = clCreateProgramWithSource(context, 1, (const char**)&source, NULL, NULL);\n"
        "    clBuildProgram(program, 1, &device, NULL, NULL, NULL);\n"
        "    kernel = clCreateKernel(program, \"minimal_test\", NULL);\n"
        "    \n"
        "    // Créer buffer\n"
        "    buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(unsigned int), NULL, NULL);\n"
        "    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);\n"
        "    \n"
        "    // Exécuter kernel (CAPTURE ICI)\n"
        "    size_t global_size = 256;\n"
        "    size_t local_size = 16;\n"
        "    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);\n"
        "    clFinish(queue);\n"
        "    \n"
        "    printf(\"Kernel OpenCL exécuté avec succès\\n\");\n"
        "    \n"
        "    // Cleanup\n"
        "    clReleaseMemObject(buffer);\n"
        "    clReleaseKernel(kernel);\n"
        "    clReleaseProgram(program);\n"
        "    clReleaseCommandQueue(queue);\n"
        "    clReleaseContext(context);\n"
        "    \n"
        "    return 0;\n"
        "}\n"
    );
    fclose(f);
    
    printf("Programme capture créé : capture_opencl_batch.cpp\n");
}

// Instructions pour capturer batch OpenCL
void print_capture_instructions() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  INSTRUCTIONS CAPTURE BATCH OPENCL                             ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("ÉTAPE 1 : Compiler programme capture\n");
    printf("--------------------------------------\n");
    printf("g++ -o capture_opencl_batch capture_opencl_batch.cpp -lOpenCL\n");
    printf("\n");
    
    printf("ÉTAPE 2 : Capturer avec intel_aubdump\n");
    printf("--------------------------------------\n");
    printf("intel_aubdump -v --output=opencl_batch.aub ./capture_opencl_batch\n");
    printf("\n");
    printf("OU avec INTEL_DEBUG :\n");
    printf("INTEL_DEBUG=bat,vs,wm,cs ./capture_opencl_batch 2>&1 | tee opencl_debug.log\n");
    printf("\n");
    
    printf("ÉTAPE 3 : Décoder batch capturé\n");
    printf("--------------------------------\n");
    printf("intel_aubdump_viewer opencl_batch.aub > opencl_batch_decoded.txt\n");
    printf("\n");
    printf("OU extraire binaire :\n");
    printf("aubinator opencl_batch.aub --gen=9 > opencl_commands.txt\n");
    printf("\n");
    
    printf("ÉTAPE 4 : Comparer avec notre batch\n");
    printf("------------------------------------\n");
    printf("./test_l_compare_opencl_batch opencl_vector_add_gen9_FULL.bin\n");
    printf("\n");
    
    printf("ÉTAPE 5 : Analyser différences\n");
    printf("-------------------------------\n");
    printf("diff -u notre_batch_decoded.txt opencl_batch_decoded.txt\n");
    printf("\n");
}

// Structures manquantes probables
void print_missing_structures() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  STRUCTURES PROBABLEMENT MANQUANTES                            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("⭐⭐⭐⭐⭐ CRITIQUE — PIPE_CONTROL\n");
    printf("--------------------------------\n");
    printf("OpenCL insère PIPE_CONTROL avant/après chaque étape :\n");
    printf("  - Flush cache L3\n");
    printf("  - Invalidate TLB\n");
    printf("  - Stall pipeline\n");
    printf("  - Wait for idle\n");
    printf("\n");
    printf("Notre batch : Probablement AUCUN PIPE_CONTROL\n");
    printf("\n");
    
    printf("⭐⭐⭐⭐⭐ CRITIQUE — STATE_BASE_ADDRESS\n");
    printf("--------------------------------------\n");
    printf("OpenCL configure 6 base addresses :\n");
    printf("  - General State Base\n");
    printf("  - Surface State Base\n");
    printf("  - Dynamic State Base\n");
    printf("  - Indirect Object Base\n");
    printf("  - Instruction Base\n");
    printf("  - Bindless Surface State Base\n");
    printf("\n");
    printf("Notre batch : Probablement INCOMPLET ou ABSENT\n");
    printf("\n");
    
    printf("⭐⭐⭐⭐ IMPORTANT — Interface Descriptor Table\n");
    printf("----------------------------------------------\n");
    printf("OpenCL configure metadata kernel :\n");
    printf("  - Kernel start pointer\n");
    printf("  - Thread count\n");
    printf("  - SLM size\n");
    printf("  - Barrier enable\n");
    printf("  - Sampler count\n");
    printf("  - Binding table entries\n");
    printf("\n");
    printf("Notre batch : Probablement VALEURS PAR DÉFAUT INVALIDES\n");
    printf("\n");
    
    printf("⭐⭐⭐⭐ IMPORTANT — Scratch Space\n");
    printf("---------------------------------\n");
    printf("OpenCL alloue scratch space pour spill registers :\n");
    printf("  - Scratch buffer allocation\n");
    printf("  - Per-thread scratch pointers\n");
    printf("  - Scratch size configuration\n");
    printf("\n");
    printf("Notre batch : Probablement ABSENT\n");
    printf("\n");
    
    printf("⭐⭐⭐ IMPORTANT — L3 Cache Configuration\n");
    printf("----------------------------------------\n");
    printf("OpenCL configure L3 cache policy :\n");
    printf("  - URB allocation\n");
    printf("  - DC allocation\n");
    printf("  - RO allocation\n");
    printf("  - I/S allocation\n");
    printf("\n");
    printf("Notre batch : Probablement DÉFAUT (peut être invalide)\n");
    printf("\n");
    
    printf("⭐⭐⭐ IMPORTANT — Preemption State\n");
    printf("-----------------------------------\n");
    printf("OpenCL configure preemption :\n");
    printf("  - Context image\n");
    printf("  - Preemption mode\n");
    printf("  - State save/restore\n");
    printf("\n");
    printf("Notre batch : Probablement ABSENT → CORRUPTION CONTEXT\n");
    printf("\n");
}

int main(int argc, char **argv) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST L — COMPARAISON BATCH OPENCL VS NOTRE BATCH             ║\n");
    printf("║  Objectif : Identifier structures GPU manquantes               ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    // Créer fichiers nécessaires
    create_opencl_test_kernel();
    create_opencl_capture_program();
    
    // Analyser notre batch si fourni
    if (argc > 1) {
        analyze_our_batch(argv[1]);
    }
    
    // Afficher structures manquantes probables
    print_missing_structures();
    
    // Afficher instructions capture
    print_capture_instructions();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  HYPOTHÈSE PRINCIPALE                                          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Notre batch est probablement :\n");
    printf("  ✅ Syntaxiquement valide (i915 accepte)\n");
    printf("  ❌ Architecturalement incomplet (GPU hang)\n");
    printf("\n");
    printf("Le GPU :\n");
    printf("  1. Accepte la soumission\n");
    printf("  2. Commence l'exécution\n");
    printf("  3. Entre dans état invalide (state cache/scoreboard)\n");
    printf("  4. Hang détecté par watchdog\n");
    printf("  5. Reset moteur\n");
    printf("\n");
    printf("Solution :\n");
    printf("  → Ajouter TOUTES les structures GPU qu'OpenCL programme\n");
    printf("  → Pas juste le kernel, mais TOUT le pipeline compute\n");
    printf("\n");
    
    return 0;
}

// Made with Bob
