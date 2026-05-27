// TEST C530 - ROOT CAUSE #37: Kernel utilise BTI=51, pas BTI=0
// Solution: Placer Surface State à BTI[51] dans la Binding Table
// Basé sur analyse ISA: instruction SENDC @ 0x70 utilise BTI=0x33 (51)

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <i915_drm.h>
#include <drm/drm.h>

// Kernel ISA minimal_write (320 bytes) - utilise BTI=51
#include "test_c491_opencl_kernel.h"

#define BATCH_SIZE 4096
#define SURFACE_STATE_SIZE 4096
#define KERNEL_SIZE 4096
#define OUTPUT_SIZE 4096

// Structure pour 2-PASS
typedef struct {
    uint32_t handle;
    uint64_t offset;
    uint64_t size;
    void *cpu_addr;
} buffer_info_t;

static void print_hex(const char *label, const void *data, size_t len) {
    const uint8_t *bytes = data;
    printf("%s (%zu bytes):\n", label, len);
    for (size_t i = 0; i < len; i += 16) {
        printf("  %04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
}

int main(void) {
    printf("=== TEST C530 - ROOT CAUSE #37: BTI=51 FIX ===\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open /dev/dri/card1");
        return 1;
    }
    
    // Créer les buffers
    buffer_info_t batch_buf = {0};
    buffer_info_t surface_buf = {0};
    buffer_info_t kernel_buf = {0};
    buffer_info_t output_buf = {0};
    
    // 1. Batch buffer
    struct drm_i915_gem_create create = {.size = BATCH_SIZE};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("create batch");
        close(fd);
        return 1;
    }
    batch_buf.handle = create.handle;
    batch_buf.size = BATCH_SIZE;
    
    // 2. Surface State + Binding Table buffer
    create.size = SURFACE_STATE_SIZE;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("create surface");
        close(fd);
        return 1;
    }
    surface_buf.handle = create.handle;
    surface_buf.size = SURFACE_STATE_SIZE;
    
    // 3. Kernel buffer
    create.size = KERNEL_SIZE;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("create kernel");
        close(fd);
        return 1;
    }
    kernel_buf.handle = create.handle;
    kernel_buf.size = KERNEL_SIZE;
    
    // 4. Output buffer
    create.size = OUTPUT_SIZE;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("create output");
        close(fd);
        return 1;
    }
    output_buf.handle = create.handle;
    output_buf.size = OUTPUT_SIZE;
    
    // Mapper les buffers
    struct drm_i915_gem_mmap_offset mmap_arg = {0};
    
    mmap_arg.handle = batch_buf.handle;
    mmap_arg.flags = I915_MMAP_OFFSET_WB;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        perror("mmap_offset batch");
        close(fd);
        return 1;
    }
    batch_buf.cpu_addr = mmap(NULL, BATCH_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    mmap_arg.handle = surface_buf.handle;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        perror("mmap_offset surface");
        close(fd);
        return 1;
    }
    surface_buf.cpu_addr = mmap(NULL, SURFACE_STATE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    mmap_arg.handle = kernel_buf.handle;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        perror("mmap_offset kernel");
        close(fd);
        return 1;
    }
    kernel_buf.cpu_addr = mmap(NULL, KERNEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    mmap_arg.handle = output_buf.handle;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        perror("mmap_offset output");
        close(fd);
        return 1;
    }
    output_buf.cpu_addr = mmap(NULL, OUTPUT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    // Initialiser output buffer
    memset(output_buf.cpu_addr, 0xFF, OUTPUT_SIZE);
    uint32_t *output = (uint32_t *)output_buf.cpu_addr;
    printf("Output buffer initialisé: output[0] = 0x%08x\n\n", output[0]);
    
    // Copier le kernel
    memcpy(kernel_buf.cpu_addr, kernel_isa_gen9, kernel_isa_gen9_size);
    printf("Kernel copié: %zu bytes\n\n", kernel_isa_gen9_size);
    
    // ========== PASS 1: Obtenir les offsets GPU ==========
    printf("=== PASS 1: Obtenir offsets GPU ===\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[4] = {0};
    
    exec_objects[0].handle = surface_buf.handle;
    exec_objects[0].flags = EXEC_OBJECT_WRITE;
    
    exec_objects[1].handle = kernel_buf.handle;
    
    exec_objects[2].handle = output_buf.handle;
    exec_objects[2].flags = EXEC_OBJECT_WRITE;
    
    exec_objects[3].handle = batch_buf.handle;
    exec_objects[3].flags = EXEC_OBJECT_WRITE;
    
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 4;
    execbuf.batch_len = 8;  // Juste un MI_BATCH_BUFFER_END
    execbuf.flags = I915_EXEC_RENDER;
    
    // Batch minimal pour PASS 1
    uint32_t *batch = (uint32_t *)batch_buf.cpu_addr;
    batch[0] = 0x05000000;  // MI_BATCH_BUFFER_END
    batch[1] = 0x00000000;
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("PASS 1 execbuffer2");
        printf("errno = %d\n", errno);
        close(fd);
        return 1;
    }
    
    // Récupérer les offsets
    surface_buf.offset = exec_objects[0].offset;
    kernel_buf.offset = exec_objects[1].offset;
    output_buf.offset = exec_objects[2].offset;
    batch_buf.offset = exec_objects[3].offset;
    
    printf("✓ PASS 1 réussi\n");
    printf("  surface_buf.offset = 0x%016lx\n", surface_buf.offset);
    printf("  kernel_buf.offset  = 0x%016lx\n", kernel_buf.offset);
    printf("  output_buf.offset  = 0x%016lx\n", output_buf.offset);
    printf("  batch_buf.offset   = 0x%016lx\n\n", batch_buf.offset);
    
    // ========== Construire Surface State + Binding Table ==========
    printf("=== Construction Surface State + Binding Table ===\n");
    
    memset(surface_buf.cpu_addr, 0, SURFACE_STATE_SIZE);
    uint32_t *surface_data = (uint32_t *)surface_buf.cpu_addr;
    
    // Binding Table: BTI[51] = offset 0x400 (vers Surface State)
    // BTI[51] est à l'offset 51 * 4 = 204 bytes = 0xCC
    surface_data[51] = 0x400;  // Offset relatif vers Surface State
    
    printf("Binding Table:\n");
    printf("  BTI[51] @ 0x00cc = 0x%08x (offset vers Surface State)\n\n", surface_data[51]);
    
    // Surface State à offset 0x400 (1024 bytes)
    uint32_t *ss = &surface_data[0x400 / 4];
    
    // DW0: Surface Type + Format + MOCS
    ss[0] = (0 << 29) |      // Surface Type = SURFTYPE_BUFFER (0)
            (0x7D << 18) |   // Surface Format = RAW (0x7D)
            (0x3 << 8) |     // MOCS = L3 + LLC (0x3)
            (0 << 0);        // Tile Mode = Linear
    
    // DW1: Base Address Low (bits 31:0)
    ss[1] = (uint32_t)(output_buf.offset & 0xFFFFFFFF);
    
    // DW2: Base Address High (bits 47:32) + MOCS high bits
    ss[2] = (uint32_t)((output_buf.offset >> 32) & 0xFFFF) | (0x3D << 16);
    
    // DW3: Width (en bytes - 1)
    ss[3] = (OUTPUT_SIZE - 1) & 0x1FFFFFF;  // 27 bits pour width
    
    // DW4: Height (0 pour buffer)
    ss[4] = 0;
    
    // DW5: Depth + LOD + Min LOD
    ss[5] = 0;
    
    // DW6-15: Réservés
    for (int i = 6; i < 16; i++) {
        ss[i] = 0;
    }
    
    printf("Surface State @ offset 0x400:\n");
    for (int i = 0; i < 16; i++) {
        printf("  DW%2d: 0x%08x\n", i, ss[i]);
    }
    printf("\n");
    
    // ========== Construire le batch buffer ==========
    printf("=== Construction batch buffer ===\n");
    
    memset(batch_buf.cpu_addr, 0, BATCH_SIZE);
    batch = (uint32_t *)batch_buf.cpu_addr;
    uint32_t batch_offset = 0;
    
    // 1. PIPE_CONTROL - Flush
    batch[batch_offset++] = 0x7a000004;  // PIPE_CONTROL, length=4
    batch[batch_offset++] = 0x00100000;  // CS Stall
    batch[batch_offset++] = 0x00000000;  // Address low
    batch[batch_offset++] = 0x00000000;  // Address high
    batch[batch_offset++] = 0x00000000;  // Data low
    batch[batch_offset++] = 0x00000000;  // Data high
    
    // 2. STATE_BASE_ADDRESS
    batch[batch_offset++] = 0x61010010;  // STATE_BASE_ADDRESS, length=16
    
    // General State Base Address
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    
    // Surface State Base Address (notre buffer surface)
    batch[batch_offset++] = (uint32_t)(surface_buf.offset & 0xFFFFFFFF) | 0x1;
    batch[batch_offset++] = (uint32_t)(surface_buf.offset >> 32);
    
    // Dynamic State Base Address
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    
    // Indirect Object Base Address
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    
    // Instruction Base Address (kernel)
    batch[batch_offset++] = (uint32_t)(kernel_buf.offset & 0xFFFFFFFF) | 0x1;
    batch[batch_offset++] = (uint32_t)(kernel_buf.offset >> 32);
    
    // Buffer sizes (0xFFFFF = no limit)
    batch[batch_offset++] = 0xFFFFF001;  // General State
    batch[batch_offset++] = 0xFFFFF001;  // Dynamic State
    batch[batch_offset++] = 0xFFFFF001;  // Indirect Object
    batch[batch_offset++] = 0xFFFFF001;  // Instruction
    
    // Stateless Data Port Access MOCS
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    
    // 3. MEDIA_VFE_STATE
    batch[batch_offset++] = 0x70000007;  // MEDIA_VFE_STATE, length=7
    batch[batch_offset++] = 0x00000000;  // Scratch Space Base Pointer
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x07820000;  // Max threads=24, URB entries=8
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    
    // 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD
    uint32_t idrt_offset = batch_offset * 4 + 64;  // IDRT après cette commande
    
    batch[batch_offset++] = 0x70020004;  // MEDIA_INTERFACE_DESCRIPTOR_LOAD, length=4
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000020;  // Interface Descriptor Total Length = 32 bytes
    batch[batch_offset++] = idrt_offset; // Interface Descriptor Data Start Address
    batch[batch_offset++] = 0x00000000;
    
    // 5. IDRT (Interface Descriptor) - 8 DWORDs
    uint32_t kernel_offset_in_isa = 0x40;  // Section .text.minimal_write
    uint64_t kernel_start = kernel_buf.offset + kernel_offset_in_isa;
    
    batch[batch_offset++] = (uint32_t)(kernel_start & 0xFFFFFFFF);  // DW0: Kernel Start Pointer low
    batch[batch_offset++] = (uint32_t)(kernel_start >> 32);         // DW1: Kernel Start Pointer high
    batch[batch_offset++] = 0x00000000;                             // DW2: Reserved
    batch[batch_offset++] = 0x00000000;                             // DW3: Binding Table Pointer = 0 (début du surface buffer)
    batch[batch_offset++] = 0x00000000;                             // DW4: Sampler State Pointer
    batch[batch_offset++] = 0x00E00000;                             // DW5: Thread Group Size (1x1x1)
    batch[batch_offset++] = 0x00000007;                             // DW6: Barrier Enable + CURBE Length = 7*32 = 224 bytes
    batch[batch_offset++] = 0x00000000;                             // DW7: Reserved
    
    // 6. GPGPU_WALKER
    batch[batch_offset++] = 0x18800101;  // GPGPU_WALKER, length=13
    batch[batch_offset++] = 0x00000000;  // Interface Descriptor Offset = 0
    batch[batch_offset++] = 0x00000000;  // Indirect Data Length = 0
    batch[batch_offset++] = 0x00000000;  // Indirect Data Start Address
    
    // Thread dimensions (1x1x1)
    batch[batch_offset++] = 0x00000001;  // Thread Width
    batch[batch_offset++] = 0x00000000;  // Thread Width Upper
    batch[batch_offset++] = 0x00000001;  // Thread Height
    batch[batch_offset++] = 0x00000000;  // Thread Height Upper
    batch[batch_offset++] = 0x00000001;  // Thread Depth
    batch[batch_offset++] = 0x00000000;  // Thread Depth Upper
    
    // Thread Group dimensions
    batch[batch_offset++] = 0x00000000;  // Thread Group ID Starting X
    batch[batch_offset++] = 0x00000000;  // Thread Group ID X Dimension
    batch[batch_offset++] = 0x00000000;  // Thread Group ID Starting Y
    batch[batch_offset++] = 0x00000000;  // Thread Group ID Y Dimension
    
    // 7. PIPE_CONTROL - Flush final
    batch[batch_offset++] = 0x7a000004;
    batch[batch_offset++] = 0x00100000;
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    batch[batch_offset++] = 0x00000000;
    
    // 8. MI_BATCH_BUFFER_END
    batch[batch_offset++] = 0x05000000;
    
    uint32_t batch_length = batch_offset * 4;
    printf("Batch buffer construit: %u bytes (%u dwords)\n", batch_length, batch_offset);
    printf("  IDRT offset: 0x%x\n", idrt_offset);
    printf("  Kernel start: 0x%016lx\n", kernel_start);
    printf("  BTI pointer: 0x%08x (début du surface buffer)\n\n", 0);
    
    // ========== PASS 2: Exécution avec NO_RELOC ==========
    printf("=== PASS 2: Exécution GPU avec NO_RELOC ===\n");
    
    exec_objects[0].offset = surface_buf.offset;
    exec_objects[0].flags = EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects[1].offset = kernel_buf.offset;
    exec_objects[1].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects[2].offset = output_buf.offset;
    exec_objects[2].flags = EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects[3].offset = batch_buf.offset;
    exec_objects[3].flags = EXEC_OBJECT_WRITE | EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    execbuf.batch_len = batch_length;
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("PASS 2 execbuffer2");
        printf("errno = %d\n", errno);
        close(fd);
        return 1;
    }
    
    printf("✓ PASS 2 réussi - GPU exécuté\n\n");
    
    // Attendre la fin
    struct drm_i915_gem_wait wait = {
        .bo_handle = output_buf.handle,
        .timeout_ns = 1000000000,  // 1 seconde
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    
    // ========== Vérification résultat ==========
    printf("=== VÉRIFICATION RÉSULTAT ===\n");
    printf("output[0] = 0x%08x\n", output[0]);
    
    if (output[0] == 0x12345678) {
        printf("\n✓✓✓ SUCCÈS! Le kernel a écrit la valeur attendue!\n");
        printf("✓✓✓ ROOT CAUSE #37 RÉSOLU: BTI=51 configuré correctement\n");
        printf("✓✓✓ PREMIER RÉSULTAT VALIDE SUR GPU NATIF!\n");
        return 0;
    } else if (output[0] == 0xFFFFFFFF) {
        printf("\n✗ ÉCHEC: output[0] inchangé (0xFFFFFFFF)\n");
        printf("Le GPU n'a pas écrit en mémoire.\n");
        return 1;
    } else {
        printf("\n⚠️  Valeur inattendue: 0x%08x\n", output[0]);
        return 1;
    }
}

// Made with Bob
