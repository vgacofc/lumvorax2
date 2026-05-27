#!/bin/bash
# Dump toutes les structures heap pour analyse

echo "🔍 DUMP STRUCTURES HEAP C584"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Recompiler avec dump
gcc -o test_c584_relocation tests/test_c584_relocation.c -ldrm -I/usr/include/libdrm -DDUMP_STRUCTURES=1

# Créer version avec dumps
cat > /tmp/test_c584_dump.c << 'EOFC'
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
#include <i915_drm.h>

#define KERNEL_ISA_SIZE 128
#define OFFSET_KERNEL_ISA 0x0000
#define OFFSET_SURFACE_STATE 0x1000
#define OFFSET_BINDING_TABLE 0x1100
#define OFFSET_IDRT 0x1200
#define OFFSET_OUTPUT 0x2000

static const unsigned char kernel_isa_gen9[KERNEL_ISA_SIZE] = {
    0x01, 0x00, 0x60, 0x00, 0x0c, 0x02, 0x40, 0x20,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x30,
    0x00, 0x10, 0x00, 0x16, 0xc0, 0x04, 0xc0, 0x04,
    0x01, 0x00, 0x00, 0x00, 0x0c, 0x43, 0x80, 0x20,
    0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x2c, 0x0e, 0xc0, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12,
    0x01, 0x4d, 0x00, 0x20, 0x07, 0x7f, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void dump_hex(const char *label, const void *data, size_t size) {
    const uint8_t *bytes = data;
    printf("\n%s (%zu bytes):\n", label, size);
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) printf("  %04zx: ", i);
        printf("%02x ", bytes[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (size % 16 != 0) printf("\n");
}

void dump_dwords(const char *label, const uint32_t *data, size_t count) {
    printf("\n%s (%zu DWords):\n", label, count);
    for (size_t i = 0; i < count; i++) {
        if (i % 4 == 0) printf("  [%2zu]: ", i);
        printf("0x%08x ", data[i]);
        if ((i + 1) % 4 == 0) printf("\n");
    }
    if (count % 4 != 0) printf("\n");
}

int main(void) {
    printf("🔍 TEST C584 - DUMP STRUCTURES HEAP\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    struct drm_i915_gem_create heap_create = { .size = 65536 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &heap_create) < 0) {
        perror("create heap");
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = heap_create.handle,
        .offset = 0,
        .size = 65536,
        .flags = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        perror("mmap");
        return 1;
    }
    
    uint8_t *heap = (uint8_t *)mmap_arg.addr_ptr;
    memset(heap, 0, 65536);
    
    // Setup structures
    memcpy(heap + OFFSET_KERNEL_ISA, kernel_isa_gen9, KERNEL_ISA_SIZE);
    
    uint32_t *surface_state = (uint32_t *)(heap + OFFSET_SURFACE_STATE);
    surface_state[0] = 0x00000000;
    surface_state[1] = 0x00000100;
    surface_state[2] = OFFSET_OUTPUT;
    surface_state[3] = 0x00000000;
    
    uint32_t *binding_table = (uint32_t *)(heap + OFFSET_BINDING_TABLE);
    binding_table[0] = OFFSET_SURFACE_STATE;
    
    uint32_t *idrt = (uint32_t *)(heap + OFFSET_IDRT);
    idrt[0] = OFFSET_KERNEL_ISA;
    idrt[1] = 0x00000000;
    idrt[2] = 0x00000000;
    idrt[3] = (OFFSET_BINDING_TABLE & 0xFFFFFFE0);
    idrt[4] = 0x00000000;
    idrt[5] = 0x00000100;
    idrt[6] = 0x00000000;
    idrt[7] = 0x00000000;
    
    uint32_t *output = (uint32_t *)(heap + OFFSET_OUTPUT);
    output[0] = 0xDEADBEEF;
    
    // Dump all structures
    dump_hex("Kernel ISA", heap + OFFSET_KERNEL_ISA, KERNEL_ISA_SIZE);
    dump_dwords("Surface State", surface_state, 16);
    dump_dwords("Binding Table", binding_table, 1);
    dump_dwords("IDRT", idrt, 8);
    dump_hex("Output Buffer", output, 16);
    
    printf("\n📊 ANALYSE POINTEURS:\n");
    printf("  Kernel ISA offset:     0x%04x\n", OFFSET_KERNEL_ISA);
    printf("  Surface State offset:  0x%04x\n", OFFSET_SURFACE_STATE);
    printf("  Binding Table offset:  0x%04x\n", OFFSET_BINDING_TABLE);
    printf("  IDRT offset:           0x%04x\n", OFFSET_IDRT);
    printf("  Output offset:         0x%04x\n", OFFSET_OUTPUT);
    printf("\n");
    printf("  IDRT[0] (Kernel Start):    0x%08x → heap+0x%04x\n", idrt[0], idrt[0]);
    printf("  IDRT[3] (Binding Table):   0x%08x → heap+0x%04x\n", idrt[3], idrt[3]);
    printf("  BT[0] (Surface State):     0x%08x → heap+0x%04x\n", binding_table[0], binding_table[0]);
    printf("  SS[2] (Output Buffer):     0x%08x → heap+0x%04x\n", surface_state[2], surface_state[2]);
    
    close(fd);
    return 0;
}
EOFC

gcc -o /tmp/test_c584_dump /tmp/test_c584_dump.c -ldrm -I/usr/include/libdrm
/tmp/test_c584_dump
