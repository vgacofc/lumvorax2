// TEST C540 - MODÈLE HEAP-RELATIVE INTEL GEN9 (ROOT CAUSE #44)
// Utiliser STATE_BASE_ADDRESS + offsets relatifs (comme OpenCL)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <drm/i915_drm.h>
#include <xf86drm.h>

// Kernel ISA minimal
static const uint8_t kernel_isa[] = {
    0x01, 0x00, 0x60, 0x00, 0x0c, 0x02, 0x40, 0x20,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x30,
    0x00, 0x10, 0x00, 0x16, 0xc0, 0x04, 0xc0, 0x04,
    0x01, 0x00, 0x00, 0x00, 0x0c, 0x43, 0x80, 0x20,
    0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x2c, 0x0e, 0xc0, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12
};

void dump_hex(const char *label, const void *data, size_t size) {
    printf("\n%s (%zu bytes):\n", label, size);
    const uint8_t *bytes = data;
    for (size_t i = 0; i < size && i < 128; i += 16) {
        printf("  %04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
}

int main(void) {
    printf("=== TEST C540 - MODÈLE HEAP-RELATIVE INTEL GEN9 ===\n");
    printf("ROOT CAUSE #44: Utiliser indirection via STATE_BASE_ADDRESS\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            perror("open DRM");
            return 1;
        }
    }
    printf("✓ DRM ouvert\n");
    
    // Créer UN SEUL BUFFER HEAP contenant TOUT
    // Layout: [Surface State | Binding Table | IDRT | Kernel ISA]
    struct drm_i915_gem_create create_heap = { .size = 8192 };
    struct drm_i915_gem_create create_output = { .size = 4096 };
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_heap) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch)) {
        perror("gem_create");
        return 1;
    }
    
    printf("✓ Buffers créés:\n");
    printf("  heap:   handle=%u (8192 bytes)\n", create_heap.handle);
    printf("  output: handle=%u (4096 bytes)\n", create_output.handle);
    printf("  batch:  handle=%u (4096 bytes)\n", create_batch.handle);
    
    // Mapper avec API moderne
    struct drm_i915_gem_mmap_offset mmap_heap = { .handle = create_heap.handle, .flags = I915_MMAP_OFFSET_WB };
    struct drm_i915_gem_mmap_offset mmap_output = { .handle = create_output.handle, .flags = I915_MMAP_OFFSET_WB };
    struct drm_i915_gem_mmap_offset mmap_batch = { .handle = create_batch.handle, .flags = I915_MMAP_OFFSET_WB };
    
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_heap) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_output) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_batch)) {
        perror("gem_mmap_offset");
        return 1;
    }
    
    uint8_t *heap = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_heap.offset);
    uint32_t *output = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_output.offset);
    uint8_t *batch = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_batch.offset);
    
    if (heap == MAP_FAILED || output == MAP_FAILED || batch == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    printf("✓ Buffers mappés\n");
    
    // Initialiser output
    for (int i = 0; i < 1024; i++) {
        output[i] = 0xFFFFFFFF;
    }
    
    // Layout HEAP (offsets relatifs):
    // 0x0000: Surface State (64 bytes)
    // 0x0040: Binding Table (16 bytes)  
    // 0x0050: IDRT (32 bytes)
    // 0x0070: Kernel ISA (64 bytes)
    
    memset(heap, 0, 8192);
    
    uint32_t *surface_state = (uint32_t *)&heap[0x0000];
    uint32_t *binding_table = (uint32_t *)&heap[0x0040];
    uint32_t *idrt = (uint32_t *)&heap[0x0050];
    uint8_t *kernel = &heap[0x0070];
    
    printf("\n=== CONSTRUCTION HEAP (OFFSETS RELATIFS) ===\n");
    
    // Surface State - AVEC RELOCATION vers output
    surface_state[0] = 0x00000004;  // DW0: BUFFER type
    surface_state[1] = 0x00000000;  // DW1: sera relocalisé vers output
    surface_state[2] = 0x00000000;  // DW2: sera relocalisé vers output
    surface_state[3] = 0x00000FFF;  // DW3: Width 4096-1
    surface_state[4] = 0x00000000;  // DW4
    surface_state[5] = 0x00001000;  // DW5: Pitch 4096
    
    printf("✓ Surface State @ offset 0x0000 (avec relocation)\n");
    
    // Binding Table - OFFSET RELATIF vers Surface State
    binding_table[0] = 0x00000000;  // BTI[0] → Surface State @ offset 0x0000
    
    printf("✓ Binding Table @ offset 0x0040 → Surface @ 0x0000\n");
    
    // IDRT - OFFSET RELATIF vers Kernel
    idrt[0] = 0x00000070;  // DW0: Kernel @ offset 0x0070 (relatif au heap)
    idrt[1] = 0x00000000;  // DW1: HIGH
    idrt[2] = 0x00000000;  // DW2: Reserved
    idrt[3] = 0x00400000;  // DW3: BTI @ offset 0x0040 (relatif au heap)
    idrt[4] = 0x00000000;  // DW4: CURBE
    idrt[5] = 0x00000001;  // DW5: 1 thread
    idrt[6] = 0x00000000;  // DW6
    idrt[7] = 0x00000000;  // DW7
    
    printf("✓ IDRT @ offset 0x0050 → Kernel @ 0x0070, BTI @ 0x0040\n");
    
    // Copier Kernel ISA
    memcpy(kernel, kernel_isa, sizeof(kernel_isa));
    
    printf("✓ Kernel ISA @ offset 0x0070 (%zu bytes)\n", sizeof(kernel_isa));
    
    dump_hex("HEAP Layout", heap, 256);
    
    // Construire BATCH avec STATE_BASE_ADDRESS pointant vers HEAP
    memset(batch, 0, 4096);
    uint32_t *batch_dw = (uint32_t *)batch;
    int idx = 0;
    
    printf("\n=== CONSTRUCTION BATCH ===\n");
    
    // PIPELINE_SELECT
    batch_dw[idx++] = 0x69040000;
    batch_dw[idx++] = 0x00000000;
    
    // STATE_BASE_ADDRESS - CRITIQUE: Pointer vers HEAP
    batch_dw[idx++] = 0x61010009;  // STATE_BASE_ADDRESS (9 DWORDs)
    batch_dw[idx++] = 0x00000001;  // General State Base: enable
    batch_dw[idx++] = 0x00000000;  // HIGH
    batch_dw[idx++] = 0x00000001;  // Surface State Base: enable (sera relocalisé vers heap)
    batch_dw[idx++] = 0x00000000;  // HIGH
    batch_dw[idx++] = 0x00000001;  // Dynamic State Base: enable (sera relocalisé vers heap)
    batch_dw[idx++] = 0x00000000;  // HIGH
    batch_dw[idx++] = 0x00000001;  // Indirect Object Base: enable (sera relocalisé vers heap)
    batch_dw[idx++] = 0x00000000;  // HIGH
    batch_dw[idx++] = 0x00000001;  // Instruction Base: enable (sera relocalisé vers heap)
    batch_dw[idx++] = 0x00000000;  // HIGH
    
    // Offsets corrects dans STATE_BASE_ADDRESS (commence à idx=2)
    int surface_state_base_idx = 5;   // DW5: Surface State Base LOW
    int dynamic_state_base_idx = 7;   // DW7: Dynamic State Base LOW
    int indirect_object_base_idx = 9; // DW9: Indirect Object Base LOW
    int instruction_base_idx = 11;    // DW11: Instruction Base LOW
    
    printf("✓ STATE_BASE_ADDRESS @ offset 0x08 (relocations: 0x%02x, 0x%02x, 0x%02x, 0x%02x)\n",
           surface_state_base_idx * 4, dynamic_state_base_idx * 4,
           indirect_object_base_idx * 4, instruction_base_idx * 4);
    
    // MEDIA_VFE_STATE - Configuration minimale valide
    batch_dw[idx++] = 0x70000006;  // Command
    batch_dw[idx++] = 0x00000000;  // DW1: Scratch Space Base (none)
    batch_dw[idx++] = 0x00000000;  // DW2: HIGH
    batch_dw[idx++] = 0x00000000;  // DW3: Max Threads (0 = use default)
    batch_dw[idx++] = 0x00000001;  // DW4: Number URB Entries = 1 (minimum)
    batch_dw[idx++] = 0x00000040;  // DW5: URB Entry Size = 64 bytes (minimum)
    batch_dw[idx++] = 0x00000000;  // DW6: CURBE Allocation Size = 0
    batch_dw[idx++] = 0x00000000;  // DW7: Scoreboard config
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD - OFFSET RELATIF
    batch_dw[idx++] = 0x70020002;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000020;  // 32 bytes
    batch_dw[idx++] = 0x00000050;  // IDRT @ offset 0x0050 (relatif au heap)
    batch_dw[idx++] = 0x00000000;
    
    printf("✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD → IDRT offset 0x0050\n");
    
    // GPGPU_WALKER
    batch_dw[idx++] = 0x71000011;
    batch_dw[idx++] = 0x00000000;  // Interface Descriptor Offset: 0
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;  // X dimension: 1
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;  // Y dimension: 1
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;  // Z dimension: 1
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0xFFFFFFFF;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0xFFFFFFFF;
    
    // MEDIA_STATE_FLUSH
    batch_dw[idx++] = 0x70040000;
    batch_dw[idx++] = 0x00000000;
    
    // PIPE_CONTROL
    batch_dw[idx++] = 0x7A000004;
    batch_dw[idx++] = 0x00100000;  // DC Flush
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch_dw[idx++] = 0x05000000;
    
    int batch_size = idx * 4;
    printf("✓ Batch construit: %d bytes\n", batch_size);
    
    // RELOCATIONS - Le cœur du modèle Intel
    printf("\n=== CONFIGURATION RELOCATIONS ===\n");
    
    struct drm_i915_gem_relocation_entry relocs[5] = {
        // #0: Surface State DW1-DW2 → output buffer
        {
            .target_handle = create_output.handle,
            .delta = 0,
            .offset = 0x0004,  // Surface State DW1
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_RENDER,
            .write_domain = I915_GEM_DOMAIN_RENDER
        },
        // #1-#4: STATE_BASE_ADDRESS → heap (4 bases)
        {
            .target_handle = create_heap.handle,
            .delta = 0,
            .offset = surface_state_base_idx * 4,  // Surface State Base @ 0x14
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        },
        {
            .target_handle = create_heap.handle,
            .delta = 0,
            .offset = dynamic_state_base_idx * 4,  // Dynamic State Base @ 0x1C
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        },
        {
            .target_handle = create_heap.handle,
            .delta = 0,
            .offset = indirect_object_base_idx * 4,  // Indirect Object Base @ 0x24
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        },
        {
            .target_handle = create_heap.handle,
            .delta = 0,
            .offset = instruction_base_idx * 4,  // Instruction Base @ 0x2C
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        }
    };
    
    printf("Relocation #0: Surface State → output (handle %u)\n", create_output.handle);
    printf("Relocations #1-#4: STATE_BASE_ADDRESS → heap (handle %u)\n", create_heap.handle);
    
    // EXECBUFFER avec relocations
    struct drm_i915_gem_exec_object2 exec_objects[3] = {
        { .handle = create_output.handle, .flags = EXEC_OBJECT_WRITE },
        { .handle = create_heap.handle, .relocation_count = 1, .relocs_ptr = (uintptr_t)&relocs[0] },
        { .handle = create_batch.handle, .relocation_count = 4, .relocs_ptr = (uintptr_t)&relocs[1] }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 3,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER
    };
    
    printf("\n=== EXÉCUTION GPU ===\n");
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret) {
        perror("EXECBUFFER2");
        return 1;
    }
    
    printf("✓ GPU exécuté avec modèle heap-relative\n");
    
    // Attendre
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_output.handle,
        .timeout_ns = 1000000000
    };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    printf("✓ GPU terminé\n");
    
    // Vérifier AVANT munmap
    printf("\n=== VÉRIFICATION RÉSULTAT ===\n");
    printf("output[0] = 0x%08x\n", output[0]);
    printf("output[1] = 0x%08x\n", output[1]);
    
    int success = (output[0] == 0x12345678);
    
    if (success) {
        printf("\n🎉🎉🎉 VICTOIRE TOTALE! 🎉🎉🎉\n");
        printf("✓✓✓ MODÈLE HEAP-RELATIVE FONCTIONNE!\n");
        printf("✓✓✓ ROOT CAUSE #44 RÉSOLU!\n");
        printf("✓✓✓ OpenCL ÉLIMINÉ À 100%%!\n");
    } else {
        printf("\n❌ ÉCHEC: output[0] = 0x%08x\n", output[0]);
    }
    
    // Cleanup APRÈS vérification
    munmap(heap, 8192);
    munmap(output, 4096);
    munmap(batch, 4096);
    close(fd);
    
    return success ? 0 : 1;
}

// Made with Bob - Test C540