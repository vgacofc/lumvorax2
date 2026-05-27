/*
 * TEST C475v2 - GPGPU COMPLET AVEC CODE INTEL BEIGNET
 * 
 * Objectif: Utiliser la séquence GPGPU complète d'Intel Beignet pour exécuter
 *           un kernel minimal sur GPU Gen9 via i915 DRM natif
 * 
 * Séquence Beignet (lignes 800-810 intel_gpgpu.c):
 * 1. PIPE_CONTROL
 * 2. Set L3 config
 * 3. PIPELINE_SELECT GPGPU
 * 4. STATE_BASE_ADDRESS
 * 5. MEDIA_VFE_STATE (ligne 513)
 * 6. CURBE_LOAD (ligne 555)
 * 7. IDRT_LOAD (ligne 577)
 * 8. GPGPU_WALKER (ligne 2261)
 * 9. MEDIA_STATE_FLUSH
 * 10. PIPE_CONTROL
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
#include <errno.h>
#include <time.h>

#include <i915_drm.h>
#include <drm.h>

// Commandes Gen9 depuis Beignet intel_defines.h
#define CMD(PIPELINE,OP,SUB_OP) ((3 << 29) | ((PIPELINE) << 27) | ((OP) << 24) | ((SUB_OP) << 16))
#define CMD_PIPE_CONTROL                CMD(3, 2, 0)
#define CMD_STATE_BASE_ADDRESS          CMD(0, 1, 1)
#define CMD_PIPELINE_SELECT             CMD(1, 1, 4)
#define CMD_MEDIA_STATE_POINTERS        CMD(2, 0, 0)
#define CMD_MEDIA_STATE_FLUSH           CMD(2, 0, 4)
#define CMD_GPGPU_WALKER                CMD(2, 1, 5)
#define CMD_CURBE_LOAD                  CMD(2, 0, 1)
#define CMD_IDRT_LOAD                   CMD(2, 0, 2)

#define PIPELINE_SELECT_GPGPU           2
#define BASE_ADDRESS_MODIFY             (1 << 0)

// Interface Descriptor Gen8 (8 DWORDs) - Beignet intel_structs.h
typedef struct {
    struct {
        uint32_t pad6:6;
        uint32_t kernel_start_pointer:26;
    } desc0;
    struct {
        uint32_t kernel_start_pointer_high:16;
        uint32_t pad6:16;
    } desc1;
    struct {
        uint32_t pad:7;
        uint32_t software_exception:1;
        uint32_t pad2:3;
        uint32_t maskstack_exception:1;
        uint32_t pad3:1;
        uint32_t illegal_opcode_exception:1;
        uint32_t pad4:2;
        uint32_t floating_point_mode:1;
        uint32_t thread_priority:1;
        uint32_t single_program_flow:1;
        uint32_t denorm_mode:1;
        uint32_t thread_preemption_disable:1;
        uint32_t pad5:11;
    } desc2;
    struct {
        uint32_t pad:2;
        uint32_t sampler_count:3;
        uint32_t sampler_state_pointer:27;
    } desc3;
    struct {
        uint32_t binding_table_entry_count:5;
        uint32_t binding_table_pointer:11;
        uint32_t pad:16;
    } desc4;
    struct {
        uint32_t curbe_read_offset:16;
        uint32_t curbe_read_len:16;
    } desc5;
    struct {
        uint32_t group_threads_num:10;
        uint32_t pad:6;
        uint32_t barrier_enable:1;
        uint32_t slm_sz:5;
        uint32_t pad2:2;
        uint32_t rounding_mode:2;
        uint32_t pad3:6;
    } desc6;
    struct {
        uint32_t cross_thread_curbe_read_len:8;
        uint32_t pad:24;
    } desc7;
} gen8_interface_descriptor_t;

// Batch null_state Gen9 (960 DWORDs)
#include "../include/gen9_null_state_batch.h"

#define KERNEL_SIZE 4096
#define OUTPUT_SIZE 4096

int main() {
    printf("=== TEST C475v2: GPGPU COMPLET BEIGNET ===\n\n");
    
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        perror("open /dev/dri/card1");
        return 1;
    }
    
    // 1. Créer GEM buffers
    struct drm_i915_gem_create create_kernel = {.size = KERNEL_SIZE};
    struct drm_i915_gem_create create_output = {.size = OUTPUT_SIZE};
    struct drm_i915_gem_create create_idrt = {.size = 4096};
    struct drm_i915_gem_create create_batch = {.size = 65536};
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0 ||
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0 ||
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_idrt) < 0 ||
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("GEM_CREATE");
        close(drm_fd);
        return 1;
    }
    
    uint32_t kernel_handle = create_kernel.handle;
    uint32_t output_handle = create_output.handle;
    uint32_t idrt_handle = create_idrt.handle;
    uint32_t batch_handle = create_batch.handle;
    
    printf("✓ GEM buffers créés: kernel=%u output=%u idrt=%u batch=%u\n",
           kernel_handle, output_handle, idrt_handle, batch_handle);
    
    // 2. Mapper kernel et écrire bytecode minimal
    struct drm_i915_gem_mmap_offset mmap_offset_kernel = {
        .handle = kernel_handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_offset_kernel) < 0) {
        perror("MMAP_OFFSET kernel");
        close(drm_fd);
        return 1;
    }
    uint32_t *kernel_ptr = mmap(NULL, KERNEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_offset_kernel.offset);
    if (kernel_ptr == MAP_FAILED) {
        perror("mmap kernel");
        close(drm_fd);
        return 1;
    }
    
    // Kernel minimal: MOV + SEND (écriture mémoire)
    kernel_ptr[0] = 0x00000001; // MOV
    kernel_ptr[1] = 0xDEADBEEF; // Valeur
    kernel_ptr[2] = 0x00000002; // SEND
    kernel_ptr[3] = 0x00000000; // EOT
    munmap(kernel_ptr, KERNEL_SIZE);
    printf("✓ Kernel bytecode écrit\n");
    
    // 3. Mapper IDRT et construire Interface Descriptor (Beignet ligne 1779)
    struct drm_i915_gem_mmap_offset mmap_offset_idrt = {
        .handle = idrt_handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_offset_idrt) < 0) {
        perror("MMAP_OFFSET idrt");
        close(drm_fd);
        return 1;
    }
    gen8_interface_descriptor_t *idrt_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_offset_idrt.offset);
    if (idrt_ptr == MAP_FAILED) {
        perror("mmap idrt");
        close(drm_fd);
        return 1;
    }
    memset(idrt_ptr, 0, sizeof(gen8_interface_descriptor_t));
    
    // Configuration Interface Descriptor (Beignet intel_gpgpu_build_idrt_gen9)
    idrt_ptr->desc0.kernel_start_pointer = 0; // Sera relocalisé
    idrt_ptr->desc2.single_program_flow = 0;
    idrt_ptr->desc2.floating_point_mode = 0;
    idrt_ptr->desc3.sampler_state_pointer = 0;
    idrt_ptr->desc4.binding_table_entry_count = 0;
    idrt_ptr->desc4.binding_table_pointer = 0;
    idrt_ptr->desc5.curbe_read_len = 0; // Pas de CURBE
    idrt_ptr->desc5.curbe_read_offset = 0;
    idrt_ptr->desc6.group_threads_num = 1; // 1 thread
    idrt_ptr->desc6.barrier_enable = 0;
    idrt_ptr->desc6.slm_sz = 0;
    idrt_ptr->desc6.rounding_mode = 0;
    
    munmap(idrt_ptr, 4096);
    printf("✓ Interface Descriptor configuré\n");
    
    // 4. Construire batch buffer avec séquence Beignet complète
    struct drm_i915_gem_mmap_offset mmap_offset_batch = {
        .handle = batch_handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_offset_batch) < 0) {
        perror("MMAP_OFFSET batch");
        close(drm_fd);
        return 1;
    }
    uint32_t *batch = mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_offset_batch.offset);
    if (batch == MAP_FAILED) {
        perror("mmap batch");
        close(drm_fd);
        return 1;
    }
    uint32_t offset = 0;
    
    // Étape 1: Batch null_state complet (960 DWORDs) - Stabilité prouvée C474
    memcpy(&batch[offset], gen9_null_state_batch, sizeof(gen9_null_state_batch));
    offset += 960;
    printf("✓ Batch null_state copié (960 DWORDs)\n");
    
    // Étape 2: PIPE_CONTROL (Beignet ligne 802)
    batch[offset++] = CMD_PIPE_CONTROL | (5-2);
    batch[offset++] = 0x00100000; // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // Étape 3: PIPELINE_SELECT GPGPU (Beignet ligne 805)
    batch[offset++] = CMD_PIPELINE_SELECT | PIPELINE_SELECT_GPGPU;
    
    // Étape 4: STATE_BASE_ADDRESS simplifié (Beignet ligne 806)
    batch[offset++] = CMD_STATE_BASE_ADDRESS | (16-2);
    for (int i = 0; i < 15; i++) batch[offset++] = 0;
    
    // Étape 5: MEDIA_VFE_STATE (Beignet ligne 513-541, 9 DWORDs)
    batch[offset++] = CMD_MEDIA_STATE_POINTERS | (9-2);
    batch[offset++] = 0; // Scratch space
    batch[offset++] = 0;
    batch[offset++] = 0 | ((64-1) << 16) | (2 << 8) | 0xc0; // max_threads=64, urb=2, gpgpu_mode
    batch[offset++] = 0;
    batch[offset++] = 2<<16 | 0; // urb_size=2, curbe_size=0
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    printf("✓ MEDIA_VFE_STATE ajouté\n");
    
    // Étape 6: CURBE_LOAD (Beignet ligne 555-563, 4 DWORDs) - Vide car pas de CURBE
    batch[offset++] = CMD_CURBE_LOAD | (4-2);
    batch[offset++] = 0;
    batch[offset++] = 0; // Taille 0
    batch[offset++] = 0;
    
    // Étape 7: IDRT_LOAD (Beignet ligne 577-585, 4 DWORDs)
    batch[offset++] = CMD_IDRT_LOAD | (4-2);
    batch[offset++] = 0;
    batch[offset++] = 1 << 5; // 1 interface descriptor
    batch[offset++] = 0; // Offset IDRT - sera relocalisé
    printf("✓ IDRT_LOAD ajouté\n");
    
    // Étape 8: GPGPU_WALKER (Beignet ligne 2261-2303, 15 DWORDs)
    batch[offset++] = CMD_GPGPU_WALKER | 13;
    batch[offset++] = 0; // Interface descriptor offset = 0
    batch[offset++] = 0; // Indirect data length
    batch[offset++] = 0; // Indirect data start
    batch[offset++] = (0 << 30) | (1-1); // SIMD8, 1 thread
    batch[offset++] = 0; // Thread group X start
    batch[offset++] = 0;
    batch[offset++] = 1; // Thread group X end = 1
    batch[offset++] = 0; // Thread group Y start
    batch[offset++] = 0;
    batch[offset++] = 1; // Thread group Y end = 1
    batch[offset++] = 0; // Thread group Z start
    batch[offset++] = 1; // Thread group Z end = 1
    batch[offset++] = 0xFF; // Right execution mask
    batch[offset++] = 0xFFFFFFFF; // Bottom execution mask
    printf("✓ GPGPU_WALKER ajouté (1x1x1 work-groups)\n");
    
    // Étape 9: MEDIA_STATE_FLUSH (Beignet ligne 2305-2308)
    batch[offset++] = CMD_MEDIA_STATE_FLUSH | 0;
    batch[offset++] = 0;
    
    // Étape 10: PIPE_CONTROL final (Beignet ligne 2310)
    batch[offset++] = CMD_PIPE_CONTROL | (5-2);
    batch[offset++] = 0x00100000; // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    uint32_t batch_len = offset * 4;
    munmap(batch, 65536);
    printf("✓ Batch construit: %u DWORDs (%u bytes)\n", offset, batch_len);
    
    // 5. Préparer relocations
    struct drm_i915_gem_relocation_entry relocs[6];
    memset(relocs, 0, sizeof(relocs));
    
    // Reloc 0-3: Batch null_state (4 relocations standard)
    for (int i = 0; i < 4; i++) {
        relocs[i].target_handle = batch_handle;
        relocs[i].delta = 0;
        relocs[i].offset = (960 - 100 + i*10) * 4; // Positions approximatives
        relocs[i].read_domains = I915_GEM_DOMAIN_RENDER;
        relocs[i].write_domain = I915_GEM_DOMAIN_RENDER;
    }
    
    // Reloc 4: IDRT kernel_start_pointer → kernel buffer
    relocs[4].target_handle = kernel_handle;
    relocs[4].delta = 0;
    relocs[4].offset = 0; // Dans IDRT buffer (pas batch)
    relocs[4].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[4].write_domain = 0;
    
    // Reloc 5: IDRT_LOAD offset → IDRT buffer
    relocs[5].target_handle = idrt_handle;
    relocs[5].delta = 0;
    relocs[5].offset = (960 + 5 + 1 + 16 + 9 + 4 + 3) * 4; // Position IDRT_LOAD offset
    relocs[5].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[5].write_domain = 0;
    
    // 6. Execbuffer
    struct drm_i915_gem_exec_object2 exec_objects[4];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = kernel_handle;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;
    exec_objects[0].alignment = 4096;
    exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[0].offset = 0x100000;
    
    exec_objects[1].handle = output_handle;
    exec_objects[1].relocation_count = 0;
    exec_objects[1].relocs_ptr = 0;
    exec_objects[1].alignment = 4096;
    exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE;
    exec_objects[1].offset = 0x200000;
    
    exec_objects[2].handle = idrt_handle;
    exec_objects[2].relocation_count = 1;
    exec_objects[2].relocs_ptr = (uintptr_t)&relocs[4];
    exec_objects[2].alignment = 4096;
    exec_objects[2].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[2].offset = 0x300000;
    
    exec_objects[3].handle = batch_handle;
    exec_objects[3].relocation_count = 5;
    exec_objects[3].relocs_ptr = (uintptr_t)relocs;
    exec_objects[3].alignment = 4096;
    exec_objects[3].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[3].offset = 0x400000;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    printf("\n🚀 Soumission batch GPGPU complet Beignet...\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    if (ret < 0) {
        perror("EXECBUFFER2");
        printf("❌ Échec soumission (errno=%d)\n", errno);
        close(drm_fd);
        return 1;
    }
    
    printf("✅ Batch soumis avec succès! Durée: %.3f ms\n", elapsed * 1000);
    
    // 7. Attendre completion
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_handle,
        .timeout_ns = 1000000000, // 1 seconde
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
    } else {
        printf("✓ GPU terminé\n");
    }
    
    // 8. Vérifier output
    struct drm_i915_gem_mmap_offset mmap_offset_output = {
        .handle = output_handle,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_offset_output) < 0) {
        perror("MMAP_OFFSET output");
        close(drm_fd);
        return 1;
    }
    uint32_t *output = mmap(NULL, OUTPUT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_offset_output.offset);
    if (output == MAP_FAILED) {
        perror("mmap output");
        close(drm_fd);
        return 1;
    }
    
    printf("\n📊 RÉSULTATS OUTPUT:\n");
    printf("output[0] = 0x%08X (attendu: 0xDEADBEEF)\n", output[0]);
    printf("output[1] = 0x%08X\n", output[1]);
    printf("output[2] = 0x%08X\n", output[2]);
    printf("output[3] = 0x%08X\n", output[3]);
    
    int success = (output[0] == 0xDEADBEEF);
    munmap(output, OUTPUT_SIZE);
    
    // Cleanup
    struct drm_gem_close close_gem = {0};
    close_gem.handle = kernel_handle; ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    close_gem.handle = output_handle; ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    close_gem.handle = idrt_handle; ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    close_gem.handle = batch_handle; ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_gem);
    close(drm_fd);
    
    if (success) {
        printf("\n✅ SUCCÈS: Kernel GPU exécuté! Output = 0xDEADBEEF\n");
        return 0;
    } else {
        printf("\n❌ ÉCHEC: Kernel non exécuté (output = 0x%08X)\n", output[0]);
        return 1;
    }
}

// Made with Bob
