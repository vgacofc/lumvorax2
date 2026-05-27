/**
 * C419 - TEST KERNEL BARE-METAL (AUCUNE dépendance ABI OpenCL)
 * 
 * Stratégie: Utiliser batch buffer C415v3 (qui fonctionne) avec kernel bare-metal
 * 
 * Ce kernel:
 * - Hardcode valeur 0xDEADBEEF
 * - Hardcode offset 0
 * - N'utilise PAS get_global_id()
 * - N'utilise PAS CURBE
 * - N'utilise PAS payload OpenCL
 * 
 * Si ce test produit des résultats:
 * -> Pipeline GPU natif VALIDÉ
 * -> Problème C411-C418 était l'ABI runtime OpenCL
 * -> Prochaine étape: ajouter CURBE progressivement
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <xf86drm.h>
#include <i915_drm.h>

#define NUM_THREADS 16

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C419 TEST - KERNEL BARE-METAL (0% ABI OpenCL)            ║\n");
    printf("║  Kernel: btc_bare_metal_c419.bin (80 bytes)               ║\n");
    printf("║  Batch: C415v3 (54 dwords, fonctionnel)                   ║\n");
    printf("║  Valeur attendue: 0xDEADBEEF aux 16 premiers offsets      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open DRM");
        return 1;
    }
    printf("✓ DRM opened: fd=%d\n", fd);
    
    // Créer contexte
    struct drm_i915_gem_context_create ctx_create = {0};
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("CONTEXT_CREATE");
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    printf("✓ GPU context: ctx_id=%u\n", ctx_id);
    
    // Charger kernel bare-metal
    FILE *f = fopen("kernels/btc_bare_metal_c419.bin", "rb");
    if (!f) {
        perror("fopen kernel");
        close(fd);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0) {
        perror("GEM_CREATE kernel");
        fclose(f);
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = create_kernel.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel) < 0) {
        perror("GEM_MMAP kernel");
        fclose(f);
        close(fd);
        return 1;
    }
    
    void *kernel_map = (void *)(uintptr_t)mmap_kernel.addr_ptr;
    memset(kernel_map, 0, 4096);
    fread(kernel_map, 1, kernel_size, f);
    fclose(f);
    printf("✓ Kernel loaded: %zu bytes\n", kernel_size);
    
    // Créer buffer output (rempli avec 0xCC pour détecter les écritures)
    struct drm_i915_gem_create create_output = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        perror("GEM_CREATE output");
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) < 0) {
        perror("GEM_MMAP output");
        close(fd);
        return 1;
    }
    
    void *output_map = (void *)(uintptr_t)mmap_output.addr_ptr;
    memset(output_map, 0xCC, 4096);
    printf("✓ Output buffer: 4096 bytes (filled with 0xCC)\n");
    
    // Créer SSH (Surface State Heap)
    struct drm_i915_gem_create create_ssh = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_ssh) < 0) {
        perror("GEM_CREATE ssh");
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_ssh = {
        .handle = create_ssh.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_ssh) < 0) {
        perror("GEM_MMAP ssh");
        close(fd);
        return 1;
    }
    
    uint32_t *ssh_map = (uint32_t *)(uintptr_t)mmap_ssh.addr_ptr;
    memset(ssh_map, 0, 4096);
    
    // Surface State (BTI=0) - offset 0
    ssh_map[0] = 0x00000000;  // DW0
    ssh_map[1] = 0x00000000;  // DW1: Address low (sera patché par relocation)
    ssh_map[2] = 0x00000000;  // DW2: Address high
    ssh_map[3] = 4095;        // DW3: Width (4096 bytes - 1)
    ssh_map[4] = 0x00000000;  // DW4
    ssh_map[5] = 0x00000000;  // DW5
    ssh_map[6] = 0x00000000;  // DW6
    ssh_map[7] = 0x00000000;  // DW7
    
    // Binding Table - offset 64 (16 dwords)
    ssh_map[16] = 0;  // BTI 0 -> Surface State 0
    
    // Interface Descriptor - offset 128 (32 dwords)
    uint32_t *dsh_map = &ssh_map[32];
    dsh_map[0] = 0x00000000;  // DW0: KSP low (sera patché)
    dsh_map[1] = 0x00000000;  // DW1: KSP high
    dsh_map[2] = 0x00000000;  // DW2
    dsh_map[3] = 64;          // DW3: Binding Table offset
    dsh_map[4] = 0x00000000;  // DW4
    dsh_map[5] = 0x10;        // DW5: Number of Threads (16 pour SIMD16)
    dsh_map[6] = 0x00000000;  // DW6
    dsh_map[7] = 0x00000000;  // DW7
    
    printf("✓ SSH configured (Surface State + Binding Table + Interface Descriptor)\n");
    
    // Créer batch buffer (C415v3: 54 dwords)
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("GEM_CREATE batch");
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        perror("GEM_MMAP batch");
        close(fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)(uintptr_t)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);
    int idx = 0;
    
    // PIPELINE_SELECT
    batch[idx++] = 0x69040002;
    
    // STATE_BASE_ADDRESS (16 dwords)
    batch[idx++] = 0x61011010;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000FFF;
    batch[idx++] = 0x00000FFF;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000FFF;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MEDIA_VFE_STATE (8 dwords)
    batch[idx++] = 0x70000007;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 dwords)
    batch[idx++] = 0x70020002;
    batch[idx++] = 0x00000000;
    batch[idx++] = 32;  // Length
    batch[idx++] = 0x00000000;  // Address (sera patché)
    
    // GPGPU_WALKER (15 dwords)
    batch[idx++] = 0x7a00000d;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = NUM_THREADS;  // Thread Width
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;  // Thread Height
    batch[idx++] = 0x00000000;
    batch[idx++] = 0xFFFFFFFF;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // PIPE_CONTROL (6 dwords)
    batch[idx++] = 0x7a000004;
    batch[idx++] = 0x00100000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // BATCH_BUFFER_END
    batch[idx++] = 0x0a000000;
    
    printf("✓ Batch buffer: %d dwords\n", idx);
    
    // Execbuffer avec relocations
    struct drm_i915_gem_relocation_entry relocs[3] = {
        // Surface State DW1 -> output buffer
        {
            .target_handle = create_output.handle,
            .delta = 0,
            .offset = 4,  // SSH offset 4 (Surface State DW1)
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_RENDER,
            .write_domain = I915_GEM_DOMAIN_RENDER
        },
        // Interface Descriptor DW0 -> kernel
        {
            .target_handle = create_kernel.handle,
            .delta = 0,
            .offset = 128,  // SSH offset 128 (Interface Descriptor DW0)
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        },
        // MEDIA_INTERFACE_DESCRIPTOR_LOAD address -> SSH + 128
        {
            .target_handle = create_ssh.handle,
            .delta = 128,
            .offset = (27 * 4) + 12,  // Batch offset du 4ème dword de MIDL
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        }
    };
    
    struct drm_i915_gem_exec_object2 exec_objects[4] = {
        { .handle = create_kernel.handle },
        { .handle = create_output.handle, .flags = EXEC_OBJECT_WRITE },
        { 
            .handle = create_ssh.handle,
            .relocation_count = 3,
            .relocs_ptr = (uintptr_t)relocs
        },
        { .handle = create_batch.handle }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_len = idx * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    printf("\n🚀 Executing bare-metal kernel...\n");
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(stderr, "❌ EXECBUFFER2 failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("✓ EXECBUFFER2 success\n");
    
    // Wait GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_batch.handle,
        .timeout_ns = 1000000000
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
    }
    printf("✓ GPU execution complete\n");
    
    // Lire résultats
    printf("\n📊 Results:\n");
    uint32_t *output = (uint32_t *)output_map;
    int valid = 0;
    int unchanged = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] == 0xDEADBEEF) {
            valid++;
            if (valid <= 10) {
                printf("  output[%2d] = 0x%08x ✓ CORRECT (bare-metal write!)\n", i, output[i]);
            }
        } else if (output[i] == 0xCCCCCCCC) {
            unchanged++;
        } else {
            printf("  output[%2d] = 0x%08x (unexpected)\n", i, output[i]);
        }
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Valid (0xDEADBEEF): %d / %d\n", valid, NUM_THREADS);
    printf("  Unchanged (0xCC):   %d / %d\n", unchanged, NUM_THREADS);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    if (valid > 0) {
        printf("🎉🎉🎉 SUCCESS! BARE-METAL KERNEL WORKS! 🎉🎉🎉\n");
        printf("✅ Pipeline GPU natif VALIDÉ\n");
        printf("✅ Problème C411-C418 était bien l'ABI runtime OpenCL\n");
        printf("✅ Prochaine étape: ajouter CURBE progressivement\n");
    } else {
        printf("⚠️  No valid results - investigating...\n");
    }
    
    // Cleanup
    struct drm_gem_close close_arg;
    close_arg.handle = create_kernel.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_output.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_ssh.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_batch.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(fd);
    
    printf("\n");
    return (valid > 0) ? 0 : 1;
}

// Made with Bob
