/*
 * TEST C197.5 - Write Minimal GPU
 * 
 * Objectif: Isoler le problème d'écriture GPU identifié en C197
 * Kernel minimal: MOV R4, 0xDEADBEEF; SEND R4 → output[0]
 * 
 * Critère succès: H[0] = 0xDEADBEEF (au lieu de 0x00000000)
 * 
 * Si échec: confirme problème binding table/SEND
 * Si succès: intégrer progressivement SHA-256
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

// Générateur kernel Gen9 minimal
static int gen9_emit_write_minimal(uint8_t *batch, size_t *offset, uint32_t output_handle) {
    uint32_t *ptr = (uint32_t*)(batch + *offset);
    int idx = 0;
    
    LOG("[KERNEL] Génération kernel write minimal Gen9");
    
    // Instruction 0: MOV R4, 0xDEADBEEF
    // Opcode MOV = 0x01, destination R4 (GRF 4), source immediate
    ptr[idx++] = 0x00000001; // MOV opcode
    ptr[idx++] = 0x00000004; // Destination: R4
    ptr[idx++] = 0xDEADBEEF; // Source: immediate value
    ptr[idx++] = 0x00000000; // Padding
    
    LOG("[KERNEL] Instruction 0: MOV R4, 0xDEADBEEF");
    
    // Instruction 1: SEND R4 → output buffer
    // Opcode SEND = 0x31
    // Message descriptor: DATAPORT_WRITE (0x0C)
    // SFID: DATAPORT (0x0A)
    // Binding table index: 1 (output buffer)
    
    uint32_t msg_desc = 0;
    msg_desc |= (0x0C << 14);  // message_type = DATAPORT_WRITE
    msg_desc |= (1 << 0);      // binding_table_index = 1
    msg_desc |= (1 << 25);     // message_length = 1 register
    
    uint32_t ext_desc = 0;
    ext_desc |= (0x0A << 0);   // SFID = DATAPORT
    ext_desc |= (1 << 5);      // EOT = 1 (End of Thread)
    
    ptr[idx++] = 0x00000031;   // SEND opcode
    ptr[idx++] = 0x00000004;   // Source: R4
    ptr[idx++] = msg_desc;     // Message descriptor
    ptr[idx++] = ext_desc;     // Extended descriptor
    
    LOG("[KERNEL] Instruction 1: SEND R4 → output[0] (msg_desc=0x%08X, ext_desc=0x%08X)", 
        msg_desc, ext_desc);
    
    *offset += idx * sizeof(uint32_t);
    
    LOG("[KERNEL] Kernel généré: %d instructions, %zu bytes", idx/4, idx * sizeof(uint32_t));
    
    return 0;
}

int main(int argc, char **argv) {
    int drm_fd = -1;
    int ret = 0;
    
    LOG("=== TEST C197.5 - Write Minimal GPU ===");
    LOG("Objectif: Vérifier écriture GPU isolée (0xDEADBEEF)");
    
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
    
    // Créer batch buffer (4KB)
    struct drm_i915_gem_create batch_create = {
        .size = 4096
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create);
    if (ret != 0) {
        LOG("[ERROR] Création batch buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[BATCH] Batch buffer créé (handle=%u, size=%llu)", 
        batch_create.handle, batch_create.size);
    
    // Créer output buffer (32 bytes = 8 uint32_t)
    struct drm_i915_gem_create output_create = {
        .size = 32
    };
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &output_create);
    if (ret != 0) {
        LOG("[ERROR] Création output buffer échouée: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG("[OUTPUT] Output buffer créé (handle=%u, size=%llu)", 
        output_create.handle, output_create.size);
    
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
    
    // Générer kernel minimal
    size_t kernel_offset = 0;
    ret = gen9_emit_write_minimal(batch_ptr, &kernel_offset, output_create.handle);
    if (ret != 0) {
        LOG("[ERROR] Génération kernel échouée");
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    
    // Configurer relocations
    struct drm_i915_gem_relocation_entry relocs[1] = {
        {
            .target_handle = output_create.handle,
            .delta = 0,
            .offset = 8,  // Instruction 1 (SEND), offset du handle
            .presumed_offset = 0,
            .read_domains = 0,
            .write_domain = I915_GEM_DOMAIN_RENDER
        }
    };
    LOG("[RELOC] 1 relocation configurée: output @ instruction 1");
    
    // Configurer execbuffer2
    struct drm_i915_gem_exec_object2 exec_objects[2] = {
        {
            .handle = output_create.handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .alignment = 0,
            .offset = 0,
            .flags = EXEC_OBJECT_WRITE,
            .rsvd1 = 0,
            .rsvd2 = 0
        },
        {
            .handle = batch_create.handle,
            .relocation_count = 1,
            .relocs_ptr = (uintptr_t)relocs,
            .alignment = 0,
            .offset = 0,
            .flags = 0,
            .rsvd1 = 0,
            .rsvd2 = 0
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 2,
        .batch_start_offset = 0,
        .batch_len = kernel_offset,
        .cliprects_ptr = 0,
        .num_cliprects = 0,
        .DR1 = 0,
        .DR4 = 0,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0
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
        munmap(batch_ptr, batch_create.size);
        close(drm_fd);
        return 1;
    }
    LOG("[EXEC] Execbuffer2 SUCCESS (dispatch: %.3f ms)", dispatch_ms);
    
    // Synchroniser GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_create.handle,
        .flags = 0,
        .timeout_ns = 1000000000  // 1 seconde
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
        LOG("[SUCCESS] ✓ Write GPU fonctionne ! H[0] = 0xDEADBEEF");
        success = 1;
    } else if (output_ptr[0] == 0x00000000) {
        LOG("[FAILURE] ✗ Write GPU ne fonctionne PAS (H[0] = 0x00000000)");
        LOG("[DIAGNOSTIC] Problème confirmé: binding table ou SEND incorrect");
    } else {
        LOG("[PARTIAL] ? Write GPU partiel (H[0] = 0x%08X)", output_ptr[0]);
    }
    
    // Cleanup
    munmap(output_ptr, output_create.size);
    munmap(batch_ptr, batch_create.size);
    
    struct drm_gem_close close_batch = { .handle = batch_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_batch);
    
    struct drm_gem_close close_output = { .handle = output_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_output);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_create.ctx_id };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(drm_fd);
    
    LOG("=== FIN TEST C197.5 ===");
    
    return success ? 0 : 1;
}

// Made with Bob
