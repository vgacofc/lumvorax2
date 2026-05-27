/*
 * CYCLE C197.9 - INJECTION ISA GEN9 EN DRM NATIF
 * ===============================================
 * 
 * OBJECTIF : Injecter ISA Gen9 extraite d'OpenCL dans pipeline DRM
 * 
 * MÉTHODE :
 * 1. Charger ISA Gen9 depuis c197_9_gen9_isa_extracted.bin
 * 2. Réutiliser pipeline C197.7 (déjà validé)
 * 3. Remplacer kernel ISA invalide par ISA OpenCL
 * 4. Exécuter via DRM i915
 * 5. Valider write GPU (H[0] = 0xDEADBEEF)
 * 
 * VALIDATION :
 * - Execbuffer2 SUCCESS
 * - H[0] = 0xDEADBEEF (attendu)
 * - Temps exécution < 1 ms
 * 
 * RÉFÉRENCE : C197.7 (pipeline complet) + C197.8 (ISA valide)
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
#include <libdrm/drm.h>
#include <libdrm/i915_drm.h>

// Fonction chargement ISA depuis fichier
static int load_isa_from_file(const char* filename, unsigned char** isa_out, size_t* size_out) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] Cannot open %s\n", filename);
        return -1;
    }
    
    // Obtenir taille
    fseek(f, 0, SEEK_END);
    *size_out = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Allouer et lire
    *isa_out = (unsigned char*)malloc(*size_out);
    size_t read_bytes = fread(*isa_out, 1, *size_out, f);
    fclose(f);
    
    if (read_bytes != *size_out) {
        fprintf(stderr, "[ERROR] Read failed: %zu/%zu bytes\n", read_bytes, *size_out);
        free(*isa_out);
        return -1;
    }
    
    printf("[SUCCESS] Loaded ISA: %zu bytes from %s\n", *size_out, filename);
    return 0;
}

// Fonction création pipeline Gen9 (réutilisé C197.7)
static int create_gen9_pipeline(unsigned char* batch_buffer, size_t* batch_size,
                                uint32_t kernel_offset, uint32_t output_offset) {
    unsigned char* ptr = batch_buffer;
    
    // 1. PIPELINE_SELECT (GPGPU mode)
    uint32_t pipeline_select[] = {
        0x69040000,  // PIPELINE_SELECT
        0x00000002   // GPGPU mode
    };
    memcpy(ptr, pipeline_select, sizeof(pipeline_select));
    ptr += sizeof(pipeline_select);
    
    // 2. STATE_BASE_ADDRESS (5 bases)
    uint32_t state_base_address[] = {
        0x61010010,  // STATE_BASE_ADDRESS (16 DWords)
        0x00000000,  // General State Base Address (relocation 0)
        0x00000000,  // Surface State Base Address (relocation 1)
        0x00000000,  // Dynamic State Base Address (relocation 2)
        0x00000000,  // Indirect Object Base Address
        0x00000000,  // Instruction Base Address (relocation 3)
        0x00000000,  // General State Buffer Size
        0x00000000,  // Dynamic State Buffer Size
        0x00000000,  // Indirect Object Buffer Size
        0x00000000,  // Instruction Buffer Size
        0x00000000,  // Bindless Surface State Base Address
        0x00000000,  // Bindless Sampler State Base Address
        0x00000000,  // Reserved
        0x00000000,  // Reserved
        0x00000000,  // Reserved
        0x00000000   // Reserved
    };
    memcpy(ptr, state_base_address, sizeof(state_base_address));
    ptr += sizeof(state_base_address);
    
    // 3. MEDIA_VFE_STATE (Virtual Front End)
    uint32_t media_vfe_state[] = {
        0x70000006,  // MEDIA_VFE_STATE
        0x00000000,  // Scratch Space Base Pointer
        0x00400040,  // Max threads (64), URB entries (64)
        0x00000000,  // Reserved
        0x00000000,  // CURBE Allocation Size
        0x00000000,  // URB Entry Allocation Size
        0x00000000   // Scoreboard
    };
    memcpy(ptr, media_vfe_state, sizeof(media_vfe_state));
    ptr += sizeof(media_vfe_state);
    
    // 4. MEDIA_CURBE_LOAD (Constant URB Entry)
    uint32_t media_curbe_load[] = {
        0x70010001,  // MEDIA_CURBE_LOAD
        0x00000000,  // Reserved
        0x00000020   // CURBE Total Data Length (32 bytes)
    };
    memcpy(ptr, media_curbe_load, sizeof(media_curbe_load));
    ptr += sizeof(media_curbe_load);
    
    // 5. MEDIA_INTERFACE_DESCRIPTOR_LOAD
    uint32_t media_interface_desc[] = {
        0x70020002,  // MEDIA_INTERFACE_DESCRIPTOR_LOAD
        0x00000000,  // Reserved
        0x00000020,  // Interface Descriptor Total Length (32 bytes)
        kernel_offset // Interface Descriptor Data Start Address (relocation 4)
    };
    memcpy(ptr, media_interface_desc, sizeof(media_interface_desc));
    ptr += sizeof(media_interface_desc);
    
    // 6. GPGPU_WALKER (Thread dispatch)
    uint32_t gpgpu_walker[] = {
        0x7502000d,  // GPGPU_WALKER
        0x00000000,  // Interface Descriptor Offset
        0x00000000,  // Indirect Data Length
        0x00000000,  // Indirect Data Start Address
        0x00000001,  // Thread Group ID X Dimension (1)
        0x00000001,  // Thread Group ID Y Dimension (1)
        0x00000001,  // Thread Group ID Z Dimension (1)
        0x00000001,  // Right Execution Mask
        0xffffffff,  // Bottom Execution Mask
        0x00000000,  // Reserved
        0x00000000,  // Reserved
        0x00000000,  // Reserved
        0x00000000,  // Reserved
        0x00000000   // Reserved
    };
    memcpy(ptr, gpgpu_walker, sizeof(gpgpu_walker));
    ptr += sizeof(gpgpu_walker);
    
    // 7. PIPE_CONTROL (Flush + sync)
    uint32_t pipe_control[] = {
        0x7a000004,  // PIPE_CONTROL
        0x00100000,  // Flush enable
        0x00000000,  // Destination Address Low
        0x00000000,  // Destination Address High
        0x00000000,  // Immediate Data Low
        0x00000000   // Immediate Data High
    };
    memcpy(ptr, pipe_control, sizeof(pipe_control));
    ptr += sizeof(pipe_control);
    
    // 8. MI_BATCH_BUFFER_END
    uint32_t batch_end = 0x0a000000;
    memcpy(ptr, &batch_end, sizeof(batch_end));
    ptr += sizeof(batch_end);
    
    *batch_size = ptr - batch_buffer;
    printf("[INFO] Pipeline created: %zu bytes\n", *batch_size);
    return 0;
}

int main(int argc, char** argv) {
    printf("=== CYCLE C197.9 - INJECTION ISA GEN9 EN DRM ===\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 1. Charger ISA Gen9 extraite
    printf("[STEP 1] Loading ISA Gen9\n");
    unsigned char* isa_data = NULL;
    size_t isa_size = 0;
    
    if (load_isa_from_file("c197_9_gen9_isa_extracted.bin", &isa_data, &isa_size) != 0) {
        fprintf(stderr, "[ERROR] ISA loading failed\n");
        return 1;
    }
    
    printf("  ISA size: %zu bytes\n", isa_size);
    printf("  First 64 bytes:\n");
    for (size_t i = 0; i < 64 && i < isa_size; i += 16) {
        printf("    %04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < isa_size; j++) {
            printf("%02x ", isa_data[i + j]);
        }
        printf("\n");
    }
    
    // 2. Ouvrir DRM
    printf("\n[STEP 2] Opening DRM device\n");
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "[ERROR] Cannot open /dev/dri/renderD128\n");
        free(isa_data);
        return 1;
    }
    printf("[SUCCESS] DRM opened (fd=%d)\n", drm_fd);
    
    // 3. Créer context i915
    printf("\n[STEP 3] Creating i915 context\n");
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) != 0) {
        fprintf(stderr, "[ERROR] Context creation failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    printf("[SUCCESS] Context created (ctx_id=%u)\n", ctx_create.ctx_id);
    
    // 4. Allouer buffers GPU
    printf("\n[STEP 4] Allocating GPU buffers\n");
    
    // Kernel buffer (ISA Gen9)
    struct drm_i915_gem_create kernel_create = {
        .size = (isa_size + 4095) & ~4095  // Align 4K
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &kernel_create) != 0) {
        fprintf(stderr, "[ERROR] Kernel buffer creation failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    printf("  Kernel buffer: handle=%u, size=%llu\n", kernel_create.handle, kernel_create.size);
    
    // Output buffer (32 bytes pour 8× uint32_t)
    struct drm_i915_gem_create output_create = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &output_create) != 0) {
        fprintf(stderr, "[ERROR] Output buffer creation failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    printf("  Output buffer: handle=%u, size=%llu\n", output_create.handle, output_create.size);
    
    // Batch buffer (pipeline)
    struct drm_i915_gem_create batch_create = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) != 0) {
        fprintf(stderr, "[ERROR] Batch buffer creation failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    printf("  Batch buffer: handle=%u, size=%llu\n", batch_create.handle, batch_create.size);
    
    // State buffer (interface descriptor)
    struct drm_i915_gem_create state_create = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &state_create) != 0) {
        fprintf(stderr, "[ERROR] State buffer creation failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    printf("  State buffer: handle=%u, size=%llu\n", state_create.handle, state_create.size);
    
    // 5. Mapper buffers
    printf("\n[STEP 5] Mapping buffers\n");
    
    // Map kernel buffer
    struct drm_i915_gem_mmap kernel_mmap = {
        .handle = kernel_create.handle,
        .offset = 0,
        .size = kernel_create.size,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &kernel_mmap) != 0) {
        fprintf(stderr, "[ERROR] Kernel buffer mapping failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    unsigned char* kernel_ptr = (unsigned char*)kernel_mmap.addr_ptr;
    
    // Map output buffer
    struct drm_i915_gem_mmap output_mmap = {
        .handle = output_create.handle,
        .offset = 0,
        .size = output_create.size,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &output_mmap) != 0) {
        fprintf(stderr, "[ERROR] Output buffer mapping failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    uint32_t* output_ptr = (uint32_t*)output_mmap.addr_ptr;
    
    // Map batch buffer
    struct drm_i915_gem_mmap batch_mmap = {
        .handle = batch_create.handle,
        .offset = 0,
        .size = batch_create.size,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &batch_mmap) != 0) {
        fprintf(stderr, "[ERROR] Batch buffer mapping failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    unsigned char* batch_ptr = (unsigned char*)batch_mmap.addr_ptr;
    
    // Map state buffer
    struct drm_i915_gem_mmap state_mmap = {
        .handle = state_create.handle,
        .offset = 0,
        .size = state_create.size,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &state_mmap) != 0) {
        fprintf(stderr, "[ERROR] State buffer mapping failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    unsigned char* state_ptr = (unsigned char*)state_mmap.addr_ptr;
    
    printf("[SUCCESS] All buffers mapped\n");
    
    // 6. Copier ISA Gen9 dans kernel buffer
    printf("\n[STEP 6] Copying ISA to kernel buffer\n");
    memcpy(kernel_ptr, isa_data, isa_size);
    printf("[SUCCESS] ISA copied: %zu bytes\n", isa_size);
    
    // 7. Initialiser output buffer
    printf("\n[STEP 7] Initializing output buffer\n");
    memset(output_ptr, 0, 32);
    printf("[SUCCESS] Output initialized (all zeros)\n");
    
    // 8. Créer pipeline Gen9
    printf("\n[STEP 8] Creating Gen9 pipeline\n");
    size_t batch_size = 0;
    create_gen9_pipeline(batch_ptr, &batch_size, 0, 0);
    
    // 9. Configurer relocations
    printf("\n[STEP 9] Configuring relocations\n");
    struct drm_i915_gem_relocation_entry relocs[5];
    memset(relocs, 0, sizeof(relocs));
    
    // Relocation 0: General State Base (batch buffer)
    relocs[0].target_handle = batch_create.handle;
    relocs[0].offset = 12;  // STATE_BASE_ADDRESS + 4
    relocs[0].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[0].write_domain = 0;
    
    // Relocation 1: Surface State Base (batch buffer)
    relocs[1].target_handle = batch_create.handle;
    relocs[1].offset = 16;  // STATE_BASE_ADDRESS + 8
    relocs[1].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[1].write_domain = 0;
    
    // Relocation 2: Dynamic State Base (batch buffer)
    relocs[2].target_handle = batch_create.handle;
    relocs[2].offset = 20;  // STATE_BASE_ADDRESS + 12
    relocs[2].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[2].write_domain = 0;
    
    // Relocation 3: Instruction Base (kernel buffer)
    relocs[3].target_handle = kernel_create.handle;
    relocs[3].offset = 28;  // STATE_BASE_ADDRESS + 20
    relocs[3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[3].write_domain = 0;
    
    // Relocation 4: Interface Descriptor (state buffer)
    relocs[4].target_handle = state_create.handle;
    relocs[4].offset = 100;  // MEDIA_INTERFACE_DESCRIPTOR_LOAD + 12
    relocs[4].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[4].write_domain = 0;
    
    printf("[SUCCESS] 5 relocations configured\n");
    
    // 10. Exécuter via execbuffer2
    printf("\n[STEP 10] Executing via execbuffer2\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[4];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = kernel_create.handle;
    exec_objects[1].handle = output_create.handle;
    exec_objects[2].handle = state_create.handle;
    exec_objects[3].handle = batch_create.handle;
    exec_objects[3].relocation_count = 5;
    exec_objects[3].relocs_ptr = (uintptr_t)relocs;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id
    };
    
    struct timespec exec_start, exec_end;
    clock_gettime(CLOCK_MONOTONIC, &exec_start);
    
    int exec_ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    clock_gettime(CLOCK_MONOTONIC, &exec_end);
    double exec_time_ms = (exec_end.tv_sec - exec_start.tv_sec) * 1000.0 +
                         (exec_end.tv_nsec - exec_start.tv_nsec) / 1000000.0;
    
    if (exec_ret != 0) {
        fprintf(stderr, "[ERROR] Execbuffer2 failed: %d\n", exec_ret);
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    
    printf("[SUCCESS] Execbuffer2 SUCCESS (%.3f ms)\n", exec_time_ms);
    
    // 11. Synchronisation GPU
    printf("\n[STEP 11] GPU synchronization\n");
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_create.handle,
        .timeout_ns = 1000000000,  // 1s
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) != 0) {
        fprintf(stderr, "[ERROR] GPU wait failed\n");
        close(drm_fd);
        free(isa_data);
        return 1;
    }
    printf("[SUCCESS] GPU synchronized\n");
    
    // 12. Vérifier résultat
    printf("\n[STEP 12] Verifying result\n");
    printf("  H[0] = 0x%08x (expected: 0xDEADBEEF)\n", output_ptr[0]);
    printf("  H[1] = 0x%08x\n", output_ptr[1]);
    printf("  H[2] = 0x%08x\n", output_ptr[2]);
    printf("  H[3] = 0x%08x\n", output_ptr[3]);
    
    int success = (output_ptr[0] == 0xDEADBEEF);
    
    // 13. Métriques finales
    clock_gettime(CLOCK_MONOTONIC, &end);
    double total_time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                          (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    printf("\n=== RÉSULTATS C197.9 ===\n");
    printf("ISA size: %zu bytes\n", isa_size);
    printf("Pipeline size: %zu bytes\n", batch_size);
    printf("Execution time: %.3f ms\n", exec_time_ms);
    printf("Total time: %.3f ms\n", total_time_ms);
    printf("Result: %s\n", success ? "✓ SUCCESS" : "✗ FAILURE");
    
    if (success) {
        printf("\n[SUCCESS] ✓ ISA Gen9 injection successful!\n");
        printf("[SUCCESS] ✓ Write GPU validated (H[0] = 0xDEADBEEF)\n");
    } else {
        printf("\n[FAILURE] ✗ Write GPU failed (H[0] = 0x%08x)\n", output_ptr[0]);
    }
    
    // Cleanup
    munmap(kernel_ptr, kernel_create.size);
    munmap(output_ptr, output_create.size);
    munmap(batch_ptr, batch_create.size);
    munmap(state_ptr, state_create.size);
    close(drm_fd);
    free(isa_data);
    
    return success ? 0 : 1;
}

// Made with Bob
