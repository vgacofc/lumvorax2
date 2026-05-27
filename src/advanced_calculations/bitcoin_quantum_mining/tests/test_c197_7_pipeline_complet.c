/*
 * TEST C197.7 - Pipeline Gen9 Compute Complet
 * 
 * Objectif: Implémenter le pipeline GPGPU minimal fonctionnel Gen9
 * 
 * Pipeline requis (ordre exact):
 * 1. PIPELINE_SELECT (GPGPU mode)
 * 2. STATE_BASE_ADDRESS (tous les base addresses)
 * 3. MEDIA_VFE_STATE (Virtual Front End)
 * 4. MEDIA_CURBE_LOAD (Constant URB Entry)
 * 5. MEDIA_INTERFACE_DESCRIPTOR_LOAD (kernel descriptor)
 * 6. GPGPU_WALKER (thread dispatch)
 * 7. PIPE_CONTROL (flush + wait)
 * 
 * Critère succès: H[0] = 0xDEADBEEF
 */

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

// DRM headers
#include <drm/i915_drm.h>
#include <xf86drm.h>

// Logging forensique
#define LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    printf("[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

// Gen9 Command Opcodes
#define GEN9_PIPELINE_SELECT           0x69040000
#define GEN9_STATE_BASE_ADDRESS        0x61010000
#define GEN9_MEDIA_VFE_STATE           0x70000000
#define GEN9_MEDIA_CURBE_LOAD          0x70010000
#define GEN9_MEDIA_INTERFACE_DESC_LOAD 0x70020000
#define GEN9_GPGPU_WALKER              0x71050000
#define GEN9_PIPE_CONTROL              0x7A000000
#define GEN9_MI_BATCH_BUFFER_END       0x0A000000

// Pipeline Select Modes
#define PIPELINE_SELECT_GPGPU          2

// PIPE_CONTROL Flags
#define PIPE_CONTROL_CS_STALL          (1 << 20)
#define PIPE_CONTROL_RENDER_TARGET_FLUSH (1 << 12)
#define PIPE_CONTROL_DC_FLUSH          (1 << 5)

// Interface Descriptor (64 bytes)
typedef struct {
    uint32_t dw0;  // Kernel Start Pointer [31:6]
    uint32_t dw1;  // Reserved
    uint32_t dw2;  // Reserved
    uint32_t dw3;  // Sampler State Pointer
    uint32_t dw4;  // Sampler Count + Binding Table Entry Count
    uint32_t dw5;  // Binding Table Pointer [31:5]
    uint32_t dw6;  // CURBE Read Offset + CURBE Read Length
    uint32_t dw7;  // Number of Threads + Barrier Enable
    uint32_t dw8_15[8];  // Reserved
} gen9_interface_descriptor_t;

// Émettre PIPELINE_SELECT
static void emit_pipeline_select(uint32_t **ptr) {
    uint32_t *p = *ptr;
    p[0] = GEN9_PIPELINE_SELECT | (1 - 2);  // Length = 1 DWord
    p[1] = PIPELINE_SELECT_GPGPU;
    *ptr = p + 2;
    LOG("[CMD] PIPELINE_SELECT (GPGPU mode)");
}

// Émettre STATE_BASE_ADDRESS
static void emit_state_base_address(uint32_t **ptr, uint64_t surface_state_base,
                                   uint64_t dynamic_state_base, uint64_t instruction_base) {
    uint32_t *p = *ptr;
    p[0] = GEN9_STATE_BASE_ADDRESS | (16 - 2);  // Length = 16 DWords
    p[1] = 0x00000000;  // General State Base Address Lower
    p[2] = 0x00000000;  // General State Base Address Upper
    p[3] = (uint32_t)(surface_state_base & 0xFFFFFFFF);  // Surface State Base Address Lower
    p[4] = (uint32_t)((surface_state_base >> 32) & 0xFFFF);  // Surface State Base Address Upper
    p[5] = (uint32_t)(dynamic_state_base & 0xFFFFFFFF);  // Dynamic State Base Address Lower
    p[6] = (uint32_t)((dynamic_state_base >> 32) & 0xFFFF);  // Dynamic State Base Address Upper
    p[7] = 0x00000000;  // Indirect Object Base Address Lower
    p[8] = 0x00000000;  // Indirect Object Base Address Upper
    p[9] = (uint32_t)(instruction_base & 0xFFFFFFFF);  // Instruction Base Address Lower
    p[10] = (uint32_t)((instruction_base >> 32) & 0xFFFF);  // Instruction Base Address Upper
    p[11] = 0xFFFFF000;  // General State Buffer Size (4GB)
    p[12] = 0xFFFFF000;  // Dynamic State Buffer Size (4GB)
    p[13] = 0xFFFFF000;  // Indirect Object Buffer Size (4GB)
    p[14] = 0xFFFFF000;  // Instruction Buffer Size (4GB)
    p[15] = 0x00000000;  // Bindless Surface State Base Address Lower
    p[16] = 0x00000000;  // Bindless Surface State Base Address Upper
    *ptr = p + 17;
    LOG("[CMD] STATE_BASE_ADDRESS (surface=0x%lX, dynamic=0x%lX, instruction=0x%lX)",
        surface_state_base, dynamic_state_base, instruction_base);
}

// Émettre MEDIA_VFE_STATE
static void emit_media_vfe_state(uint32_t **ptr) {
    uint32_t *p = *ptr;
    p[0] = GEN9_MEDIA_VFE_STATE | (9 - 2);  // Length = 9 DWords
    p[1] = 0x00000000;  // Scratch Space Base Pointer
    p[2] = 0x00000000;  // Scratch Space Base Pointer Upper
    p[3] = (64 << 16) | (64 << 8);  // Max Threads = 64, URB Entries = 64
    p[4] = 0x00000000;  // Reserved
    p[5] = 0x00000000;  // Reserved
    p[6] = 0x00000000;  // Reserved
    p[7] = 0x00000000;  // Reserved
    p[8] = 0x00000000;  // Reserved
    p[9] = 0x00000000;  // Reserved
    *ptr = p + 10;
    LOG("[CMD] MEDIA_VFE_STATE (max_threads=64, urb_entries=64)");
}

// Émettre MEDIA_CURBE_LOAD
static void emit_media_curbe_load(uint32_t **ptr, uint32_t curbe_total_data_length,
                                 uint32_t curbe_data_start_address) {
    uint32_t *p = *ptr;
    p[0] = GEN9_MEDIA_CURBE_LOAD | (4 - 2);  // Length = 4 DWords
    p[1] = 0x00000000;  // Reserved
    p[2] = curbe_total_data_length;
    p[3] = curbe_data_start_address;
    *ptr = p + 4;
    LOG("[CMD] MEDIA_CURBE_LOAD (length=%u, start=0x%X)",
        curbe_total_data_length, curbe_data_start_address);
}

// Émettre MEDIA_INTERFACE_DESCRIPTOR_LOAD
static void emit_media_interface_descriptor_load(uint32_t **ptr, uint32_t interface_desc_length,
                                                uint32_t interface_desc_offset) {
    uint32_t *p = *ptr;
    p[0] = GEN9_MEDIA_INTERFACE_DESC_LOAD | (4 - 2);  // Length = 4 DWords
    p[1] = 0x00000000;  // Reserved
    p[2] = interface_desc_length;
    p[3] = interface_desc_offset;
    *ptr = p + 4;
    LOG("[CMD] MEDIA_INTERFACE_DESCRIPTOR_LOAD (length=%u, offset=0x%X)",
        interface_desc_length, interface_desc_offset);
}

// Émettre GPGPU_WALKER
static void emit_gpgpu_walker(uint32_t **ptr, uint32_t thread_width, uint32_t thread_height) {
    uint32_t *p = *ptr;
    p[0] = GEN9_GPGPU_WALKER | (15 - 2);  // Length = 15 DWords
    p[1] = 0x00000000;  // Interface Descriptor Offset
    p[2] = 0x00000000;  // Indirect Data Length + Indirect Data Start Address
    p[3] = 0x00000000;  // Reserved
    p[4] = 0x00000000;  // Thread Group ID Starting X
    p[5] = 0x00000000;  // Thread Group ID X Dimension
    p[6] = 0x00000000;  // Thread Group ID Starting Y
    p[7] = 0x00000000;  // Thread Group ID Y Dimension
    p[8] = 0x00000000;  // Thread Group ID Starting Z
    p[9] = 0x00000000;  // Thread Group ID Z Dimension
    p[10] = thread_width;   // Thread Group Width
    p[11] = thread_height;  // Thread Group Height
    p[12] = 1;  // Thread Group Depth
    p[13] = 0x00000000;  // Right Execution Mask
    p[14] = 0xFFFFFFFF;  // Bottom Execution Mask
    p[15] = 0x00000000;  // Reserved
    *ptr = p + 16;
    LOG("[CMD] GPGPU_WALKER (width=%u, height=%u)", thread_width, thread_height);
}

// Émettre PIPE_CONTROL
static void emit_pipe_control(uint32_t **ptr, uint32_t flags) {
    uint32_t *p = *ptr;
    p[0] = GEN9_PIPE_CONTROL | (6 - 2);  // Length = 6 DWords
    p[1] = flags;
    p[2] = 0x00000000;  // Address Lower
    p[3] = 0x00000000;  // Address Upper
    p[4] = 0x00000000;  // Immediate Data Lower
    p[5] = 0x00000000;  // Immediate Data Upper
    *ptr = p + 6;
    LOG("[CMD] PIPE_CONTROL (flags=0x%08X)", flags);
}

// Générer kernel minimal
static void generate_minimal_kernel(uint8_t *kernel_ptr) {
    uint32_t *ptr = (uint32_t*)kernel_ptr;
    
    // Instruction 0: MOV R4, 0xDEADBEEF
    ptr[0] = 0x00000001;  // MOV opcode
    ptr[1] = 0x00000004;  // Destination: R4
    ptr[2] = 0xDEADBEEF;  // Source: immediate
    ptr[3] = 0x00000000;  // Padding
    
    // Instruction 1: SEND R4 → output buffer
    uint32_t msg_desc = (0x0C << 14) | (0 << 0) | (1 << 25);  // DATAPORT_WRITE, binding=0, length=1
    uint32_t ext_desc = (0x0A << 0) | (1 << 5);  // SFID=DATAPORT, EOT=1
    
    ptr[4] = 0x00000031;  // SEND opcode
    ptr[5] = 0x00000004;  // Source: R4
    ptr[6] = msg_desc;
    ptr[7] = ext_desc;
    
    LOG("[KERNEL] Généré: MOV R4, 0xDEADBEEF + SEND (32 bytes)");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    int drm_fd = -1;
    int ret = 0;
    
    LOG("=== TEST C197.7 - Pipeline Gen9 Compute Complet ===");
    LOG("Objectif: Pipeline GPGPU minimal fonctionnel");
    
    // Ouvrir DRM
    drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        LOG("[ERROR] Impossible d'ouvrir /dev/dri/renderD128: %s", strerror(errno));
        return 1;
    }
    LOG("[DRM] Ouverture /dev/dri/renderD128 réussie (fd=%d)", drm_fd);
    
    // Créer context i915
    struct drm_i915_gem_context_create ctx_create = {0};
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    if (ret != 0) {
        LOG("[ERROR] Création context i915 échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[CONTEXT] Context i915 créé (ctx_id=%u)", ctx_create.ctx_id);
    
    // Créer batch buffer (16KB pour pipeline complet)
    struct drm_i915_gem_create batch_create = { .size = 16384 };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create);
    if (ret != 0) {
        LOG("[ERROR] Création batch buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[BATCH] Batch buffer créé (handle=%u, size=%llu)", batch_create.handle, batch_create.size);
    
    // Créer state buffer (8KB pour surface states + descriptors)
    struct drm_i915_gem_create state_create = { .size = 8192 };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &state_create);
    if (ret != 0) {
        LOG("[ERROR] Création state buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[STATE] State buffer créé (handle=%u, size=%llu)", state_create.handle, state_create.size);
    
    // Créer kernel buffer (4KB)
    struct drm_i915_gem_create kernel_create = { .size = 4096 };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &kernel_create);
    if (ret != 0) {
        LOG("[ERROR] Création kernel buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[KERNEL] Kernel buffer créé (handle=%u, size=%llu)", kernel_create.handle, kernel_create.size);
    
    // Créer output buffer (4KB)
    struct drm_i915_gem_create output_create = { .size = 4096 };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &output_create);
    if (ret != 0) {
        LOG("[ERROR] Création output buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[OUTPUT] Output buffer créé (handle=%u, size=%llu)", output_create.handle, output_create.size);
    
    // Mapper batch buffer
    struct drm_i915_gem_mmap batch_mmap = {
        .handle = batch_create.handle,
        .offset = 0,
        .size = batch_create.size,
        .flags = 0
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &batch_mmap);
    if (ret != 0) {
        LOG("[ERROR] Mapping batch buffer échoué: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint8_t *batch_ptr = (uint8_t*)batch_mmap.addr_ptr;
    LOG("[BATCH] Batch buffer mappé @ %p", batch_ptr);
    
    // Mapper state buffer
    struct drm_i915_gem_mmap state_mmap = {
        .handle = state_create.handle,
        .offset = 0,
        .size = state_create.size,
        .flags = 0
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &state_mmap);
    if (ret != 0) {
        LOG("[ERROR] Mapping state buffer échoué: %s", strerror(errno));
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    uint8_t *state_ptr = (uint8_t*)state_mmap.addr_ptr;
    LOG("[STATE] State buffer mappé @ %p", state_ptr);
    
    // Mapper kernel buffer
    struct drm_i915_gem_mmap kernel_mmap = {
        .handle = kernel_create.handle,
        .offset = 0,
        .size = kernel_create.size,
        .flags = 0
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &kernel_mmap);
    if (ret != 0) {
        LOG("[ERROR] Mapping kernel buffer échoué: %s", strerror(errno));
        munmap(state_ptr, state_create.size);
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    uint8_t *kernel_ptr = (uint8_t*)kernel_mmap.addr_ptr;
    LOG("[KERNEL] Kernel buffer mappé @ %p", kernel_ptr);
    
    // Générer kernel
    generate_minimal_kernel(kernel_ptr);
    
    // Créer interface descriptor
    gen9_interface_descriptor_t *interface_desc = (gen9_interface_descriptor_t*)state_ptr;
    memset(interface_desc, 0, sizeof(*interface_desc));
    interface_desc->dw0 = 0;  // Kernel Start Pointer (sera patché par relocation)
    interface_desc->dw4 = (0 << 3) | 1;  // Sampler Count=0, Binding Table Entry Count=1
    interface_desc->dw5 = 256;  // Binding Table Pointer @ offset 256
    interface_desc->dw7 = 1;  // Number of Threads = 1
    LOG("[INTERFACE_DESC] Créé @ offset 0 (kernel_ptr sera patché)");
    
    // Créer binding table
    uint32_t *binding_table = (uint32_t*)(state_ptr + 256);
    binding_table[0] = 512;  // Surface State @ offset 512
    LOG("[BINDING_TABLE] Créé @ offset 256, entry[0] → surface_state @ 512");
    
    // Créer surface state pour output buffer
    uint32_t *surface_state = (uint32_t*)(state_ptr + 512);
    memset(surface_state, 0, 64);
    surface_state[0] = (0 << 29) | (511 << 18);  // Type=BUFFER, Format=RAW
    surface_state[1] = 0;  // Base Address Lower (sera patché)
    surface_state[2] = 4095;  // Width = 4096 - 1
    surface_state[3] = (4095 << 3);  // Pitch = 4096 - 1
    LOG("[SURFACE_STATE] Créé @ offset 512 (output buffer)");
    
    // Construire batch buffer avec pipeline complet
    uint32_t *batch_cmd = (uint32_t*)batch_ptr;
    
    emit_pipeline_select(&batch_cmd);
    emit_state_base_address(&batch_cmd, 0, 0, 0);  // Adresses seront patchées
    emit_media_vfe_state(&batch_cmd);
    emit_media_curbe_load(&batch_cmd, 0, 0);
    emit_media_interface_descriptor_load(&batch_cmd, 64, 0);
    emit_gpgpu_walker(&batch_cmd, 1, 1);
    emit_pipe_control(&batch_cmd, PIPE_CONTROL_CS_STALL | PIPE_CONTROL_RENDER_TARGET_FLUSH | PIPE_CONTROL_DC_FLUSH);
    
    // MI_BATCH_BUFFER_END
    *batch_cmd++ = GEN9_MI_BATCH_BUFFER_END;
    LOG("[CMD] MI_BATCH_BUFFER_END");
    
    size_t batch_len = (uint8_t*)batch_cmd - batch_ptr;
    LOG("[BATCH] Pipeline complet généré: %zu bytes", batch_len);
    
    // Configurer relocations
    // NOTE: Les offsets sont relatifs au BATCH BUFFER où les commandes sont écrites
    // Les relocations patchent les adresses dans les commandes du batch buffer
    struct drm_i915_gem_relocation_entry relocs[3] = {
        // Relocation 0: Surface State Base Address dans STATE_BASE_ADDRESS (batch offset 12)
        { .target_handle = state_create.handle, .delta = 0, .offset = 12, .presumed_offset = 0,
          .read_domains = I915_GEM_DOMAIN_INSTRUCTION, .write_domain = 0 },
        // Relocation 1: Dynamic State Base Address dans STATE_BASE_ADDRESS (batch offset 20)
        { .target_handle = state_create.handle, .delta = 0, .offset = 20, .presumed_offset = 0,
          .read_domains = I915_GEM_DOMAIN_INSTRUCTION, .write_domain = 0 },
        // Relocation 2: Instruction Base Address dans STATE_BASE_ADDRESS (batch offset 36)
        { .target_handle = kernel_create.handle, .delta = 0, .offset = 36, .presumed_offset = 0,
          .read_domains = I915_GEM_DOMAIN_INSTRUCTION, .write_domain = 0 }
    };
    LOG("[RELOC] 3 relocations configurées (batch buffer)");
    
    // Relocations pour le state buffer (interface descriptor + surface state)
    struct drm_i915_gem_relocation_entry state_relocs[2] = {
        // Relocation 0: Kernel Start Pointer dans Interface Descriptor (state offset 0)
        { .target_handle = kernel_create.handle, .delta = 0, .offset = 0, .presumed_offset = 0,
          .read_domains = I915_GEM_DOMAIN_INSTRUCTION, .write_domain = 0 },
        // Relocation 1: Output buffer address dans Surface State (state offset 516)
        { .target_handle = output_create.handle, .delta = 0, .offset = 516, .presumed_offset = 0,
          .read_domains = 0, .write_domain = I915_GEM_DOMAIN_RENDER }
    };
    LOG("[RELOC] 2 relocations configurées (state buffer)");
    
    // Configurer execbuffer2
    struct drm_i915_gem_exec_object2 exec_objects[4] = {
        { .handle = output_create.handle, .relocation_count = 0, .relocs_ptr = 0, .flags = EXEC_OBJECT_WRITE },
        { .handle = kernel_create.handle, .relocation_count = 0, .relocs_ptr = 0, .flags = 0 },
        { .handle = state_create.handle, .relocation_count = 2, .relocs_ptr = (uintptr_t)state_relocs, .flags = 0 },
        { .handle = batch_create.handle, .relocation_count = 3, .relocs_ptr = (uintptr_t)relocs, .flags = 0 }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id
    };
    
    LOG("[EXEC] Execbuffer2 configuré (batch_len=%zu, ctx_id=%llu)", batch_len, (unsigned long long)execbuf.rsvd1);
    
    // Dispatcher GPU
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double dispatch_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 + 
                         (t_end.tv_nsec - t_start.tv_nsec) / 1000000.0;
    
    if (ret != 0) {
        LOG("[ERROR] Execbuffer2 échoué: %s (errno=%d)", strerror(errno), errno);
        munmap(kernel_ptr, kernel_create.size);
        munmap(state_ptr, state_create.size);
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    LOG("[EXEC] Execbuffer2 SUCCESS (dispatch: %.3f ms)", dispatch_ms);
    
    // Synchroniser GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_create.handle,
        .flags = 0,
        .timeout_ns = 1000000000
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    if (ret != 0) {
        LOG("[ERROR] Synchronisation GPU échouée: %s", strerror(errno));
    } else {
        LOG("[SYNC] GPU execution complete");
    }
    
    // Lire output buffer
    struct drm_i915_gem_mmap output_mmap = {
        .handle = output_create.handle,
        .offset = 0,
        .size = output_create.size,
        .flags = 0
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &output_mmap);
    if (ret != 0) {
        LOG("[ERROR] Mapping output buffer échoué: %s", strerror(errno));
        munmap(kernel_ptr, kernel_create.size);
        munmap(state_ptr, state_create.size);
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    uint32_t *output_ptr = (uint32_t*)output_mmap.addr_ptr;
    
    LOG("[OUTPUT] Lecture résultat GPU:");
    LOG("  H[0] = 0x%08X (attendu: 0xDEADBEEF)", output_ptr[0]);
    LOG("  H[1] = 0x%08X", output_ptr[1]);
    LOG("  H[2] = 0x%08X", output_ptr[2]);
    LOG("  H[3] = 0x%08X", output_ptr[3]);
    
    // Vérifier résultat
    int success = 0;
    if (output_ptr[0] == 0xDEADBEEF) {
        LOG("[SUCCESS] ✓ Pipeline Gen9 complet fonctionne ! H[0] = 0xDEADBEEF");
        success = 1;
    } else if (output_ptr[0] == 0x00000000) {
        LOG("[FAILURE] ✗ Pipeline Gen9 ne fonctionne toujours pas");
        LOG("[DIAGNOSTIC] Vérifier relocations et descriptors");
    } else {
        LOG("[PARTIAL] ? Write GPU partiel (H[0] = 0x%08X)", output_ptr[0]);
    }
    
    // Cleanup
    munmap(output_ptr, output_create.size);
    munmap(kernel_ptr, kernel_create.size);
    munmap(state_ptr, state_create.size);
    munmap(batch_ptr, batch_create.size);
    
    struct drm_gem_close close_batch = { .handle = batch_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_batch);
    
    struct drm_gem_close close_state = { .handle = state_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_state);
    
    struct drm_gem_close close_kernel = { .handle = kernel_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_kernel);
    
    struct drm_gem_close close_output = { .handle = output_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_output);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_create.ctx_id };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(drm_fd);
    
    LOG("=== FIN TEST C197.7 ===");
    
    return success ? 0 : 1;
}

// Made with Bob
