/*
 * DRM Batch Buffer Capture Tool
 * Capture et analyse batch buffers OpenCL via strace/ioctl monitoring
 * 
 * Objectif: Identifier états GPU manquants en comparant batch OpenCL vs manuel
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>

// i915 DRM structures minimales
#define DRM_IOCTL_BASE 'd'
#define DRM_I915_GEM_EXECBUFFER2 0x29
#define DRM_IOWR(nr,type) _IOWR(DRM_IOCTL_BASE,nr,type)

struct drm_i915_gem_exec_object2 {
    uint32_t handle;
    uint32_t relocation_count;
    uint64_t relocs_ptr;
    uint64_t alignment;
    uint64_t offset;
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
};

struct drm_i915_gem_execbuffer2 {
    uint64_t buffers_ptr;
    uint32_t buffer_count;
    uint32_t batch_start_offset;
    uint32_t batch_len;
    uint32_t DR1;
    uint32_t DR4;
    uint32_t num_cliprects;
    uint64_t cliprects_ptr;
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
};

#define DRM_IOCTL_I915_GEM_EXECBUFFER2 DRM_IOWR(DRM_I915_GEM_EXECBUFFER2, struct drm_i915_gem_execbuffer2)

// Analyse commandes Gen9
void analyze_gen9_command(uint32_t *cmd, size_t offset) {
    uint32_t opcode = (cmd[0] >> 23) & 0x1FF;
    uint32_t subopcode = (cmd[0] >> 16) & 0x7F;
    uint32_t length = (cmd[0] & 0xFF) + 2;
    
    printf("  [%04zx] ", offset);
    
    // Decode commandes principales
    if ((cmd[0] & 0xFFFF0000) == 0x78000000) {
        printf("STATE_BASE_ADDRESS (len=%u)\n", length);
        if (length >= 10) {
            printf("    General State Base: 0x%08x%08x\n", cmd[2], cmd[1]);
            printf("    Surface State Base: 0x%08x%08x\n", cmd[4], cmd[3]);
            printf("    Dynamic State Base: 0x%08x%08x\n", cmd[6], cmd[5]);
            printf("    Indirect Object Base: 0x%08x%08x\n", cmd[8], cmd[7]);
            printf("    Instruction Base: 0x%08x%08x\n", cmd[10], cmd[9]);
        }
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x78010000) {
        printf("STATE_SIP (len=%u)\n", length);
        printf("    System Instruction Pointer: 0x%08x%08x\n", cmd[2], cmd[1]);
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x78050000) {
        printf("3DSTATE_URB_VS/HS/DS/GS (len=%u)\n", length);
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x78100000) {
        printf("MEDIA_VFE_STATE (len=%u)\n", length);
        if (length >= 9) {
            printf("    Scratch Space Base: 0x%08x%08x\n", cmd[2], cmd[1]);
            printf("    Max Threads: %u\n", (cmd[3] >> 16) & 0xFFFF);
            printf("    URB Entry Size: %u\n", cmd[5] & 0xFFFF);
            printf("    CURBE Allocation Size: %u\n", (cmd[6] >> 16) & 0xFFFF);
        }
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x78110000) {
        printf("MEDIA_CURBE_LOAD (len=%u)\n", length);
        printf("    CURBE Total Data Length: %u\n", cmd[1]);
        printf("    CURBE Data Start Address: 0x%08x\n", cmd[2]);
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x78120000) {
        printf("MEDIA_INTERFACE_DESCRIPTOR_LOAD (len=%u)\n", length);
        printf("    Interface Descriptor Total Length: %u\n", cmd[1]);
        printf("    Interface Descriptor Data Start Address: 0x%08x\n", cmd[2]);
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x71000000) {
        printf("GPGPU_WALKER (len=%u)\n", length);
        if (length >= 15) {
            printf("    Interface Descriptor Offset: %u\n", cmd[1] & 0x3F);
            printf("    Thread Group ID X/Y/Z Dimension: %u/%u/%u\n", 
                   cmd[4], cmd[5], cmd[6]);
            printf("    Thread Group ID Starting X/Y/Z: %u/%u/%u\n",
                   cmd[7], cmd[8], cmd[9]);
        }
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x7A000000) {
        printf("PIPE_CONTROL (len=%u)\n", length);
        uint32_t flags = cmd[1];
        printf("    Flags: 0x%08x\n", flags);
        if (flags & (1 << 20)) printf("      - CS Stall\n");
        if (flags & (1 << 18)) printf("      - TLB Invalidate\n");
        if (flags & (1 << 17)) printf("      - Instruction Cache Invalidate\n");
        if (flags & (1 << 16)) printf("      - Texture Cache Invalidate\n");
        if (flags & (1 << 14)) printf("      - Post-Sync Write\n");
        if (length >= 5) {
            printf("    Address: 0x%08x%08x\n", cmd[3], cmd[2]);
            printf("    Immediate Data: 0x%08x%08x\n", cmd[5], cmd[4]);
        }
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x05000000) {
        printf("MI_BATCH_BUFFER_END\n");
    }
    else if ((cmd[0] & 0xFFFF0000) == 0x00000000) {
        printf("MI_NOOP\n");
    }
    else {
        printf("UNKNOWN (opcode=0x%03x, subop=0x%02x, len=%u)\n", 
               opcode, subopcode, length);
        printf("    DW0: 0x%08x\n", cmd[0]);
    }
}

// Analyse batch buffer complet
void analyze_batch_buffer(void *batch, size_t size) {
    printf("\n=== BATCH BUFFER ANALYSIS ===\n");
    printf("Size: %zu bytes (%zu dwords)\n\n", size, size/4);
    
    uint32_t *cmd = (uint32_t*)batch;
    size_t offset = 0;
    size_t dwords = size / 4;
    
    while (offset < dwords) {
        uint32_t cmd_header = cmd[offset];
        
        // Détection fin batch
        if ((cmd_header & 0xFFFF0000) == 0x05000000) {
            analyze_gen9_command(&cmd[offset], offset * 4);
            break;
        }
        
        // Analyse commande
        analyze_gen9_command(&cmd[offset], offset * 4);
        
        // Calcul longueur commande
        uint32_t length = 1;
        if ((cmd_header & 0xE0000000) == 0x60000000 || // 3D commands
            (cmd_header & 0xE0000000) == 0x70000000) { // GPGPU commands
            length = (cmd_header & 0xFF) + 2;
        }
        
        offset += length;
        
        // Protection overflow
        if (offset > dwords) {
            printf("\n!!! BATCH BUFFER OVERFLOW DETECTED !!!\n");
            break;
        }
    }
    
    printf("\n=== END BATCH ANALYSIS ===\n\n");
}

int main(int argc, char **argv) {
    printf("=== DRM Batch Buffer Capture Tool ===\n\n");
    
    if (argc < 2) {
        printf("Usage: %s <opencl_program>\n", argv[0]);
        printf("\nCe programme va:\n");
        printf("1. Exécuter le programme OpenCL fourni\n");
        printf("2. Capturer les batch buffers via strace\n");
        printf("3. Analyser les commandes GPU Gen9\n");
        printf("4. Identifier les états GPU manquants\n\n");
        return 1;
    }
    
    // Créer commande strace pour capturer ioctl
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "strace -e trace=ioctl -v -s 65536 -o /tmp/drm_trace.txt %s 2>&1",
             argv[1]);
    
    printf("Exécution avec strace: %s\n\n", argv[1]);
    int ret = system(cmd);
    
    if (ret != 0) {
        printf("Erreur exécution programme: %d\n", ret);
        return 1;
    }
    
    printf("\nTrace DRM capturée dans /tmp/drm_trace.txt\n");
    printf("Analysez avec: grep -A 50 'DRM_IOCTL_I915_GEM_EXECBUFFER2' /tmp/drm_trace.txt\n\n");
    
    // Instructions pour extraction manuelle
    printf("=== INSTRUCTIONS EXTRACTION BATCH BUFFER ===\n\n");
    printf("1. Identifier handle batch buffer dans trace:\n");
    printf("   grep 'DRM_IOCTL_I915_GEM_EXECBUFFER2' /tmp/drm_trace.txt\n\n");
    
    printf("2. Extraire batch buffer avec gdb:\n");
    printf("   gdb --batch --pid $(pgrep opencl_reference) \\\n");
    printf("       -ex 'dump binary memory /tmp/batch.bin <addr> <addr+size>'\n\n");
    
    printf("3. Analyser batch avec cet outil:\n");
    printf("   hexdump -C /tmp/batch.bin | head -100\n\n");
    
    return 0;
}

// Made with Bob
