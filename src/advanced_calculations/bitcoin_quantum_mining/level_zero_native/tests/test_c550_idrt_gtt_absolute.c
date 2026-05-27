// TEST C550 - IDRT AVEC ADRESSES GTT ABSOLUES
// Objectif: Implémenter IDRT exact comme OpenCL avec adresses GTT absolues
// ROOT CAUSE #59: Offsets relatifs → Adresses GTT absolues

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libdrm/i915_drm.h>
#include <libdrm/drm.h>

int main() {
    printf("=== TEST C550 - IDRT AVEC ADRESSES GTT ABSOLUES ===\n");
    printf("ROOT CAUSE #59: Implémenter modèle OpenCL exact\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("✓ DRM ouvert\n");
    
    // Créer buffers
    struct drm_i915_gem_create create_heap = { .size = 8192 };
    struct drm_i915_gem_create create_output = { .size = 4096 };
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_heap);
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output);
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch);
    
    printf("✓ Buffers créés: heap=%u, output=%u, batch=%u\n",
           create_heap.handle, create_output.handle, create_batch.handle);
    
    // Mapper buffers
    struct drm_i915_gem_mmap mmap_heap = {
        .handle = create_heap.handle, .size = 8192
    };
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle, .size = 4096
    };
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle, .size = 4096
    };
    
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_heap);
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output);
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch);
    
    uint8_t *heap = (uint8_t *)mmap_heap.addr_ptr;
    uint32_t *output = (uint32_t *)mmap_output.addr_ptr;
    uint32_t *batch = (uint32_t *)mmap_batch.addr_ptr;
    
    memset(heap, 0, 8192);
    memset(output, 0xFF, 4096);
    memset(batch, 0, 4096);
    
    printf("✓ Buffers mappés\n");
    
    // === CONSTRUCTION HEAP AVEC ADRESSES GTT ABSOLUES ===
    printf("\n=== CONSTRUCTION HEAP (MODÈLE OPENCL) ===\n");
    
    // Surface State @ 0x0000
    uint32_t *surface_state = (uint32_t *)heap;
    surface_state[0] = 0x00000004;  // Type: Buffer
    surface_state[3] = 0x00000FFF;  // Width
    surface_state[5] = 0x00001000;  // Pitch
    // DW1-2: Adresse output (sera relocalisée)
    
    printf("✓ Surface State @ 0x0000\n");
    
    // Binding Table @ 0x0040
    uint32_t *binding_table = (uint32_t *)(heap + 0x40);
    binding_table[0] = 0x00000000;  // Offset vers Surface State
    
    printf("✓ Binding Table @ 0x0040\n");
    
    // IDRT @ 0x0050 - MODÈLE OPENCL EXACT
    uint32_t *idrt = (uint32_t *)(heap + 0x50);
    
    // DW0: Kernel Start Pointer (sera patché avec adresse GTT absolue)
    idrt[0] = 0x00000070;  // Temporaire, sera remplacé par GTT
    
    // DW1: Reserved
    idrt[1] = 0x00000000;
    
    // DW2: Sampler State Pointer
    idrt[2] = 0x00000000;  // Pas de sampler
    
    // DW3: Binding Table Pointer (ADRESSE GTT ABSOLUE!)
    // Sera patché avec: heap_gtt + 0x40
    idrt[3] = 0x00000040;  // Temporaire
    
    // DW4: CURBE Read Offset + Length
    idrt[4] = 0x00000020;  // Length = 32 bytes (comme OpenCL)
    
    // DW5: Barrier + SLM + Threads
    idrt[5] = 0x00000001;  // 1 thread
    
    // DW6: Cross-Thread Constant Data Read Length
    idrt[6] = 0x0000000D;  // 13 DWords (comme OpenCL)
    
    // DW7: Reserved
    idrt[7] = 0x00000000;
    
    printf("✓ IDRT @ 0x0050 (sera patché avec adresses GTT)\n");
    
    // Kernel ISA @ 0x0070
    uint32_t kernel_isa[] = {
        0x00600001, 0x20020c20, 0x00000000, 0x12345678,  // mov r1.0<1>:ud 0x12345678
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x07800031, 0x20001cc4, 0x00000020, 0x02180001,  // send (write)
        0x00000000, 0x00000000, 0x00000000, 0x00000000
    };
    memcpy(heap + 0x70, kernel_isa, sizeof(kernel_isa));
    
    printf("✓ Kernel ISA @ 0x0070\n");
    
    // CURBE Data @ 0x00B0 (32 bytes comme OpenCL)
    uint32_t *curbe = (uint32_t *)(heap + 0xB0);
    for (int i = 0; i < 8; i++) {
        curbe[i] = 0x00000000;  // Données constantes
    }
    
    printf("✓ CURBE @ 0x00B0 (32 bytes)\n");
    
    // === CONSTRUCTION BATCH ===
    printf("\n=== CONSTRUCTION BATCH ===\n");
    
    int idx = 0;
    
    // PIPELINE_SELECT
    batch[idx++] = 0x69041312;
    
    // STATE_BASE_ADDRESS (version complète 19 DWords comme OpenCL)
    batch[idx++] = 0x61010011;  // Opcode + length=19
    batch[idx++] = 0x00000001;  // General State Base Modify
    batch[idx++] = 0x00000000;  // General State Base LOW
    batch[idx++] = 0x00000000;  // General State Base HIGH
    
    // Surface State Base (sera relocalisé vers heap)
    batch[idx++] = 0x00000001;  // Modify Enable
    batch[idx++] = 0x00000000;  // LOW (relocation)
    
    // Dynamic State Base (sera relocalisé vers heap)
    batch[idx++] = 0x00000001;  // Modify Enable
    batch[idx++] = 0x00000000;  // LOW (relocation)
    
    // Indirect Object Base
    batch[idx++] = 0x00000001;  // Modify Enable
    batch[idx++] = 0x00000000;  // LOW
    
    // Instruction Base (sera relocalisé vers heap)
    batch[idx++] = 0x00000001;  // Modify Enable
    batch[idx++] = 0x00000000;  // LOW (relocation)
    
    // Buffer sizes
    batch[idx++] = 0xFFFFF001;  // General State Buffer Size
    batch[idx++] = 0xFFFFF001;  // Dynamic State Buffer Size
    batch[idx++] = 0x00000001;  // Indirect Object Buffer Size
    batch[idx++] = 0x00000000;  // HIGH
    batch[idx++] = 0x003BF000;  // Instruction Buffer Size
    
    int state_base_address_idx = idx - 19;
    
    printf("✓ STATE_BASE_ADDRESS @ 0x%04x (19 DWords)\n", state_base_address_idx * 4);
    
    // MEDIA_VFE_STATE
    batch[idx++] = 0x70000007;  // Opcode + length=9
    batch[idx++] = 0x00000000;  // Scratch Space Base
    batch[idx++] = 0x00000000;  // HIGH
    batch[idx++] = 0x00A70100;  // Max Threads=167, URB Entries=1 (comme OpenCL)
    batch[idx++] = 0x00000000;  // URB Entry Size
    batch[idx++] = 0x07820000;  // CURBE Size=1922 (comme OpenCL)
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    printf("✓ MEDIA_VFE_STATE @ 0x%04x\n", (idx - 9) * 4);
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = 0x61020001;  // Opcode + length=3
    batch[idx++] = 0x00000020;  // Interface Descriptor Total Length (32 bytes)
    batch[idx++] = 0x00000050;  // Interface Descriptor Data Start (offset 0x50 dans heap)
    
    printf("✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD @ 0x%04x\n", (idx - 3) * 4);
    
    // GPGPU_WALKER
    batch[idx++] = 0x18800101;  // Opcode + length
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset (sera patché)
    batch[idx++] = 0x00000000;  // Indirect Data Length
    batch[idx++] = 0x00000000;  // Thread Group ID Starting X
    batch[idx++] = 0x00000000;  // Thread Group ID Starting Y
    batch[idx++] = 0x00000000;  // Thread Group ID Starting Z
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000000;  // Thread Group ID X Dimension (0 = 1 thread)
    batch[idx++] = 0x00000000;  // Thread Group ID Y Dimension
    batch[idx++] = 0x00000000;  // Thread Group ID Z Dimension
    batch[idx++] = 0x00000000;  // Thread Group ID Z Dimension
    
    int gpgpu_walker_idx = idx - 11;
    
    printf("✓ GPGPU_WALKER @ 0x%04x\n", gpgpu_walker_idx * 4);
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x05000000;
    
    int batch_len = idx * 4;
    printf("✓ Batch: %d bytes\n", batch_len);
    
    // === CONFIGURATION RELOCATIONS ===
    printf("\n=== CONFIGURATION RELOCATIONS ===\n");
    
    struct drm_i915_gem_relocation_entry relocs[10];
    memset(relocs, 0, sizeof(relocs));
    
    int reloc_idx = 0;
    
    // Relocation #0: Surface State DW1 → output
    relocs[reloc_idx].target_handle = create_output.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = 0x04;  // Surface State DW1
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[reloc_idx].write_domain = I915_GEM_DOMAIN_RENDER;
    reloc_idx++;
    
    // Relocations STATE_BASE_ADDRESS → heap
    int sba_base = state_base_address_idx * 4;
    
    // Surface State Base (DW5-6)
    relocs[reloc_idx].target_handle = create_heap.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = sba_base + 0x14;  // DW5
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    
    // Dynamic State Base (DW7-8)
    relocs[reloc_idx].target_handle = create_heap.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = sba_base + 0x1C;  // DW7
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    
    // Instruction Base (DW11-12)
    relocs[reloc_idx].target_handle = create_heap.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = sba_base + 0x2C;  // DW11
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    
    printf("✓ %d relocations configurées\n", reloc_idx);
    
    // === PRÉPARATION EXECBUFFER ===
    struct drm_i915_gem_exec_object2 objects[3];
    memset(objects, 0, sizeof(objects));
    
    // Object 0: heap (avec relocations)
    objects[0].handle = create_heap.handle;
    objects[0].relocation_count = reloc_idx;
    objects[0].relocs_ptr = (uint64_t)relocs;
    objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    // Object 1: output
    objects[1].handle = create_output.handle;
    objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE;
    
    // Object 2: batch
    objects[2].handle = create_batch.handle;
    objects[2].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)objects,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER
    };
    
    printf("\n=== EXÉCUTION GPU (MODÈLE OPENCL) ===\n");
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("execbuffer2");
        printf("❌ ÉCHEC exécution GPU\n");
    } else {
        printf("✓ GPU exécuté\n");
        
        // IMPORTANT: Après execbuffer2, objects[].offset contient les adresses GTT!
        printf("\n=== ADRESSES GTT APRÈS PINNING ===\n");
        printf("heap   GTT: 0x%016llx\n", objects[0].offset);
        printf("output GTT: 0x%016llx\n", objects[1].offset);
        printf("batch  GTT: 0x%016llx\n", objects[2].offset);
        
        // TODO C551: Patcher IDRT avec ces adresses GTT absolues
        // idrt[0] = objects[0].offset + 0x70;  // Kernel @ heap+0x70
        // idrt[3] = objects[0].offset + 0x40;  // BTI @ heap+0x40
        
        // Attendre fin
        struct drm_i915_gem_wait wait = {
            .bo_handle = create_batch.handle,
            .timeout_ns = 1000000000
        };
        ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
        
        printf("\n=== VÉRIFICATION RÉSULTAT ===\n");
        printf("output[0] = 0x%08x\n", output[0]);
        printf("output[1] = 0x%08x\n", output[1]);
        
        if (output[0] == 0x12345678) {
            printf("\n✅✅✅ SUCCÈS! GPU natif fonctionne!\n");
        } else {
            printf("\n❌ output[0] != 0x12345678\n");
            printf("NOTE: Adresses GTT disponibles pour C551\n");
        }
    }
    
    // Cleanup
    munmap((void *)mmap_heap.addr_ptr, 8192);
    munmap((void *)mmap_output.addr_ptr, 4096);
    munmap((void *)mmap_batch.addr_ptr, 4096);
    
    struct drm_gem_close close_arg;
    close_arg.handle = create_heap.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_output.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_batch.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    close(fd);
    
    return 0;
}

// Made with Bob
