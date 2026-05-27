// TEST C556v2 - CORRECTION ROOT CAUSE #63: STRATÉGIE 2-PASS CORRECTE
// Objectif: PASS 1 = batch minimal pour obtenir GTT, PASS 2 = batch complet avec GTT
// Référence: SOLUTION_DRM.md - ROOT CAUSE #43, #58, #59, #63

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
    printf("=== TEST C556v2 - STRATÉGIE 2-PASS CORRECTE ===\n");
    printf("ROOT CAUSE #63: PASS 1 = batch minimal, PASS 2 = batch complet\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0) {
            perror("open DRM");
            return 1;
        }
    }
    printf("[2%%] ✓ DRM ouvert (fd=%d)\n", fd);
    
    // === CRÉATION BUFFERS ===
    printf("\n=== CRÉATION BUFFERS (5%%) ===\n");
    
    // Batch buffer (4KB)
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("create batch");
        close(fd);
        return 1;
    }
    uint32_t batch_handle = create_batch.handle;
    printf("[7%%] ✓ Batch BO créé (handle=%u)\n", batch_handle);
    
    // Heap buffer (16KB) - Contient Surface State, BTI, IDRT, Kernel
    struct drm_i915_gem_create create_heap = { .size = 16384 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_heap) < 0) {
        perror("create heap");
        close(fd);
        return 1;
    }
    uint32_t heap_handle = create_heap.handle;
    printf("[10%%] ✓ Heap BO créé (handle=%u)\n", heap_handle);
    
    // Output buffer (4KB)
    struct drm_i915_gem_create create_output = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        perror("create output");
        close(fd);
        return 1;
    }
    uint32_t output_handle = create_output.handle;
    printf("[12%%] ✓ Output BO créé (handle=%u)\n", output_handle);
    
    // === MAPPING BUFFERS ===
    printf("\n=== MAPPING BUFFERS (15%%) ===\n");
    
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
    printf("[18%%] ✓ Batch mappé\n");
    
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
    printf("[20%%] ✓ Heap mappé\n");
    
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
    printf("[22%%] ✓ Output mappé\n");
    
    // === CONSTRUCTION HEAP (STRUCTURES VIDES POUR PASS 1) ===
    printf("\n=== CONSTRUCTION HEAP INITIAL (25%%) ===\n");
    
    // Surface State @ 0x0000 (64 bytes) - VIDE pour PASS 1
    uint32_t *surface_state = (uint32_t *)heap;
    surface_state[0] = 0x00000004;  // Type: BUFFER
    surface_state[3] = 0x00000FFF;  // Width: 4095
    surface_state[5] = 0x00001000;  // Pitch: 4096
    printf("[28%%] ✓ Surface State structure créée (adresses à patcher)\n");
    
    // Binding Table @ 0x0080 (16 bytes)
    uint32_t *binding_table = (uint32_t *)(heap + 0x80);
    binding_table[0] = 0x00000000;  // Offset relatif vers Surface State
    printf("[30%%] ✓ Binding Table créé\n");
    
    // IDRT @ 0x00A0 (32 bytes) - VIDE pour PASS 1
    uint32_t *idrt = (uint32_t *)(heap + 0xA0);
    idrt[2] = 0x00000000;  // Sampler State
    idrt[3] = 0x00000080;  // BTI offset (relatif au heap)
    idrt[4] = 0x00000001;  // Binding Table Entry Count
    idrt[5] = (1 << 31);   // Barrier Enable
    printf("[32%%] ✓ IDRT structure créée (kernel pointer à patcher)\n");
    
    // Kernel ISA @ 0x0100 (64 bytes)
    memcpy(heap + 0x100, kernel_isa, sizeof(kernel_isa));
    printf("[35%%] ✓ Kernel ISA copié\n");
    
    // === PASS 1: BATCH MINIMAL POUR OBTENIR GTT ===
    printf("\n=== PASS 1: BATCH MINIMAL (40%%) ===\n");
    
    uint32_t idx = 0;
    
    // PIPELINE_SELECT GPGPU
    batch[idx++] = 0x69041312;  // Opcode + Length
    batch[idx++] = 0x00000002;  // Mode GPGPU
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x05000000;  // Opcode
    
    uint32_t batch_minimal_len = idx * 4;
    printf("[42%%] ✓ Batch minimal construit: %u DWords (%u bytes)\n", idx, batch_minimal_len);
    
    // Soumettre PASS 1 pour obtenir adresses GTT
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
        .batch_len = batch_minimal_len,
        .flags = I915_EXEC_RENDER
    };
    
    printf("[45%%] Soumission PASS 1 (batch minimal)...\n");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1) < 0) {
        fprintf(stderr, "❌ PASS 1 ÉCHEC: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    // Lire les adresses GTT assignées par i915
    uint64_t heap_gtt = objects_pass1[0].offset;
    uint64_t output_gtt = objects_pass1[1].offset;
    uint64_t batch_gtt = objects_pass1[2].offset;
    
    printf("[50%%] ✓ PASS 1 RÉUSSI - Adresses GTT obtenues:\n");
    printf("       Heap:   0x%016lx\n", heap_gtt);
    printf("       Output: 0x%016lx\n", output_gtt);
    printf("       Batch:  0x%016lx\n", batch_gtt);
    
    // === PATCHER HEAP AVEC ADRESSES GTT ===
    printf("\n=== PATCHER HEAP (55%%) ===\n");
    
    // Patcher Surface State avec adresse GTT output
    surface_state[1] = (uint32_t)(output_gtt & 0xFFFFFFFF);
    surface_state[2] = (uint32_t)(output_gtt >> 32);
    printf("[58%%] ✓ Surface State patché: output @ 0x%016lx\n", output_gtt);
    
    // Patcher IDRT avec adresse GTT kernel (heap + 0x100)
    uint64_t kernel_gtt = heap_gtt + 0x100;
    idrt[0] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_gtt >> 32);
    printf("[60%%] ✓ IDRT patché: kernel @ 0x%016lx\n", kernel_gtt);
    
    // === CONSTRUCTION BATCH COMPLET POUR PASS 2 ===
    printf("\n=== CONSTRUCTION BATCH COMPLET (65%%) ===\n");
    
    idx = 0;
    memset(batch, 0, 4096);
    
    // PIPELINE_SELECT GPGPU
    batch[idx++] = 0x69041312;  // Opcode + Length
    batch[idx++] = 0x00000002;  // Mode GPGPU
    printf("[68%%] ✓ PIPELINE_SELECT\n");
    
    // STATE_BASE_ADDRESS (19 DWords) - AVEC ADRESSES GTT
    batch[idx++] = 0x61010011;  // Opcode + Length
    batch[idx++] = 0x00000001;  // General State Base Modify Enable
    batch[idx++] = 0x00000000;  // General State Base LOW (pas utilisé)
    batch[idx++] = 0x00000000;  // General State Base HIGH
    // Surface State Base = heap GTT
    batch[idx++] = 0x00000001;  // Surface State Base Modify Enable
    batch[idx++] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(heap_gtt >> 32);
    // Dynamic State Base = heap GTT
    batch[idx++] = 0x00000001;  // Dynamic State Base Modify Enable
    batch[idx++] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(heap_gtt >> 32);
    batch[idx++] = 0x00000000;  // Indirect Object Base
    batch[idx++] = 0x00000000;  // Indirect Object Base HIGH
    // Instruction Base = heap GTT
    batch[idx++] = 0x00000001;  // Instruction Base Modify Enable
    batch[idx++] = (uint32_t)(heap_gtt & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(heap_gtt >> 32);
    batch[idx++] = 0xFFFFF001;  // General Buffer Size
    batch[idx++] = 0xFFFFF001;  // Dynamic Buffer Size
    batch[idx++] = 0xFFFFFFFF;  // Indirect Buffer Size
    batch[idx++] = 0x003BF000;  // Instruction Buffer Size
    printf("[70%%] ✓ STATE_BASE_ADDRESS (heap GTT: 0x%016lx)\n", heap_gtt);
    
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
    printf("[75%%] ✓ MEDIA_VFE_STATE\n");
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWords)
    batch[idx++] = 0x61020001;  // Opcode + Length
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000020;  // Interface Descriptor Total Length = 32 bytes
    batch[idx++] = 0x000000A0;  // Interface Descriptor Data Start Address (offset dans heap)
    printf("[78%%] ✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD\n");
    
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
    printf("[80%%] ✓ GPGPU_WALKER\n");
    
    // MEDIA_STATE_FLUSH
    batch[idx++] = 0x70040000;  // Opcode + Length
    printf("[82%%] ✓ MEDIA_STATE_FLUSH\n");
    
    // PIPE_CONTROL (6 DWords)
    batch[idx++] = 0x7A000004;  // Opcode + Length
    batch[idx++] = 0x00100000;  // Render Target Cache Flush
    batch[idx++] = 0x00000000;  // Address LOW
    batch[idx++] = 0x00000000;  // Address HIGH
    batch[idx++] = 0x00000000;  // Immediate Data LOW
    batch[idx++] = 0x00000000;  // Immediate Data HIGH
    printf("[85%%] ✓ PIPE_CONTROL\n");
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x05000000;  // Opcode
    printf("[88%%] ✓ MI_BATCH_BUFFER_END\n");
    
    uint32_t batch_complet_len = idx * 4;
    printf("[90%%] ✓ Batch complet: %u DWords (%u bytes)\n", idx, batch_complet_len);
    
    // === PASS 2: EXÉCUTION GPU AVEC ADRESSES FIXÉES ===
    printf("\n=== PASS 2: EXÉCUTION GPU (92%%) ===\n");
    
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
        .batch_len = batch_complet_len,
        .flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC
    };
    
    printf("[94%%] Soumission PASS 2 (batch complet avec GTT)...\n");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass2) < 0) {
        fprintf(stderr, "❌ PASS 2 ÉCHEC: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    printf("[95%%] ✓ PASS 2 RÉUSSI - Batch soumis au GPU\n");
    
    // === SYNCHRONISATION GPU ===
    printf("\n=== SYNCHRONISATION GPU (96%%) ===\n");
    
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
    printf("[97%%] ✓ GEM_WAIT réussi - GPU terminé\n");
    
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
    printf("[98%%] ✓ GEM_SET_DOMAIN réussi - Cache synchronisé\n");
    
    // === VÉRIFICATION RÉSULTAT ===
    printf("\n=== VÉRIFICATION RÉSULTAT (99%%) ===\n");
    
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
        printf("\n[100%%] 🎉 SUCCÈS TOTAL C556v2!\n");
        printf("✓ ROOT CAUSE #63 RÉSOLU\n");
        printf("✓ Stratégie 2-PASS correcte implémentée\n");
        printf("✓ PASS 1: Batch minimal → GTT addresses\n");
        printf("✓ PASS 2: Batch complet avec GTT → GPU execution\n");
        printf("✓ GPU natif i915 DRM 100%%\n");
        printf("✓ Élimination OpenCL: 100%%\n");
        return 0;
    } else {
        printf("\n❌ ÉCHEC - Valeur incorrecte\n");
        printf("Analyse: GPU n'a pas écrit ou kernel n'a pas exécuté\n");
        return 1;
    }
}

// Made with Bob