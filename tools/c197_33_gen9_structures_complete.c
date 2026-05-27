/*
 * C197.33 - Construction Structures Gen9 Complètes
 * 
 * OBJECTIF: Créer toutes les structures Gen9 nécessaires pour exécuter ISA
 * MÉTHODE: Construire manuellement interface descriptor, binding table, surface state, batch buffer
 * 
 * Architecture:
 *   ISA Gen9 (4448 bytes de C197.29) → Nos structures Gen9 → Pipeline DRM → GPU
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

// DRM/i915 headers
#include <libdrm/i915_drm.h>
#include <libdrm/drm.h>

// Charger ISA depuis C197.29
#include "/tmp/gen9_kernel_write_test.h"

// Structures Gen9 (documentées dans C197.27)

// 1. PIPE_CONTROL (6 DWORDs)
typedef struct {
    uint32_t dw0;           // 0x7A000004
    uint32_t dw1;           // Control flags
    uint32_t address_low;   // Target address [31:0]
    uint32_t address_high;  // Target address [63:32]
    uint32_t data_low;      // Immediate data
    uint32_t data_high;
} __attribute__((packed)) gen9_pipe_control_t;

// 2. STATE_BASE_ADDRESS (19 DWORDs)
typedef struct {
    uint32_t dw0;           // 0x61010011 (command + length)
    uint32_t dw1;           // General state base address
    uint32_t dw2;           // General state base address upper
    uint32_t dw3;           // Stateless data port access
    uint32_t dw4;           // Surface state base address
    uint32_t dw5;           // Surface state base address upper
    uint32_t dw6;           // Dynamic state base address
    uint32_t dw7;           // Dynamic state base address upper
    uint32_t dw8;           // Indirect object base address
    uint32_t dw9;           // Indirect object base address upper
    uint32_t dw10;          // Instruction base address
    uint32_t dw11;          // Instruction base address upper
    uint32_t dw12;          // General state buffer size
    uint32_t dw13;          // Dynamic state buffer size
    uint32_t dw14;          // Indirect object buffer size
    uint32_t dw15;          // Instruction buffer size
    uint32_t dw16;          // Bindless surface state base address
    uint32_t dw17;          // Bindless surface state base address upper
    uint32_t dw18;          // Bindless surface state size
} __attribute__((packed)) gen9_state_base_address_t;

// 3. MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWORDs)
typedef struct {
    uint32_t dw0;           // 0x70020002
    uint32_t dw1;           // Reserved
    uint32_t dw2;           // Length (32 bytes)
    uint32_t dw3;           // Offset to interface descriptor
} __attribute__((packed)) gen9_media_interface_descriptor_load_t;

// 4. INTERFACE_DESCRIPTOR (8 DWORDs)
typedef struct {
    uint32_t kernel_start_pointer;      // [31:6] ISA offset
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t sampler_state_pointer;     // [31:5]
    uint32_t binding_table_pointer;     // [31:5]
    uint32_t constant_urb_entry;
    uint32_t num_threads;
    uint32_t slm_size;
} __attribute__((packed)) gen9_interface_descriptor_t;

// 5. SURFACE_STATE (8 DWORDs)
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

// 6. GPGPU_WALKER (12 DWORDs)
typedef struct {
    uint32_t dw0;           // 0x75020008
    uint32_t dw1;           // Interface descriptor offset
    uint32_t dw2;           // Indirect data length
    uint32_t dw3;           // Indirect data start
    uint32_t dw4;           // Thread group ID X start
    uint32_t dw5;           // Thread group ID X dimension
    uint32_t dw6;           // Thread group ID Y start
    uint32_t dw7;           // Thread group ID Y dimension
    uint32_t dw8;           // Thread group ID Z start
    uint32_t dw9;           // Thread group ID Z dimension
    uint32_t dw10;          // Right execution mask
    uint32_t dw11;          // Bottom execution mask
} __attribute__((packed)) gen9_gpgpu_walker_t;

// 7. MI_BATCH_BUFFER_END (1 DWORD)
#define MI_BATCH_BUFFER_END 0x05000000

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("=== C197.33 - Construction Structures Gen9 Complètes ===\n\n");
    
    // Étape 1: Ouvrir DRM device
    printf("[1] Ouverture DRM device...\n");
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        printf("❌ Erreur open /dev/dri/renderD128: %s\n", strerror(errno));
        return 1;
    }
    printf("✅ DRM device ouvert: fd=%d\n", drm_fd);
    
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
    
    // Étape 4: Allouer buffers GEM
    printf("\n[4] Allocation buffers GEM...\n");
    
    // Buffer 1: ISA kernel (4448 bytes aligné à 4096)
    size_t isa_size = 4096;
    struct drm_i915_gem_create gem_isa = {
        .size = isa_size,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_isa) < 0) {
        printf("❌ Erreur GEM_CREATE (ISA): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Buffer ISA: handle=%u, size=%zu\n", gem_isa.handle, isa_size);
    
    // Buffer 2: Interface descriptor (32 bytes aligné à 4096)
    size_t interface_desc_size = 4096;
    struct drm_i915_gem_create gem_interface_desc = {
        .size = interface_desc_size,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_interface_desc) < 0) {
        printf("❌ Erreur GEM_CREATE (interface_desc): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Buffer interface_desc: handle=%u, size=%zu\n", gem_interface_desc.handle, interface_desc_size);
    
    // Buffer 3: Binding table (4 bytes aligné à 4096)
    size_t binding_table_size = 4096;
    struct drm_i915_gem_create gem_binding_table = {
        .size = binding_table_size,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_binding_table) < 0) {
        printf("❌ Erreur GEM_CREATE (binding_table): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Buffer binding_table: handle=%u, size=%zu\n", gem_binding_table.handle, binding_table_size);
    
    // Buffer 4: Surface state (32 bytes aligné à 4096)
    size_t surface_state_size = 4096;
    struct drm_i915_gem_create gem_surface_state = {
        .size = surface_state_size,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_surface_state) < 0) {
        printf("❌ Erreur GEM_CREATE (surface_state): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Buffer surface_state: handle=%u, size=%zu\n", gem_surface_state.handle, surface_state_size);
    
    // Buffer 5: Output (4096 bytes)
    size_t output_size = 4096;
    struct drm_i915_gem_create gem_output = {
        .size = output_size,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_output) < 0) {
        printf("❌ Erreur GEM_CREATE (output): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Buffer output: handle=%u, size=%zu\n", gem_output.handle, output_size);
    
    // Buffer 6: Batch buffer (4096 bytes)
    size_t batch_size = 4096;
    struct drm_i915_gem_create gem_batch = {
        .size = batch_size,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_batch) < 0) {
        printf("❌ Erreur GEM_CREATE (batch): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Buffer batch: handle=%u, size=%zu\n", gem_batch.handle, batch_size);
    
    // Étape 5: Mapper buffers
    printf("\n[5] Mapping buffers...\n");
    
    // Mapper ISA
    struct drm_i915_gem_mmap_offset mmap_isa = {
        .handle = gem_isa.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_isa) < 0) {
        printf("❌ Erreur MMAP_OFFSET (ISA): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    void* isa_ptr = mmap(NULL, isa_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_isa.offset);
    if (isa_ptr == MAP_FAILED) {
        printf("❌ Erreur mmap (ISA): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ ISA mappé: %p\n", isa_ptr);
    
    // Mapper interface_desc
    struct drm_i915_gem_mmap_offset mmap_interface_desc = {
        .handle = gem_interface_desc.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_interface_desc) < 0) {
        printf("❌ Erreur MMAP_OFFSET (interface_desc): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    void* interface_desc_ptr = mmap(NULL, interface_desc_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_interface_desc.offset);
    if (interface_desc_ptr == MAP_FAILED) {
        printf("❌ Erreur mmap (interface_desc): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Interface descriptor mappé: %p\n", interface_desc_ptr);
    
    // Mapper binding_table
    struct drm_i915_gem_mmap_offset mmap_binding_table = {
        .handle = gem_binding_table.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_binding_table) < 0) {
        printf("❌ Erreur MMAP_OFFSET (binding_table): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    void* binding_table_ptr = mmap(NULL, binding_table_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_binding_table.offset);
    if (binding_table_ptr == MAP_FAILED) {
        printf("❌ Erreur mmap (binding_table): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Binding table mappé: %p\n", binding_table_ptr);
    
    // Mapper surface_state
    struct drm_i915_gem_mmap_offset mmap_surface_state = {
        .handle = gem_surface_state.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_surface_state) < 0) {
        printf("❌ Erreur MMAP_OFFSET (surface_state): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    void* surface_state_ptr = mmap(NULL, surface_state_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_surface_state.offset);
    if (surface_state_ptr == MAP_FAILED) {
        printf("❌ Erreur mmap (surface_state): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Surface state mappé: %p\n", surface_state_ptr);
    
    // Mapper output
    struct drm_i915_gem_mmap_offset mmap_output = {
        .handle = gem_output.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_output) < 0) {
        printf("❌ Erreur MMAP_OFFSET (output): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    void* output_ptr = mmap(NULL, output_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_output.offset);
    if (output_ptr == MAP_FAILED) {
        printf("❌ Erreur mmap (output): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Output mappé: %p\n", output_ptr);
    
    // Mapper batch
    struct drm_i915_gem_mmap_offset mmap_batch = {
        .handle = gem_batch.handle,
        .flags = I915_MMAP_OFFSET_WB,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_batch) < 0) {
        printf("❌ Erreur MMAP_OFFSET (batch): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    void* batch_ptr = mmap(NULL, batch_size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_batch.offset);
    if (batch_ptr == MAP_FAILED) {
        printf("❌ Erreur mmap (batch): %s\n", strerror(errno));
        close(drm_fd);
        return 1;
    }
    printf("✅ Batch mappé: %p\n", batch_ptr);
    
    // Étape 6: Copier ISA
    printf("\n[6] Copie ISA Gen9...\n");
    memcpy(isa_ptr, gen9_kernel_write_test_isa, gen9_kernel_write_test_isa_size);
    printf("✅ ISA copié: %zu bytes\n", gen9_kernel_write_test_isa_size);
    
    // Étape 7: Construire structures Gen9
    printf("\n[7] Construction structures Gen9...\n");
    
    // Interface descriptor
    gen9_interface_descriptor_t* interface_desc = (gen9_interface_descriptor_t*)interface_desc_ptr;
    memset(interface_desc, 0, sizeof(gen9_interface_descriptor_t));
    interface_desc->kernel_start_pointer = 0;  // Offset 0 dans ISA buffer
    interface_desc->binding_table_pointer = 0; // Offset 0 dans binding table buffer
    interface_desc->num_threads = 1;           // 1 thread pour test
    printf("✅ Interface descriptor créé\n");
    
    // Binding table (1 entrée: surface state offset 0)
    uint32_t* binding_table = (uint32_t*)binding_table_ptr;
    binding_table[0] = 0;  // Offset 0 dans surface state buffer
    printf("✅ Binding table créé (1 entrée)\n");
    
    // Surface state (pour output buffer)
    gen9_surface_state_t* surface_state = (gen9_surface_state_t*)surface_state_ptr;
    memset(surface_state, 0, sizeof(gen9_surface_state_t));
    surface_state->surface_type_format = 0x00000000;  // RAW buffer
    surface_state->base_address_low = 0;              // Sera relocalisé par kernel
    surface_state->width_height = (output_size - 1);  // Taille buffer
    printf("✅ Surface state créé\n");
    
    // Batch buffer
    uint32_t* batch = (uint32_t*)batch_ptr;
    int batch_idx = 0;
    
    // PIPE_CONTROL (flush)
    batch[batch_idx++] = 0x7A000004;  // PIPE_CONTROL
    batch[batch_idx++] = 0x00100000;  // DC Flush
    batch[batch_idx++] = 0x00000000;  // Address low
    batch[batch_idx++] = 0x00000000;  // Address high
    batch[batch_idx++] = 0x00000000;  // Data low
    batch[batch_idx++] = 0x00000000;  // Data high
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[batch_idx++] = 0x70020002;  // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[batch_idx++] = 0x00000000;  // Reserved
    batch[batch_idx++] = 0x00000020;  // Length (32 bytes)
    batch[batch_idx++] = 0x00000000;  // Offset (0 dans interface_desc buffer)
    
    // GPGPU_WALKER (1 thread)
    batch[batch_idx++] = 0x75020008;  // GPGPU_WALKER
    batch[batch_idx++] = 0x00000000;  // Interface descriptor offset
    batch[batch_idx++] = 0x00000000;  // Indirect data length
    batch[batch_idx++] = 0x00000000;  // Indirect data start
    batch[batch_idx++] = 0x00000000;  // Thread group ID X start
    batch[batch_idx++] = 0x00000001;  // Thread group ID X dimension (1)
    batch[batch_idx++] = 0x00000000;  // Thread group ID Y start
    batch[batch_idx++] = 0x00000001;  // Thread group ID Y dimension (1)
    batch[batch_idx++] = 0x00000000;  // Thread group ID Z start
    batch[batch_idx++] = 0x00000001;  // Thread group ID Z dimension (1)
    batch[batch_idx++] = 0xFFFFFFFF;  // Right execution mask
    batch[batch_idx++] = 0xFFFFFFFF;  // Bottom execution mask
    
    // PIPE_CONTROL (final)
    batch[batch_idx++] = 0x7A000004;  // PIPE_CONTROL
    batch[batch_idx++] = 0x00100000;  // DC Flush
    batch[batch_idx++] = 0x00000000;  // Address low
    batch[batch_idx++] = 0x00000000;  // Address high
    batch[batch_idx++] = 0x00000000;  // Data low
    batch[batch_idx++] = 0x00000000;  // Data high
    
    // MI_BATCH_BUFFER_END
    batch[batch_idx++] = MI_BATCH_BUFFER_END;
    
    printf("✅ Batch buffer créé: %d DWORDs\n", batch_idx);
    
    // Étape 8: Sauvegarder structures
    printf("\n[8] Sauvegarde structures...\n");
    
    FILE* f = fopen("/tmp/c197_33_structures.bin", "wb");
    if (f) {
        fwrite(&batch_idx, sizeof(int), 1, f);
        fwrite(batch, sizeof(uint32_t), batch_idx, f);
        fclose(f);
        printf("✅ Structures sauvegardées: /tmp/c197_33_structures.bin\n");
    }
    
    // Cleanup
    printf("\n[9] Cleanup...\n");
    munmap(isa_ptr, isa_size);
    munmap(interface_desc_ptr, interface_desc_size);
    munmap(binding_table_ptr, binding_table_size);
    munmap(surface_state_ptr, surface_state_size);
    munmap(output_ptr, output_size);
    munmap(batch_ptr, batch_size);
    close(drm_fd);
    
    printf("\n=== SUCCÈS C197.33 ===\n");
    printf("✅ 6 buffers GEM créés et mappés\n");
    printf("✅ ISA Gen9 copié (4448 bytes)\n");
    printf("✅ Interface descriptor créé\n");
    printf("✅ Binding table créé (1 entrée)\n");
    printf("✅ Surface state créé\n");
    printf("✅ Batch buffer créé (%d DWORDs)\n", batch_idx);
    printf("\n🎯 Prochaine étape: C197.34 - Soumettre au GPU via EXECBUFFER2\n");
    
    return 0;
}

// Made with Bob
