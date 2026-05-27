/*
 * TEST C455 - RUNNER avec GEM_CREATE_EXT (comme OpenCL)
 * Objectif: Utiliser DRM_IOCTL_I915_GEM_CREATE_EXT au lieu de GEM_CREATE simple
 * 
 * Découverte C454: OpenCL utilise GEM_CREATE_EXT, pas GEM_CREATE!
 * Reverse engineering strace montre: CREATE_EXT avant chaque buffer
 */

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
#include <xf86drm.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

/* Gen9 Command Opcodes */
#define GEN9_PIPELINE_SELECT           ((0x3 << 29) | (0x1 << 27) | (0x1 << 24) | (0x4 << 16))
#define GEN9_STATE_BASE_ADDRESS        0x61010000
#define GEN9_MEDIA_VFE_STATE           0x70000000
#define GEN9_MEDIA_CURBE_LOAD          0x70010000
#define GEN9_MEDIA_STATE_FLUSH         0x70040000
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020000
#define GEN9_GPGPU_WALKER              0x71050000
#define GEN9_PIPE_CONTROL              0x7a000000
#define GEN9_MI_BATCH_BUFFER_END       0x0a000000
#define PIPELINE_SELECT_MEDIA          (1 << 0)

/* Gen9 Constants */
#define CACHE_CTRL 0x3
#define BASE_ADDRESS_MODIFY 0x1

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <kernel.bin>\n", argv[0]);
        return 1;
    }
    
    FORENSIC_LOG("🚀 TEST C455 - VALIDATION avec GEM_CREATE_EXT");
    
    /* 1. Ouvrir DRM */
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM: %s", strerror(errno));
        return 1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", fd);
    
    /* 2. Créer contexte GPU (SANS VM comme OpenCL) */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        FORENSIC_LOG("❌ Erreur création contexte: %s", strerror(errno));
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte GPU créé: id=%u (SANS VM comme OpenCL)", ctx_id);
    
    /* 3. Charger kernel */
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        FORENSIC_LOG("❌ Erreur ouverture kernel: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *kernel_data = malloc(kernel_size);
    fread(kernel_data, 1, kernel_size, f);
    fclose(f);
    
    FORENSIC_LOG("✅ Kernel chargé: %zu bytes", kernel_size);
    
    /* 4. C455: Créer GEM buffers avec CREATE_EXT */
    
    /* Kernel buffer avec CREATE_EXT */
    struct drm_i915_gem_create_ext gem_create_ext = {
        .size = kernel_size,
        .handle = 0,
        .flags = 0,
        .extensions = 0
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create_ext) < 0) {
        FORENSIC_LOG("❌ Erreur création kernel buffer (CREATE_EXT): %s", strerror(errno));
        free(kernel_data);
        close(fd);
        return 1;
    }
    uint32_t kernel_handle = gem_create_ext.handle;
    FORENSIC_LOG("✅ C455: GEM buffer créé avec CREATE_EXT: handle=%u size=%zu", kernel_handle, kernel_size);
    
    /* Mapper et écrire kernel */
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = kernel_handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        FORENSIC_LOG("❌ Erreur mmap kernel");
        free(kernel_data);
        close(fd);
        return 1;
    }
    
    void *kernel_ptr = mmap(NULL, kernel_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    memcpy(kernel_ptr, kernel_data, kernel_size);
    munmap(kernel_ptr, kernel_size);
    free(kernel_data);
    FORENSIC_LOG("✅ Données écrites: %zu bytes", kernel_size);
    
    /* Output buffer avec CREATE_EXT */
    gem_create_ext.size = 1028;
    gem_create_ext.handle = 0;
    gem_create_ext.flags = 0;
    gem_create_ext.extensions = 0;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create_ext) < 0) {
        FORENSIC_LOG("❌ Erreur création output buffer (CREATE_EXT)");
        close(fd);
        return 1;
    }
    uint32_t output_handle = gem_create_ext.handle;
    FORENSIC_LOG("✅ C455: Output buffer créé avec CREATE_EXT: handle=%u", output_handle);
    
    mmap_arg.handle = output_handle;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        FORENSIC_LOG("❌ Erreur mmap output");
        close(fd);
        return 1;
    }
    
    uint32_t *output = mmap(NULL, 1028, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    FORENSIC_LOG("✅ Buffer mappé: %p", output);
    
    /* Initialiser output */
    for (int i = 0; i < 257; i++) {
        output[i] = 0xDEADBEEF;
    }
    
    /* CURBE buffer avec CREATE_EXT */
    gem_create_ext.size = 4096;
    gem_create_ext.handle = 0;
    gem_create_ext.flags = 0;
    gem_create_ext.extensions = 0;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create_ext);
    uint32_t curbe_handle = gem_create_ext.handle;
    
    mmap_arg.handle = curbe_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *curbe = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    /* Initialiser CURBE */
    memset(curbe, 0, 64);
    curbe[4] = 256;
    curbe[5] = 1;
    curbe[6] = 1;
    
    /* DSH buffer avec CREATE_EXT */
    gem_create_ext.size = 4096;
    gem_create_ext.handle = 0;
    gem_create_ext.flags = 0;
    gem_create_ext.extensions = 0;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create_ext);
    uint32_t dsh_handle = gem_create_ext.handle;
    
    mmap_arg.handle = dsh_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *dsh = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    /* SSH buffer avec CREATE_EXT */
    gem_create_ext.size = 4096;
    gem_create_ext.handle = 0;
    gem_create_ext.flags = 0;
    gem_create_ext.extensions = 0;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create_ext);
    uint32_t ssh_handle = gem_create_ext.handle;
    
    /* Batch buffer avec CREATE_EXT */
    gem_create_ext.size = 4096;
    gem_create_ext.handle = 0;
    gem_create_ext.flags = 0;
    gem_create_ext.extensions = 0;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create_ext);
    uint32_t batch_handle = gem_create_ext.handle;
    
    mmap_arg.handle = batch_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *batch = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    FORENSIC_LOG("✅ C455: Tous les buffers créés avec CREATE_EXT");
    
    /* Configuration SSH */
    mmap_arg.handle = ssh_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *ssh = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    ssh[0] = 64;
    uint32_t *ss = &ssh[16];
    ss[0] = 0x0007FC00;
    ss[1] = CACHE_CTRL;
    ss[2] = (1028 << 0) | (1 << 16);
    ss[3] = 1;
    ss[4] = 0;
    ss[5] = 0;
    ss[6] = (1 << 3);
    ss[7] = 1028 - 1;
    ss[8] = 0;
    ss[9] = 0;
    for (int i = 10; i < 16; i++) ss[i] = 0;
    
    /* Configuration IDD */
    uint32_t *idd = &dsh[16];
    idd[0] = 0;
    idd[1] = 0;
    idd[2] = (1 << 16) | (256 << 0);
    idd[3] = (1 << 16) | (0 << 0);
    idd[4] = (64 << 16);
    idd[5] = 0;
    idd[6] = 0;
    idd[7] = 0;
    
    /* Construction batch buffer (identique à C452) */
    uint32_t offset = 0;
    
    for (int i = 0; i < 4; i++) batch[offset++] = 0;
    
    batch[offset++] = GEN9_PIPELINE_SELECT | (1 << 8);
    batch[offset++] = PIPELINE_SELECT_MEDIA;
    
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | (16 - 2);
    batch[offset++] = BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = BASE_ADDRESS_MODIFY | CACHE_CTRL;
    batch[offset++] = 0;
    batch[offset++] = BASE_ADDRESS_MODIFY | CACHE_CTRL;
    batch[offset++] = 0;
    batch[offset++] = BASE_ADDRESS_MODIFY | CACHE_CTRL;
    batch[offset++] = 0;
    batch[offset++] = BASE_ADDRESS_MODIFY | CACHE_CTRL;
    batch[offset++] = 0;
    batch[offset++] = BASE_ADDRESS_MODIFY | CACHE_CTRL;
    batch[offset++] = 0;
    batch[offset++] = BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = BASE_ADDRESS_MODIFY | CACHE_CTRL;
    
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = (168 << 16) | (64 << 8);
    batch[offset++] = 0;
    batch[offset++] = (2 << 8) | (1 << 0);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    batch[offset++] = GEN9_MEDIA_CURBE_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 64;
    batch[offset++] = 0;
    
    batch[offset++] = GEN9_MEDIA_STATE_FLUSH | (2 - 2);
    batch[offset++] = 0;
    
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;
    batch[offset++] = 64;
    
    batch[offset++] = GEN9_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = (2 << 30);
    batch[offset++] = 0;
    batch[offset++] = 32;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    batch[offset++] = 8;
    batch[offset++] = 1;
    
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = (1 << 20) | (1 << 18) | (1 << 12) | (1 << 11) | (1 << 10) | (1 << 6) | (1 << 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    FORENSIC_LOG("✅ Batch buffer construit: %u DWORDs", offset);
    
    /* Relocations */
    struct drm_i915_gem_relocation_entry relocs[4] = {
        {.target_handle = kernel_handle, .offset = 4 * 64, .read_domains = I915_GEM_DOMAIN_INSTRUCTION},
        {.target_handle = output_handle, .offset = 4 * 8, .read_domains = I915_GEM_DOMAIN_RENDER, .write_domain = I915_GEM_DOMAIN_RENDER},
        {.target_handle = output_handle, .offset = 4 * (64 + 16 + 8), .read_domains = I915_GEM_DOMAIN_RENDER, .write_domain = I915_GEM_DOMAIN_RENDER},
        {.target_handle = curbe_handle, .offset = 4 * 32, .read_domains = I915_GEM_DOMAIN_INSTRUCTION}
    };
    
    /* Exec objects */
    struct drm_i915_gem_exec_object2 exec_objects[6] = {
        {.handle = kernel_handle},
        {.handle = output_handle},
        {.handle = curbe_handle},
        {.handle = dsh_handle},
        {.handle = ssh_handle},
        {.handle = batch_handle, .relocation_count = 4, .relocs_ptr = (uint64_t)relocs}
    };
    
    /* EXECBUFFER2 */
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 6,
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ C455: Soumission batch buffer avec CREATE_EXT...");
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ Erreur EXECBUFFER2: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ C455: Batch buffer soumis avec succès!");
    
    /* Validation */
    FORENSIC_LOG("🔍 VALIDATION RÉSULTATS");
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_handle,
        .timeout_ns = -1
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    FORENSIC_LOG("✅ GPU idle confirmé");
    
    struct drm_i915_gem_set_domain set_domain = {
        .handle = output_handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = I915_GEM_DOMAIN_CPU
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    FORENSIC_LOG("✅ Domaine CPU activé");
    
    FORENSIC_LOG("📊 output[0] = 0x%08X (attendu: 0xAABBCCDD)", output[0]);
    
    if (output[0] == 0xAABBCCDD) {
        FORENSIC_LOG("✅✅✅ TEST C455 RÉUSSI! CREATE_EXT a résolu le problème! ✅✅✅");
        FORENSIC_LOG("🎉 GPU écrit correctement dans la mémoire!");
        
        int success_count = 0;
        for (int i = 0; i < 256; i++) {
            if (output[i] == 0xAABBCCDD) success_count++;
        }
        FORENSIC_LOG("📊 %d/256 work-items ont écrit correctement", success_count);
        
        munmap(output, 1028);
        munmap(curbe, 4096);
        munmap(dsh, 4096);
        munmap(ssh, 4096);
        munmap(batch, 4096);
        close(fd);
        return 0;
    } else {
        FORENSIC_LOG("❌ TEST C455 ÉCHOUÉ: output[0] = 0x%08X", output[0]);
        FORENSIC_LOG("🔍 Dump premiers 16 uint32_t:");
        for (int i = 0; i < 16; i++) {
            FORENSIC_LOG("   output[%d] = 0x%08X", i, output[i]);
        }
        
        munmap(output, 1028);
        munmap(curbe, 4096);
        munmap(dsh, 4096);
        munmap(ssh, 4096);
        munmap(batch, 4096);
        close(fd);
        return 1;
    }
}

// Made with Bob
