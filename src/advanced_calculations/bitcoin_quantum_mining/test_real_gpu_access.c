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
    printf("=== TEST ACCÈS GPU RÉEL INTEL UHD 620 ===\n\n");
    
    /* Ouvrir resource0 (BAR0 = MMIO registres) */
    const char* resource_path = "/sys/bus/pci/devices/0000:00:02.0/resource0";
    int fd = open(resource_path, O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("❌ Cannot open %s: %s\n", resource_path, strerror(errno));
        printf("   Trying with sudo...\n");
        return 1;
    }
    
    printf("✅ Opened %s (fd=%d)\n", resource_path, fd);
    
    /* Mapper 16MB MMIO */
    size_t mmio_size = 16 * 1024 * 1024;
    void* mmio = mmap(NULL, mmio_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    if (mmio == MAP_FAILED) {
        printf("❌ Cannot mmap: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("✅ MMIO mapped: %p (size=16MB)\n\n", mmio);
    
    /* Lire registres GPU réels */
    printf("=== LECTURE REGISTRES GPU RÉELS ===\n");
    
    volatile uint32_t* regs = (volatile uint32_t*)mmio;
    
    /* Registre 0x0000: Device ID */
    uint32_t device_id = regs[0x0000 / 4];
    printf("0x0000 (Device ID):     0x%08x\n", device_id);
    
    /* Registre 0x2000: GPU Status (hypothétique) */
    uint32_t gpu_status = regs[0x2000 / 4];
    printf("0x2000 (GPU Status):    0x%08x\n", gpu_status);
    
    /* Registre 0x2004: GPU Control */
    uint32_t gpu_control = regs[0x2004 / 4];
    printf("0x2004 (GPU Control):   0x%08x\n", gpu_control);
    
    /* Registre 0x8000: EU 0 Status */
    uint32_t eu0_status = regs[0x8000 / 4];
    printf("0x8000 (EU 0 Status):   0x%08x\n", eu0_status);
    
    /* Scan premiers 64 registres */
    printf("\n=== SCAN 64 PREMIERS REGISTRES ===\n");
    for (int i = 0; i < 16; i++) {
        printf("0x%04x: %08x %08x %08x %08x\n",
               i * 16,
               regs[i*4 + 0], regs[i*4 + 1],
               regs[i*4 + 2], regs[i*4 + 3]);
    }
    
    munmap(mmio, mmio_size);
    close(fd);
    
    printf("\n✅ Test terminé\n");
    return 0;
}
