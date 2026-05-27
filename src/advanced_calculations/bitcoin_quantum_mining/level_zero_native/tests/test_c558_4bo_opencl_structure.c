// TEST C558 - CORRECTION ROOT CAUSE #65: STRUCTURE 4 BOs COMME OPENCL
// Objectif: Utiliser 4 BOs séparés (output, surface, idrt, kernel) + batch = 5 total
// Référence: RAPPORT_C537 - OpenCL utilise toujours 4 BOs + batch

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libdrm/i915_drm.h>
#include <libdrm/drm.h>
#include <errno.h>

// Kernel ISA Gen9: output[0] = 0x12345678
static const unsigned char kernel_isa[] = {
    0x01, 0x00, 0x60, 0x00, 0x0c, 0x02, 0x40, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x30, 0x00, 0x10, 0x00, 0x16, 0xc0, 0x04, 0xc0, 0x04,
    0x01, 0x00, 0x00, 0x00, 0x0c, 0x47, 0x80, 0x20, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x60, 0x00, 0x6c, 0x26, 0x70, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc8, 0x00, 0x00
};

int main() {
    printf("=== TEST C558 - STRUCTURE 4 BOs COMME OPENCL ===\n");
    printf("ROOT CAUSE #65: Utiliser 4 BOs séparés comme OpenCL\n");
    printf("Structure: output + surface_state + idrt + kernel + batch\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            perror("open DRM");
            return 1;
        }
    }
    printf("[1%%] ✓ DRM ouvert (fd=%d)\n", fd);
    
    // Créer context
    struct drm_i915_gem_context_create ctx_create = { .ctx_id = 0 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("context_create");
        close(fd);
        return 1;
    }
    uint32_t context_id = ctx_create.ctx_id;
    printf("[2%%] ✓ Context créé: ID=%u\n", context_id);
    
    // === CRÉER 5 BOs SÉPARÉS ===
    printf("\n=== CRÉATION 5 BOs SÉPARÉS (5%%) ===\n");
    
    // 1. Output BO (4KB)
    struct drm_i915_gem_create create_output = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        perror("create output");
        close(fd);
        return 1;
    }
    uint32_t output_handle = create_output.handle;
    printf("[7%%] ✓ Output BO créé (handle=%u)\n", output_handle);
    
    // 2. Surface State BO (4KB)
    struct drm_i915_gem_create create_surface = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_surface) < 0) {
        perror("create surface");
        close(fd);
        return 1;
    }
    uint32_t surface_handle = create_surface.handle;
    printf("[9%%] ✓ Surface State BO créé (handle=%u)\n", surface_handle);
    
    // 3. IDRT BO (4KB)
    struct drm_i915_gem_create create_idrt = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_idrt) < 0) {
        perror("create idrt");
        close(fd);
        return 1;
    }
    uint32_t idrt_handle = create_idrt.handle;
    printf("[11%%] ✓ IDRT BO créé (handle=%u)\n", idrt_handle);
    
    // 4. Kernel BO (4KB)
    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0) {
        perror("create kernel");
        close(fd);
        return 1;
    }
    uint32_t kernel_handle = create_kernel.handle;
    printf("[13%%] ✓ Kernel BO créé (handle=%u)\n", kernel_handle);
    
    // 5. Batch BO (4KB)
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("create batch");
        close(fd);
        return 1;
    }
    uint32_t batch_handle = create_batch.handle;
    printf("[15%%] ✓ Batch BO créé (handle=%u)\n", batch_handle);
    
    // === MAPPER TOUS LES BOs ===
    printf("\n=== MAPPING BOs (17%%) ===\n");
    
    // Mapper output
    struct drm_i915_gem_mmap mmap_output = {
        .handle = output_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) < 0) {
        perror("mmap output");
        close(fd);
        return 1;
    }
    uint32_t *output = (uint32_t *)mmap_output.addr_ptr;
    memset(output, 0xFF, 4096);
    printf("[19%%] ✓ Output mappé\n");
    
    // Mapper surface state
    struct drm_i915_gem_mmap mmap_surface = {
        .handle = surface_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_surface) < 0) {
        perror("mmap surface");
        close(fd);
        return 1;
    }
    uint32_t *surface_state = (uint32_t *)mmap_surface.addr_ptr;
    memset(surface_state, 0, 4096);
    printf("[21%%] ✓ Surface State mappé\n");
    
    // Mapper IDRT
    struct drm_i915_gem_mmap mmap_idrt = {
        .handle = idrt_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_idrt) < 0) {
        perror("mmap idrt");
        close(fd);
        return 1;
    }
    uint32_t *idrt = (uint32_t *)mmap_idrt.addr_ptr;
    memset(idrt, 0, 4096);
    printf("[23%%] ✓ IDRT mappé\n");
    
    // Mapper kernel
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = kernel_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel) < 0) {
        perror("mmap kernel");
        close(fd);
        return 1;
    }
    uint8_t *kernel_mem = (uint8_t *)mmap_kernel.addr_ptr;
    memset(kernel_mem, 0, 4096);
    memcpy(kernel_mem, kernel_isa, sizeof(kernel_isa));
    printf("[25%%] ✓ Kernel mappé et ISA copié (%zu bytes)\n", sizeof(kernel_isa));
    
    // Mapper batch
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = batch_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        perror("mmap batch");
        close(fd);
        return 1;
    }
    uint32_t *batch = (uint32_t *)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);
    printf("[27%%] ✓ Batch mappé\n");
    
    // === CONFIGURER STRUCTURES (AVANT PASS 1) ===
    printf("\n=== CONFIGURATION STRUCTURES (29%%) ===\n");
    
    // Surface State (sera patché avec output GTT)
    surface_state[0] = 0x00000004;  // Type: BUFFER
    surface_state[3] = 0x00000FFF;  // Width: 4095
    surface_state[5] = 0x00001000;  // Pitch: 4096
    printf("[31%%] ✓ Surface State configuré\n");
    
    // IDRT (sera patché avec kernel GTT)
    idrt[2] = 0x00000000;  // Sampler State
    idrt[3] = 0x00000000;  // BTI offset (0 car surface_state est BO séparé)
    idrt[4] = 0x00000001;  // BTI Entry Count
    idrt[5] = (1 << 31);   // Barrier Enable
    printf("[33%%] ✓ IDRT configuré\n");
    
    // === PASS 1: BATCH MINIMAL ===
    printf("\n=== PASS 1: BATCH MINIMAL (35%%) ===\n");
    
    uint32_t idx = 0;
    batch[idx++] = 0x69041312;  // PIPELINE_SELECT GPGPU
    batch[idx++] = 0x00000002;
    batch[idx++] = 0x05000000;  // MI_BATCH_BUFFER_END
    
    uint32_t batch_minimal_len = idx * 4;
    printf("[37%%] ✓ Batch minimal: %u DWords (%u bytes)\n", idx, batch_minimal_len);
    
    // Objets PASS 1 - ORDRE CRITIQUE: output, surface, idrt, kernel, batch (dernier!)
    struct drm_i915_gem_exec_object2 objects_pass1[5] = {
        {
            .handle = output_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE,
            .offset = 0
        },
        {
            .handle = surface_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0
        },
        {
            .handle = idrt_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0
        },
        {
            .handle = kernel_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0
        },
        {
            .handle = batch_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf_pass1 = {
        .buffers_ptr = (uint64_t)objects_pass1,
        .buffer_count = 5,  // ✅ 5 BOs comme OpenCL (4 + batch)
        .batch_start_offset = 0,
        .batch_len = batch_minimal_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = context_id,
        .rsvd2 = 0
    };
    
    printf("[39%%] Soumission PASS 1 (5 BOs, context %u)...\n", context_id);
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1) < 0) {
        fprintf(stderr, "❌ PASS 1 ÉCHEC: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    // Lire adresses GTT
    uint64_t output_gtt = objects_pass1[0].offset;
    uint64_t surface_gtt = objects_pass1[1].offset;
    uint64_t idrt_gtt = objects_pass1[2].offset;
    uint64_t kernel_gtt = objects_pass1[3].offset;
    uint64_t batch_gtt = objects_pass1[4].offset;
    
    printf("[45%%] ✓ PASS 1 RÉUSSI!\n");
    printf("       Output:  0x%016lx\n", output_gtt);
    printf("       Surface: 0x%016lx\n", surface_gtt);
    printf("       IDRT:    0x%016lx\n", idrt_gtt);
    printf("       Kernel:  0x%016lx\n", kernel_gtt);
    printf("       Batch:   0x%016lx\n", batch_gtt);
    
    // === PATCHER AVEC GTT ===
    printf("\n=== PATCHER STRUCTURES (50%%) ===\n");
    
    // Patcher Surface State
    surface_state[1] = (uint32_t)(output_gtt & 0xFFFFFFFF);
    surface_state[2] = (uint32_t)(output_gtt >> 32);
    printf("[52%%] ✓ Surface State patché\n");
    
    // Patcher IDRT
    idrt[0] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_gtt >> 32);
    printf("[54%%] ✓ IDRT patché\n");
    
    // === CONSTRUCTION BATCH COMPLET ===
    printf("\n=== BATCH COMPLET (56%%) ===\n");
    
    idx = 0;
    memset(batch, 0, 4096);
    
    // PIPELINE_SELECT
    batch[idx++] = 0x69041312;
    batch[idx++] = 0x00000002;
    printf("[58%%] ✓ PIPELINE_SELECT\n");
    
    // STATE_BASE_ADDRESS avec GTT
    batch[idx++] = 0x61010011;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;  // Surface Modify
    batch[idx++] = (uint32_t)(surface_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(surface_gtt >> 32);
    batch[idx++] = 0x00000001;  // Dynamic Modify
    batch[idx++] = (uint32_t)(idrt_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(idrt_gtt >> 32);
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;  // Instruction Modify
    batch[idx++] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(kernel_gtt >> 32);
    batch[idx++] = 0xFFFFF001;
    batch[idx++] = 0xFFFFF001;
    batch[idx++] = 0xFFFFFFFF;
    batch[idx++] = 0x003BF000;
    printf("[62%%] ✓ STATE_BASE_ADDRESS\n");
    
    // MEDIA_VFE_STATE
    batch[idx++] = 0x70000007;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00A70000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000040;
    batch[idx++] = 0x00000782;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    printf("[66%%] ✓ MEDIA_VFE_STATE\n");
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = 0x61020001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000020;
    batch[idx++] = 0x00000000;  // Offset 0 dans IDRT BO
    printf("[70%%] ✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD\n");
    
    // GPGPU_WALKER
    batch[idx++] = 0x11010014;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    printf("[75%%] ✓ GPGPU_WALKER\n");
    
    // MEDIA_STATE_FLUSH
    batch[idx++] = 0x70040000;
    printf("[78%%] ✓ MEDIA_STATE_FLUSH\n");
    
    // PIPE_CONTROL
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00100000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    printf("[82%%] ✓ PIPE_CONTROL\n");
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x05000000;
    printf("[85%%] ✓ MI_BATCH_BUFFER_END\n");
    
    uint32_t batch_complet_len = idx * 4;
    printf("[87%%] ✓ Batch complet: %u DWords (%u bytes)\n", idx, batch_complet_len);
    
    // === PASS 2: EXÉCUTION GPU ===
    printf("\n=== PASS 2: EXÉCUTION GPU (89%%) ===\n");
    
    struct drm_i915_gem_exec_object2 objects_pass2[5] = {
        {
            .handle = output_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED,
            .offset = output_gtt
        },
        {
            .handle = surface_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = surface_gtt
        },
        {
            .handle = idrt_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = idrt_gtt
        },
        {
            .handle = kernel_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = kernel_gtt
        },
        {
            .handle = batch_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = batch_gtt
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf_pass2 = {
        .buffers_ptr = (uint64_t)objects_pass2,
        .buffer_count = 5,
        .batch_start_offset = 0,
        .batch_len = batch_complet_len,
        .flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC,
        .rsvd1 = context_id,
        .rsvd2 = 0
    };
    
    printf("[91%%] Soumission PASS 2...\n");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass2) < 0) {
        fprintf(stderr, "❌ PASS 2 ÉCHEC: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    printf("[93%%] ✓ PASS 2 RÉUSSI!\n");
    
    // === SYNCHRONISATION ===
    printf("\n=== SYNCHRONISATION (94%%) ===\n");
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = output_handle,
        .timeout_ns = 10000000000LL
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("gem_wait");
        close(fd);
        return 1;
    }
    printf("[95%%] ✓ GEM_WAIT\n");
    
    struct drm_i915_gem_set_domain set_domain = {
        .handle = output_handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) < 0) {
        perror("gem_set_domain");
        close(fd);
        return 1;
    }
    printf("[96%%] ✓ GEM_SET_DOMAIN\n");
    
    // === VÉRIFICATION ===
    printf("\n=== VÉRIFICATION (97%%) ===\n");
    
    printf("output[0] = 0x%08x (attendu: 0x12345678)\n", output[0]);
    printf("output[1] = 0x%08x\n", output[1]);
    printf("output[2] = 0x%08x\n", output[2]);
    printf("output[3] = 0x%08x\n", output[3]);
    
    bool success = (output[0] == 0x12345678);
    
    // Cleanup
    munmap(output, 4096);
    munmap(surface_state, 4096);
    munmap(idrt, 4096);
    munmap(kernel_mem, 4096);
    munmap(batch, 4096);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = context_id };
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    printf("[99%%] ✓ Context détruit\n");
    
    close(fd);
    
    if (success) {
        printf("\n[100%%] 🎉🎉🎉 SUCCÈS HISTORIQUE C558! 🎉🎉🎉\n");
        printf("✓ ROOT CAUSE #65 RÉSOLU (structure 4 BOs)\n");
        printf("✓ Structure OpenCL exacte reproduite\n");
        printf("✓ GPU natif i915 DRM: 100%%\n");
        printf("✓ Élimination OpenCL: 100%%\n");
        printf("✓ PREMIER RÉSULTAT GPU NATIF VALIDE!\n");
        return 0;
    } else {
        printf("\n❌ ÉCHEC - output[0] = 0x%08x\n", output[0]);
        return 1;
    }
}

// Made with Bob