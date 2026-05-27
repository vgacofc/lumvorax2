// TEST C556 - CORRECTION ROOT CAUSE #63: STRATÉGIE 2-PASS AVEC ADRESSES GTT
// Objectif: Implémenter la stratégie 2-PASS d'OpenCL pour résoudre adresses GTT
// Référence: SOLUTION_DRM.md - ROOT CAUSE #43, #58, #59

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
    printf("=== TEST C556 - STRATÉGIE 2-PASS AVEC ADRESSES GTT ===\n");
    printf("Correction ROOT CAUSE #63: Utiliser adresses GPU format Intel\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            perror("open DRM");
            return 1;
        }
    }
    printf("[5%%] ✓ DRM ouvert (fd=%d)\n", fd);
    
    // === CRÉATION BUFFERS ===
    printf("\n=== CRÉATION BUFFERS (10%%) ===\n");
    
    // Batch buffer (4KB)
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("create batch");
        close(fd);
        return 1;
    }
    uint32_t batch_handle = create_batch.handle;
    printf("[15%%] ✓ Batch BO créé (handle=%u)\n", batch_handle);
    
    // Heap buffer (16KB) - Contient Surface State, BTI, IDRT, Kernel
    struct drm_i915_gem_create create_heap = { .size = 16384 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_heap) < 0) {
        perror("create heap");
        close(fd);
        return 1;
    }
    uint32_t heap_handle = create_heap.handle;
    printf("[20%%] ✓ Heap BO créé (handle=%u)\n", heap_handle);
    
    // Output buffer (4KB)
    struct drm_i915_gem_create create_output = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        perror("create output");
        close(fd);
        return 1;
    }
    uint32_t output_handle = create_output.handle;
    printf("[25%%] ✓ Output BO créé (handle=%u)\n", output_handle);
    
    // === MAPPING BUFFERS ===
    printf("\n=== MAPPING BUFFERS (30%%) ===\n");
    
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
    printf("[35%%] ✓ Batch mappé\n");
    
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
    printf("[40%%] ✓ Heap mappé\n");
    
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
    printf("[45%%] ✓ Output mappé\n");
    
    // === CONSTRUCTION HEAP (AVANT PASS 1) ===
    printf("\n=== CONSTRUCTION HEAP (50%%) ===\n");
    
    // Surface State @ 0x0000 (64 bytes)
    uint32_t *surface_state = (uint32_t *)heap;
    surface_state[0] = 0x00000004;  // Type: BUFFER
    // surface_state[1-2] seront patchés avec adresse GTT output après PASS 1
    surface_state[3] = 0x00000FFF;  // Width: 4095
    surface_state[4] = 0x00000000;  // Height/Depth
    surface_state[5] = 0x00001000;  // Pitch: 4096
    printf("[55%%] ✓ Surface State configuré (sera patché après PASS 1)\n");
    
    // Binding Table @ 0x0080 (16 bytes)
    uint32_t *binding_table = (uint32_t *)(heap + 0x80);
    binding_table[0] = 0x00000000;  // Offset relatif vers Surface State
    printf("[60%%] ✓ Binding Table configuré\n");
    
    // IDRT @ 0x00A0 (32 bytes)
    uint32_t *idrt = (uint32_t *)(heap + 0xA0);
    // idrt[0-1] seront patchés avec adresse GTT kernel après PASS 1
    idrt[2] = 0x00000000;  // Sampler State
    idrt[3] = 0x00000080;  // BTI offset (relatif au heap)
    idrt[4] = 0x00000001;  // Binding Table Entry Count
    idrt[5] = (1 << 31);   // Barrier Enable
    idrt[6] = 0x00000000;  // Reserved
    idrt[7] = 0x00000000;  // Reserved
    printf("[65%%] ✓ IDRT configuré (sera patché après PASS 1)\n");
    
    // Kernel ISA @ 0x0100 (64 bytes)
    memcpy(heap + 0x100, kernel_isa, sizeof(kernel_isa));
    printf("[70%%] ✓ Kernel ISA copié\n");
    
    // === CONSTRUCTION BATCH (AVANT PASS 1) ===
    printf("\n=== CONSTRUCTION BATCH (75%%) ===\n");
    
    uint32_t idx = 0;
    
    // PIPELINE_SELECT GPGPU
    batch[idx++] = 0x69041312;  // Opcode + Length
    batch[idx++] = 0x00000002;  // Mode GPGPU
    printf("[80%%] ✓ PIPELINE_SELECT émis\n");
    
    // STATE_BASE_ADDRESS (19 DWords)
    batch[idx++] = 0x61010011;  // Opcode + Length
    batch[idx++] = 0x00000001;  // General State Base Modify Enable
    batch[idx++] = 0x00000000;  // General State Base LOW (pas utilisé)
    batch[idx++] = 0x00000000;  // General State Base HIGH
    // Surface State Base (sera relocalisé vers heap)
    batch[idx++] = 0x00000001;  // Surface State Base Modify Enable
    uint32_t surface_base_offset = idx * 4;  // Offset pour relocation
    batch[idx++] = 0x00000000;  // Surface State Base LOW (RELOCATION)
    batch[idx++] = 0x00000000;  // Surface State Base HIGH (RELOCATION)
    // Dynamic State Base (sera relocalisé vers heap)
    batch[idx++] = 0x00000001;  // Dynamic State Base Modify Enable
    uint32_t dynamic_base_offset = idx * 4;  // Offset pour relocation
    batch[idx++] = 0x00000000;  // Dynamic State Base LOW (RELOCATION)
    batch[idx++] = 0x00000000;  // Dynamic State Base HIGH (RELOCATION)
    batch[idx++] = 0x00000000;  // Indirect Object Base
    batch[idx++] = 0x00000000;  // Indirect Object Base HIGH
    // Instruction Base (sera relocalisé vers heap)
    batch[idx++] = 0x00000001;  // Instruction Base Modify Enable
    uint32_t instruction_base_offset = idx * 4;  // Offset pour relocation
    batch[idx++] = 0x00000000;  // Instruction Base LOW (RELOCATION)
    batch[idx++] = 0x00000000;  // Instruction Base HIGH (RELOCATION)
    batch[idx++] = 0xFFFFF001;  // General Buffer Size
    batch[idx++] = 0xFFFFF001;  // Dynamic Buffer Size
    batch[idx++] = 0xFFFFFFFF;  // Indirect Buffer Size
    batch[idx++] = 0x003BF000;  // Instruction Buffer Size
    printf("[85%%] ✓ STATE_BASE_ADDRESS émis (avec offsets pour relocations)\n");
    
    // MEDIA_VFE_STATE (9 DWords)
    batch[idx++] = 0x70000007;  // Opcode + Length
    batch[idx++] = 0x00000000;  // Scratch Space Base LOW
    batch[idx++] = 0x00000000;  // Scratch Space Base HIGH
    batch[idx++] = 0x00A70000;  // Max Threads = 167 (Gen9 max)
    batch[idx++] = 0x00000001;  // URB Entries = 1
    batch[idx++] = 0x00000040;  // URB Entry Size = 64
    batch[idx++] = 0x00000782;  // CURBE Allocation Size = 1922
    batch[idx++] = 0x00000000;  // Scoreboard Mask
    batch[idx++] = 0x00000000;  // Reserved
    printf("[90%%] ✓ MEDIA_VFE_STATE émis\n");
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWords)
    batch[idx++] = 0x61020001;  // Opcode + Length
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000020;  // Interface Descriptor Total Length = 32 bytes
    batch[idx++] = 0x000000A0;  // Interface Descriptor Data Start Address (offset dans heap)
    printf("[92%%] ✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD émis\n");
    
    // GPGPU_WALKER (15 DWords)
    batch[idx++] = 0x11010014;  // Opcode + Length (Gen9 format)
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset = 0
    batch[idx++] = 0x00000000;  // Indirect Data Length
    batch[idx++] = 0x00000000;  // Indirect Data Start Address LOW
    batch[idx++] = 0x00000000;  // Indirect Data Start Address HIGH
    batch[idx++] = 0x00000000;  // Thread Group ID Start X
    batch[idx++] = 0x00000000;  // Thread Group ID Start Y
    batch[idx++] = 0x00000000;  // Thread Group ID Start Z
    batch[idx++] = 0x00000001;  // Thread Group ID X Dimension = 1
    batch[idx++] = 0x00000001;  // Thread Group ID Y Dimension = 1
    batch[idx++] = 0x00000001;  // Thread Group ID Z Dimension = 1
    batch[idx++] = 0x00000000;  // Thread Width Counter Max X = 0 (1 thread)
    batch[idx++] = 0x00000000;  // Thread Width Counter Max Y
    batch[idx++] = 0x00000000;  // Thread Width Counter Max Z
    batch[idx++] = 0x00000000;  // Reserved
    printf("[94%%] ✓ GPGPU_WALKER émis\n");
    
    // MEDIA_STATE_FLUSH
    batch[idx++] = 0x70040000;  // Opcode + Length
    printf("[96%%] ✓ MEDIA_STATE_FLUSH émis\n");
    
    // PIPE_CONTROL (6 DWords)
    batch[idx++] = 0x7A000004;  // Opcode + Length
    batch[idx++] = 0x00100000;  // Render Target Cache Flush
    batch[idx++] = 0x00000000;  // Address LOW
    batch[idx++] = 0x00000000;  // Address HIGH
    batch[idx++] = 0x00000000;  // Immediate Data LOW
    batch[idx++] = 0x00000000;  // Immediate Data HIGH
    printf("[98%%] ✓ PIPE_CONTROL émis\n");
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x05000000;  // Opcode
    printf("[100%%] ✓ MI_BATCH_BUFFER_END émis\n");
    
    uint32_t batch_len = idx * 4;
    printf("\n✓ Batch construit: %u DWords (%u bytes)\n", idx, batch_len);
    
    // === PASS 1: OBTENIR ADRESSES GTT ===
    printf("\n=== PASS 1: OBTENIR ADRESSES GTT (PASS1) ===\n");
    
    struct drm_i915_gem_exec_object2 objects_pass1[3] = {
        {
            .handle = heap_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
            .offset = 0
        },
        {
            .handle = output_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE,
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
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER
    };
    
    // Exécuter PASS 1 (juste pour obtenir adresses GTT, pas pour exécuter GPU)
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1) < 0) {
        fprintf(stderr, "❌ PASS 1 ÉCHEC: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    // Lire les adresses GTT assignées par i915
    uint64_t heap_gtt = objects_pass1[0].offset;
    uint64_t output_gtt = objects_pass1[1].offset;
    uint64_t batch_gtt = objects_pass1[2].offset;
    
    printf("✓ PASS 1 RÉUSSI - Adresses GTT obtenues:\n");
    printf("  Heap:   0x%016lx\n", heap_gtt);
    printf("  Output: 0x%016lx\n", output_gtt);
    printf("  Batch:  0x%016lx\n", batch_gtt);
    
    // === PATCHER AVEC ADRESSES GTT ===
    printf("\n=== PATCHER AVEC ADRESSES GTT (PATCH) ===\n");
    
    // Patcher Surface State avec adresse GTT output
    surface_state[1] = (uint32_t)(output_gtt & 0xFFFFFFFF);
    surface_state[2] = (uint32_t)(output_gtt >> 32);
    printf("✓ Surface State patché avec output GTT\n");
    
    // Patcher IDRT avec adresse GTT kernel (heap + 0x100)
    uint64_t kernel_gtt = heap_gtt + 0x100;
    idrt[0] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_gtt >> 32);
    printf("✓ IDRT patché avec kernel GTT\n");
    
    // Patcher STATE_BASE_ADDRESS dans batch avec adresses GTT heap
    batch[surface_base_offset / 4] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[surface_base_offset / 4 + 1] = (uint32_t)(heap_gtt >> 32);
    batch[dynamic_base_offset / 4] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[dynamic_base_offset / 4 + 1] = (uint32_t)(heap_gtt >> 32);
    batch[instruction_base_offset / 4] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[instruction_base_offset / 4 + 1] = (uint32_t)(heap_gtt >> 32);
    printf("✓ STATE_BASE_ADDRESS patché avec heap GTT\n");
    
    // === PASS 2: EXÉCUTION GPU AVEC ADRESSES FIXÉES ===
    printf("\n=== PASS 2: EXÉCUTION GPU (PASS2) ===\n");
    
    struct drm_i915_gem_exec_object2 objects_pass2[3] = {
        {
            .handle = heap_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = heap_gtt
        },
        {
            .handle = output_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED,
            .offset = output_gtt
        },
        {
            .handle = batch_handle,
            .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED,
            .offset = batch_gtt
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf_pass2 = {
        .buffers_ptr = (uint64_t)objects_pass2,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass2) < 0) {
        fprintf(stderr, "❌ PASS 2 ÉCHEC: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    printf("✓ PASS 2 RÉUSSI - Batch soumis au GPU\n");
    
    // === SYNCHRONISATION GPU ===
    printf("\n=== SYNCHRONISATION GPU (SYNC) ===\n");
    
    // GEM_WAIT
    struct drm_i915_gem_wait wait = {
        .bo_handle = output_handle,
        .timeout_ns = 10000000000LL  // 10 secondes
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("gem_wait");
        close(fd);
        return 1;
    }
    printf("✓ GEM_WAIT réussi - GPU terminé\n");
    
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
    printf("✓ GEM_SET_DOMAIN réussi - Cache synchronisé\n");
    
    // === VÉRIFICATION RÉSULTAT ===
    printf("\n=== VÉRIFICATION RÉSULTAT (VERIFY) ===\n");
    
    printf("output[0] = 0x%08x (attendu: 0x12345678)\n", output[0]);
    printf("output[1] = 0x%08x\n", output[1]);
    printf("output[2] = 0x%08x\n", output[2]);
    printf("output[3] = 0x%08x\n", output[3]);
    
    bool success = (output[0] == 0x12345678);
    
    // Cleanup
    munmap(batch, 4096);
    munmap(heap, 16384);
    munmap(output, 4096);
    close(fd);
    
    if (success) {
        printf("\n🎉 SUCCÈS TOTAL C556!\n");
        printf("✓ ROOT CAUSE #63 RÉSOLU\n");
        printf("✓ Stratégie 2-PASS fonctionnelle\n");
        printf("✓ Adresses GTT correctes\n");
        printf("✓ GPU natif i915 DRM 100%%\n");
        printf("✓ Élimination OpenCL: 100%%\n");
        return 0;
    } else {
        printf("\n❌ ÉCHEC - Valeur incorrecte\n");
        return 1;
    }
}

// Made with Bob
