#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

/* Adresses typiques Intel Gen9 Graphics */
#define GPU_BAR0_BASE    0xa0000000ULL
#define GPU_BAR2_BASE    0x90000000ULL

/* Offsets registres Intel Gen9 (documentation) */
#define GEN9_RENDER_RING_BASE    0x02000
#define GEN9_RENDER_HWS_PGA      0x02080
#define GEN9_GT_MODE             0x07000
#define GEN9_GT_THREAD_STATUS    0x07094
#define GEN9_EU_DISABLE          0x09134

int main() {
    printf("=== SCAN REGISTRES INTEL GEN9 GPU ===\n\n");
    
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("❌ Cannot open /dev/mem\n");
        return 1;
    }
    
    /* Mapper 64KB à partir de BAR0 */
    size_t size = 65536;
    void* mmio = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPU_BAR0_BASE);
    
    if (mmio == MAP_FAILED) {
        printf("❌ Cannot mmap\n");
        close(fd);
        return 1;
    }
    
    printf("✅ Mapped 0x%lx (64KB)\n\n", GPU_BAR0_BASE);
    
    volatile uint32_t* regs = (volatile uint32_t*)mmio;
    
    /* Lire registres connus Intel Gen9 */
    printf("=== REGISTRES INTEL GEN9 ===\n");
    printf("0x%05x (RENDER_RING_BASE):   0x%08x\n", GEN9_RENDER_RING_BASE, regs[GEN9_RENDER_RING_BASE/4]);
    printf("0x%05x (RENDER_HWS_PGA):     0x%08x\n", GEN9_RENDER_HWS_PGA, regs[GEN9_RENDER_HWS_PGA/4]);
    printf("0x%05x (GT_MODE):             0x%08x\n", GEN9_GT_MODE, regs[GEN9_GT_MODE/4]);
    printf("0x%05x (GT_THREAD_STATUS):   0x%08x\n", GEN9_GT_THREAD_STATUS, regs[GEN9_GT_THREAD_STATUS/4]);
    printf("0x%05x (EU_DISABLE):          0x%08x\n", GEN9_EU_DISABLE, regs[GEN9_EU_DISABLE/4]);
    
    /* Scan zone 0x2000-0x2100 (Render Ring) */
    printf("\n=== ZONE 0x2000-0x2100 (Render Ring) ===\n");
    for (int i = 0; i < 16; i++) {
        uint32_t offset = 0x2000 + i*16;
        printf("0x%04x: %08x %08x %08x %08x\n",
               offset,
               regs[offset/4 + 0], regs[offset/4 + 1],
               regs[offset/4 + 2], regs[offset/4 + 3]);
    }
    
    /* Scan zone 0x7000-0x7100 (GT Mode) */
    printf("\n=== ZONE 0x7000-0x7100 (GT Mode/Thread) ===\n");
    for (int i = 0; i < 16; i++) {
        uint32_t offset = 0x7000 + i*16;
        printf("0x%04x: %08x %08x %08x %08x\n",
               offset,
               regs[offset/4 + 0], regs[offset/4 + 1],
               regs[offset/4 + 2], regs[offset/4 + 3]);
    }
    
    munmap(mmio, size);
    close(fd);
    
    printf("\n✅ Scan terminé\n");
    return 0;
}
