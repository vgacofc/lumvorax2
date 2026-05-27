// TEST C539 - SINGLE PASS SANS NO_RELOC (Option B)
// Laisser i915 gérer les relocations automatiquement

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
    printf("=== TEST C539 - SINGLE PASS SANS NO_RELOC ===\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            perror("open DRM");
            return 1;
        }
    }
    printf("✓ DRM ouvert: fd=%d\n", fd);
    
    // Créer les buffers
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
    
    printf("✓ Buffers créés\n");
    
    // Mapper les buffers
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
    
    printf("✓ Buffers mappés\n");
    
    // Initialiser output
    for (int i = 0; i < 1024; i++) {
        output[i] = 0xFFFFFFFF;
    }
    
    // Copier kernel ISA
    memcpy(kernel, kernel_isa, sizeof(kernel_isa));
    printf("✓ Kernel ISA copié: %zu bytes\n", sizeof(kernel_isa));
    
    // Construire IDRT avec OFFSETS RELATIFS (i915 fera les relocations)
    memset(idrt, 0, 4096);
    idrt[0] = 0x00000000;  // DW0: Kernel offset LOW (sera relocalisé)
    idrt[1] = 0x00000000;  // DW1: Kernel offset HIGH
    idrt[2] = 0x00000000;  // DW2: Reserved
    idrt[3] = 0x001A0000;  // DW3: BTI offset 0x1A0
    idrt[4] = 0x00000000;  // DW4: CURBE
    idrt[5] = 0x00000001;  // DW5: 1 thread
    idrt[6] = 0x00000000;  // DW6: Cross-thread
    idrt[7] = 0x00000000;  // DW7: Reserved
    
    printf("✓ IDRT construit (offsets relatifs)\n");
    
    // Construire Surface State avec OFFSETS RELATIFS
    memset(surface_state, 0, 4096);
    surface_state[0] = 0x00000004;  // DW0: BUFFER type
    surface_state[1] = 0x00000000;  // DW1: Address LOW (sera relocalisé)
    surface_state[2] = 0x00000000;  // DW2: Address HIGH
    surface_state[3] = 0x00000FFF;  // DW3: Width 4096-1
    surface_state[4] = 0x00000000;  // DW4: Height
    surface_state[5] = 0x00001000;  // DW5: Pitch 4096
    
    // Binding Table
    uint32_t *binding_table = &surface_state[0x1A0 / 4];
    binding_table[0] = 0x00000000;  // BTI[0] → Surface State offset 0
    
    printf("✓ Surface State construit (offsets relatifs)\n");
    
    // Construire batch avec OFFSETS RELATIFS
    memset(batch, 0, 4096);
    uint32_t *batch_dw = (uint32_t *)batch;
    int idx = 0;
    
    // PIPELINE_SELECT
    batch_dw[idx++] = 0x69040000;
    batch_dw[idx++] = 0x00000000;
    
    // STATE_BASE_ADDRESS
    batch_dw[idx++] = 0x61010009;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    
    // MEDIA_VFE_STATE
    batch_dw[idx++] = 0x70000006;
    for (int i = 0; i < 7; i++) batch_dw[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (offset relatif)
    batch_dw[idx++] = 0x70020002;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000020;  // 32 bytes
    batch_dw[idx++] = 0x00000000;  // IDRT offset LOW (sera relocalisé)
    batch_dw[idx++] = 0x00000000;  // IDRT offset HIGH
    
    // GPGPU_WALKER
    batch_dw[idx++] = 0x71000011;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000001;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0xFFFFFFFF;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0xFFFFFFFF;
    
    // MEDIA_STATE_FLUSH
    batch_dw[idx++] = 0x70040000;
    batch_dw[idx++] = 0x00000000;
    
    // PIPE_CONTROL
    batch_dw[idx++] = 0x7A000004;
    batch_dw[idx++] = 0x00100000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    batch_dw[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch_dw[idx++] = 0x05000000;
    
    int batch_size = idx * 4;
    printf("✓ Batch construit: %d bytes (offsets relatifs)\n", batch_size);
    
    // SINGLE PASS - i915 fera les relocations
    printf("\n=== SINGLE PASS - RELOCATIONS AUTOMATIQUES ===\n");
    
    struct drm_i915_gem_relocation_entry relocs[3] = {
        // Relocation #0: IDRT DW0-DW1 → kernel
        {
            .target_handle = create_kernel.handle,
            .delta = 0,
            .offset = (uintptr_t)idrt - (uintptr_t)mmap_idrt.addr_ptr,  // Offset dans buffer IDRT
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        },
        // Relocation #1: Surface State DW1-DW2 → output
        {
            .target_handle = create_output.handle,
            .delta = 0,
            .offset = (uintptr_t)&surface_state[1] - (uintptr_t)mmap_surface.addr_ptr,
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_RENDER,
            .write_domain = I915_GEM_DOMAIN_RENDER
        },
        // Relocation #2: Batch MEDIA_INTERFACE_DESCRIPTOR_LOAD → IDRT
        {
            .target_handle = create_idrt.handle,
            .delta = 0,
            .offset = 0x5C,  // Offset de l'adresse IDRT dans batch
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        }
    };
    
    struct drm_i915_gem_exec_object2 exec_objects[5] = {
        { .handle = create_output.handle, .flags = EXEC_OBJECT_WRITE },
        { .handle = create_surface.handle, .relocation_count = 1, .relocs_ptr = (uintptr_t)&relocs[1] },
        { .handle = create_idrt.handle, .relocation_count = 1, .relocs_ptr = (uintptr_t)&relocs[0] },
        { .handle = create_kernel.handle },
        { .handle = create_batch.handle, .relocation_count = 1, .relocs_ptr = (uintptr_t)&relocs[2] }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 5,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER
    };
    
    printf("Relocations configurées:\n");
    printf("  #0: IDRT[0-1] → kernel (handle %u)\n", create_kernel.handle);
    printf("  #1: Surface[1-2] → output (handle %u)\n", create_output.handle);
    printf("  #2: Batch[0x5C] → IDRT (handle %u)\n", create_idrt.handle);
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret) {
        perror("EXECBUFFER2");
        printf("❌ Exécution échouée\n");
        return 1;
    }
    
    printf("✓ GPU exécuté avec relocations automatiques\n");
    
    // Attendre completion
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_output.handle,
        .timeout_ns = 1000000000
    };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    printf("✓ GPU terminé\n");
    
    // Vérifier résultat
    printf("\n=== VÉRIFICATION RÉSULTAT ===\n");
    printf("output[0] = 0x%08x\n", output[0]);
    printf("output[1] = 0x%08x\n", output[1]);
    
    if (output[0] == 0x12345678) {
        printf("\n🎉🎉🎉 VICTOIRE TOTALE! 🎉🎉🎉\n");
        printf("✓✓✓ output[0] = 0x12345678\n");
        printf("✓✓✓ GPU NATIF i915 FONCTIONNE!\n");
        printf("✓✓✓ OpenCL ÉLIMINÉ À 100%%!\n");
    } else {
        printf("\n❌ ÉCHEC: output[0] = 0x%08x\n", output[0]);
    }
    
    // Cleanup
    munmap(output, 4096);
    munmap(surface_state, 4096);
    munmap(idrt, 4096);
    munmap(kernel, 4096);
    munmap(batch, 4096);
    
    close(fd);
    
    return (output[0] == 0x12345678) ? 0 : 1;
}

// Made with Bob - Test C539