/*
 * TEST C197.6 - Binding Table Complète
 * 
 * Objectif: Créer binding table + surface states pour fix write GPU
 * 
 * Architecture Gen9 requiert:
 * 1. Surface State (décrit le buffer: adresse, format, tiling)
 * 2. Binding Table (pointe vers surface states)
 * 3. State Base Address (contient binding table pointer)
 * 4. Instruction SEND avec binding_table_index correct
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

// Surface State Gen9 (64 bytes)
typedef struct {
    uint32_t dw0;  // Surface Type, Format, etc.
    uint32_t dw1;  // Base Address [31:0]
    uint32_t dw2;  // Width, Height
    uint32_t dw3;  // Depth, Pitch
    uint32_t dw4;  // Min LOD, Mip Count
    uint32_t dw5;  // X Offset, Y Offset
    uint32_t dw6;  // Reserved
    uint32_t dw7;  // Resource Min LOD
    uint32_t dw8;  // Base Address [47:32]
    uint32_t dw9;  // Reserved
    uint32_t dw10; // Reserved
    uint32_t dw11; // Reserved
    uint32_t dw12; // Reserved
    uint32_t dw13; // Reserved
    uint32_t dw14; // Reserved
    uint32_t dw15; // Reserved
} gen9_surface_state_t;

// Créer surface state pour buffer
static void gen9_create_surface_state(gen9_surface_state_t *ss, uint64_t address, uint32_t size) {
    memset(ss, 0, sizeof(*ss));
    
    // DW0: Surface Type = BUFFER (0), Format = RAW (511)
    ss->dw0 = (0 << 29) |      // Surface Type = BUFFER
              (511 << 18) |     // Format = RAW
              (1 << 10);        // Surface Array = false
    
    // DW1: Base Address [31:0]
    ss->dw1 = (uint32_t)(address & 0xFFFFFFFF);
    
    // DW2: Width = size - 1 (en bytes)
    ss->dw2 = (size - 1) & 0x7F;
    
    // DW3: Depth = 0, Pitch = size - 1
    ss->dw3 = ((size - 1) & 0x1FFFFF) << 3;
    
    // DW8: Base Address [47:32]
    ss->dw8 = (uint32_t)((address >> 32) & 0xFFFF);
    
    LOG("[SURFACE] Surface state créé: addr=0x%lX, size=%u", address, size);
}

// Générateur kernel Gen9 avec binding table
static int gen9_emit_write_with_binding(uint8_t *batch, size_t *offset) {
    uint32_t *ptr = (uint32_t*)(batch + *offset);
    int idx = 0;
    
    LOG("[KERNEL] Génération kernel write avec binding table");
    
    // Instruction 0: STATE_BASE_ADDRESS (configure binding table pointer)
    // Opcode = 0x61010000 (3DSTATE_BASE_ADDRESS)
    ptr[idx++] = 0x61010000 | (16 - 2);  // Length = 16 DWords
    ptr[idx++] = 0x00000000;  // General State Base Address
    ptr[idx++] = 0x00000000;  // General State Base Address Upper
    ptr[idx++] = 0x00000000;  // Surface State Base Address (sera patché par relocation)
    ptr[idx++] = 0x00000000;  // Surface State Base Address Upper
    ptr[idx++] = 0x00000000;  // Dynamic State Base Address
    ptr[idx++] = 0x00000000;  // Dynamic State Base Address Upper
    ptr[idx++] = 0x00000000;  // Indirect Object Base Address
    ptr[idx++] = 0x00000000;  // Indirect Object Base Address Upper
    ptr[idx++] = 0x00000000;  // Instruction Base Address
    ptr[idx++] = 0x00000000;  // Instruction Base Address Upper
    ptr[idx++] = 0x00000000;  // General State Buffer Size
    ptr[idx++] = 0x00000000;  // Dynamic State Buffer Size
    ptr[idx++] = 0x00000000;  // Indirect Object Buffer Size
    ptr[idx++] = 0x00000000;  // Instruction Buffer Size
    ptr[idx++] = 0x00000000;  // Bindless Surface State Base Address
    
    LOG("[KERNEL] Instruction 0: STATE_BASE_ADDRESS (16 DWords)");
    
    // Instruction 1: MOV R4, 0xDEADBEEF
    ptr[idx++] = 0x00000001;  // MOV opcode
    ptr[idx++] = 0x00000004;  // Destination: R4
    ptr[idx++] = 0xDEADBEEF;  // Source: immediate
    ptr[idx++] = 0x00000000;  // Padding
    
    LOG("[KERNEL] Instruction 1: MOV R4, 0xDEADBEEF");
    
    // Instruction 2: SEND R4 → output buffer (binding table index 0)
    uint32_t msg_desc = 0;
    msg_desc |= (0x0C << 14);  // message_type = DATAPORT_WRITE
    msg_desc |= (0 << 0);      // binding_table_index = 0 (première entrée)
    msg_desc |= (1 << 25);     // message_length = 1 register
    
    uint32_t ext_desc = 0;
    ext_desc |= (0x0A << 0);   // SFID = DATAPORT
    ext_desc |= (1 << 5);      // EOT = 1
    
    ptr[idx++] = 0x00000031;   // SEND opcode
    ptr[idx++] = 0x00000004;   // Source: R4
    ptr[idx++] = msg_desc;     // Message descriptor
    ptr[idx++] = ext_desc;     // Extended descriptor
    
    LOG("[KERNEL] Instruction 2: SEND R4 → binding_table[0] (msg_desc=0x%08X)", msg_desc);
    
    *offset += idx * sizeof(uint32_t);
    
    LOG("[KERNEL] Kernel généré: %d instructions, %zu bytes", idx/4, idx * sizeof(uint32_t));
    
    return 0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    int drm_fd = -1;
    int ret = 0;
    
    LOG("=== TEST C197.6 - Binding Table Complète ===");
    LOG("Objectif: Fix write GPU avec surface states + binding table");
    
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
    
    // Créer batch buffer (8KB pour STATE_BASE_ADDRESS)
    struct drm_i915_gem_create batch_create = {
        .size = 8192
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create);
    if (ret != 0) {
        LOG("[ERROR] Création batch buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[BATCH] Batch buffer créé (handle=%u, size=%llu)", 
        batch_create.handle, batch_create.size);
    
    // Créer output buffer (4KB aligné)
    struct drm_i915_gem_create output_create = {
        .size = 4096
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &output_create);
    if (ret != 0) {
        LOG("[ERROR] Création output buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[OUTPUT] Output buffer créé (handle=%u, size=%llu)", 
        output_create.handle, output_create.size);
    
    // Créer surface state buffer (4KB pour surface states + binding table)
    struct drm_i915_gem_create surface_create = {
        .size = 4096
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &surface_create);
    if (ret != 0) {
        LOG("[ERROR] Création surface state buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[SURFACE] Surface state buffer créé (handle=%u, size=%llu)", 
        surface_create.handle, surface_create.size);
    
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
    
    // Mapper surface state buffer
    struct drm_i915_gem_mmap surface_mmap = {
        .handle = surface_create.handle,
        .offset = 0,
        .size = surface_create.size,
        .flags = 0
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &surface_mmap);
    if (ret != 0) {
        LOG("[ERROR] Mapping surface state buffer échoué: %s", strerror(errno));
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    uint8_t *surface_ptr = (uint8_t*)surface_mmap.addr_ptr;
    LOG("[SURFACE] Surface state buffer mappé @ %p", surface_ptr);
    
    // Créer surface state pour output buffer
    gen9_surface_state_t *ss = (gen9_surface_state_t*)surface_ptr;
    gen9_create_surface_state(ss, 0, output_create.size);  // Adresse sera patchée par relocation
    
    // Créer binding table (après surface states, aligné 32 bytes)
    uint32_t *binding_table = (uint32_t*)(surface_ptr + 256);
    binding_table[0] = 0;  // Offset vers surface state 0 (sera patché)
    LOG("[BINDING] Binding table créée @ offset 256, entry[0] → surface_state[0]");
    
    // Générer kernel
    size_t kernel_offset = 0;
    ret = gen9_emit_write_with_binding(batch_ptr, &kernel_offset);
    if (ret != 0) {
        LOG("[ERROR] Génération kernel échouée");
        munmap(surface_ptr, surface_create.size);
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    
    // Configurer relocations
    struct drm_i915_gem_relocation_entry relocs[3] = {
        // Relocation 0: Surface state base address dans STATE_BASE_ADDRESS
        {
            .target_handle = surface_create.handle,
            .delta = 0,
            .offset = 12,  // DW3 de STATE_BASE_ADDRESS
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        },
        // Relocation 1: Output buffer address dans surface state
        {
            .target_handle = output_create.handle,
            .delta = 0,
            .offset = 256 + 4,  // DW1 de surface state (dans surface buffer)
            .presumed_offset = 0,
            .read_domains = 0,
            .write_domain = I915_GEM_DOMAIN_RENDER
        },
        // Relocation 2: Binding table offset dans surface state buffer
        {
            .target_handle = surface_create.handle,
            .delta = 256,  // Offset de la binding table
            .offset = 256,  // Entry[0] de binding table
            .presumed_offset = 0,
            .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
            .write_domain = 0
        }
    };
    LOG("[RELOC] 3 relocations configurées");
    
    // Configurer execbuffer2
    struct drm_i915_gem_exec_object2 exec_objects[3] = {
        {
            .handle = output_create.handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .flags = EXEC_OBJECT_WRITE
        },
        {
            .handle = surface_create.handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .flags = 0
        },
        {
            .handle = batch_create.handle,
            .relocation_count = 3,
            .relocs_ptr = (uintptr_t)relocs,
            .flags = 0
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = kernel_offset,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id
    };
    
    LOG("[EXEC] Execbuffer2 configuré (batch_len=%u, ctx_id=%u)", 
        execbuf.batch_len, execbuf.rsvd1);
    
    // Dispatcher GPU
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double dispatch_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 + 
                         (t_end.tv_nsec - t_start.tv_nsec) / 1000000.0;
    
    if (ret != 0) {
        LOG("[ERROR] Execbuffer2 échoué: %s (errno=%d)", strerror(errno), errno);
        munmap(surface_ptr, surface_create.size);
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
        munmap(surface_ptr, surface_create.size);
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
        LOG("[SUCCESS] ✓ Write GPU fonctionne avec binding table ! H[0] = 0xDEADBEEF");
        success = 1;
    } else if (output_ptr[0] == 0x00000000) {
        LOG("[FAILURE] ✗ Write GPU ne fonctionne toujours pas");
        LOG("[DIAGNOSTIC] Vérifier STATE_BASE_ADDRESS et relocations");
    } else {
        LOG("[PARTIAL] ? Write GPU partiel (H[0] = 0x%08X)", output_ptr[0]);
    }
    
    // Cleanup
    munmap(output_ptr, output_create.size);
    munmap(surface_ptr, surface_create.size);
    munmap(batch_ptr, batch_create.size);
    
    struct drm_gem_close close_batch = { .handle = batch_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_batch);
    
    struct drm_gem_close close_output = { .handle = output_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_output);
    
    struct drm_gem_close close_surface = { .handle = surface_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_surface);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_create.ctx_id };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(drm_fd);
    
    LOG("=== FIN TEST C197.6 ===");
    
    return success ? 0 : 1;
}

// Made with Bob
