// TEST C560 - CORRECTION ROOT CAUSE #68: RETIRER PINNED POUR OUTPUT
// Objectif: Éviter EINVAL avec output_gtt=0x0 et PINNED
// Référence: C559 - ROOT CAUSE #68 identifié
// Stratégie: Output → Heap → Batch + PINNED seulement pour GTT!=0x0

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

// Kernel ISA Gen9 simple: output[0] = 0x12345678
static const unsigned char kernel_isa[] = {
    0x01, 0x00, 0x60, 0x00, 0x0c, 0x02, 0x40, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x30, 0x00, 0x10, 0x00, 0x16, 0xc0, 0x04, 0xc0, 0x04,
    0x01, 0x00, 0x00, 0x00, 0x0c, 0x47, 0x80, 0x20, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x60, 0x00, 0x6c, 0x26, 0x70, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc8, 0x00, 0x00
};

int main() {
    printf("=== TEST C560 - CORRECTION ROOT CAUSE #68 ===\n");
    printf("ROOT CAUSE #68: output_gtt=0x0 avec PINNED invalide\n");
    printf("CORRECTION: Retirer PINNED pour output (GTT=0x0)\n\n");
    
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
    
    // === CRÉER CONTEXT i915 ===
    printf("\n=== CRÉATION CONTEXT i915 (2%%) ===\n");
    struct drm_i915_gem_context_create ctx_create = { .ctx_id = 0 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("context_create");
        close(fd);
        return 1;
    }
    uint32_t context_id = ctx_create.ctx_id;
    printf("[3%%] ✓ Context créé: ID=%u\n", context_id);
    
    // === CRÉATION BUFFERS (ORDRE CRITIQUE MODIFIÉ) ===
    printf("\n=== CRÉATION BUFFERS - ORDRE MODIFIÉ (5%%) ===\n");
    printf("STRATÉGIE C560: Output → Heap → Batch (batch EN DERNIER)\n");
    
    // Output buffer (4KB) - PREMIER (obtient 0x0)
    struct drm_i915_gem_create create_output = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        perror("create output");
        close(fd);
        return 1;
    }
    uint32_t output_handle = create_output.handle;
    printf("[7%%] ✓ Output BO créé (handle=%u) - PREMIER (GTT=0x0 attendu)\n", output_handle);
    
    // Heap buffer (16KB) - DEUXIÈME (obtient 0x1000)
    struct drm_i915_gem_create create_heap = { .size = 16384 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_heap) < 0) {
        perror("create heap");
        close(fd);
        return 1;
    }
    uint32_t heap_handle = create_heap.handle;
    printf("[9%%] ✓ Heap BO créé (handle=%u) - DEUXIÈME (GTT=0x1000 attendu)\n", heap_handle);
    
    // Batch buffer (4KB) - DERNIER (obtient 0x5000 = 0x1000 + 0x4000)
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("create batch");
        close(fd);
        return 1;
    }
    uint32_t batch_handle = create_batch.handle;
    printf("[11%%] ✓ Batch BO créé (handle=%u) - DERNIER (GTT!=0x0 attendu) ✅\n", batch_handle);
    
    // === MAPPING BUFFERS ===
    printf("\n=== MAPPING BUFFERS (13%%) ===\n");
    
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
    printf("[15%%] ✓ Batch mappé @ %p\n", batch);
    
    // Mapper heap
    struct drm_i915_gem_mmap mmap_heap = {
        .handle = heap_handle,
        .offset = 0,
        .size = 16384,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_heap) < 0) {
        perror("mmap heap");
        close(fd);
        return 1;
    }
    uint8_t *heap = (uint8_t *)mmap_heap.addr_ptr;
    memset(heap, 0, 16384);
    printf("[17%%] ✓ Heap mappé @ %p\n", heap);
    
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
    printf("[19%%] ✓ Output mappé @ %p\n", output);
    
    // === CONSTRUCTION HEAP INITIAL ===
    printf("\n=== CONSTRUCTION HEAP (21%%) ===\n");
    
    // Surface State @ 0x0000 (64 bytes)
    uint32_t *surface_state = (uint32_t *)heap;
    surface_state[0] = 0x00000004;  // Type: BUFFER
    surface_state[3] = 0x00000FFF;  // Width: 4095
    surface_state[5] = 0x00001000;  // Pitch: 4096
    printf("[23%%] ✓ Surface State créé\n");
    
    // Binding Table @ 0x0080
    uint32_t *binding_table = (uint32_t *)(heap + 0x80);
    binding_table[0] = 0x00000000;  // Offset vers Surface State
    printf("[25%%] ✓ Binding Table créé\n");
    
    // IDRT @ 0x00A0 (32 bytes)
    uint32_t *idrt = (uint32_t *)(heap + 0xA0);
    idrt[2] = 0x00000000;  // Sampler State
    idrt[3] = 0x00000080;  // BTI offset
    idrt[4] = 0x00000001;  // BTI Entry Count
    idrt[5] = (1 << 31);   // Barrier Enable
    printf("[27%%] ✓ IDRT créé\n");
    
    // Kernel ISA @ 0x0100
    memcpy(heap + 0x100, kernel_isa, sizeof(kernel_isa));
    printf("[29%%] ✓ Kernel ISA copié (%zu bytes)\n", sizeof(kernel_isa));
    
    // === PASS 1: BATCH MINIMAL 16 BYTES ===
    printf("\n=== PASS 1: OBTENIR GTT ADDRESSES (31%%) ===\n");
    
    uint32_t idx = 0;
    batch[idx++] = 0x00000000;  // MI_NOOP
    batch[idx++] = 0x00000000;  // MI_NOOP
    batch[idx++] = 0x00000000;  // MI_NOOP
    batch[idx++] = 0x0A000000;  // MI_BATCH_BUFFER_END
    
    uint32_t batch_minimal_len = idx * 4;
    printf("[33%%] ✓ Batch minimal: %u DWords (%u bytes)\n", idx, batch_minimal_len);
    
    // Logging forensique détaillé
    fprintf(stderr, "[DEBUG] PASS 1 Configuration:\n");
    fprintf(stderr, "[DEBUG]   buffer_count = 3\n");
    fprintf(stderr, "[DEBUG]   batch_len = %u bytes\n", batch_minimal_len);
    fprintf(stderr, "[DEBUG]   context_id = %u\n", context_id);
    fprintf(stderr, "[DEBUG]   Ordre création: output(%u) → heap(%u) → batch(%u)\n",
            output_handle, heap_handle, batch_handle);
    
    // Objets PASS 1 (ORDRE: output, heap, batch)
    struct drm_i915_gem_exec_object2 objects_pass1[3] = {
        {
            .handle = output_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE,
            .offset = 0
        },
        {
            .handle = heap_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0
        },
        {
            .handle = batch_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0
        }
    };
    
    // Execbuffer2 PASS 1 AVEC CONTEXT
    struct drm_i915_gem_execbuffer2 execbuf_pass1 = {
        .buffers_ptr = (uint64_t)objects_pass1,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = batch_minimal_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = context_id,
        .rsvd2 = 0
    };
    
    printf("[35%%] Soumission PASS 1 (obtenir GTT addresses)...\n");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1) < 0) {
        fprintf(stderr, "❌ PASS 1 ÉCHEC: %s (errno=%d)\n", strerror(errno), errno);
        close(fd);
        return 1;
    }
    
    // Lire adresses GTT PASS 1 (ordre: output, heap, batch)
    uint64_t output_gtt = objects_pass1[0].offset;
    uint64_t heap_gtt = objects_pass1[1].offset;
    uint64_t batch_gtt = objects_pass1[2].offset;
    
    printf("[40%%] ✓ PASS 1 RÉUSSI! 🎉\n");
    printf("       Output: 0x%016lx %s\n", output_gtt,
           output_gtt == 0 ? "✅ NULL (premier)" : "⚠️ Non-NULL");
    printf("       Heap:   0x%016lx %s\n", heap_gtt,
           heap_gtt == 0 ? "❌ NULL!" : "✅ NON-NULL");
    printf("       Batch:  0x%016lx %s\n", batch_gtt,
           batch_gtt == 0 ? "❌ NULL!" : "✅ NON-NULL (DERNIER)");
    
    fprintf(stderr, "[DEBUG] GTT Addresses PASS 1:\n");
    fprintf(stderr, "[DEBUG]   output_gtt = 0x%016lx\n", output_gtt);
    fprintf(stderr, "[DEBUG]   heap_gtt = 0x%016lx\n", heap_gtt);
    fprintf(stderr, "[DEBUG]   batch_gtt = 0x%016lx\n", batch_gtt);
    
    // Vérification critique
    if (batch_gtt == 0) {
        fprintf(stderr, "❌ ERREUR: batch_gtt=0x0 même en créant batch EN DERNIER!\n");
        fprintf(stderr, "   Comportement i915 inattendu\n");
        fprintf(stderr, "   Solution alternative nécessaire\n");
        close(fd);
        return 1;
    }
    
    if (heap_gtt == 0) {
        fprintf(stderr, "❌ ERREUR: heap_gtt=0x0!\n");
        close(fd);
        return 1;
    }
    
    printf("\n[45%%] ✅ VALIDATION: batch_gtt != 0x0 (0x%016lx)\n", batch_gtt);
    printf("       ✅ VALIDATION: heap_gtt != 0x0 (0x%016lx)\n", heap_gtt);
    
    // === PATCHER STRUCTURES AVEC GTT VALIDES ===
    printf("\n=== PATCHER STRUCTURES (47%%) ===\n");
    
    // Patcher Surface State
    surface_state[1] = (uint32_t)(output_gtt & 0xFFFFFFFF);
    surface_state[2] = (uint32_t)(output_gtt >> 32);
    printf("[49%%] ✓ Surface State patché (output @ 0x%016lx)\n", output_gtt);
    
    // Patcher IDRT
    uint64_t kernel_gtt = heap_gtt + 0x100;
    idrt[0] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_gtt >> 32);
    printf("[51%%] ✓ IDRT patché (kernel @ 0x%016lx)\n", kernel_gtt);
    
    // === CONSTRUCTION BATCH COMPLET ===
    printf("\n=== CONSTRUCTION BATCH COMPLET (53%%) ===\n");
    
    idx = 0;
    memset(batch, 0, 4096);
    
    // PIPELINE_SELECT
    batch[idx++] = 0x69041312;
    batch[idx++] = 0x00000002;
    printf("[55%%] ✓ PIPELINE_SELECT\n");
    
    // STATE_BASE_ADDRESS (19 DWords) avec heap_gtt VALIDE
    batch[idx++] = 0x61010011;
    batch[idx++] = 0x00000001;  // General Modify
    batch[idx++] = 0x00000000;  // General LOW
    batch[idx++] = 0x00000000;  // General HIGH
    batch[idx++] = 0x00000001;  // Surface Modify
    batch[idx++] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(heap_gtt >> 32);
    batch[idx++] = 0x00000001;  // Dynamic Modify
    batch[idx++] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(heap_gtt >> 32);
    batch[idx++] = 0x00000000;  // Indirect LOW
    batch[idx++] = 0x00000000;  // Indirect HIGH
    batch[idx++] = 0x00000001;  // Instruction Modify
    batch[idx++] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(heap_gtt >> 32);
    batch[idx++] = 0xFFFFF001;  // General Size
    batch[idx++] = 0xFFFFF001;  // Dynamic Size
    batch[idx++] = 0xFFFFFFFF;  // Indirect Size
    batch[idx++] = 0x003BF000;  // Instruction Size
    printf("[60%%] ✓ STATE_BASE_ADDRESS (heap GTT=0x%016lx)\n", heap_gtt);
    
    // MEDIA_VFE_STATE (9 DWords)
    batch[idx++] = 0x70000007;
    batch[idx++] = 0x00000000;  // Scratch LOW
    batch[idx++] = 0x00000000;  // Scratch HIGH
    batch[idx++] = 0x00A70000;  // Max Threads = 167
    batch[idx++] = 0x00000001;  // URB Entries = 1
    batch[idx++] = 0x00000040;  // URB Size = 64
    batch[idx++] = 0x00000782;  // CURBE Size = 1922
    batch[idx++] = 0x00000000;  // Scoreboard
    batch[idx++] = 0x00000000;  // Reserved
    printf("[65%%] ✓ MEDIA_VFE_STATE\n");
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWords)
    batch[idx++] = 0x61020001;
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000020;  // Length = 32
    batch[idx++] = 0x000000A0;  // Offset = 0xA0
    printf("[70%%] ✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD\n");
    
    // GPGPU_WALKER (15 DWords)
    batch[idx++] = 0x11010014;
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset
    batch[idx++] = 0x00000000;  // Indirect Length
    batch[idx++] = 0x00000000;  // Indirect LOW
    batch[idx++] = 0x00000000;  // Indirect HIGH
    batch[idx++] = 0x00000000;  // Start X
    batch[idx++] = 0x00000000;  // Start Y
    batch[idx++] = 0x00000000;  // Start Z
    batch[idx++] = 0x00000001;  // Dim X = 1
    batch[idx++] = 0x00000001;  // Dim Y = 1
    batch[idx++] = 0x00000001;  // Dim Z = 1
    batch[idx++] = 0x00000000;  // Thread Width X = 0 (1 thread)
    batch[idx++] = 0x00000000;  // Thread Width Y
    batch[idx++] = 0x00000000;  // Thread Width Z
    batch[idx++] = 0x00000000;  // Reserved
    printf("[75%%] ✓ GPGPU_WALKER (1 work-item)\n");
    
    // MEDIA_STATE_FLUSH
    batch[idx++] = 0x70040000;
    printf("[80%%] ✓ MEDIA_STATE_FLUSH\n");
    
    // PIPE_CONTROL (6 DWords)
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00100000;  // RT Cache Flush
    batch[idx++] = 0x00000000;  // Address LOW
    batch[idx++] = 0x00000000;  // Address HIGH
    batch[idx++] = 0x00000000;  // Data LOW
    batch[idx++] = 0x00000000;  // Data HIGH
    printf("[85%%] ✓ PIPE_CONTROL\n");
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    printf("[87%%] ✓ MI_BATCH_BUFFER_END\n");
    
    uint32_t batch_complet_len = idx * 4;
    printf("[89%%] ✓ Batch complet: %u DWords (%u bytes)\n", idx, batch_complet_len);
    
    // === PASS 2: EXÉCUTION GPU FINALE AVEC PINNED SÉLECTIF ===
    printf("\n=== PASS 2: EXÉCUTION GPU FINALE (90%%) ===\n");
    printf("STRATÉGIE: PINNED seulement pour GTT != 0x0\n");
    
    struct drm_i915_gem_exec_object2 objects_pass2[3] = {
        {
            .handle = output_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE,  // ✅ PAS PINNED (GTT=0x0)
            .offset = 0  // Laisser i915 choisir
        },
        {
            .handle = heap_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = heap_gtt
        },
        {
            .handle = batch_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = batch_gtt  // ✅ NON-NULL maintenant!
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf_pass2 = {
        .buffers_ptr = (uint64_t)objects_pass2,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = batch_complet_len,
        .flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC,
        .rsvd1 = context_id,
        .rsvd2 = 0
    };
    
    fprintf(stderr, "[DEBUG] PASS 2 Configuration:\n");
    fprintf(stderr, "[DEBUG]   batch_gtt = 0x%016lx (PINNED) ✅\n", batch_gtt);
    fprintf(stderr, "[DEBUG]   heap_gtt = 0x%016lx (PINNED) ✅\n", heap_gtt);
    fprintf(stderr, "[DEBUG]   output_gtt = 0x%016lx (PAS PINNED) ✅\n", output_gtt);
    fprintf(stderr, "[DEBUG]   batch_len = %u bytes\n", batch_complet_len);
    
    printf("[92%%] Soumission PASS 2 (batch complet, PINNED sélectif)...\n");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass2) < 0) {
        fprintf(stderr, "❌ PASS 2 ÉCHEC: %s (errno=%d)\n", strerror(errno), errno);
        fprintf(stderr, "[DEBUG] Vérifier dmesg pour GPU HANG\n");
        close(fd);
        return 1;
    }
    printf("[93%%] ✓ PASS 2 RÉUSSI - Batch soumis au GPU! 🎉\n");
    
    // === SYNCHRONISATION GPU ===
    printf("\n=== SYNCHRONISATION GPU (94%%) ===\n");
    
    // GEM_WAIT
    struct drm_i915_gem_wait wait = {
        .bo_handle = output_handle,
        .timeout_ns = 10000000000LL
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("gem_wait");
        close(fd);
        return 1;
    }
    printf("[95%%] ✓ GEM_WAIT - GPU terminé\n");
    
    // GEM_SET_DOMAIN
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
    printf("[96%%] ✓ GEM_SET_DOMAIN - Cache synchronisé\n");
    
    // === VÉRIFICATION RÉSULTAT ===
    printf("\n=== VÉRIFICATION RÉSULTAT (97%%) ===\n");
    
    printf("output[0] = 0x%08x (attendu: 0x12345678)\n", output[0]);
    printf("output[1] = 0x%08x\n", output[1]);
    printf("output[2] = 0x%08x\n", output[2]);
    printf("output[3] = 0x%08x\n", output[3]);
    
    bool success = (output[0] == 0x12345678);
    
    // === CLEANUP ===
    printf("\n=== CLEANUP (98%%) ===\n");
    
    munmap(batch, 4096);
    munmap(heap, 16384);
    munmap(output, 4096);
    
    // Détruire context
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = context_id };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy) < 0) {
        perror("context_destroy");
    } else {
        printf("[99%%] ✓ Context détruit\n");
    }
    
    close(fd);
    printf("[100%%] ✓ DRM fermé\n");
    
    // === RÉSULTAT FINAL ===
    printf("\n=== RÉSULTAT FINAL ===\n");
    if (success) {
        printf("✅ SUCCÈS! GPU a écrit 0x12345678\n");
        printf("✅ ROOT CAUSE #68 RÉSOLU!\n");
        printf("✅ Infrastructure i915 native FONCTIONNELLE!\n");
        return 0;
    } else {
        printf("❌ ÉCHEC: output[0]=0x%08x != 0x12345678\n", output[0]);
        printf("⚠️  GPU a exécuté mais résultat incorrect\n");
        printf("⚠️  Analyser batch buffer ou kernel ISA\n");
        return 1;
    }
}

// Made with Bob
