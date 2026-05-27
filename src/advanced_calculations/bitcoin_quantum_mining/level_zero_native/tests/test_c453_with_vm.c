/*
 * TEST C453 - RUNNER avec VM (Virtual Memory)
 * Objectif: Implémenter VM comme OpenCL pour résoudre le problème des zéros
 * 
 * Différences vs C433:
 * - Création VM avec DRM_IOCTL_I915_GEM_VM_CREATE
 * - Association VM au contexte avec I915_CONTEXT_PARAM_VM
 * - Utilisation contexte étendu si nécessaire
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

/* i915 Context Parameters */
#define I915_CONTEXT_PARAM_VM 0x9

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <kernel.bin>\n", argv[0]);
        return 1;
    }
    
    FORENSIC_LOG("🚀 TEST C453 - VALIDATION STORE avec VM");
    
    /* 1. Ouvrir DRM */
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM: %s", strerror(errno));
        return 1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", fd);
    
    /* 2. C453: Créer VM AVANT contexte */
    struct drm_i915_gem_vm_control vm_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create) < 0) {
        FORENSIC_LOG("❌ Erreur création VM: %s", strerror(errno));
        close(fd);
        return 1;
    }
    uint32_t vm_id = vm_create.vm_id;
    FORENSIC_LOG("✅ C453: VM créée: id=%u", vm_id);
    
    /* 3. Créer contexte GPU */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        FORENSIC_LOG("❌ Erreur création contexte: %s", strerror(errno));
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte GPU créé: id=%u", ctx_id);
    
    /* 4. C453: Associer VM au contexte */
    struct drm_i915_gem_context_param ctx_param = {
        .ctx_id = ctx_id,
        .param = I915_CONTEXT_PARAM_VM,
        .value = vm_id
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &ctx_param) < 0) {
        FORENSIC_LOG("❌ Erreur association VM→contexte: %s", strerror(errno));
        close(fd);
        return 1;
    }
    FORENSIC_LOG("✅ C453: VM associée au contexte");
    
    /* 5. Charger kernel */
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
    
    /* 6. Créer GEM buffers */
    struct drm_i915_gem_create gem_create;
    
    /* Kernel buffer */
    gem_create.size = kernel_size;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        FORENSIC_LOG("❌ Erreur création kernel buffer");
        free(kernel_data);
        close(fd);
        return 1;
    }
    uint32_t kernel_handle = gem_create.handle;
    FORENSIC_LOG("✅ GEM buffer créé: handle=%u size=%zu", kernel_handle, kernel_size);
    
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
    
    /* Output buffer (1028 bytes = 257 uint32_t) */
    gem_create.size = 1028;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        FORENSIC_LOG("❌ Erreur création output buffer");
        close(fd);
        return 1;
    }
    uint32_t output_handle = gem_create.handle;
    FORENSIC_LOG("✅ GEM buffer créé: handle=%u size=1028", output_handle);
    
    mmap_arg.handle = output_handle;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        FORENSIC_LOG("❌ Erreur mmap output");
        close(fd);
        return 1;
    }
    
    uint32_t *output = mmap(NULL, 1028, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    FORENSIC_LOG("✅ Buffer mappé: %p", output);
    
    /* Initialiser output à 0xDEADBEEF pour détecter si GPU écrit */
    for (int i = 0; i < 257; i++) {
        output[i] = 0xDEADBEEF;
    }
    
    /* CURBE buffer */
    gem_create.size = 4096;
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        FORENSIC_LOG("❌ Erreur création CURBE");
        close(fd);
        return 1;
    }
    uint32_t curbe_handle = gem_create.handle;
    
    mmap_arg.handle = curbe_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *curbe = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    FORENSIC_LOG("✅ Buffer mappé: %p", curbe);
    
    /* Initialiser CURBE */
    memset(curbe, 0, 64);
    curbe[4] = 256;  /* r1.0: global_size[0] */
    curbe[5] = 1;    /* r1.1: global_size[1] */
    curbe[6] = 1;    /* r1.2: global_size[2] */
    /* r2.0-r2.1: output buffer address (sera relocalisé) */
    
    /* DSH buffer (Dynamic State Heap) */
    gem_create.size = 4096;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t dsh_handle = gem_create.handle;
    
    mmap_arg.handle = dsh_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *dsh = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    FORENSIC_LOG("✅ Buffer mappé: %p", dsh);
    
    /* SSH buffer (Surface State Heap) */
    gem_create.size = 4096;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t ssh_handle = gem_create.handle;
    
    /* Batch buffer */
    gem_create.size = 4096;
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    uint32_t batch_handle = gem_create.handle;
    
    mmap_arg.handle = batch_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *batch = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    FORENSIC_LOG("✅ Buffer mappé: %p", batch);
    
    /* Configuration SSH (Surface State Heap) */
    mmap_arg.handle = ssh_handle;
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
    uint32_t *ssh = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    
    FORENSIC_LOG("🔧 C439: Configuration SSH Layout Intel");
    
    /* Binding Table @ offset 0 */
    ssh[0] = 64;  /* BTI[0] pointe vers Surface State @ offset 64 */
    FORENSIC_LOG("   BTI[0] = 64 (offset Surface State)");
    
    /* Surface State @ offset 64 (16 DWORDs = 64 bytes) */
    uint32_t *ss = &ssh[16];  /* offset 64 bytes = 16 DWORDs */
    
    /* DW0: Surface Type + Format + VAlign + Tile Mode */
    uint32_t surface_type = (0 << 29);       /* SURFTYPE_BUFFER */
    uint32_t surface_format = (0x1FF << 10); /* RAW format (9 bits) */
    uint32_t valign = (0 << 16);             /* VALIGN_4 */
    uint32_t tile_mode = 0;                  /* LINEAR */
    ss[0] = surface_type | surface_format | valign | tile_mode;
    
    /* DW1: MOCS */
    ss[1] = CACHE_CTRL;  /* LLC + L3 cached */
    
    /* DW2: Width-1, Height-1 */
    ss[2] = (1028 << 0) | (1 << 16);
    
    /* DW3: Depth-1 */
    ss[3] = 1;
    
    /* DW4-5: Reserved */
    ss[4] = 0;
    ss[5] = 0;
    
    /* DW6: Coherency */
    ss[6] = (1 << 3);  /* GPU coherent */
    
    /* DW7: Surface Pitch */
    ss[7] = 1028 - 1;
    
    /* DW8-9: Base Address (sera relocalisé) */
    ss[8] = 0;
    ss[9] = 0;
    
    /* DW10-15: Reserved */
    for (int i = 10; i < 16; i++) ss[i] = 0;
    
    FORENSIC_LOG("✅ C439: SSH Layout Intel configuré:");
    FORENSIC_LOG("   Binding Table @ offset 0: BTI[0]=64");
    FORENSIC_LOG("   Surface State @ offset 64:");
    FORENSIC_LOG("     Size=1028 → Aligned=1028");
    FORENSIC_LOG("     Width=1028, Height=1, Depth=1");
    FORENSIC_LOG("     DW0=0x%08X (VAlign=4, Format=RAW)", ss[0]);
    FORENSIC_LOG("     DW1=0x%08X (MOCS=CACHED_L3)", ss[1]);
    FORENSIC_LOG("     DW6=0x%08X (Coherency=GPU_COHERENT)", ss[6]);
    
    /* Configuration IDD (Interface Descriptor Data) dans DSH */
    uint32_t *idd = &dsh[16];  /* IDD @ offset 64 */
    
    idd[0] = 0;  /* Kernel Start Pointer (sera relocalisé) */
    idd[1] = 0;
    idd[2] = (1 << 16) | (256 << 0);  /* Barrier Enable | Number of Threads */
    idd[3] = (1 << 16) | (0 << 0);    /* BTI Count | BTI Entry Offset */
    idd[4] = (64 << 16);              /* CURBE Read Length (64 bytes) */
    idd[5] = 0;
    idd[6] = 0;
    idd[7] = 0;
    
    FORENSIC_LOG("✅ Interface Descriptor configuré");
    
    /* Dump IDD */
    FORENSIC_LOG("🔍 C445: IDD Dump:");
    FORENSIC_LOG("   IDD[0] = 0x%08X (Kernel Start Pointer)", idd[0]);
    FORENSIC_LOG("   IDD[2] = 0x%08X (Barrier=1, Threads=256)", idd[2]);
    FORENSIC_LOG("   IDD[3] = 0x%08X (BTI Count=1, BTI Ptr=0)", idd[3]);
    FORENSIC_LOG("   IDD[4] = 0x%08X (CURBE Length=64 bytes)", idd[4]);
    
    /* Dump CURBE */
    FORENSIC_LOG("🔍 C445: CURBE Dump (premiers 16 DWORDs):");
    for (int i = 0; i < 4; i++) {
        FORENSIC_LOG("   [r%d] 0x%08X 0x%08X 0x%08X 0x%08X ", 
            i, curbe[i*4], curbe[i*4+1], curbe[i*4+2], curbe[i*4+3]);
    }
    
    /* Construction batch buffer */
    uint32_t offset = 0;
    
    /* Padding initial */
    for (int i = 0; i < 4; i++) batch[offset++] = 0;
    
    /* PIPELINE_SELECT */
    batch[offset++] = GEN9_PIPELINE_SELECT | (1 << 8);
    batch[offset++] = PIPELINE_SELECT_MEDIA;
    
    /* STATE_BASE_ADDRESS */
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
    
    /* MEDIA_VFE_STATE */
    FORENSIC_LOG("🔧 C443: Programmation MEDIA_VFE_STATE (position correcte)...");
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = (168 << 16) | (64 << 8);
    batch[offset++] = 0;
    batch[offset++] = (2 << 8) | (1 << 0);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    FORENSIC_LOG("✅ C443: MEDIA_VFE_STATE programmé (Max Threads: 168, URB: 64)");
    
    /* MEDIA_CURBE_LOAD */
    batch[offset++] = GEN9_MEDIA_CURBE_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 64;
    batch[offset++] = 0;
    FORENSIC_LOG("✅ C433: MEDIA_CURBE_LOAD ajouté");
    
    /* MEDIA_STATE_FLUSH */
    batch[offset++] = GEN9_MEDIA_STATE_FLUSH | (2 - 2);
    batch[offset++] = 0;
    FORENSIC_LOG("✅ C433: MEDIA_STATE_FLUSH ajouté");
    
    /* MEDIA_INTERFACE_DESCRIPTOR_LOAD */
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;
    batch[offset++] = 64;
    
    /* GPGPU_WALKER */
    batch[offset++] = GEN9_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = (2 << 30);  /* SIMD32 */
    batch[offset++] = 0;
    batch[offset++] = 32;  /* Thread Group ID X Dimension = SIMD32 */
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    batch[offset++] = 8;  /* Thread Group Count X = 8 (256/32) */
    batch[offset++] = 1;
    
    /* PIPE_CONTROL */
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = (1 << 20) | (1 << 18) | (1 << 12) | (1 << 11) | (1 << 10) | (1 << 6) | (1 << 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* MI_BATCH_BUFFER_END */
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    FORENSIC_LOG("✅ Batch buffer construit: %u DWORDs", offset);
    
    /* Dump batch buffer */
    FORENSIC_LOG("🔍 C444: Dump batch buffer (premiers 32 DWORDs):");
    for (uint32_t i = 0; i < (offset < 32 ? offset : 32); i += 4) {
        FORENSIC_LOG("   [%02u-%02u] 0x%08X 0x%08X 0x%08X 0x%08X ", 
            i, i+3, batch[i], batch[i+1], batch[i+2], batch[i+3]);
    }
    
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
    
    FORENSIC_LOG("⚡ Soumission batch buffer...");
    FORENSIC_LOG("🔍 C444 FORENSIC: EXECBUFFER2 params:");
    FORENSIC_LOG("   buffers_ptr = 0x%016lX", execbuf.buffers_ptr);
    FORENSIC_LOG("   buffer_count = %u", execbuf.buffer_count);
    FORENSIC_LOG("   batch_len = %u bytes (%u DWORDs)", execbuf.batch_len, offset);
    FORENSIC_LOG("   flags = 0x%08X", execbuf.flags);
    FORENSIC_LOG("   rsvd1 (ctx_id) = %u", execbuf.rsvd1);
    FORENSIC_LOG("   batch_start_offset = %u", execbuf.batch_start_offset);
    
    FORENSIC_LOG("🔍 C444: exec_objects[5] (batch):");
    FORENSIC_LOG("   handle = %u", exec_objects[5].handle);
    FORENSIC_LOG("   relocation_count = %lu", exec_objects[5].relocation_count);
    FORENSIC_LOG("   relocs_ptr = 0x%016lX", exec_objects[5].relocs_ptr);
    FORENSIC_LOG("   alignment = %lu", exec_objects[5].alignment);
    FORENSIC_LOG("   offset = 0x%016lX", exec_objects[5].offset);
    FORENSIC_LOG("   flags = 0x%016lX", exec_objects[5].flags);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ Erreur EXECBUFFER2: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ Batch buffer soumis avec succès");
    
    /* Dump CURBE post-relocation */
    FORENSIC_LOG("🔍 C447: CURBE POST-RELOCATION (après EXECBUFFER2):");
    for (int i = 0; i < 4; i++) {
        FORENSIC_LOG("   [r%d] 0x%08X 0x%08X 0x%08X 0x%08X ", 
            i, curbe[i*4], curbe[i*4+1], curbe[i*4+2], curbe[i*4+3]);
    }
    uint64_t r8_ptr = ((uint64_t)curbe[9] << 32) | curbe[8];
    FORENSIC_LOG("   r8 (64-bit pointer) = 0x%016lX", r8_ptr);
    
    FORENSIC_LOG("🔍 C447: Output BO GPU address:");
    FORENSIC_LOG("   exec_objects[1].offset = 0x%016lX", exec_objects[1].offset);
    
    if (r8_ptr == exec_objects[1].offset) {
        FORENSIC_LOG("✅ C447: CURBE r8 = output_bo address (relocation OK!)");
        FORENSIC_LOG("   → Problème ailleurs (cache coherency? surface state?)");
    } else {
        FORENSIC_LOG("❌ C447: CURBE r8 ≠ output_bo address!");
        FORENSIC_LOG("   Expected: 0x%016lX", exec_objects[1].offset);
        FORENSIC_LOG("   Got:      0x%016lX", r8_ptr);
    }
    
    /* Dump SSH post-relocation */
    FORENSIC_LOG("🔍 C448: SSH POST-RELOCATION:");
    FORENSIC_LOG("   Binding Table:");
    FORENSIC_LOG("     BTI[0] = %u (offset vers Surface State)", ssh[0]);
    FORENSIC_LOG("   Surface State @ offset 64:");
    for (int i = 0; i < 4; i++) {
        FORENSIC_LOG("     DW%02d-%02d: 0x%08X 0x%08X 0x%08X 0x%08X ", 
            i*4, i*4+3, ss[i*4], ss[i*4+1], ss[i*4+2], ss[i*4+3]);
    }
    
    uint64_t ss_base = ((uint64_t)ss[9] << 32) | ss[8];
    FORENSIC_LOG("   Base Address (DW8-9) = 0x%016lX", ss_base);
    FORENSIC_LOG("   Expected (output_bo) = 0x%016lX", exec_objects[1].offset);
    
    if (ss_base == exec_objects[1].offset) {
        FORENSIC_LOG("✅ C448: Surface State Base Address OK!");
        FORENSIC_LOG("   → Problème ailleurs (format? coherency?)");
    } else {
        FORENSIC_LOG("❌ C448: Surface State Base Address INCORRECT!");
    }
    
    /* Validation */
    FORENSIC_LOG("🔍 VALIDATION RÉSULTATS");
    
    /* Attendre GPU */
    FORENSIC_LOG("⏳ Attente GPU idle...");
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_handle,
        .timeout_ns = -1
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    FORENSIC_LOG("✅ GPU idle confirmé");
    
    /* Activer domaine CPU */
    struct drm_i915_gem_set_domain set_domain = {
        .handle = output_handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = I915_GEM_DOMAIN_CPU
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    FORENSIC_LOG("✅ Domaine CPU activé");
    
    /* Vérifier résultat */
    FORENSIC_LOG("📊 output[0] = 0x%08X (attendu: 0xAABBCCDD)", output[0]);
    
    if (output[0] == 0xAABBCCDD) {
        FORENSIC_LOG("✅✅✅ TEST C453 RÉUSSI! VM a résolu le problème! ✅✅✅");
        FORENSIC_LOG("🎉 GPU écrit correctement dans la mémoire!");
        
        /* Vérifier tous les éléments */
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
        FORENSIC_LOG("❌ TEST C453 ÉCHOUÉ: output[0] = 0x%08X", output[0]);
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
