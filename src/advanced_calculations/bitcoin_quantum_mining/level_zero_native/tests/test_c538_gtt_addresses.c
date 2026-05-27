// TEST C538 - 2-PASS AVEC ADRESSES GTT CORRECTES
// ROOT CAUSE #43: Utiliser adresses GTT GPU au lieu d'offsets CPU relatifs

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

// Kernel ISA minimal (section .text de test_c532)
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

#define KERNEL_SIZE sizeof(kernel_isa)

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
    printf("=== TEST C538 - 2-PASS AVEC ADRESSES GTT CORRECTES ===\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            perror("open DRM");
            return 1;
        }
    }
    printf("✓ DRM ouvert: fd=%d\n", fd);
    
    // 1. Créer les buffers
    struct drm_i915_gem_create create_output = { .size = 4096 };
    struct drm_i915_gem_create create_surface = { .size = 4096 };
    struct drm_i915_gem_create create_idrt = { .size = 4096 };
    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_surface) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_idrt) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch)) {
        perror("gem_create");
        return 1;
    }
    
    printf("✓ Buffers créés:\n");
    printf("  output:  handle=%u\n", create_output.handle);
    printf("  surface: handle=%u\n", create_surface.handle);
    printf("  idrt:    handle=%u\n", create_idrt.handle);
    printf("  kernel:  handle=%u\n", create_kernel.handle);
    printf("  batch:   handle=%u\n", create_batch.handle);
    
    // 2. Mapper les buffers
    struct drm_i915_gem_mmap mmap_output = { .handle = create_output.handle, .size = 4096 };
    struct drm_i915_gem_mmap mmap_surface = { .handle = create_surface.handle, .size = 4096 };
    struct drm_i915_gem_mmap mmap_idrt = { .handle = create_idrt.handle, .size = 4096 };
    struct drm_i915_gem_mmap mmap_kernel = { .handle = create_kernel.handle, .size = 4096 };
    struct drm_i915_gem_mmap mmap_batch = { .handle = create_batch.handle, .size = 4096 };
    
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_surface) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_idrt) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel) ||
        drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch)) {
        perror("gem_mmap");
        return 1;
    }
    
    uint32_t *output = (uint32_t *)(uintptr_t)mmap_output.addr_ptr;
    uint32_t *surface_state = (uint32_t *)(uintptr_t)mmap_surface.addr_ptr;
    uint32_t *idrt = (uint32_t *)(uintptr_t)mmap_idrt.addr_ptr;
    uint8_t *kernel = (uint8_t *)(uintptr_t)mmap_kernel.addr_ptr;
    uint8_t *batch = (uint8_t *)(uintptr_t)mmap_batch.addr_ptr;
    
    printf("✓ Buffers mappés (adresses CPU):\n");
    printf("  output:  %p\n", (void*)output);
    printf("  surface: %p\n", (void*)surface_state);
    printf("  idrt:    %p\n", (void*)idrt);
    printf("  kernel:  %p\n", (void*)kernel);
    printf("  batch:   %p\n", (void*)batch);
    
    // 3. Initialiser output avec pattern
    for (int i = 0; i < 1024; i++) {
        output[i] = 0xFFFFFFFF;
    }
    printf("✓ Output initialisé: 0xFFFFFFFF\n");
    
    // 4. Copier kernel ISA
    memcpy(kernel, kernel_isa, KERNEL_SIZE);
    printf("✓ Kernel ISA copié: %zu bytes\n", KERNEL_SIZE);
    dump_hex("Kernel ISA", kernel, KERNEL_SIZE);
    
    // 5. PASS 1 - Obtenir les adresses GTT assignées par i915
    printf("\n=== PASS 1 - OBTENTION ADRESSES GTT (OPTION A: SUGGÉRER ADRESSES) ===\n");
    
    struct drm_i915_gem_exec_object2 exec_objects_pass1[5] = {
        { .handle = create_output.handle, .offset = 0x100000, .flags = EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED },
        { .handle = create_surface.handle, .offset = 0x101000, .flags = EXEC_OBJECT_PINNED },
        { .handle = create_idrt.handle, .offset = 0x102000, .flags = EXEC_OBJECT_PINNED },
        { .handle = create_kernel.handle, .offset = 0x103000, .flags = EXEC_OBJECT_PINNED },
        { .handle = create_batch.handle, .offset = 0x104000, .flags = EXEC_OBJECT_PINNED }
    };
    
    // Batch minimal pour PASS 1 (juste BATCH_BUFFER_END)
    uint32_t *batch32 = (uint32_t *)batch;
    batch32[0] = 0x05000000;  // MI_BATCH_BUFFER_END
    
    struct drm_i915_gem_execbuffer2 execbuf_pass1 = {
        .buffers_ptr = (uintptr_t)exec_objects_pass1,
        .buffer_count = 5,
        .flags = I915_EXEC_RENDER
    };
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1);
    if (ret) {
        perror("EXECBUFFER2 PASS 1");
        return 1;
    }
    
    // LIRE LES ADRESSES GTT ASSIGNÉES
    uint64_t output_gtt = exec_objects_pass1[0].offset;
    uint64_t surface_gtt = exec_objects_pass1[1].offset;
    uint64_t idrt_gtt = exec_objects_pass1[2].offset;
    uint64_t kernel_gtt = exec_objects_pass1[3].offset;
    uint64_t batch_gtt = exec_objects_pass1[4].offset;
    
    printf("✓ PASS 1 réussi - Adresses GTT assignées:\n");
    printf("  output_gtt:  0x%016lx\n", output_gtt);
    printf("  surface_gtt: 0x%016lx\n", surface_gtt);
    printf("  idrt_gtt:    0x%016lx\n", idrt_gtt);
    printf("  kernel_gtt:  0x%016lx\n", kernel_gtt);
    printf("  batch_gtt:   0x%016lx\n", batch_gtt);
    
    // 6. PATCHER LES STRUCTURES AVEC ADRESSES GTT
    printf("\n=== PATCHING AVEC ADRESSES GTT ===\n");
    
    // 6a. Patcher IDRT avec adresse kernel GTT
    memset(idrt, 0, 4096);
    idrt[0] = kernel_gtt & 0xFFFFFFFF;           // DW0: Kernel LOW
    idrt[1] = (kernel_gtt >> 32) & 0xFFFFFFFF;   // DW1: Kernel HIGH
    idrt[2] = 0x00000000;                         // DW2: Reserved
    idrt[3] = 0x001A0000;                         // DW3: BTI offset 0x1A0
    idrt[4] = 0x00000000;                         // DW4: CURBE
    idrt[5] = 0x00000001;                         // DW5: 1 thread
    idrt[6] = 0x00000000;                         // DW6: Cross-thread
    idrt[7] = 0x00000000;                         // DW7: Reserved
    
    printf("✓ IDRT patché avec kernel_gtt=0x%016lx\n", kernel_gtt);
    dump_hex("IDRT", idrt, 32);
    
    // 6b. Patcher Surface State avec adresse output GTT
    memset(surface_state, 0, 4096);
    surface_state[0] = 0x00000004;                    // DW0: BUFFER type
    surface_state[1] = output_gtt & 0xFFFFFFFF;       // DW1: Address LOW
    surface_state[2] = (output_gtt >> 32) & 0xFFFFFFFF; // DW2: Address HIGH
    surface_state[3] = 0x00000FFF;                    // DW3: Width 4096-1
    surface_state[4] = 0x00000000;                    // DW4: Height
    surface_state[5] = 0x00001000;                    // DW5: Pitch 4096
    
    // Binding Table à offset 0x1A0
    uint32_t *binding_table = &surface_state[0x1A0 / 4];
    binding_table[0] = 0x00000000;  // BTI[0] pointe vers Surface State à offset 0
    
    printf("✓ Surface State patché avec output_gtt=0x%016lx\n", output_gtt);
    printf("✓ Binding Table à offset 0x1A0\n");
    dump_hex("Surface State", surface_state, 64);
    dump_hex("Binding Table", binding_table, 16);
    
    // 6c. Construire batch avec adresses GTT
    memset(batch, 0, 4096);
    uint32_t *batch_dw = (uint32_t *)batch;
    int idx = 0;
    
    // PIPELINE_SELECT
    batch_dw[idx++] = 0x69040000;  // PIPELINE_SELECT
    batch_dw[idx++] = 0x00000000;  // GPGPU mode
    
    // STATE_BASE_ADDRESS
    batch_dw[idx++] = 0x61010009;  // STATE_BASE_ADDRESS (9 DWORDs)
    batch_dw[idx++] = 0x00000001;  // General State Base: enable
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;  // Surface State Base: enable
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;  // Dynamic State Base: enable
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;  // Indirect Object Base: enable
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;  // Instruction Base: enable
    batch_dw[idx++] = 0x00000000;
    
    // MEDIA_VFE_STATE
    batch_dw[idx++] = 0x70000006;  // MEDIA_VFE_STATE (6 DWORDs)
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD avec adresse IDRT GTT
    int idrt_cmd_offset = idx * 4;
    batch_dw[idx++] = 0x70020002;  // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000020;  // 32 bytes (1 IDRT)
    batch_dw[idx++] = idrt_gtt & 0xFFFFFFFF;      // IDRT address LOW
    batch_dw[idx++] = (idrt_gtt >> 32) & 0xFFFFFFFF; // IDRT address HIGH
    
    printf("✓ Batch: MEDIA_INTERFACE_DESCRIPTOR_LOAD @ offset 0x%x\n", idrt_cmd_offset);
    printf("  → idrt_gtt = 0x%016lx\n", idrt_gtt);
    
    // GPGPU_WALKER
    batch_dw[idx++] = 0x71000011;  // GPGPU_WALKER (17 DWORDs)
    batch_dw[idx++] = 0x00000000;  // Interface Descriptor Offset: 0
    batch_dw[idx++] = 0x00000000;  // Indirect Data Length: 0
    batch_dw[idx++] = 0x00000000;  // Indirect Data Start Address: 0
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;  // Thread Group ID Starting X: 0
    batch_dw[idx++] = 0x00000000;  // Thread Group ID X Dimension: 1
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;  // Thread Group ID Starting Y: 0
    batch_dw[idx++] = 0x00000000;  // Thread Group ID Y Dimension: 1
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;  // Thread Group ID Starting Z: 0
    batch_dw[idx++] = 0x00000000;  // Thread Group ID Z Dimension: 1
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;  // Right Execution Mask
    batch_dw[idx++] = 0xFFFFFFFF;
    batch_dw[idx++] = 0x00000000;  // Bottom Execution Mask
    batch_dw[idx++] = 0xFFFFFFFF;
    
    // MEDIA_STATE_FLUSH
    batch_dw[idx++] = 0x70040000;
    batch_dw[idx++] = 0x00000000;
    
    // PIPE_CONTROL avec flush
    batch_dw[idx++] = 0x7A000004;  // PIPE_CONTROL
    batch_dw[idx++] = 0x00100000;  // DC Flush Enable
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch_dw[idx++] = 0x05000000;
    
    int batch_size = idx * 4;
    printf("✓ Batch construit: %d bytes\n", batch_size);
    dump_hex("Batch", batch, batch_size);
    
    // 7. PASS 2 - Exécuter avec adresses GTT fixées
    printf("\n=== PASS 2 - EXÉCUTION AVEC ADRESSES GTT FIXÉES ===\n");
    
    struct drm_i915_gem_exec_object2 exec_objects_pass2[5] = {
        { .handle = create_output.handle, .offset = output_gtt, .flags = EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED },
        { .handle = create_surface.handle, .offset = surface_gtt, .flags = EXEC_OBJECT_PINNED },
        { .handle = create_idrt.handle, .offset = idrt_gtt, .flags = EXEC_OBJECT_PINNED },
        { .handle = create_kernel.handle, .offset = kernel_gtt, .flags = EXEC_OBJECT_PINNED },
        { .handle = create_batch.handle, .offset = batch_gtt, .flags = EXEC_OBJECT_PINNED }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf_pass2 = {
        .buffers_ptr = (uintptr_t)exec_objects_pass2,
        .buffer_count = 5,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC
    };
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass2);
    if (ret) {
        perror("EXECBUFFER2 PASS 2");
        printf("❌ PASS 2 échoué\n");
        return 1;
    }
    
    printf("✓ PASS 2 réussi - GPU exécuté\n");
    
    // 8. Attendre completion
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_output.handle,
        .timeout_ns = 1000000000  // 1 seconde
    };
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    if (ret) {
        perror("gem_wait");
    }
    printf("✓ GPU terminé\n");
    
    // 9. Vérifier résultat
    printf("\n=== VÉRIFICATION RÉSULTAT ===\n");
    printf("output[0] = 0x%08x\n", output[0]);
    printf("output[1] = 0x%08x\n", output[1]);
    printf("output[2] = 0x%08x\n", output[2]);
    printf("output[3] = 0x%08x\n", output[3]);
    
    if (output[0] == 0x12345678) {
        printf("\n✓✓✓ SUCCÈS! output[0] = 0x12345678\n");
        printf("✓✓✓ GPU NATIF i915 FONCTIONNE!\n");
        printf("✓✓✓ ROOT CAUSE #43 RÉSOLU!\n");
    } else {
        printf("\n❌ ÉCHEC: output[0] = 0x%08x (attendu 0x12345678)\n", output[0]);
    }
    
    // Cleanup
    munmap(output, 4096);
    munmap(surface_state, 4096);
    munmap(idrt, 4096);
    munmap(kernel, 4096);
    munmap(batch, 4096);
    
    struct drm_gem_close close_arg;
    close_arg.handle = create_output.handle; drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_surface.handle; drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_idrt.handle; drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_kernel.handle; drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_batch.handle; drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    close(fd);
    
    return (output[0] == 0x12345678) ? 0 : 1;
}

// Made with Bob - Test C538