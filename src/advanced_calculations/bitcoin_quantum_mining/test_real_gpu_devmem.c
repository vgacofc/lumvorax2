#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

int main() {
    printf("=== TEST ACCÈS GPU RÉEL via /dev/mem ===\n\n");
    
    /* Ouvrir /dev/mem */
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("❌ Cannot open /dev/mem: %s\n", strerror(errno));
        return 1;
    }
    
    printf("✅ Opened /dev/mem (fd=%d)\n", fd);
    
    /* Adresse GPU BAR0 */
    uint64_t gpu_base = 0xa0000000ULL;
    size_t mmio_size = 4096;  /* Essayer 4KB seulement */
    
    /* Mapper MMIO */
    void* mmio = mmap(NULL, mmio_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpu_base);
    
    if (mmio == MAP_FAILED) {
        printf("❌ Cannot mmap 0x%lx: %s\n", gpu_base, strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("✅ MMIO mapped: %p -> 0x%lx (size=4KB)\n\n", mmio, gpu_base);
    
    /* Lire registres GPU réels */
    printf("=== LECTURE REGISTRES GPU RÉELS ===\n");
    
    volatile uint32_t* regs = (volatile uint32_t*)mmio;
    
    /* Scan premiers 256 bytes */
    printf("Offset  | +0x00      +0x04      +0x08      +0x0C\n");
    printf("--------|------------------------------------------\n");
    for (int i = 0; i < 16; i++) {
        printf("0x%04x  | %08x   %08x   %08x   %08x\n",
               i * 16,
               regs[i*4 + 0], regs[i*4 + 1],
               regs[i*4 + 2], regs[i*4 + 3]);
    }
    
    munmap(mmio, mmio_size);
    close(fd);
    
    printf("\n✅ Test terminé\n");
    return 0;
}
