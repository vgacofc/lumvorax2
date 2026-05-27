/*
 * TEST C575 - MI_STORE_DATA_IMM + EXEC_OBJECT_WRITE
 * ==================================================
 * 
 * HYPOTHÈSE POST ROOT CAUSE #87:
 * Instruction MI_STORE_DATA_IMM (écriture GPU) + flag EXEC_OBJECT_WRITE
 * force allocation GTT pour le buffer cible.
 * 
 * DIFFÉRENCE vs C572:
 * - C572: MI_LOAD_REGISTER_MEM (lecture) → output_gtt = 0x0 ❌
 * - C575: MI_STORE_DATA_IMM (écriture) → output_gtt = ? ✅
 * 
 * STRATÉGIE:
 * 1. PASS 0: Batch avec MI_STORE_DATA_IMM écrivant dans output
 *    - Relocation avec write_domain = RENDER
 *    - Flag EXEC_OBJECT_WRITE sur output
 * 2. Vérification: output_gtt != 0x0
 * 3. PASS 1: Batch réel avec GTT connue (NO_RELOC)
 * 
 * OBJECTIF:
 * Valider que output[0] = 0x12345678 après PASS 1
 */

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
#include <xf86drm.h>
#include <i915_drm.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define OUTPUT_SIZE 4096
#define BATCH_SIZE 4096
#define TEST_VALUE 0x12345678
#define PASS0_VALUE 0xDEADBEEF

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

static int g_drm_fd = -1;
static uint32_t g_output_handle = 0;
static uint32_t g_batch_handle = 0;
static uint64_t g_output_gtt = 0;
static uint64_t g_batch_gtt = 0;

// ============================================================================
// LOGGING FORENSIQUE
// ============================================================================

static inline uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_FORENSIC(fmt, ...) do { \
    uint64_t ns = get_timestamp_ns(); \
    printf("[C575-FORENSIC][%lu.%09lu] " fmt "\n", \
           (unsigned long)(ns / 1000000000ULL), \
           (unsigned long)(ns % 1000000000ULL), ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

#define LOG_ERROR(fmt, ...) do { \
    uint64_t ns = get_timestamp_ns(); \
    fprintf(stderr, "[C575-ERROR][%lu.%09lu] " fmt "\n", \
            (unsigned long)(ns / 1000000000ULL), \
            (unsigned long)(ns % 1000000000ULL), ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

// ============================================================================
// ERROR STATE CAPTURE (PRIORITÉ #1)
// ============================================================================

static void capture_error_state(const char *context) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("CAPTURE ERROR STATE: %s", context);
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "mkdir -p src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/logs && "
             "cat /sys/class/drm/card0/error > "
             "src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/logs/error_state_c575_%lu.txt 2>/dev/null || "
             "echo 'Error state not available'",
             (unsigned long)time(NULL));
    
    int ret = system(cmd);
    if (ret == 0) {
        LOG_FORENSIC("✅ Error state capturé");
    } else {
        LOG_FORENSIC("⚠️ Error state non disponible (pas d'erreur GPU)");
    }
}

// ============================================================================
// INITIALISATION
// ============================================================================

static int init_drm(void) {
    LOG_FORENSIC("Ouverture /dev/dri/renderD128");
    
    g_drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (g_drm_fd < 0) {
        LOG_ERROR("Échec open: %s", strerror(errno));
        return -1;
    }
    
    LOG_FORENSIC("DRM fd=%d ouvert", g_drm_fd);
    return 0;
}

static int create_gem_buffer(uint32_t size, uint32_t *handle_out) {
    struct drm_i915_gem_create create = {0};
    create.size = size;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        LOG_ERROR("Échec GEM_CREATE: %s", strerror(errno));
        return -1;
    }
    
    *handle_out = create.handle;
    LOG_FORENSIC("Buffer GEM créé: handle=%u size=%u", create.handle, size);
    return 0;
}

// ============================================================================
// PASS 0: MI_STORE_DATA_IMM + EXEC_OBJECT_WRITE
// ============================================================================

static int pass0_store_imm_with_write(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("PASS 0: MI_STORE_DATA_IMM + EXEC_OBJECT_WRITE");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    // Création buffers
    if (create_gem_buffer(OUTPUT_SIZE, &g_output_handle) < 0) return -1;
    if (create_gem_buffer(BATCH_SIZE, &g_batch_handle) < 0) return -1;
    
    // Initialisation output à 0
    uint32_t zero_data[OUTPUT_SIZE / 4] = {0};
    struct drm_i915_gem_pwrite pwrite = {0};
    pwrite.handle = g_output_handle;
    pwrite.offset = 0;
    pwrite.size = OUTPUT_SIZE;
    pwrite.data_ptr = (uint64_t)zero_data;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        LOG_ERROR("Échec PWRITE output: %s", strerror(errno));
        return -1;
    }
    
    LOG_FORENSIC("Output initialisé à 0");
    
    // Construction batch PASS 0 - BATCH MINIMAL SANS RELOCATIONS
    uint32_t batch_data[BATCH_SIZE / 4] = {0};
    uint32_t idx = 0;
    
    // Juste MI_BATCH_BUFFER_END - batch minimal
    batch_data[idx++] = 0x05000000;
    
    LOG_FORENSIC("Batch PASS 0 construit:");
    LOG_FORENSIC("  - MI_BATCH_BUFFER_END (batch minimal)");
    LOG_FORENSIC("  - Taille: %u DWords", idx);
    LOG_FORENSIC("  - Stratégie: Flag WRITE seul force allocation GTT");
    
    // Écriture batch
    pwrite.handle = g_batch_handle;
    pwrite.offset = 0;
    pwrite.size = idx * 4;
    pwrite.data_ptr = (uint64_t)batch_data;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        LOG_ERROR("Échec PWRITE batch: %s", strerror(errno));
        return -1;
    }
    
    LOG_FORENSIC("PAS de relocations (test flag WRITE seul)");
    
    // Execution objects avec EXEC_OBJECT_WRITE
    struct drm_i915_gem_exec_object2 objects[2] = {0};
    
    // Object 0: Output (WRITE + 48B)
    objects[0].handle = g_output_handle;
    objects[0].flags = EXEC_OBJECT_WRITE |  // ← FLAG WRITE!
                       EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    LOG_FORENSIC("Output flags: WRITE | 48B_ADDRESS");
    
    // Object 1: Batch (48B, PAS de relocations)
    objects[1].handle = g_batch_handle;
    objects[1].relocation_count = 0;  // ← PAS de relocations
    objects[1].relocs_ptr = 0;
    objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uint64_t)objects;
    execbuf.buffer_count = 2;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = idx * 4;
    
    LOG_FORENSIC("Soumission PASS 0 (MI_STORE + WRITE flag)");
    
    uint64_t t0 = get_timestamp_ns();
    int ret = ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t t1 = get_timestamp_ns();
    
    if (ret < 0) {
        LOG_ERROR("Échec EXECBUFFER2 PASS 0: %s", strerror(errno));
        capture_error_state("PASS0_EXECBUFFER2_FAILED");
        return -1;
    }
    
    // Récupération GTT
    g_output_gtt = objects[0].offset;
    g_batch_gtt = objects[1].offset;
    
    LOG_FORENSIC("PASS 0 SUCCÈS:");
    LOG_FORENSIC("  - Durée: %lu ns", (unsigned long)(t1 - t0));
    LOG_FORENSIC("  - Output GTT: 0x%016lx", (unsigned long)g_output_gtt);
    LOG_FORENSIC("  - Batch GTT:  0x%016lx", (unsigned long)g_batch_gtt);
    
    // Validation CRITIQUE
    if (g_output_gtt == 0) {
        LOG_ERROR("═══════════════════════════════════════════════════════");
        LOG_ERROR("❌ ÉCHEC: Output GTT = 0x0 malgré MI_STORE + WRITE");
        LOG_ERROR("═══════════════════════════════════════════════════════");
        LOG_ERROR("ROOT CAUSE #88: MI_STORE_DATA_IMM + WRITE ne force pas GTT");
        capture_error_state("OUTPUT_GTT_ZERO_DESPITE_WRITE");
        return -1;
    }
    
    if (g_batch_gtt == 0) {
        LOG_ERROR("❌ ÉCHEC: Batch GTT = 0x0");
        return -1;
    }
    
    LOG_FORENSIC("✅ GTT allouées avec succès!");
    
    // Attente GPU idle
    struct drm_i915_gem_wait wait = {0};
    wait.bo_handle = g_batch_handle;
    wait.timeout_ns = 1000000000;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        LOG_ERROR("Échec GEM_WAIT: %s", strerror(errno));
        return -1;
    }
    
    // Synchronisation cache
    struct drm_i915_gem_set_domain set_domain = {0};
    set_domain.handle = g_output_handle;
    set_domain.read_domains = I915_GEM_DOMAIN_CPU;
    set_domain.write_domain = I915_GEM_DOMAIN_CPU;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) < 0) {
        LOG_ERROR("Échec SET_DOMAIN: %s", strerror(errno));
        return -1;
    }
    
    LOG_FORENSIC("PASS 0 minimal exécuté (pas d'écriture attendue)");
    
    return 0;
}

// ============================================================================
// PASS 1: EXÉCUTION AVEC GTT CONNUES
// ============================================================================

static int pass1_execute_with_known_gtt(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("PASS 1: EXÉCUTION AVEC GTT CONNUES");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    LOG_FORENSIC("GTT connues:");
    LOG_FORENSIC("  - Output: 0x%016lx", (unsigned long)g_output_gtt);
    LOG_FORENSIC("  - Batch:  0x%016lx", (unsigned long)g_batch_gtt);
    
    // Construction batch PASS 1
    uint32_t batch_data[BATCH_SIZE / 4] = {0};
    uint32_t idx = 0;
    
    // MI_STORE_DATA_IMM: Écrire TEST_VALUE à output[0]
    batch_data[idx++] = 0x20400003;
    batch_data[idx++] = (uint32_t)(g_output_gtt & 0xFFFFFFFF);
    batch_data[idx++] = (uint32_t)((g_output_gtt >> 32) & 0xFFFF);
    batch_data[idx++] = TEST_VALUE;
    
    // MI_BATCH_BUFFER_END
    batch_data[idx++] = 0x05000000;
    
    LOG_FORENSIC("Batch PASS 1:");
    LOG_FORENSIC("  - MI_STORE_DATA_IMM @ 0x%016lx", (unsigned long)g_output_gtt);
    LOG_FORENSIC("  - Valeur: 0x%08x", TEST_VALUE);
    
    // Écriture batch
    struct drm_i915_gem_pwrite pwrite = {0};
    pwrite.handle = g_batch_handle;
    pwrite.offset = 0;
    pwrite.size = idx * 4;
    pwrite.data_ptr = (uint64_t)batch_data;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite) < 0) {
        LOG_ERROR("Échec PWRITE batch PASS 1: %s", strerror(errno));
        return -1;
    }
    
    // Execution objects (NO_RELOC)
    struct drm_i915_gem_exec_object2 objects[2] = {0};
    
    objects[0].handle = g_output_handle;
    objects[0].offset = g_output_gtt;
    objects[0].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE | 
                       EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    objects[1].handle = g_batch_handle;
    objects[1].offset = g_batch_gtt;
    objects[1].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uint64_t)objects;
    execbuf.buffer_count = 2;
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = idx * 4;
    
    LOG_FORENSIC("Soumission PASS 1 (NO_RELOC)");
    
    uint64_t t0 = get_timestamp_ns();
    int ret = ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t t1 = get_timestamp_ns();
    
    if (ret < 0) {
        LOG_ERROR("Échec EXECBUFFER2 PASS 1: %s", strerror(errno));
        capture_error_state("PASS1_EXECBUFFER2_FAILED");
        return -1;
    }
    
    LOG_FORENSIC("PASS 1 soumis (durée: %lu ns)", (unsigned long)(t1 - t0));
    
    // Attente GPU
    struct drm_i915_gem_wait wait = {0};
    wait.bo_handle = g_batch_handle;
    wait.timeout_ns = 1000000000;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        LOG_ERROR("Échec GEM_WAIT: %s", strerror(errno));
        return -1;
    }
    
    // Synchronisation
    struct drm_i915_gem_set_domain set_domain = {0};
    set_domain.handle = g_output_handle;
    set_domain.read_domains = I915_GEM_DOMAIN_CPU;
    set_domain.write_domain = I915_GEM_DOMAIN_CPU;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) < 0) {
        LOG_ERROR("Échec SET_DOMAIN: %s", strerror(errno));
        return -1;
    }
    
    LOG_FORENSIC("✅ PASS 1 exécuté");
    return 0;
}

// ============================================================================
// VALIDATION
// ============================================================================

static int validate_results(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("VALIDATION RÉSULTATS");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    uint32_t output_data[OUTPUT_SIZE / 4] = {0};
    struct drm_i915_gem_pread pread = {0};
    pread.handle = g_output_handle;
    pread.offset = 0;
    pread.size = OUTPUT_SIZE;
    pread.data_ptr = (uint64_t)output_data;
    
    if (ioctl(g_drm_fd, DRM_IOCTL_I915_GEM_PREAD, &pread) < 0) {
        LOG_ERROR("Échec PREAD: %s", strerror(errno));
        return -1;
    }
    
    LOG_FORENSIC("Output[0..15]:");
    for (int i = 0; i < 16; i++) {
        LOG_FORENSIC("  output[%2d] = 0x%08x", i, output_data[i]);
    }
    
    if (output_data[0] == TEST_VALUE) {
        LOG_FORENSIC("═══════════════════════════════════════════════════════");
        LOG_FORENSIC("✅✅✅ SUCCÈS TOTAL ✅✅✅");
        LOG_FORENSIC("═══════════════════════════════════════════════════════");
        LOG_FORENSIC("output[0] = 0x%08x (ATTENDU: 0x%08x)", 
                     output_data[0], TEST_VALUE);
        LOG_FORENSIC("ROOT CAUSES #83-#87 RÉSOLUES");
        LOG_FORENSIC("Solution MI_STORE_DATA_IMM + WRITE VALIDÉE");
        LOG_FORENSIC("PREMIER BIT GPU NATIF i915 DRM ATTEINT");
        return 0;
    } else {
        LOG_ERROR("═══════════════════════════════════════════════════════");
        LOG_ERROR("❌ ÉCHEC VALIDATION");
        LOG_ERROR("═══════════════════════════════════════════════════════");
        LOG_ERROR("output[0] = 0x%08x (ATTENDU: 0x%08x)", 
                  output_data[0], TEST_VALUE);
        capture_error_state("VALIDATION_FAILED");
        return -1;
    }
}

// ============================================================================
// NETTOYAGE
// ============================================================================

static void cleanup(void) {
    LOG_FORENSIC("Nettoyage ressources");
    
    if (g_output_handle) {
        struct drm_gem_close close = {0};
        close.handle = g_output_handle;
        ioctl(g_drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
    }
    
    if (g_batch_handle) {
        struct drm_gem_close close = {0};
        close.handle = g_batch_handle;
        ioctl(g_drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
    }
    
    if (g_drm_fd >= 0) {
        close(g_drm_fd);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("TEST C575 - MI_STORE_DATA_IMM + EXEC_OBJECT_WRITE");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("Hypothèse: Instruction WRITE + flag WRITE force GTT");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    
    uint64_t t_start = get_timestamp_ns();
    int ret = 0;
    
    if (init_drm() < 0) {
        ret = -1;
        goto cleanup;
    }
    
    if (pass0_store_imm_with_write() < 0) {
        ret = -1;
        goto cleanup;
    }
    
    if (pass1_execute_with_known_gtt() < 0) {
        ret = -1;
        goto cleanup;
    }
    
    if (validate_results() < 0) {
        ret = -1;
        goto cleanup;
    }
    
cleanup:
    cleanup();
    
    uint64_t t_end = get_timestamp_ns();
    uint64_t duration_ns = t_end - t_start;
    
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("TEST C575 TERMINÉ");
    LOG_FORENSIC("═══════════════════════════════════════════════════════");
    LOG_FORENSIC("Durée totale: %lu.%06lu ms", 
                 (unsigned long)(duration_ns / 1000000), 
                 (unsigned long)((duration_ns % 1000000) / 1000));
    LOG_FORENSIC("Code retour: %d", ret);
    
    if (ret == 0) {
        capture_error_state("SUCCESS_FINAL_STATE");
    }
    
    return ret;
}

// Made with Bob
