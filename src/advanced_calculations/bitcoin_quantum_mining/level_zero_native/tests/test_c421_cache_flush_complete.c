/*
 * TEST C421 - CACHE FLUSH COMPLET
 * Ajout de TOUS les bits de flush/invalidate dans PIPE_CONTROL
 * 
 * HYPOTHÈSE: GPU écrit dans cache L3, CPU lit depuis RAM
 * SOLUTION: Flush agressif de tous les caches GPU
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <xf86drm.h>
#include <i915_drm.h>

#define KERNEL_SIZE 80
#define OUTPUT_SIZE 4096
#define SSH_SIZE 256
#define DSH_SIZE 256

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

#define LOG_FORENSIC(fmt, ...) do { \
    uint64_t ts = get_timestamp_ns(); \
    printf("[%lu ns] " fmt "\n", ts, ##__VA_ARGS__); \
} while(0)

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C421 TEST - CACHE FLUSH COMPLET (TOUS LES BITS)          ║\n");
    printf("║  DC + RT + Texture + Instruction + Constant Cache         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    LOG_FORENSIC("C421 START - Complete Cache Flush Test");

    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("Failed to open DRM");
        return 1;
    }
    LOG_FORENSIC("DRM opened: fd=%d", fd);

    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("Failed to create context");
        close(fd);
        return 1;
    }
    LOG_FORENSIC("GPU context created: ctx_id=%u", ctx_create.ctx_id);

    FILE *f = fopen("kernels/btc_bare_metal_c419.bin", "rb");
    if (!f) {
        perror("Failed to open kernel");
        close(fd);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *kernel_data = malloc(kernel_size);
    fread(kernel_data, 1, kernel_size, f);
    fclose(f);
    LOG_FORENSIC("Kernel loaded: %ld bytes", kernel_size);

    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel);
    
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = create_kernel.handle,
        .size = 4096
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel);
    uint8_t *kernel_map = (uint8_t*)mmap_kernel.addr_ptr;
    memcpy(kernel_map, kernel_data, kernel_size);
    free(kernel_data);
    LOG_FORENSIC("Kernel buffer: handle=%u", create_kernel.handle);

    struct drm_i915_gem_create create_output = { .size = OUTPUT_SIZE };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output);
    
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle,
        .size = OUTPUT_SIZE
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output);
    uint32_t *output_map = (uint32_t*)mmap_output.addr_ptr;
    memset(output_map, 0xCC, OUTPUT_SIZE);
    LOG_FORENSIC("Output buffer: handle=%u (filled 0xCC)", create_output.handle);

    struct drm_i915_gem_create create_ssh = { .size = SSH_SIZE };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_ssh);
    
    struct drm_i915_gem_mmap mmap_ssh = {
        .handle = create_ssh.handle,
        .size = SSH_SIZE
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_ssh);
    uint32_t *ssh_map = (uint32_t*)mmap_ssh.addr_ptr;
    memset(ssh_map, 0, SSH_SIZE);

    // Surface State (C420 fix)
    ssh_map[0] = (0x00 << 18) | (0x0D << 7) | (0x02 << 24);
    ssh_map[1] = 0;
    ssh_map[2] = 1023;
    ssh_map[3] = 0;
    ssh_map[4] = 0;
    ssh_map[5] = 0;
    ssh_map[6] = 0;
    ssh_map[7] = 0;
    ssh_map[16] = 0;

    LOG_FORENSIC("SSH configured - SURFTYPE=0x00 FORMAT=R32_UINT MOCS=LLC");

    struct drm_i915_gem_create create_dsh = { .size = DSH_SIZE };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_dsh);
    
    struct drm_i915_gem_mmap mmap_dsh = {
        .handle = create_dsh.handle,
        .size = DSH_SIZE
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_dsh);
    uint32_t *dsh_map = (uint32_t*)mmap_dsh.addr_ptr;
    memset(dsh_map, 0, DSH_SIZE);

    dsh_map[0] = 0;
    dsh_map[1] = 0;
    dsh_map[2] = 0;
    dsh_map[3] = 64;
    dsh_map[4] = 0;
    dsh_map[5] = 0x10;
    dsh_map[6] = 0;
    dsh_map[7] = 0;

    LOG_FORENSIC("DSH configured - Interface Descriptor ready");

    struct drm_i915_gem_create create_batch = { .size = 4096 };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch);
    
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle,
        .size = 4096
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch);
    uint32_t *batch = (uint32_t*)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);

    int idx = 0;

    // PIPELINE_SELECT
    batch[idx++] = 0x69040002;

    // STATE_BASE_ADDRESS (17 dwords)
    batch[idx++] = 0x61010010;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000FFF;
    batch[idx++] = 0x00000FFF;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000FFF;

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
    batch[idx++] = 0x00000020;
    batch[idx++] = 0x00000000;

    // GPGPU_WALKER (15 dwords)
    batch[idx++] = 0x7A00000D;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000010;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0xFFFFFFFF;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;

    // *** C421 FIX: PIPE_CONTROL AVEC TOUS LES FLUSH ***
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00100000 |  // DC Flush Enable
                   0x00200000 |  // Render Target Cache Flush Enable
                   0x00400000 |  // Texture Cache Invalidate
                   0x00800000 |  // Instruction Cache Invalidate
                   0x01000000;   // Constant Cache Invalidate
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;

    LOG_FORENSIC("PIPE_CONTROL configured: ALL FLUSH BITS (0x01F00000)");

    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;

    LOG_FORENSIC("Batch buffer: %d dwords", idx);

    struct drm_i915_gem_relocation_entry relocs[6] = {0};
    
    relocs[0].target_handle = create_ssh.handle;
    relocs[0].offset = 4 * 4;
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[0].write_domain = 0;
    
    relocs[1].target_handle = create_dsh.handle;
    relocs[1].offset = 6 * 4;
    relocs[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[1].write_domain = 0;
    
    relocs[2].target_handle = create_kernel.handle;
    relocs[2].offset = 10 * 4;
    relocs[2].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[2].write_domain = 0;
    
    relocs[3].target_handle = create_dsh.handle;
    relocs[3].offset = 31 * 4;
    relocs[3].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[3].write_domain = 0;
    
    relocs[4].target_handle = create_output.handle;
    relocs[4].offset = 1 * 4;
    relocs[4].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[4].write_domain = I915_GEM_DOMAIN_RENDER;
    
    relocs[5].target_handle = create_kernel.handle;
    relocs[5].offset = 0;
    relocs[5].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[5].write_domain = 0;

    struct drm_i915_gem_exec_object2 exec_ssh = {
        .handle = create_ssh.handle,
        .relocation_count = 1,
        .relocs_ptr = (uintptr_t)&relocs[4]
    };

    struct drm_i915_gem_exec_object2 exec_dsh = {
        .handle = create_dsh.handle,
        .relocation_count = 1,
        .relocs_ptr = (uintptr_t)&relocs[5]
    };

    struct drm_i915_gem_exec_object2 exec_batch = {
        .handle = create_batch.handle,
        .relocation_count = 4,
        .relocs_ptr = (uintptr_t)&relocs[0]
    };

    struct drm_i915_gem_exec_object2 exec_objects[] = {
        { .handle = create_kernel.handle },
        { .handle = create_output.handle },
        exec_ssh,
        exec_dsh,
        exec_batch
    };

    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 5,
        .batch_len = idx * 4,
        .rsvd1 = ctx_create.ctx_id
    };

    LOG_FORENSIC("Executing C421 with complete cache flush...");
    uint64_t exec_start = get_timestamp_ns();
    
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t exec_end = get_timestamp_ns();
    
    if (ret < 0) {
        perror("EXECBUFFER2 failed");
        LOG_FORENSIC("EXECBUFFER2 FAILED after %lu ns", exec_end - exec_start);
        close(fd);
        return 1;
    }
    
    LOG_FORENSIC("✓ EXECBUFFER2 success: %lu ns", exec_end - exec_start);

    struct drm_i915_gem_wait wait = {
        .bo_handle = create_batch.handle,
        .timeout_ns = -1
    };
    uint64_t wait_start = get_timestamp_ns();
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    uint64_t wait_end = get_timestamp_ns();
    
    LOG_FORENSIC("✓ GPU execution complete: %lu ns", wait_end - wait_start);

    printf("\n📊 RÉSULTATS C421:\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    int valid = 0;
    int unchanged = 0;
    
    for (int i = 0; i < 16; i++) {
        if (output_map[i] == 0xDEADBEEF) {
            printf("  output[%2d] = 0x%08X ✅ VALID\n", i, output_map[i]);
            valid++;
        } else if (output_map[i] == 0xCCCCCCCC) {
            unchanged++;
        } else {
            printf("  output[%2d] = 0x%08X ⚠️  UNEXPECTED\n", i, output_map[i]);
        }
    }
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Valid (0xDEADBEEF): %d / 16\n", valid);
    printf("  Unchanged (0xCC):   %d / 16\n", unchanged);
    printf("═══════════════════════════════════════════════════════════\n\n");

    if (valid > 0) {
        printf("🎉 SUCCESS C421! Cache flush worked!\n");
        LOG_FORENSIC("C421 SUCCESS - %d valid results", valid);
    } else {
        printf("⚠️  Still no results - trying C422 (CPU cache invalidation)...\n");
        LOG_FORENSIC("C421 FAILED - 0 valid results, proceeding to C422");
    }

    close(fd);
    return (valid > 0) ? 0 : 1;
}

// Made with Bob
