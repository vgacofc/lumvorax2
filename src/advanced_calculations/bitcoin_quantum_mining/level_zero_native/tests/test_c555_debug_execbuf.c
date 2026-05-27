// TEST C555 - DEBUG EXECBUFFER2 STRUCTURE
// Objectif: Capturer et afficher la structure exacte envoyée au kernel

#include "../include/i915_batch_builder_auto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

// Copie des structures i915 pour debug
struct debug_reloc {
    uint32_t target_handle;
    uint32_t delta;
    uint64_t offset;
    uint64_t presumed_offset;
    uint32_t read_domains;
    uint32_t write_domain;
};

struct debug_exec_object {
    uint32_t handle;
    uint32_t relocation_count;
    uint64_t relocs_ptr;
    uint64_t alignment;
    uint64_t offset;
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
};

struct debug_execbuffer {
    uint64_t buffers_ptr;
    uint32_t buffer_count;
    uint32_t batch_start_offset;
    uint32_t batch_len;
    uint32_t DR1;
    uint32_t DR4;
    uint32_t num_cliprects;
    uint64_t cliprects_ptr;
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
};

void dump_execbuffer_structure(int fd) {
    printf("\n=== DUMP STRUCTURE EXECBUFFER2 ===\n\n");
    
    // Créer BOs
    struct drm_i915_gem_create create = {.size = 4096};
    
    // Batch BO
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("create batch");
        return;
    }
    uint32_t batch_handle = create.handle;
    printf("Batch BO créé: handle=%u\n", batch_handle);
    
    // Heap BO
    create.size = 16384;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("create heap");
        return;
    }
    uint32_t heap_handle = create.handle;
    printf("Heap BO créé: handle=%u\n\n", heap_handle);
    
    // Construire relocations
    struct debug_reloc relocs[5];
    memset(relocs, 0, sizeof(relocs));
    
    relocs[0].target_handle = heap_handle;
    relocs[0].offset = 0x14;
    relocs[0].delta = 0;
    relocs[0].presumed_offset = 0;  // PROBLÈME POTENTIEL
    relocs[0].read_domains = 0x2;
    relocs[0].write_domain = 0x0;
    
    relocs[1].target_handle = heap_handle;
    relocs[1].offset = 0x1c;
    relocs[1].delta = 0;
    relocs[1].presumed_offset = 0;
    relocs[1].read_domains = 0x2;
    relocs[1].write_domain = 0x0;
    
    relocs[2].target_handle = heap_handle;
    relocs[2].offset = 0x2c;
    relocs[2].delta = 0;
    relocs[2].presumed_offset = 0;
    relocs[2].read_domains = 0x10;
    relocs[2].write_domain = 0x0;
    
    relocs[3].target_handle = heap_handle;
    relocs[3].offset = 0x58;
    relocs[3].delta = 0;
    relocs[3].presumed_offset = 0;
    relocs[3].read_domains = 0x2;
    relocs[3].write_domain = 0x0;
    
    relocs[4].target_handle = heap_handle;
    relocs[4].offset = 0x90;
    relocs[4].delta = 0xf0;
    relocs[4].presumed_offset = 0;
    relocs[4].read_domains = 0x2;
    relocs[4].write_domain = 0x0;
    
    printf("=== RELOCATIONS ===\n");
    for (int i = 0; i < 5; i++) {
        printf("Reloc[%d]:\n", i);
        printf("  target_handle    = 0x%08x (%u)\n", relocs[i].target_handle, relocs[i].target_handle);
        printf("  offset           = 0x%016lx\n", relocs[i].offset);
        printf("  delta            = 0x%08x\n", relocs[i].delta);
        printf("  presumed_offset  = 0x%016lx\n", relocs[i].presumed_offset);
        printf("  read_domains     = 0x%08x\n", relocs[i].read_domains);
        printf("  write_domain     = 0x%08x\n\n", relocs[i].write_domain);
    }
    
    // Construire exec objects
    struct debug_exec_object objects[2];
    memset(objects, 0, sizeof(objects));
    
    // Heap BO
    objects[0].handle = heap_handle;
    objects[0].relocation_count = 0;  // Pas de relocations sur heap
    objects[0].relocs_ptr = 0;
    objects[0].alignment = 0;
    objects[0].offset = 0;
    objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    objects[0].rsvd1 = 0;
    objects[0].rsvd2 = 0;
    
    // Batch BO avec relocations
    objects[1].handle = batch_handle;
    objects[1].relocation_count = 5;
    objects[1].relocs_ptr = (uint64_t)relocs;
    objects[1].alignment = 0;
    objects[1].offset = 0;
    objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    objects[1].rsvd1 = 0;
    objects[1].rsvd2 = 0;
    
    printf("=== EXEC OBJECTS ===\n");
    for (int i = 0; i < 2; i++) {
        printf("Object[%d]:\n", i);
        printf("  handle           = 0x%08x (%u)\n", objects[i].handle, objects[i].handle);
        printf("  relocation_count = %u\n", objects[i].relocation_count);
        printf("  relocs_ptr       = 0x%016lx\n", objects[i].relocs_ptr);
        printf("  alignment        = 0x%016lx\n", objects[i].alignment);
        printf("  offset           = 0x%016lx\n", objects[i].offset);
        printf("  flags            = 0x%016lx\n", objects[i].flags);
        printf("  rsvd1            = 0x%016lx\n", objects[i].rsvd1);
        printf("  rsvd2            = 0x%016lx\n\n", objects[i].rsvd2);
    }
    
    // Construire execbuffer2
    struct debug_execbuffer execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    
    execbuf.buffers_ptr = (uint64_t)objects;
    execbuf.buffer_count = 2;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = 260;  // 65 DWords * 4
    execbuf.DR1 = 0;
    execbuf.DR4 = 0;
    execbuf.num_cliprects = 0;
    execbuf.cliprects_ptr = 0;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = 0;
    execbuf.rsvd2 = 0;
    
    printf("=== EXECBUFFER2 ===\n");
    printf("buffers_ptr        = 0x%016lx\n", execbuf.buffers_ptr);
    printf("buffer_count       = %u\n", execbuf.buffer_count);
    printf("batch_start_offset = %u\n", execbuf.batch_start_offset);
    printf("batch_len          = %u\n", execbuf.batch_len);
    printf("DR1                = 0x%08x\n", execbuf.DR1);
    printf("DR4                = 0x%08x\n", execbuf.DR4);
    printf("num_cliprects      = %u\n", execbuf.num_cliprects);
    printf("cliprects_ptr      = 0x%016lx\n", execbuf.cliprects_ptr);
    printf("flags              = 0x%016lx (I915_EXEC_RENDER=%d)\n", execbuf.flags, I915_EXEC_RENDER);
    printf("rsvd1              = 0x%016lx\n", execbuf.rsvd1);
    printf("rsvd2              = 0x%016lx\n\n", execbuf.rsvd2);
    
    // Tenter l'appel
    printf("=== APPEL IOCTL ===\n");
    printf("Appel: ioctl(%d, DRM_IOCTL_I915_GEM_EXECBUFFER2, %p)\n", fd, &execbuf);
    
    int ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    if (ret < 0) {
        printf("❌ ÉCHEC: ret=%d errno=%d (%s)\n", ret, errno, strerror(errno));
        
        // Analyser errno
        switch (errno) {
            case EINVAL:
                printf("\nEINVAL - Arguments invalides. Causes possibles:\n");
                printf("  - batch_len invalide\n");
                printf("  - flags invalides\n");
                printf("  - relocations mal formées\n");
                printf("  - buffer_count = 0\n");
                printf("  - batch BO non mappable\n");
                break;
            case EFAULT:
                printf("\nEFAULT - Pointeur invalide\n");
                break;
            case ENOENT:
                printf("\nENOENT - Handle BO invalide\n");
                break;
            case ENOMEM:
                printf("\nENOMEM - Mémoire insuffisante\n");
                break;
            default:
                printf("\nErreur inconnue\n");
        }
    } else {
        printf("✓ SUCCÈS: ret=%d\n", ret);
        printf("\nOffsets GTT après exécution:\n");
        printf("  Heap:  0x%016lx\n", objects[0].offset);
        printf("  Batch: 0x%016lx\n", objects[1].offset);
    }
    
    // Cleanup
    struct drm_gem_close close_args = {.handle = batch_handle};
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_args);
    close_args.handle = heap_handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_args);
}

int main() {
    printf("=== TEST C555 - DEBUG EXECBUFFER2 ===\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    dump_execbuffer_structure(fd);
    
    close(fd);
    return 0;
}

// Made with Bob
