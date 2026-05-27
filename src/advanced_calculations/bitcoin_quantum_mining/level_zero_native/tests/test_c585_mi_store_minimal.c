/* TEST C585-A - MI_STORE_DATA_IMM - Write GPU minimal SANS EU
 * 
 * Objectif: Valider write GPU natif le plus simple possible
 * - Pas de kernel
 * - Pas de EU
 * - Pas de WALKER
 * - Pas de MEDIA pipeline
 * - Juste MI_STORE_DATA_IMM
 * 
 * Si ça échoue: problème mémoire/cache/domain
 * Si ça réussit: problème purement media/EU dispatch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <xf86drm.h>
#include <errno.h>
#include <i915_drm.h>

/* Gen9 Command Opcodes */
#define GEN9_MI_STORE_DATA_IMM      0x20400000
#define GEN9_MI_BATCH_BUFFER_END    0x0A000000
#define GEN9_PIPE_CONTROL           0x7A000000

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    uint64_t t_start = get_timestamp_ns();
    uint64_t t0, t1;
    
    printf("🚀 TEST C585-A - MI_STORE_DATA_IMM (WRITE GPU MINIMAL)\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    // 1. Open DRM device
    printf("[  5%%] Opening DRM device...\n");
    t0 = get_timestamp_ns();
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("❌ Failed to open DRM device");
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ DRM device opened: fd=%d (%.3f µs)\n\n", fd, (t1 - t0) / 1000.0);
    
    // 2. Create GEM context
    printf("[ 10%%] Creating GEM Context...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("❌ Failed to create context");
        close(fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ GEM Context created: ctx_id=%u (%.3f µs)\n\n", 
           ctx_create.ctx_id, (t1 - t0) / 1000.0);
    
    // 3. Create output buffer (4KB)
    printf("[ 20%%] Creating output buffer (4KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create output_create = {
        .size = 4096
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &output_create) < 0) {
        perror("❌ Failed to create output buffer");
        close(fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Output buffer created: handle=%u, size=%llu (%.3f µs)\n\n",
           output_create.handle, output_create.size, (t1 - t0) / 1000.0);
    
    // 4. Create batch buffer (4KB)
    printf("[ 30%%] Creating batch buffer (4KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create batch_create = {
        .size = 4096
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) < 0) {
        perror("❌ Failed to create batch buffer");
        close(fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Batch buffer created: handle=%u, size=%llu (%.3f µs)\n\n",
           batch_create.handle, batch_create.size, (t1 - t0) / 1000.0);
    
    // 5. Map output buffer
    printf("[ 40%%] Mapping output buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap output_mmap = {
        .handle = output_create.handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &output_mmap) < 0) {
        perror("❌ Failed to map output buffer");
        close(fd);
        return 1;
    }
    
    uint32_t *output = (uint32_t *)output_mmap.addr_ptr;
    
    t1 = get_timestamp_ns();
    printf("✅ Output buffer mapped: %p (%.3f µs)\n\n", output, (t1 - t0) / 1000.0);
    
    // 6. Map batch buffer
    printf("[ 50%%] Mapping batch buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap batch_mmap = {
        .handle = batch_create.handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &batch_mmap) < 0) {
        perror("❌ Failed to map batch buffer");
        close(fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)batch_mmap.addr_ptr;
    memset(batch, 0, 4096);
    
    t1 = get_timestamp_ns();
    printf("✅ Batch buffer mapped: %p (%.3f µs)\n\n", batch, (t1 - t0) / 1000.0);
    
    // 7. Initialize output buffer (sentinel)
    printf("[ 60%%] Initializing output buffer...\n");
    t0 = get_timestamp_ns();
    
    output[0] = 0xDEADBEEF;  // Sentinel value
    
    t1 = get_timestamp_ns();
    printf("✅ Output initialized: 0x%08x (%.3f µs)\n\n", output[0], (t1 - t0) / 1000.0);
    
    // 8. Build batch commands (ULTRA MINIMAL)
    printf("[ 70%%] Building batch commands...\n");
    t0 = get_timestamp_ns();
    
    int offset = 0;
    
    /* 1. PIPE_CONTROL (flush initial) - 6 DWords */
    printf("  Building batch commands:\n");
    printf("    [%2d] PIPE_CONTROL (flush initial)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 2. MI_STORE_DATA_IMM - 4 DWords */
    /* ← 1 RELOCATION ICI (DWord 1-2: adresse output) */
    printf("    [%2d] MI_STORE_DATA_IMM (relocation target)\n", offset);
    int store_offset = offset;
    batch[offset++] = GEN9_MI_STORE_DATA_IMM | (4 - 2);
    batch[offset++] = 0;  // Address Low (patché par relocation)
    batch[offset++] = 0;  // Address High (patché par relocation)
    batch[offset++] = 0x12345678;  // Data to write
    
    /* 3. PIPE_CONTROL (flush final) - 6 DWords */
    printf("    [%2d] PIPE_CONTROL (flush final)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 4. MI_BATCH_BUFFER_END - 1 DWord */
    printf("    [%2d] MI_BATCH_BUFFER_END\n", offset);
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    t1 = get_timestamp_ns();
    printf("✅ Batch built: %d DWords (%d bytes, %.3f µs)\n\n", 
           offset, offset * 4, (t1 - t0) / 1000.0);
    
    // 9. Create relocation entry
    printf("[ 80%%] Creating relocation entry...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_relocation_entry reloc = {
        .target_handle = output_create.handle,
        .delta = 0,  // Output buffer base
        .offset = (store_offset + 1) * 4,  // DWord 1 MI_STORE_DATA_IMM
        .read_domains = I915_GEM_DOMAIN_RENDER,
        .write_domain = I915_GEM_DOMAIN_RENDER,
        .presumed_offset = 0
    };
    
    t1 = get_timestamp_ns();
    printf("✅ Relocation created (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("  Target handle: %u (output)\n", reloc.target_handle);
    printf("  Delta: %u (output base)\n", reloc.delta);
    printf("  Offset: 0x%llx (batch DWord %d)\n", reloc.offset, (int)(reloc.offset / 4));
    printf("  Read domains: 0x%08x (RENDER)\n", reloc.read_domains);
    printf("  Write domain: 0x%08x (RENDER)\n\n", reloc.write_domain);
    
    // 10. Prepare exec objects
    printf("[ 90%%] Preparing EXECBUFFER2...\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[2] = {
        {
            .handle = output_create.handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .alignment = 0,
            .offset = 0,
            .flags = EXEC_OBJECT_WRITE,  // Output modifié par GPU
            .rsvd1 = 0,
            .rsvd2 = 0
        },
        {
            .handle = batch_create.handle,
            .relocation_count = 1,
            .relocs_ptr = (uint64_t)&reloc,
            .alignment = 0,
            .offset = 0,
            .flags = 0,
            .rsvd1 = 0,
            .rsvd2 = 0
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 2,
        .batch_start_offset = 0,
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0
    };
    
    printf("  Configuration:\n");
    printf("    Context ID: %u\n", ctx_create.ctx_id);
    printf("    Ring: RCS (Render)\n");
    printf("    Buffer count: %u (output + batch)\n", execbuf.buffer_count);
    printf("    Batch length: %u bytes\n", execbuf.batch_len);
    printf("    Relocations: 1 (MI_STORE_DATA_IMM → output)\n\n");
    
    // 11. Submit to GPU
    printf("[100%%] Submitting to GPU...\n");
    t0 = get_timestamp_ns();
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        t1 = get_timestamp_ns();
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d)\n", strerror(errno), errno);
        printf("   Duration: %.3f µs\n\n", (t1 - t0) / 1000.0);
        
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST C585-A ÉCHOUÉ                                    ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        
        close(fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ EXECBUFFER2 SUCCESS! (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("  Output GTT: 0x%016llx\n", exec_objects[0].offset);
    printf("  Batch GTT:  0x%016llx\n\n", exec_objects[1].offset);
    
    // 12. Wait for GPU completion
    printf("Waiting for GPU completion...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_wait wait_args = {
        .bo_handle = batch_create.handle,
        .timeout_ns = 1000000000,  // 1 second
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait_args) < 0) {
        perror("❌ Failed to wait for GPU");
        close(fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ GPU execution completed (%.3f ms)\n\n", (t1 - t0) / 1000000.0);
    
    // 13. Read result
    printf("Reading output buffer...\n");
    t0 = get_timestamp_ns();
    
    uint32_t result = output[0];
    
    t1 = get_timestamp_ns();
    printf("  Output[0] = 0x%08x (%.3f µs)\n\n", result, (t1 - t0) / 1000.0);
    
    // 14. Validate
    printf("Validating result...\n\n");
    
    uint64_t t_total = get_timestamp_ns() - t_start;
    
    if (result == 0x12345678) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ TEST C585-A RÉUSSI!                                   ║\n");
        printf("║  Result: 0x12345678 (attendu: 0x12345678)                ║\n");
        printf("║  ⏱️  Temps total: %.3f ms                                   ║\n", t_total / 1000000.0);
        printf("║                                                            ║\n");
        printf("║  📝 CONCLUSION:                                            ║\n");
        printf("║  - Write GPU natif FONCTIONNE                             ║\n");
        printf("║  - Mémoire/cache/domain OK                                ║\n");
        printf("║  - Relocations OK                                         ║\n");
        printf("║  - Problème = MEDIA/EU dispatch uniquement               ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  RÉSULTAT INATTENDU                                   ║\n");
        printf("║  Expected: 0x12345678                                     ║\n");
        printf("║  Got:      0x%08x                                     ║\n", result);
        printf("║  ⏱️  Temps total: %.3f ms                                   ║\n", t_total / 1000000.0);
        printf("║                                                            ║\n");
        printf("║  📝 CONCLUSION:                                            ║\n");
        printf("║  - Problème mémoire/cache/domain                          ║\n");
        printf("║  - OU relocations incorrectes                             ║\n");
        printf("║  - OU PIPE_CONTROL flush insuffisant                      ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    // Cleanup
    printf("\nCleanup...\n");
    
    struct drm_i915_gem_context_destroy ctx_destroy = {
        .ctx_id = ctx_create.ctx_id
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    printf("✅ Context destroyed\n");
    
    close(fd);
    
    printf("\n✅ TEST C585-A TERMINÉ\n");
    
    return (result == 0x12345678) ? 0 : 1;
}

// Made with Bob
