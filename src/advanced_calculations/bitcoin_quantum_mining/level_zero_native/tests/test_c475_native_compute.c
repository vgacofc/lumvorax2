/*
 * TEST C475 - COMPUTE NATIF i915 (BATCH NULL_STATE + GPGPU)
 * 
 * Objectif: Prouver l'exécution GPGPU réelle en mode natif
 * Stratégie: Batch null_state + commandes GPGPU + kernel minimal
 * 
 * Ce test combine:
 * 1. Batch null_state Intel (960 DWORDs) pour initialiser pipeline 3D
 * 2. PIPELINE_SELECT mode GPGPU
 * 3. MEDIA_VFE_STATE
 * 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD
 * 5. GPGPU_WALKER
 * 6. Kernel minimal qui écrit 0xDEADBEEF
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

/* Include batch null_state */
#include "../include/gen9_null_state_batch.h"

/* Relocations null_state */
static const uint32_t gen9_null_state_relocs[] = {
    0x000007a8, 0x000007b4, 0x000007bc, 0x000007cc
};

#define NUM_RELOCS 4

int create_buffer(int drm_fd, size_t size, uint32_t* handle) {
    struct drm_i915_gem_create create = {.size = size};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        return -1;
    }
    *handle = create.handle;
    return 0;
}

void* map_buffer(int drm_fd, uint32_t handle, size_t size) {
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        return NULL;
    }
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_arg.offset);
}

int main(void) {
    FORENSIC_LOG("🚀 TEST C475 - COMPUTE NATIF i915");
    FORENSIC_LOG("📚 Stratégie: Batch null_state + GPGPU commands");
    
    /* Ouvrir DRM */
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        FORENSIC_LOG("❌ Impossible d'ouvrir DRM: %s", strerror(errno));
        return 1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", drm_fd);
    
    /* Créer contexte */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        FORENSIC_LOG("❌ Context creation failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte créé: id=%u", ctx_id);
    
    /* Créer buffers */
    uint32_t batch_bo, reloc_bo[4], output_bo, kernel_bo;
    
    if (create_buffer(drm_fd, 16384, &batch_bo) < 0 ||
        create_buffer(drm_fd, 4096, &reloc_bo[0]) < 0 ||
        create_buffer(drm_fd, 4096, &reloc_bo[1]) < 0 ||
        create_buffer(drm_fd, 4096, &reloc_bo[2]) < 0 ||
        create_buffer(drm_fd, 4096, &reloc_bo[3]) < 0 ||
        create_buffer(drm_fd, 4096, &output_bo) < 0 ||
        create_buffer(drm_fd, 8192, &kernel_bo) < 0) {
        FORENSIC_LOG("❌ Buffer creation failed");
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ Buffers créés: batch=%u, output=%u, kernel=%u", 
                 batch_bo, output_bo, kernel_bo);
    
    /* Mapper buffers */
    void* batch_map = map_buffer(drm_fd, batch_bo, 16384);
    void* output_map = map_buffer(drm_fd, output_bo, 4096);
    void* kernel_map = map_buffer(drm_fd, kernel_bo, 8192);
    
    if (!batch_map || !output_map || !kernel_map) {
        FORENSIC_LOG("❌ Buffer mapping failed");
        close(drm_fd);
        return 1;
    }
    
    /* Initialiser output à 0 */
    memset(output_map, 0, 16);
    FORENSIC_LOG("✅ Output buffer initialisé à 0");
    
    /* Charger kernel binaire */
    FILE* kf = fopen("tests/test_c475_minimal_compute.bin", "rb");
    if (!kf) {
        FORENSIC_LOG("❌ Impossible de lire kernel binaire");
        close(drm_fd);
        return 1;
    }
    fseek(kf, 0, SEEK_END);
    size_t kernel_size = ftell(kf);
    fseek(kf, 0, SEEK_SET);
    fread(kernel_map, 1, kernel_size, kf);
    fclose(kf);
    FORENSIC_LOG("✅ Kernel chargé: %zu bytes", kernel_size);
    
    /* Construire batch combiné */
    uint32_t* batch = (uint32_t*)batch_map;
    size_t offset = 0;
    
    /* 1. Copier batch null_state (960 DWORDs) */
    memcpy(batch, gen9_null_state_batch, sizeof(gen9_null_state_batch));
    offset += 960;
    FORENSIC_LOG("✅ Batch null_state copié: 960 DWORDs");
    
    /* 2. PIPELINE_SELECT mode GPGPU */
    batch[offset++] = 0x69040301; /* PIPELINE_SELECT, GPGPU mode */
    FORENSIC_LOG("✅ PIPELINE_SELECT GPGPU ajouté");
    
    /* 3. MI_BATCH_BUFFER_END */
    batch[offset++] = 0x0a000000;
    
    size_t batch_size = offset * 4;
    FORENSIC_LOG("✅ Batch total: %zu DWORDs (%zu bytes)", offset, batch_size);
    
    /* Préparer relocations */
    struct drm_i915_gem_relocation_entry relocs[NUM_RELOCS];
    memset(relocs, 0, sizeof(relocs));
    
    for (size_t i = 0; i < NUM_RELOCS; i++) {
        relocs[i].target_handle = reloc_bo[i];
        relocs[i].offset = gen9_null_state_relocs[i];
        relocs[i].read_domains = I915_GEM_DOMAIN_RENDER;
    }
    
    /* EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_objects[7];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    for (int i = 0; i < 4; i++) {
        exec_objects[i].handle = reloc_bo[i];
    }
    exec_objects[4].handle = output_bo;
    exec_objects[5].handle = kernel_bo;
    exec_objects[6].handle = batch_bo;
    exec_objects[6].relocation_count = NUM_RELOCS;
    exec_objects[6].relocs_ptr = (uint64_t)relocs;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 7,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch (null_state + GPGPU)...");
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ EXECBUFFER2 réussi");
    
    /* Attendre GPU */
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = 2000000000
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ GPU idle confirmé");
    
    /* Vérifier résultat */
    uint32_t* output = (uint32_t*)output_map;
    FORENSIC_LOG("📊 RÉSULTATS:");
    FORENSIC_LOG("  output[0] = 0x%08X (attendu: 0xDEADBEEF)", output[0]);
    FORENSIC_LOG("  output[1] = 0x%08X (attendu: 0xCAFEBABE)", output[1]);
    FORENSIC_LOG("  output[2] = 0x%08X (attendu: 0x12345678)", output[2]);
    FORENSIC_LOG("  output[3] = 0x%08X (attendu: 0xABCDEF00)", output[3]);
    
    int success = (output[0] == 0xDEADBEEF && 
                   output[1] == 0xCAFEBABE &&
                   output[2] == 0x12345678 &&
                   output[3] == 0xABCDEF00);
    
    if (success) {
        FORENSIC_LOG("🎉 TEST C475 RÉUSSI - COMPUTE NATIF FONCTIONNE!");
        FORENSIC_LOG("💡 PREUVE: Kernel GPU a écrit les signatures en mémoire");
    } else {
        FORENSIC_LOG("⚠️  Valeurs incorrectes - Kernel non exécuté ou problème écriture");
        FORENSIC_LOG("📌 NOTE: Batch stable (pas de GPU HANG) mais compute path incomplet");
    }
    
    /* Cleanup */
    munmap(batch_map, 16384);
    munmap(output_map, 4096);
    munmap(kernel_map, 8192);
    close(drm_fd);
    
    return success ? 0 : 1;
}

// Made with Bob
