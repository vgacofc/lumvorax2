/*
 * C197.30 - Test Exécution GPU Complet avec ISA Réel
 * 
 * OBJECTIF: Intégrer ISA Gen9 réel extrait + tester exécution GPU complète
 * 
 * PIPELINE COMPLET:
 * 1. Charger ISA Gen9 réel (4448 bytes depuis C197.29)
 * 2. Créer toutes structures Gen9 (interface descriptor, binding table, surface state)
 * 3. Construire batch buffer complet
 * 4. Créer context + VM (code C197.21)
 * 5. Soumettre via execbuffer2
 * 6. Valider output GPU (0x12345678)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>

#include <drm/i915_drm.h>
#include <xf86drm.h>

// Charger ISA depuis fichier généré par C197.29
#include "/tmp/gen9_kernel_write_test.h"

// ============================================================================
// GEN9 STRUCTURES
// ============================================================================

typedef struct {
    uint32_t kernel_start_pointer;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t sampler_state_pointer;
    uint32_t binding_table_pointer;
    uint32_t constant_urb_entry;
    uint32_t num_threads;
    uint32_t slm_size;
} gen9_interface_descriptor_t;

typedef struct {
    uint32_t surface_type_format;
    uint32_t base_address_low;
    uint32_t width_height;
    uint32_t depth_pitch;
    uint32_t min_lod_mip_count;
    uint32_t x_y_offset;
    uint32_t reserved;
    uint32_t base_address_high;
} gen9_surface_state_t;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

int create_gem_buffer(int fd, uint64_t size, uint32_t* handle) {
    struct drm_i915_gem_create create = { .size = size };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) != 0) {
        perror("GEM_CREATE");
        return -1;
    }
    *handle = create.handle;
    return 0;
}

void* map_gem_buffer(int fd, uint32_t handle, uint64_t size) {
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) != 0) {
        perror("GEM_MMAP_OFFSET");
        return NULL;
    }
    
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    
    return ptr;
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

int main() {
    printf("=== C197.30 - Test Exécution GPU Complet ===\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 1. Ouvrir DRM
    printf("[1/12] Ouverture DRM device...\n");
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        perror("open");
        return 1;
    }
    printf("✅ DRM ouvert (fd=%d)\n", drm_fd);
    
    // 2. Créer VM
    printf("\n[2/12] Création VM...\n");
    struct drm_i915_gem_vm_control vm_create = { .flags = 0 };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create) != 0) {
        perror("VM_CREATE");
        close(drm_fd);
        return 1;
    }
    printf("✅ VM créée (vm_id=%u)\n", vm_create.vm_id);
    
    // 3. Créer context
    printf("\n[3/12] Création context...\n");
    struct drm_i915_gem_context_create_ext ctx_create = {
        .flags = I915_CONTEXT_CREATE_FLAGS_SINGLE_TIMELINE
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create) != 0) {
        perror("CONTEXT_CREATE_EXT");
        close(drm_fd);
        return 1;
    }
    printf("✅ Context créé (ctx_id=%u)\n", ctx_create.ctx_id);
    
    // 4. Créer buffers
    printf("\n[4/12] Création buffers GEM...\n");
    uint32_t kernel_handle, interface_desc_handle, binding_table_handle;
    uint32_t surface_state_handle, output_handle, batch_handle;
    
    if (create_gem_buffer(drm_fd, 8192, &kernel_handle) != 0) return 1;
    if (create_gem_buffer(drm_fd, 4096, &interface_desc_handle) != 0) return 1;
    if (create_gem_buffer(drm_fd, 4096, &binding_table_handle) != 0) return 1;
    if (create_gem_buffer(drm_fd, 4096, &surface_state_handle) != 0) return 1;
    if (create_gem_buffer(drm_fd, 4096, &output_handle) != 0) return 1;
    if (create_gem_buffer(drm_fd, 4096, &batch_handle) != 0) return 1;
    
    printf("✅ 6 buffers créés\n");
    
    // 5. Mapper buffers
    printf("\n[5/12] Mapping buffers...\n");
    uint8_t* kernel_ptr = map_gem_buffer(drm_fd, kernel_handle, 8192);
    gen9_interface_descriptor_t* interface_desc = map_gem_buffer(drm_fd, interface_desc_handle, 4096);
    uint32_t* binding_table = map_gem_buffer(drm_fd, binding_table_handle, 4096);
    gen9_surface_state_t* surface_state = map_gem_buffer(drm_fd, surface_state_handle, 4096);
    uint32_t* output = map_gem_buffer(drm_fd, output_handle, 4096);
    uint32_t* batch = map_gem_buffer(drm_fd, batch_handle, 4096);
    
    if (!kernel_ptr || !interface_desc || !binding_table || !surface_state || !output || !batch) {
        printf("❌ Mapping failed\n");
        return 1;
    }
    printf("✅ Buffers mappés\n");
    
    // 6. Copier ISA réel
    printf("\n[6/12] Copie ISA Gen9 réel...\n");
    size_t isa_size = sizeof(gen9_kernel_write_test);
    memcpy(kernel_ptr, gen9_kernel_write_test, isa_size);
    printf("✅ ISA copié (%zu bytes)\n", isa_size);
    
    // 7. Remplir structures
    printf("\n[7/12] Remplissage structures Gen9...\n");
    
    // Interface descriptor
    memset(interface_desc, 0, sizeof(gen9_interface_descriptor_t));
    interface_desc->kernel_start_pointer = 0;
    interface_desc->binding_table_pointer = 0;
    interface_desc->num_threads = 1;
    
    // Binding table
    binding_table[0] = 0;
    
    // Surface state
    memset(surface_state, 0, sizeof(gen9_surface_state_t));
    surface_state->surface_type_format = 0x00000000;
    surface_state->width_height = 0x00001000;
    
    // Output buffer
    output[0] = 0xDEADBEEF;
    
    printf("✅ Structures remplies\n");
    printf("   Output initial: 0x%08X\n", output[0]);
    
    // 8. Construire batch buffer
    printf("\n[8/12] Construction batch buffer...\n");
    int idx = 0;
    
    // PIPE_CONTROL
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00100000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = 0x70020002;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000020;
    batch[idx++] = 0x00000000;
    
    // GPGPU_WALKER
    batch[idx++] = 0x75020008;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;
    batch[idx++] = 0xFFFFFFFF;
    batch[idx++] = 0xFFFFFFFF;
    
    // PIPE_CONTROL final
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00100000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x05000000;
    
    printf("✅ Batch buffer construit (%d DWORDs)\n", idx);
    
    // 9. Préparer exec_objects
    printf("\n[9/12] Préparation exec_objects...\n");
    struct drm_i915_gem_exec_object2 exec_objects[6] = {
        { .handle = kernel_handle },
        { .handle = interface_desc_handle },
        { .handle = binding_table_handle },
        { .handle = surface_state_handle },
        { .handle = output_handle },
        { .handle = batch_handle, .flags = EXEC_OBJECT_WRITE }
    };
    printf("✅ 6 exec_objects préparés\n");
    
    // 10. Préparer execbuffer2
    printf("\n[10/12] Préparation execbuffer2...\n");
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 6,
        .batch_len = idx * 4,
        .rsvd1 = ctx_create.ctx_id,
        .flags = I915_EXEC_RENDER
    };
    printf("✅ execbuffer2 préparé\n");
    
    // 11. Soumettre batch buffer
    printf("\n[11/12] Soumission batch buffer...\n");
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        perror("EXECBUFFER2");
        printf("❌ Soumission échouée\n");
        printf("⚠️  Cause probable: Structures Gen9 incomplètes ou ISA invalide\n");
    } else {
        printf("✅ Batch buffer soumis avec succès!\n");
    }
    
    // 12. Vérifier output
    printf("\n[12/12] Vérification output GPU...\n");
    printf("Output après GPU: 0x%08X\n", output[0]);
    
    if (output[0] == 0x12345678) {
        printf("✅✅✅ SUCCÈS! GPU a écrit 0x12345678!\n");
    } else if (output[0] == 0xDEADBEEF) {
        printf("❌ ÉCHEC: Output inchangé (GPU n'a pas exécuté)\n");
    } else {
        printf("⚠️  Output modifié mais valeur inattendue\n");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("\n=== RÉSULTATS ===\n");
    printf("ISA size: %zu bytes\n", isa_size);
    printf("Batch buffer: %d DWORDs\n", idx);
    printf("Output: 0x%08X\n", output[0]);
    printf("Temps total: %.3f ms\n", elapsed * 1000);
    
    printf("\n=== PROCHAINES ÉTAPES ===\n");
    if (output[0] == 0x12345678) {
        printf("C197.31: Extraire ISA SHA256 pour Bitcoin mining\n");
        printf("C197.32: Intégrer SHA256 dans pipeline\n");
        printf("C197.33: Test mining complet\n");
    } else {
        printf("DEBUG: Analyser pourquoi GPU n'a pas exécuté\n");
        printf("- Vérifier structures Gen9\n");
        printf("- Vérifier ISA format\n");
        printf("- Vérifier batch buffer\n");
    }
    
    // Cleanup
    munmap(kernel_ptr, 8192);
    munmap(interface_desc, 4096);
    munmap(binding_table, 4096);
    munmap(surface_state, 4096);
    munmap(output, 4096);
    munmap(batch, 4096);
    
    close(drm_fd);
    
    return (output[0] == 0x12345678) ? 0 : 1;
}

// Made with Bob
