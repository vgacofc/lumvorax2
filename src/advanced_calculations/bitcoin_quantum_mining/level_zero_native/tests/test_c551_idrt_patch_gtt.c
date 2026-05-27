// TEST C551 - PATCH IDRT AVEC ADRESSES GTT ABSOLUES
// Objectif: 2-PASS - Patcher IDRT après pinning avec adresses GTT réelles
// ROOT CAUSE #59 SOLUTION FINALE

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
    printf("=== TEST C551 - PATCH IDRT AVEC ADRESSES GTT (2-PASS) ===\n");
    printf("ROOT CAUSE #59 SOLUTION: Patcher IDRT après pinning\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    // Créer buffers
    struct drm_i915_gem_create create_heap = { .size = 8192 };
    struct drm_i915_gem_create create_output = { .size = 4096 };
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_heap);
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output);
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch);
    
    // Mapper buffers
    struct drm_i915_gem_mmap mmap_heap = { .handle = create_heap.handle, .size = 8192 };
    struct drm_i915_gem_mmap mmap_output = { .handle = create_output.handle, .size = 4096 };
    struct drm_i915_gem_mmap mmap_batch = { .handle = create_batch.handle, .size = 4096 };
    
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_heap);
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output);
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch);
    
    uint8_t *heap = (uint8_t *)mmap_heap.addr_ptr;
    uint32_t *output = (uint32_t *)mmap_output.addr_ptr;
    uint32_t *batch = (uint32_t *)mmap_batch.addr_ptr;
    
    memset(heap, 0, 8192);
    memset(output, 0xFF, 4096);
    memset(batch, 0, 4096);
    
    printf("✓ Buffers créés et mappés\n");
    
    // === CONSTRUCTION HEAP ===
    // Surface State @ 0x0000
    uint32_t *surface_state = (uint32_t *)heap;
    surface_state[0] = 0x00000004;
    surface_state[3] = 0x00000FFF;
    surface_state[5] = 0x00001000;
    
    // Binding Table @ 0x0040
    uint32_t *binding_table = (uint32_t *)(heap + 0x40);
    binding_table[0] = 0x00000000;
    
    // IDRT @ 0x0050 (sera patché)
    uint32_t *idrt = (uint32_t *)(heap + 0x50);
    idrt[0] = 0x00000070;  // Temporaire
    idrt[1] = 0x00000000;
    idrt[2] = 0x00000000;
    idrt[3] = 0x00000040;  // Temporaire
    idrt[4] = 0x00000020;  // CURBE 32 bytes
    idrt[5] = 0x00000001;  // 1 thread
    idrt[6] = 0x0000000D;  // Cross-thread 13 DWords
    idrt[7] = 0x00000000;
    
    // Kernel ISA @ 0x0070
    uint32_t kernel_isa[] = {
        0x00600001, 0x20020c20, 0x00000000, 0x12345678,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x07800031, 0x20001cc4, 0x00000020, 0x02180001,
        0x00000000, 0x00000000, 0x00000000, 0x00000000
    };
    memcpy(heap + 0x70, kernel_isa, sizeof(kernel_isa));
    
    printf("✓ Heap construit\n");
    
    // === CONSTRUCTION BATCH ===
    int idx = 0;
    batch[idx++] = 0x69041312;  // PIPELINE_SELECT
    
    // STATE_BASE_ADDRESS (19 DWords)
    batch[idx++] = 0x61010011;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;  // Surface State Base Modify
    batch[idx++] = 0x00000000;  // Will be relocated
    batch[idx++] = 0x00000001;  // Dynamic State Base Modify
    batch[idx++] = 0x00000000;  // Will be relocated
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;  // Instruction Base Modify
    batch[idx++] = 0x00000000;  // Will be relocated
    batch[idx++] = 0xFFFFF001;
    batch[idx++] = 0xFFFFF001;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x003BF000;
    
    int sba_idx = idx - 19;
    
    // MEDIA_VFE_STATE
    batch[idx++] = 0x70000007;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00A70100;  // Max Threads=167
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x07820000;  // CURBE=1922
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = 0x61020001;
    batch[idx++] = 0x00000020;  // 32 bytes
    batch[idx++] = 0x00000050;  // IDRT @ offset 0x50
    
    // GPGPU_WALKER
    batch[idx++] = 0x18800101;
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    batch[idx++] = 0x05000000;  // MI_BATCH_BUFFER_END
    
    int batch_len = idx * 4;
    printf("✓ Batch construit: %d bytes\n", batch_len);
    
    // === RELOCATIONS ===
    struct drm_i915_gem_relocation_entry relocs[10];
    memset(relocs, 0, sizeof(relocs));
    
    int reloc_idx = 0;
    
    // Surface State → output
    relocs[reloc_idx].target_handle = create_output.handle;
    relocs[reloc_idx].offset = 0x04;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[reloc_idx].write_domain = I915_GEM_DOMAIN_RENDER;
    reloc_idx++;
    
    // STATE_BASE_ADDRESS → heap
    relocs[reloc_idx].target_handle = create_heap.handle;
    relocs[reloc_idx].offset = sba_idx * 4 + 0x14;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_RENDER;
    reloc_idx++;
    
    relocs[reloc_idx].target_handle = create_heap.handle;
    relocs[reloc_idx].offset = sba_idx * 4 + 0x1C;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_RENDER;
    reloc_idx++;
    
    relocs[reloc_idx].target_handle = create_heap.handle;
    relocs[reloc_idx].offset = sba_idx * 4 + 0x2C;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    reloc_idx++;
    
    printf("✓ %d relocations\n", reloc_idx);
    
    // === PASS 1: PINNING POUR OBTENIR ADRESSES GTT ===
    printf("\n=== PASS 1: PINNING (obtenir adresses GTT) ===\n");
    
    struct drm_i915_gem_exec_object2 objects[3];
    memset(objects, 0, sizeof(objects));
    
    objects[0].handle = create_heap.handle;
    objects[0].relocation_count = reloc_idx;
    objects[0].relocs_ptr = (uint64_t)relocs;
    objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    objects[1].handle = create_output.handle;
    objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE;
    
    objects[2].handle = create_batch.handle;
    objects[2].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)objects,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER
    };
    
    // Exécuter pour obtenir les offsets GTT
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("execbuffer2 pass1");
        return 1;
    }
    
    printf("✓ PASS 1 exécuté\n");
    printf("Adresses GTT obtenues:\n");
    printf("  heap:   0x%016llx\n", objects[0].offset);
    printf("  output: 0x%016llx\n", objects[1].offset);
    printf("  batch:  0x%016llx\n", objects[2].offset);
    
    uint64_t heap_gtt = objects[0].offset;
    
    // === PASS 2: PATCHER IDRT ET RÉEXÉCUTER ===
    printf("\n=== PASS 2: PATCH IDRT avec adresses GTT absolues ===\n");
    
    // Patcher IDRT DW0: Kernel Start Pointer (adresse GTT absolue)
    uint64_t kernel_gtt = heap_gtt + 0x70;
    idrt[0] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
    printf("✓ IDRT DW0 patché: 0x%08x (kernel @ GTT 0x%llx)\n", idrt[0], kernel_gtt);
    
    // Patcher IDRT DW3: Binding Table Pointer (adresse GTT absolue)
    uint64_t bti_gtt = heap_gtt + 0x40;
    idrt[3] = (uint32_t)(bti_gtt & 0xFFFFFFFF);
    printf("✓ IDRT DW3 patché: 0x%08x (BTI @ GTT 0x%llx)\n", idrt[3], bti_gtt);
    
    // Réinitialiser output
    memset(output, 0xFF, 4096);
    
    // Réexécuter avec IDRT patché
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("execbuffer2 pass2");
        return 1;
    }
    
    printf("✓ PASS 2 exécuté avec IDRT patché\n");
    
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
        printf("\n🎉🎉🎉 SUCCÈS TOTAL! GPU NATIF FONCTIONNE! 🎉🎉🎉\n");
        printf("OpenCL éliminé à 100%%!\n");
    } else {
        printf("\n❌ output[0] != 0x12345678\n");
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
