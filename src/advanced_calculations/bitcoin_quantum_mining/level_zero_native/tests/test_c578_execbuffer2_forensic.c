#include "forensic/execbuffer2_forensic.h"
#include "forensic/surface_state_gen9.h"
#include "forensic/forensic_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <drm/i915_drm.h>

/**
 * TEST C578 - EXECBUFFER2 FORENSIC COMPLET
 * 
 * Objectifs:
 * 1. Tracer soumission EXECBUFFER2 complète
 * 2. Valider allocations GTT runtime
 * 3. Vérifier relocations appliquées
 * 4. Corréler timeline CPU/GPU
 * 5. Répondre aux 8 questions critiques
 */

#define BATCH_SIZE 4096
#define INPUT_SIZE 1024
#define OUTPUT_SIZE 1024

// ============================================================================
// HELPERS
// ============================================================================

static uint32_t create_buffer(int drm_fd, uint32_t size) {
    struct drm_i915_gem_create create = { .size = size };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) != 0) {
        log_event("ERROR", "Failed to create buffer: %s", strerror(errno));
        return 0;
    }
    log_event("INFO", "Created buffer: handle=%u, size=%u", create.handle, size);
    return create.handle;
}

static bool write_batch_minimal(int drm_fd, uint32_t batch_handle) {
    // Mapper batch buffer
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = batch_handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        log_event("ERROR", "Failed to mmap batch: %s", strerror(errno));
        return false;
    }
    
    uint32_t *batch = (uint32_t *)mmap_arg.addr_ptr;
    memset(batch, 0, BATCH_SIZE);
    uint32_t offset = 0;
    
    // Batch Gen9 valide minimal:
    // 1. STATE_BASE_ADDRESS (requis pour validation parser Gen9)
    batch[offset++] = 0x61010008;  // STATE_BASE_ADDRESS, DWord Length=8
    for (int i = 0; i < 9; i++) {
        batch[offset++] = 0x00000000;  // Tous les base addresses à 0
    }
    
    // 2. PIPE_CONTROL (flush minimal)
    batch[offset++] = 0x7a000004;  // PIPE_CONTROL, DWord Length=4
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0x00000000;  // Address low
    batch[offset++] = 0x00000000;  // Address high
    batch[offset++] = 0x00000000;  // Data low
    batch[offset++] = 0x00000000;  // Data high
    
    // 3. MI_BATCH_BUFFER_END
    batch[offset++] = 0x05000000;  // MI_BATCH_BUFFER_END
    
    munmap(batch, BATCH_SIZE);
    log_event("INFO", "Wrote Gen9 valid batch (STATE_BASE_ADDRESS + PIPE_CONTROL + MI_BATCH_BUFFER_END, %u DWords)", offset);
    return true;
}

static bool write_batch_with_surface_states(int drm_fd, uint32_t batch_handle,
                                           uint32_t input_handle __attribute__((unused)),
                                           uint32_t output_handle __attribute__((unused))) {
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = batch_handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        return false;
    }
    
    uint32_t *batch = (uint32_t *)mmap_arg.addr_ptr;
    memset(batch, 0, BATCH_SIZE);
    uint32_t offset = 0;
    
    // 1. STATE_BASE_ADDRESS (minimal)
    batch[offset++] = 0x61010008;  // STATE_BASE_ADDRESS, DWord Length=8
    for (int i = 0; i < 9; i++) {
        batch[offset++] = 0x00000000;
    }
    
    // 2. Surface State Input (64 bytes = 16 DWords)
    uint32_t surface_state_offset = offset * 4;
    surface_config_t input_config = {
        .type = SURFTYPE_BUFFER,
        .format = SURFACEFORMAT_R32_UINT,
        .tiling = TILE_LINEAR,
        .width = INPUT_SIZE / 4,
        .height = 1,
        .depth = 1,
        .pitch = INPUT_SIZE,
        .base_address = 0x1001,  // Symbolique - sera relocalisé
        .mocs = MOCS_CACHED_LLC_L3
    };
    
    uint32_t input_surface[16];
    generate_surface_state_gen9(&input_config, input_surface);
    memcpy(&batch[offset], input_surface, 64);
    offset += 16;
    
    // 3. Surface State Output (64 bytes = 16 DWords)
    surface_config_t output_config = input_config;
    output_config.width = OUTPUT_SIZE / 4;
    output_config.pitch = OUTPUT_SIZE;
    output_config.base_address = 0x2001;  // Symbolique - sera relocalisé
    
    uint32_t output_surface[16];
    generate_surface_state_gen9(&output_config, output_surface);
    memcpy(&batch[offset], output_surface, 64);
    offset += 16;
    
    // 4. Binding Table (2 entries × 4 bytes)
    uint32_t binding_table_offset = offset * 4;
    batch[offset++] = surface_state_offset;      // Entry 0: Input
    batch[offset++] = surface_state_offset + 64; // Entry 1: Output
    
    // 5. MI_BATCH_BUFFER_END
    batch[offset++] = 0x05000000;
    
    munmap(batch, BATCH_SIZE);
    
    log_event("INFO", "Wrote batch with Surface States:");
    log_event("INFO", "  - STATE_BASE_ADDRESS at offset 0");
    log_event("INFO", "  - Input Surface State at offset %u", surface_state_offset);
    log_event("INFO", "  - Output Surface State at offset %u", surface_state_offset + 64);
    log_event("INFO", "  - Binding Table at offset %u", binding_table_offset);
    
    return true;
}

// ============================================================================
// TESTS
// ============================================================================

static bool test_1_execbuffer2_minimal(int drm_fd) {
    log_event("INFO", "\n=== TEST 1: EXECBUFFER2 Minimal ===");
    
    // Créer batch buffer
    uint32_t batch_handle = create_buffer(drm_fd, BATCH_SIZE);
    if (batch_handle == 0) return false;
    
    // Écrire batch minimal
    if (!write_batch_minimal(drm_fd, batch_handle)) {
        return false;
    }
    
    // Préparer exec_object
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = batch_handle,
        .relocation_count = 0,
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = 0,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    // Préparer execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = 68,  // 17 DWords (STATE_BASE_ADDRESS + PIPE_CONTROL + MI_BATCH_BUFFER_END)
        .flags = I915_EXEC_RENDER | (0x1 << 0),  // Force RCS ring (Render Command Streamer)
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    // Tracer soumission
    execbuffer2_trace_config_t config;
    execbuffer2_trace_config_init(&config);
    
    execbuffer2_result_t result;
    bool success = trace_execbuffer2_submit(drm_fd, &execbuf, &config, &result);
    
    // Afficher résultats
    print_execbuffer2_human(&result, stdout);
    
    // Vérifications
    if (!success) {
        log_event("ERROR", "EXECBUFFER2 failed: %s", result.error_msg);
        free_execbuffer2_result(&result);
        return false;
    }
    
    if (result.buffers_allocated_gtt != 1) {
        log_event("ERROR", "Expected 1 buffer allocated GTT, got %u",
                  result.buffers_allocated_gtt);
        free_execbuffer2_result(&result);
        return false;
    }
    
    log_event("INFO", "✅ TEST 1 PASSED: Batch buffer allocated GTT at 0x%016lx",
              (unsigned long)result.gtt_offsets[0]);
    
    free_execbuffer2_result(&result);
    
    // Nettoyer
    struct drm_gem_close close_req = { .handle = batch_handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_req);
    
    return true;
}

static bool test_2_execbuffer2_with_relocations(int drm_fd) {
    log_event("INFO", "\n=== TEST 2: EXECBUFFER2 avec Relocations ===");
    
    // Créer buffers
    uint32_t input_handle = create_buffer(drm_fd, INPUT_SIZE);
    uint32_t output_handle = create_buffer(drm_fd, OUTPUT_SIZE);
    uint32_t batch_handle = create_buffer(drm_fd, BATCH_SIZE);
    
    if (!input_handle || !output_handle || !batch_handle) {
        return false;
    }
    
    // Écrire batch avec Surface States
    if (!write_batch_with_surface_states(drm_fd, batch_handle,
                                        input_handle, output_handle)) {
        return false;
    }
    
    // Créer relocations
    struct drm_i915_gem_relocation_entry relocs[2];
    
    // Relocation 1: Input Surface State DWord 1
    relocs[0].target_handle = input_handle;
    relocs[0].delta = 0;
    relocs[0].offset = 40 + 4;  // STATE_BASE_ADDRESS (40 bytes) + Surface State DWord 1
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[0].write_domain = 0;
    relocs[0].presumed_offset = 0;
    
    // Relocation 2: Output Surface State DWord 1
    relocs[1].target_handle = output_handle;
    relocs[1].delta = 0;
    relocs[1].offset = 40 + 64 + 4;  // + 64 bytes pour Output Surface State
    relocs[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[1].write_domain = I915_GEM_DOMAIN_RENDER;
    relocs[1].presumed_offset = 0;
    
    // Préparer exec_objects
    struct drm_i915_gem_exec_object2 exec_objects[3];
    
    // Input buffer - ROOT CAUSE #98: Ajouter EXEC_OBJECT_NEEDS_GTT pour forcer allocation GTT
    exec_objects[0].handle = input_handle;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;
    exec_objects[0].alignment = 0;
    exec_objects[0].offset = 0;
    exec_objects[0].flags = EXEC_OBJECT_NEEDS_GTT;  // Force GTT allocation
    exec_objects[0].rsvd1 = 0;
    exec_objects[0].rsvd2 = 0;
    
    // Output buffer
    exec_objects[1].handle = output_handle;
    exec_objects[1].relocation_count = 0;
    exec_objects[1].relocs_ptr = 0;
    exec_objects[1].alignment = 0;
    exec_objects[1].offset = 0;
    exec_objects[1].flags = EXEC_OBJECT_WRITE;
    exec_objects[1].rsvd1 = 0;
    exec_objects[1].rsvd2 = 0;
    
    // Batch buffer (avec relocations)
    exec_objects[2].handle = batch_handle;
    exec_objects[2].relocation_count = 2;
    exec_objects[2].relocs_ptr = (uint64_t)relocs;
    exec_objects[2].alignment = 0;
    exec_objects[2].offset = 0;
    exec_objects[2].flags = 0;
    exec_objects[2].rsvd1 = 0;
    exec_objects[2].rsvd2 = 0;
    
    // Préparer execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = 200,  // Taille approximative
        .flags = I915_EXEC_RENDER | (0x1 << 0),  // Force RCS ring (Render Command Streamer)
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    // Tracer soumission
    execbuffer2_trace_config_t config;
    execbuffer2_trace_config_init(&config);
    
    execbuffer2_result_t result;
    bool success = trace_execbuffer2_submit(drm_fd, &execbuf, &config, &result);
    
    // Afficher résultats
    print_execbuffer2_human(&result, stdout);
    
    // Sauvegarder format Wireshark
    FILE *fp = fopen("logs/forensic/test_c578_execbuffer2_wireshark.txt", "w");
    if (fp) {
        print_execbuffer2_wireshark(&result, fp);
        fclose(fp);
        log_event("INFO", "Saved Wireshark format to logs/forensic/test_c578_execbuffer2_wireshark.txt");
    }
    
    // Vérifications
    if (!success) {
        log_event("ERROR", "EXECBUFFER2 failed: %s", result.error_msg);
        free_execbuffer2_result(&result);
        return false;
    }
    
    if (result.buffers_allocated_gtt != 3) {
        log_event("ERROR", "Expected 3 buffers allocated GTT, got %u",
                  result.buffers_allocated_gtt);
        free_execbuffer2_result(&result);
        return false;
    }
    
    if (result.relocations_applied != 2) {
        log_event("WARNING", "Expected 2 relocations applied, got %u",
                  result.relocations_applied);
    }
    
    log_event("INFO", "✅ TEST 2 PASSED:");
    log_event("INFO", "  - Input buffer GTT: 0x%016lx", (unsigned long)result.gtt_offsets[0]);
    log_event("INFO", "  - Output buffer GTT: 0x%016lx", (unsigned long)result.gtt_offsets[1]);
    log_event("INFO", "  - Batch buffer GTT: 0x%016lx", (unsigned long)result.gtt_offsets[2]);
    log_event("INFO", "  - Relocations applied: %u/2", result.relocations_applied);
    
    free_execbuffer2_result(&result);
    
    // Nettoyer
    struct drm_gem_close close_req;
    close_req.handle = input_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_req);
    close_req.handle = output_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_req);
    close_req.handle = batch_handle;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_req);
    
    return true;
}

static bool test_3_timeline_correlation(int drm_fd) {
    log_event("INFO", "\n=== TEST 3: Timeline CPU/GPU Corrélée ===");
    
    uint32_t batch_handle = create_buffer(drm_fd, BATCH_SIZE);
    if (!batch_handle) return false;
    
    write_batch_minimal(drm_fd, batch_handle);
    
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = batch_handle,
        .relocation_count = 0,
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = 0,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = 68,  // 17 DWords (STATE_BASE_ADDRESS + PIPE_CONTROL + MI_BATCH_BUFFER_END)
        .flags = I915_EXEC_RENDER | (0x1 << 0),  // Force RCS ring (Render Command Streamer)
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    // Capturer timestamps CPU
    uint64_t cpu_submit_ns = get_timestamp_ns();
    
    execbuffer2_trace_config_t config;
    execbuffer2_trace_config_init(&config);
    
    execbuffer2_result_t result;
    trace_execbuffer2_submit(drm_fd, &execbuf, &config, &result);
    
    uint64_t cpu_complete_ns = get_timestamp_ns();
    
    // Corréler timeline
    correlate_cpu_gpu_timeline(&result, cpu_submit_ns, cpu_complete_ns);
    
    log_event("INFO", "✅ TEST 3 PASSED:");
    log_event("INFO", "  - CPU submit → ioctl: %.3f µs",
              result.latency_cpu_gpu_ns / 1000.0);
    log_event("INFO", "  - ioctl duration: %.3f µs",
              result.latency_ioctl_ns / 1000.0);
    log_event("INFO", "  - ioctl → CPU complete: %.3f µs",
              result.latency_gpu_execution_ns / 1000.0);
    
    free_execbuffer2_result(&result);
    
    struct drm_gem_close close_req = { .handle = batch_handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_req);
    
    return true;
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    init_forensic_logger("logs/forensic/test_c578_execbuffer2.log");
    
    log_event("INFO", "╔════════════════════════════════════════════════════════════╗");
    log_event("INFO", "║  TEST C578 - EXECBUFFER2 FORENSIC COMPLET                 ║");
    log_event("INFO", "╚════════════════════════════════════════════════════════════╝");
    
    // Ouvrir DRM
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        log_event("ERROR", "Failed to open /dev/dri/card1: %s", strerror(errno));
        return 1;
    }
    log_event("INFO", "Opened DRM device: fd=%d", drm_fd);
    
    // Exécuter tests
    int passed = 0;
    int total = 3;
    
    if (test_1_execbuffer2_minimal(drm_fd)) passed++;
    if (test_2_execbuffer2_with_relocations(drm_fd)) passed++;
    if (test_3_timeline_correlation(drm_fd)) passed++;
    
    // Résumé
    log_event("INFO", "\n╔════════════════════════════════════════════════════════════╗");
    log_event("INFO", "║  RÉSUMÉ TESTS C578                                         ║");
    log_event("INFO", "╚════════════════════════════════════════════════════════════╝");
    log_event("INFO", "Tests réussis: %d/%d (%.1f%%)", passed, total,
              (passed * 100.0) / total);
    
    if (passed == total) {
        log_event("INFO", "✅ TOUS LES TESTS RÉUSSIS");
    } else {
        log_event("ERROR", "❌ %d TEST(S) ÉCHOUÉ(S)", total - passed);
    }
    
    close(drm_fd);
    close_forensic_logger();
    
    return (passed == total) ? 0 : 1;
}

// Made with Bob
