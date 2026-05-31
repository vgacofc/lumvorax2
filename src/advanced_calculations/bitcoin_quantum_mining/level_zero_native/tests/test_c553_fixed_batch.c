
// TEST C553 - CORRECTIONS ROOT CAUSE #60
// Objectif: Batch valide avec Dynamic State Base + GPGPU_WALKER correct

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libdrm/i915_drm.h>
#include <libdrm/drm.h>

int main() {
    printf("=== TEST C553 - CORRECTIONS ROOT CAUSE #60 ===\n");
    printf("ROOT CAUSE #60: Dynamic State Base + GPGPU_WALKER opcode\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
