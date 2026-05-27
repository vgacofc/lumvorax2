/*
 * C197.34 - Exécution GPU via EXECBUFFER2
 * 
 * OBJECTIF: Soumettre batch buffer au GPU et valider exécution
 * MÉTHODE: Utiliser structures de C197.33 + EXECBUFFER2
 * 
 * Architecture:
 *   Structures Gen9 (C197.33) → EXECBUFFER2 → GPU → Résultats
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

// DRM/i915 headers
#include <libdrm/i915_drm.h>
#include <libdrm/drm.h>

// Charger ISA
#include "/tmp/gen9_kernel_write_test.h"

// Structures Gen9
typedef struct {
    uint32_t kernel_start_pointer;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t sampler_state_pointer;
    uint32_t binding_table_pointer;
    uint32_t constant_urb_entry;
    uint32_t num_threads;
    uint32_t slm_size;
} __attribute__((packed)) gen9_interface_descriptor_t;

typedef struct {
    uint32_t surface_type_format;
    uint32_t base_address_low;
    uint32_t width_height;
    uint32_t depth_pitch;
    uint32_t min_lod_mip_count;
    uint32_t x_y_offset;
    uint32_t reserved;
    uint32_t base_address_high;
} __attribute__((packed)) gen9_surface_state_t;

#define MI_BATCH_BUFFER_END 0x05000000

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("=== C197.34 - Exécution GPU via EXECBUFFER2 ===\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Étape 1: Ouvrir DRM
    printf("[1] Ouverture DRM device...\n");
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        printf("❌ Erreur open: %s\n", strerror(errno));
        return 1;
    }
    printf("✅ DRM ouvert: fd=%d\n", drm_fd);
    
    // Étape 2: Créer VM
    printf("\n[2] Création VM...\n");
    struct drm_i915_gem_vm_control vm_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create) < 0) {
        printf("❌ Erreur VM_CREATE: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ VM créée: id=%u\n", vm_create.vm_id);
    
    // Étape 3: Créer context
    printf("\n[3] Création context...\n");
    struct drm_i915_gem_context_create_ext ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create) < 0) {
        printf("❌ Erreur CONTEXT_CREATE: %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Context créé: id=%u\n", ctx_create.ctx_id);
    
    // Étape 4: Allouer buffers
    printf("\n[4] Allocation buffers...\n");
    
    size_t isa_size = 4096;
    struct drm_i915_gem_create gem_isa = { .size = isa_size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_isa) < 0) {
        printf("❌ Erreur GEM_CREATE (ISA): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    size_t interface_desc_size = 4096;
    struct drm_i915_gem_create gem_interface_desc = { .size = interface_desc_size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_interface_desc) < 0) {
        printf("❌ Erreur GEM_CREATE (interface_desc): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    size_t binding_table_size = 4096;
    struct drm_i915_gem_create gem_binding_table = { .size = binding_table_size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_binding_table) < 0) {
        printf("❌ Erreur GEM_CREATE (binding_table): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    size_t surface_state_size = 4096;
    struct drm_i915_gem_create gem_surface_state = { .size = surface_state_size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_surface_state) < 0) {
        printf("❌ Erreur GEM_CREATE (surface_state): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    size_t output_size = 4096;
    struct drm_i915_gem_create gem_output = { .size = output_size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_output) < 0) {
        printf("❌ Erreur GEM_CREATE (output): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    size_t batch_size = 4096;
    struct drm_i915_gem_create gem_batch = { .size = batch_size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_batch) < 0) {
        printf("❌ Erreur GEM_CREATE (batch): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    printf("✅ 6 buffers créés\n");
    
    // Étape 5: Mapper buffers
    printf("\n[5] Mapping buffers...\n");
    
    struct drm_i915_gem_mmap_offset mmap_isa = {
        .handle = gem_isa.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_isa);
    void* isa_ptr = mmap(NULL, isa_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_isa.offset);
    
    struct drm_i915_gem_mmap_offset mmap_interface_desc = {
        .handle = gem_interface_desc.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_interface_desc);
    void* interface_desc_ptr = mmap(NULL, interface_desc_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_interface_desc.offset);
    
    struct drm_i915_gem_mmap_offset mmap_binding_table = {
        .handle = gem_binding_table.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_binding_table);
    void* binding_table_ptr = mmap(NULL, binding_table_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_binding_table.offset);
    
    struct drm_i915_gem_mmap_offset mmap_surface_state = {
        .handle = gem_surface_state.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_surface_state);
    void* surface_state_ptr = mmap(NULL, surface_state_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_surface_state.offset);
    
    struct drm_i915_gem_mmap_offset mmap_output = {
        .handle = gem_output.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_output);
    void* output_ptr = mmap(NULL, output_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_output.offset);
    
    struct drm_i915_gem_mmap_offset mmap_batch = {
        .handle = gem_batch.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_batch);
    void* batch_ptr = mmap(NULL, batch_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_batch.offset);
    
    printf("✅ 6 buffers mappés\n");
    
    // Étape 6: Préparer données
    printf("\n[6] Préparation données...\n");
    
    // Copier ISA
    memcpy(isa_ptr, gen9_kernel_write_test_isa, gen9_kernel_write_test_isa_size);
    
    // Interface descriptor
    gen9_interface_descriptor_t* interface_desc = (gen9_interface_descriptor_t*)interface_desc_ptr;
    memset(interface_desc, 0, sizeof(gen9_interface_descriptor_t));
    interface_desc->kernel_start_pointer = 0;
    interface_desc->binding_table_pointer = 0;
    interface_desc->num_threads = 1;
    
    // Binding table
    uint32_t* binding_table = (uint32_t*)binding_table_ptr;
    binding_table[0] = 0;
    
    // Surface state
    gen9_surface_state_t* surface_state = (gen9_surface_state_t*)surface_state_ptr;
    memset(surface_state, 0, sizeof(gen9_surface_state_t));
    surface_state->surface_type_format = 0x00000000;
    surface_state->width_height = (output_size - 1);
    
    // Initialiser output avec pattern
    uint32_t* output = (uint32_t*)output_ptr;
    for (size_t i = 0; i < output_size / 4; i++) {
        output[i] = 0xDEADBEEF;
    }
    
    // Batch buffer
    uint32_t* batch = (uint32_t*)batch_ptr;
    int batch_idx = 0;
    
    // PIPE_CONTROL
    batch[batch_idx++] = 0x7A000004;
    batch[batch_idx++] = 0x00100000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[batch_idx++] = 0x70020002;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000020;
    batch[batch_idx++] = 0x00000000;
    
    // GPGPU_WALKER
    batch[batch_idx++] = 0x75020008;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000001;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000001;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000001;
    batch[batch_idx++] = 0xFFFFFFFF;
    batch[batch_idx++] = 0xFFFFFFFF;
    
    // PIPE_CONTROL final
    batch[batch_idx++] = 0x7A000004;
    batch[batch_idx++] = 0x00100000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    batch[batch_idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch[batch_idx++] = MI_BATCH_BUFFER_END;
    
    printf("✅ Données préparées (batch: %d DWORDs)\n", batch_idx);
    
    // Étape 7: Préparer EXECBUFFER2
    printf("\n[7] Préparation EXECBUFFER2...\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[6] = {
        { .handle = gem_isa.handle },
        { .handle = gem_interface_desc.handle },
        { .handle = gem_binding_table.handle },
        { .handle = gem_surface_state.handle },
        { .handle = gem_output.handle, .flags = EXEC_OBJECT_WRITE },
        { .handle = gem_batch.handle },
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 6,
        .batch_start_offset = 0,
        .batch_len = batch_idx * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
    };
    
    printf("✅ EXECBUFFER2 préparé\n");
    printf("   Buffers: %u\n", execbuf.buffer_count);
    printf("   Batch length: %u bytes\n", execbuf.batch_len);
    printf("   Context: %u\n", execbuf.rsvd1);
    
    // Étape 8: Soumettre au GPU
    printf("\n[8] Soumission GPU...\n");
    
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret < 0) {
        printf("❌ Erreur EXECBUFFER2: %s (errno=%d)\n", strerror(errno), errno);
        printf("   Détails:\n");
        printf("   - batch_len: %u\n", execbuf.batch_len);
        printf("   - buffer_count: %u\n", execbuf.buffer_count);
        printf("   - flags: 0x%llx\n", execbuf.flags);
        printf("   - ctx_id: %u\n", execbuf.rsvd1);
        
        // Cleanup
        munmap(isa_ptr, isa_size);
        munmap(interface_desc_ptr, interface_desc_size);
        munmap(binding_table_ptr, binding_table_size);
        munmap(surface_state_ptr, surface_state_size);
        munmap(output_ptr, output_size);
        munmap(batch_ptr, batch_size);
        close(drm_fd);
        return 1;
    }
    
    printf("✅ Batch soumis au GPU !\n");
    
    // Étape 9: Attendre completion
    printf("\n[9] Attente completion GPU...\n");
    
    struct drm_i915_gem_wait gem_wait = {
        .bo_handle = gem_batch.handle,
        .timeout_ns = 1000000000, // 1 seconde
    };
    
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &gem_wait);
    if (ret < 0) {
        printf("⚠️  Erreur GEM_WAIT: %s\n", strerror(errno));
    } else {
        printf("✅ GPU terminé\n");
    }
    
    // Étape 10: Lire résultats
    printf("\n[10] Lecture résultats...\n");
    
    printf("Output buffer (premiers 16 DWORDs):\n");
    for (int i = 0; i < 16; i++) {
        printf("  [%2d] 0x%08x", i, output[i]);
        if (output[i] != 0xDEADBEEF) {
            printf(" ← MODIFIÉ !");
        }
        printf("\n");
    }
    
    // Vérifier si valeur magique présente
    int magic_found = 0;
    for (size_t i = 0; i < output_size / 4; i++) {
        if (output[i] == 0x12345678) {
            printf("\n✅ Valeur magique 0x12345678 trouvée à offset %zu !\n", i * 4);
            magic_found = 1;
            break;
        }
    }
    
    if (!magic_found) {
        printf("\n⚠️  Valeur magique 0x12345678 non trouvée\n");
    }
    
    // Timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // Cleanup
    printf("\n[11] Cleanup...\n");
    munmap(isa_ptr, isa_size);
    munmap(interface_desc_ptr, interface_desc_size);
    munmap(binding_table_ptr, binding_table_size);
    munmap(surface_state_ptr, surface_state_size);
    munmap(output_ptr, output_size);
    munmap(batch_ptr, batch_size);
    close(drm_fd);
    
    printf("\n=== RÉSULTAT C197.34 ===\n");
    if (ret == 0) {
        printf("✅ EXECBUFFER2 soumis avec succès\n");
        printf("✅ GPU a exécuté le batch buffer\n");
        if (magic_found) {
            printf("✅ Résultats validés (valeur magique trouvée)\n");
        } else {
            printf("⚠️  Résultats à valider (pas de valeur magique)\n");
        }
    } else {
        printf("❌ EXECBUFFER2 a échoué\n");
    }
    printf("⏱️  Temps total: %.3f ms\n", elapsed * 1000);
    printf("\n🎯 Prochaine étape: C197.35 - Validation complète + intégration Bitcoin\n");
    
    return (ret == 0) ? 0 : 1;
}

// Made with Bob
