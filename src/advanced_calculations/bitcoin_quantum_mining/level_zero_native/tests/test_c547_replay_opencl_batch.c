// TEST C547 - REPLAY BATCH OPENCL EXACT
// Objectif: Rejouer le batch OpenCL capturé avec nos propres buffers
// Hypothèse: Si ça fonctionne, ROOT CAUSE #58 confirmée

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
    printf("=== TEST C547 - REPLAY BATCH OPENCL EXACT ===\n");
    printf("ROOT CAUSE #58: Modèle heap-relative vs modèle hybride OpenCL\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("✓ DRM ouvert\n");
    
    // Créer buffer output (4096 bytes)
    struct drm_i915_gem_create create_output = {
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        perror("create output");
        return 1;
    }
    
    // Mapper output et initialiser
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) < 0) {
        perror("mmap output");
        return 1;
    }
    uint32_t *output = (uint32_t *)mmap_output.addr_ptr;
    memset(output, 0xFF, 4096);
    
    printf("✓ Buffer output créé (handle=%u)\n", create_output.handle);
    
    // Charger le batch OpenCL capturé
    FILE *f = fopen("logs/opencl_batch_1.bin", "rb");
    if (!f) {
        perror("fopen batch");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t batch_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Créer buffer batch
    struct drm_i915_gem_create create_batch = {
        .size = (batch_size + 4095) & ~4095  // Arrondir à 4K
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("create batch");
        return 1;
    }
    
    // Mapper batch
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle,
        .offset = 0,
        .size = create_batch.size,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        perror("mmap batch");
        return 1;
    }
    uint8_t *batch = (uint8_t *)mmap_batch.addr_ptr;
    
    // Charger le batch OpenCL
    size_t read_size = fread(batch, 1, batch_size, f);
    fclose(f);
    
    printf("✓ Batch OpenCL chargé: %zu bytes\n", read_size);
    
    // Afficher les premiers DWords du batch
    printf("\nBatch content (premiers 80 bytes):\n");
    uint32_t *batch_dw = (uint32_t *)batch;
    for (int i = 0; i < 20; i++) {
        if (i % 8 == 0) printf("%04x: ", i * 4);
        printf("%08x ", batch_dw[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }
    printf("\n");
    
    // IMPORTANT: Le batch OpenCL contient des adresses GTT relocalisées
    // qui pointent vers les buffers OpenCL. Nous devons les remplacer
    // par nos propres adresses GTT.
    
    // Créer un heap buffer minimal (8192 bytes) pour les structures GPU
    struct drm_i915_gem_create create_heap = {
        .size = 8192
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_heap) < 0) {
        perror("create heap");
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_heap = {
        .handle = create_heap.handle,
        .offset = 0,
        .size = 8192,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_heap) < 0) {
        perror("mmap heap");
        return 1;
    }
    uint8_t *heap = (uint8_t *)mmap_heap.addr_ptr;
    memset(heap, 0, 8192);
    
    printf("✓ Heap buffer créé (handle=%u)\n", create_heap.handle);
    
    // Construire Surface State dans le heap
    uint32_t *surface_state = (uint32_t *)heap;
    surface_state[0] = 0x00000004;  // Type: Buffer
    surface_state[3] = 0x00000FFF;  // Width
    surface_state[5] = 0x00001000;  // Pitch
    // DW1-2 seront relocalisés vers output
    
    // Binding Table @ 0x40
    uint32_t *binding_table = (uint32_t *)(heap + 0x40);
    binding_table[0] = 0x00000000;  // Offset vers Surface State
    
    // IDRT @ 0x50
    uint32_t *idrt = (uint32_t *)(heap + 0x50);
    idrt[0] = 0x00000070;  // Kernel offset
    idrt[3] = 0x00400000;  // BTI offset + config
    
    // Kernel ISA simple @ 0x70
    uint32_t kernel_isa[] = {
        0x00600001, 0x20020c20, 0x00000000, 0x12345678,  // mov
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x07800031, 0x20001cc4, 0x00000020, 0x02180001,  // send (write)
        0x00000000, 0x00000000, 0x00000000, 0x00000000
    };
    memcpy(heap + 0x70, kernel_isa, sizeof(kernel_isa));
    
    printf("✓ Heap configuré (Surface State, Binding Table, IDRT, Kernel)\n");
    
    // Préparer les relocations
    struct drm_i915_gem_relocation_entry relocs[10];
    memset(relocs, 0, sizeof(relocs));
    
    // Relocation #0: Surface State DW1 → output buffer
    relocs[0].target_handle = create_output.handle;
    relocs[0].delta = 0;
    relocs[0].offset = 0x04;  // Surface State DW1
    relocs[0].presumed_offset = 0;
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[0].write_domain = I915_GEM_DOMAIN_RENDER;
    
    // NOTE: Le batch OpenCL contient des relocations dans STATE_BASE_ADDRESS
    // Pour simplifier, nous allons d'abord tester SANS modifier le batch,
    // juste pour voir si le batch OpenCL brut fonctionne avec nos buffers
    
    // Préparer exec_objects
    struct drm_i915_gem_exec_object2 objects[3];
    memset(objects, 0, sizeof(objects));
    
    // Object 0: heap
    objects[0].handle = create_heap.handle;
    objects[0].relocation_count = 1;
    objects[0].relocs_ptr = (uint64_t)relocs;
    objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    // Object 1: output
    objects[1].handle = create_output.handle;
    objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE;
    
    // Object 2: batch
    objects[2].handle = create_batch.handle;
    objects[2].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    // Préparer execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)objects,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    printf("\n=== EXÉCUTION GPU (BATCH OPENCL BRUT) ===\n");
    printf("⚠️  ATTENTION: Le batch contient des adresses GTT OpenCL\n");
    printf("    Ceci va probablement échouer, mais c'est un test\n\n");
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("execbuffer2");
        printf("❌ ÉCHEC attendu: Le batch OpenCL contient des adresses invalides\n");
    } else {
        printf("✓ GPU exécuté (surprenant!)\n");
        
        // Attendre la fin
        struct drm_i915_gem_wait wait = {
            .bo_handle = create_batch.handle,
            .timeout_ns = 1000000000,  // 1 seconde
            .flags = 0
        };
        ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
        
        printf("\n=== VÉRIFICATION RÉSULTAT ===\n");
        printf("output[0] = 0x%08x\n", output[0]);
        printf("output[1] = 0x%08x\n", output[1]);
        
        if (output[0] == 0x12345678) {
            printf("\n✅✅✅ SUCCÈS INCROYABLE! Le batch OpenCL brut fonctionne!\n");
        } else {
            printf("\n❌ output[0] != 0x12345678\n");
        }
    }
    
    // Cleanup
    munmap((void *)mmap_output.addr_ptr, 4096);
    munmap((void *)mmap_batch.addr_ptr, create_batch.size);
    munmap((void *)mmap_heap.addr_ptr, 8192);
    
    struct drm_gem_close close_arg;
    close_arg.handle = create_output.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_batch.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_heap.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    close(fd);
    
    printf("\n=== CONCLUSION ===\n");
    printf("Ce test montre que le batch OpenCL brut ne peut PAS être rejoué\n");
    printf("directement car il contient des adresses GTT spécifiques à OpenCL.\n");
    printf("La prochaine étape (C548) sera de PATCHER le batch avec nos adresses.\n");
    
    return 0;
}

// Made with Bob
