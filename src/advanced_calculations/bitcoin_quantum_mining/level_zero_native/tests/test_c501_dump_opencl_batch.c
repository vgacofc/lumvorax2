/*
 * CYCLE C501: DUMP BATCH OPENCL VIA DEBUGFS
 * 
 * Stratégie alternative: Utiliser i915_gem_objects debugfs
 * pour dumper les buffers GPU après exécution OpenCL
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>

// Fonction pour lire debugfs i915
void dump_i915_gem_objects(const char *output_file)
{
    FILE *debugfs = fopen("/sys/kernel/debug/dri/0/i915_gem_objects", "r");
    if (!debugfs) {
        printf("⚠ Impossible d'ouvrir i915_gem_objects (besoin de sudo)\n");
        return;
    }
    
    FILE *out = fopen(output_file, "w");
    if (!out) {
        fclose(debugfs);
        return;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), debugfs)) {
        fputs(line, out);
    }
    
    fclose(out);
    fclose(debugfs);
    printf("✓ i915_gem_objects dumpé dans %s\n", output_file);
}

// Fonction pour dumper un buffer spécifique via handle
int dump_buffer_by_handle(int drm_fd, uint32_t handle, const char *filename)
{
    // Mapper le buffer
    struct drm_i915_gem_mmap_offset {
        uint32_t handle;
        uint32_t pad;
        uint64_t offset;
        uint64_t flags;
        uint64_t extensions;
    } mmap_arg = {
        .handle = handle,
        .flags = 0, // I915_MMAP_OFFSET_WB
    };
    
    // Note: Cette approche nécessite les privilèges appropriés
    printf("⚠ Dump buffer handle %u nécessite privilèges root\n", handle);
    return -1;
}

int main(int argc, char **argv)
{
    printf("\n=== CYCLE C501: DUMP BATCH OPENCL ===\n\n");
    
    // Stratégie 1: Dumper i915_gem_objects avant et après
    printf("[1/3] Dump i915_gem_objects AVANT exécution OpenCL...\n");
    dump_i915_gem_objects("logs/c501_gem_objects_before.txt");
    
    printf("\n[2/3] Exécution test OpenCL...\n");
    system("./tests/test_c427_store_opencl > /dev/null 2>&1");
    
    printf("\n[3/3] Dump i915_gem_objects APRÈS exécution OpenCL...\n");
    dump_i915_gem_objects("logs/c501_gem_objects_after.txt");
    
    printf("\n=== ANALYSE ===\n");
    printf("Comparer les deux fichiers pour identifier les nouveaux buffers:\n");
    printf("  diff logs/c501_gem_objects_before.txt logs/c501_gem_objects_after.txt\n");
    
    printf("\n=== ALTERNATIVE: INTEL_DEBUG ===\n");
    printf("OpenCL Intel supporte INTEL_DEBUG pour dumper les batches:\n");
    printf("  INTEL_DEBUG=bat ./tests/test_c427_store_opencl\n");
    printf("  INTEL_DEBUG=bat,vs,wm ./tests/test_c427_store_opencl\n");
    
    return 0;
}

// Made with Bob
