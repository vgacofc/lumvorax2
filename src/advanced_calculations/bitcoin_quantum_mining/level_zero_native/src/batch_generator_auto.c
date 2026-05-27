/**
 * BATCH GENERATOR AUTOMATIQUE i915 DRM NATIF - IMPLÉMENTATION
 * 
 * Implémentation du système automatique de génération de batch buffers
 * basé sur le reverse engineering complet d'OpenCL (C545, C585).
 * 
 * Date: 2026-05-26
 * Cycle: C596 - Début développement système automatique
 */

#include "batch_generator_auto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

/**
 * Offsets GTT de base pour soft-pinning automatique
 * Basé sur analyse C585: adresses OpenCL observées
 */
#define GTT_BASE_BATCH    0x0000000000040000ULL  // 256 KB
#define GTT_BASE_HEAP     0x0000000000080000ULL  // 512 KB
#define GTT_BASE_OUTPUT   0x0000000000100000ULL  // 1 MB
#define GTT_BASE_SURFACE  0x0000000000200000ULL  // 2 MB
#define GTT_BASE_BINDING  0x0000000000300000ULL  // 3 MB
#define GTT_BASE_IDRT     0x0000000000400000ULL  // 4 MB
#define GTT_BASE_CURBE    0x0000000000500000ULL  // 5 MB
#define GTT_BASE_SCRATCH  0x0000000000600000ULL  // 6 MB
#define GTT_BASE_KERNEL   0x0000000000700000ULL  // 7 MB
#define GTT_BASE_DYNAMIC  0x0000000000800000ULL  // 8 MB
#define GTT_BASE_GENERAL  0x0000000000900000ULL  // 9 MB
#define GTT_BASE_INDIRECT 0x0000000000A00000ULL  // 10 MB

/**
 * Tailles par défaut des buffers
 */
#define SIZE_BATCH    4096      // 4 KB
#define SIZE_HEAP     65536     // 64 KB
#define SIZE_OUTPUT   4096      // 4 KB
#define SIZE_SURFACE  4096      // 4 KB
#define SIZE_BINDING  4096      // 4 KB
#define SIZE_IDRT     4096      // 4 KB
#define SIZE_CURBE    4096      // 4 KB
#define SIZE_SCRATCH  4096      // 4 KB
#define SIZE_KERNEL   4096      // 4 KB
#define SIZE_DYNAMIC  4096      // 4 KB
#define SIZE_GENERAL  4096      // 4 KB
#define SIZE_INDIRECT 4096      // 4 KB

/**
 * Opcodes Gen9 identifiés (C545)
 */
#define MI_BATCH_BUFFER_END         0x05000000
#define MI_NOOP                     0x00000000
#define PIPELINE_SELECT             0x69040000
#define STATE_BASE_ADDRESS          0x61010008  // Variante 1
#define MEDIA_VFE_STATE             0x70020000
#define MEDIA_INTERFACE_DESC_LOAD   0x61010011
#define GPGPU_WALKER                0x71050000
#define PIPE_CONTROL                0x7A000004
#define _3DSTATE_BINDING_TABLE_POOL_ALLOC 0x79190000

/**
 * Noms des types de buffers pour debug
 */
const char* buffer_type_name(buffer_type_t type) {
    static const char* names[] = {
        "BATCH", "HEAP", "OUTPUT", "SURFACE_STATE",
        "BINDING_TABLE", "IDRT", "CURBE", "SCRATCH",
        "KERNEL", "DYNAMIC_STATE", "GENERAL_STATE", "INDIRECT"
    };
    if (type < sizeof(names)/sizeof(names[0])) {
        return names[type];
    }
    return "UNKNOWN";
}

/**
 * Calculer offset GTT automatique pour soft-pinning
 */
uint64_t calculate_gtt_offset_auto(batch_generator_t *gen, 
                                    buffer_type_t type,
                                    uint64_t size) {
    static const uint64_t base_offsets[] = {
        GTT_BASE_BATCH, GTT_BASE_HEAP, GTT_BASE_OUTPUT, GTT_BASE_SURFACE,
        GTT_BASE_BINDING, GTT_BASE_IDRT, GTT_BASE_CURBE, GTT_BASE_SCRATCH,
        GTT_BASE_KERNEL, GTT_BASE_DYNAMIC, GTT_BASE_GENERAL, GTT_BASE_INDIRECT
    };
    
    if (type < sizeof(base_offsets)/sizeof(base_offsets[0])) {
        return base_offsets[type];
    }
    
    // Fallback: après les buffers prédéfinis
    return 0x0000000000B00000ULL + (type * 0x100000ULL);
}

/**
 * Allouer et configurer automatiquement les 12 buffers
 */
int allocate_buffers_auto(batch_generator_t *gen) {
    printf("[AUTO] Allocation automatique des 12 buffers avec soft-pinning...\n");
    
    // Configuration des 12 buffers (comme OpenCL C585)
    struct {
        buffer_type_t type;
        uint64_t size;
    } buffer_configs[] = {
        {BUFFER_TYPE_BATCH, SIZE_BATCH},
        {BUFFER_TYPE_HEAP, SIZE_HEAP},
        {BUFFER_TYPE_OUTPUT, SIZE_OUTPUT},
        {BUFFER_TYPE_SURFACE_STATE, SIZE_SURFACE},
        {BUFFER_TYPE_BINDING_TABLE, SIZE_BINDING},
        {BUFFER_TYPE_IDRT, SIZE_IDRT},
        {BUFFER_TYPE_CURBE, SIZE_CURBE},
        {BUFFER_TYPE_SCRATCH, SIZE_SCRATCH},
        {BUFFER_TYPE_KERNEL, SIZE_KERNEL},
        {BUFFER_TYPE_DYNAMIC_STATE, SIZE_DYNAMIC},
        {BUFFER_TYPE_GENERAL_STATE, SIZE_GENERAL},
        {BUFFER_TYPE_INDIRECT, SIZE_INDIRECT}
    };
    
    for (int i = 0; i < OPENCL_BUFFER_COUNT; i++) {
        buffer_type_t type = buffer_configs[i].type;
        uint64_t size = buffer_configs[i].size;
        
        // Créer GEM buffer
        struct drm_i915_gem_create create = {
            .size = size
        };
        
        if (ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
            fprintf(stderr, "[AUTO-ERROR] Échec création buffer %s: %s\n",
                    buffer_type_name(type), strerror(errno));
            return -1;
        }
        
        // Calculer offset GTT automatique
        uint64_t gtt_offset = calculate_gtt_offset_auto(gen, type, size);
        
        // Mapper en CPU
        struct drm_i915_gem_mmap mmap_arg = {
            .handle = create.handle,
            .offset = 0,
            .size = size,
            .flags = 0
        };
        
        if (ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
            fprintf(stderr, "[AUTO-ERROR] Échec mapping buffer %s: %s\n",
                    buffer_type_name(type), strerror(errno));
            return -1;
        }
        
        // Configurer buffer
        gen->buffers[i].handle = create.handle;
        gen->buffers[i].offset = gtt_offset;
        gen->buffers[i].size = size;
        gen->buffers[i].flags = OPENCL_OBJECT_FLAGS;  // 0x18 = PINNED + 48B
        gen->buffers[i].cpu_addr = (void*)mmap_arg.addr_ptr;
        gen->buffers[i].name = buffer_type_name(type);
        
        printf("[AUTO]   Buffer %2d: %-15s handle=%u offset=0x%016lx size=%lu flags=0x%x\n",
               i, gen->buffers[i].name, gen->buffers[i].handle,
               gen->buffers[i].offset, gen->buffers[i].size, gen->buffers[i].flags);
    }
    
    gen->buffer_count = OPENCL_BUFFER_COUNT;
    printf("[AUTO] ✓ 12 buffers alloués avec succès\n");
    
    return 0;
}

/**
 * Initialiser générateur automatique
 */
int batch_generator_init(batch_generator_t *gen, int drm_fd) {
    printf("\n[AUTO] ========================================\n");
    printf("[AUTO] INITIALISATION GÉNÉRATEUR AUTOMATIQUE\n");
    printf("[AUTO] ========================================\n\n");
    
    memset(gen, 0, sizeof(*gen));
    gen->drm_fd = drm_fd;
    
    // Créer context i915 (comme OpenCL C585: ctx_id=1)
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec création context i915: %s\n", strerror(errno));
        return -1;
    }
    gen->ctx_id = ctx_create.ctx_id;
    printf("[AUTO] ✓ Context i915 créé: ctx_id=%u\n", gen->ctx_id);
    
    // Allouer les 12 buffers automatiquement
    if (allocate_buffers_auto(gen) < 0) {
        return -1;
    }
    
    // Allouer espace batch (320 bytes comme OpenCL)
    gen->batch_data = calloc(OPENCL_BATCH_SIZE / 4, sizeof(uint32_t));
    if (!gen->batch_data) {
        fprintf(stderr, "[AUTO-ERROR] Échec allocation batch data\n");
        return -1;
    }
    gen->batch_dwords = 0;
    
    // Configuration flags automatiques (comme OpenCL C585)
    gen->exec_flags = OPENCL_FLAGS;  // 0x800 = I915_EXEC_NO_RELOC
    gen->object_flags = OPENCL_OBJECT_FLAGS;  // 0x18 = PINNED + 48B
    
    gen->initialized = true;
    
    printf("\n[AUTO] ✓ Générateur automatique initialisé\n");
    printf("[AUTO]   - 12 buffers avec soft-pinning\n");
    printf("[AUTO]   - Context i915: %u\n", gen->ctx_id);
    printf("[AUTO]   - Flags EXEC: 0x%lx (I915_EXEC_NO_RELOC)\n", gen->exec_flags);
    printf("[AUTO]   - Flags Object: 0x%x (PINNED + 48B)\n", gen->object_flags);
    printf("[AUTO] ========================================\n\n");
    
    return 0;
}

/**
 * Configurer kernel à exécuter
 */
int batch_generator_set_kernel(batch_generator_t *gen, 
                                void *kernel_isa, 
                                uint32_t kernel_size,
                                uint32_t global_work_size,
                                uint32_t local_work_size) {
    if (!gen->initialized) {
        fprintf(stderr, "[AUTO-ERROR] Générateur non initialisé\n");
        return -1;
    }
    
    printf("[AUTO] Configuration kernel:\n");
    printf("[AUTO]   - Taille ISA: %u bytes\n", kernel_size);
    printf("[AUTO]   - Global work size: %u\n", global_work_size);
    printf("[AUTO]   - Local work size: %u\n", local_work_size);
    
    gen->kernel_isa = kernel_isa;
    gen->kernel_size = kernel_size;
    gen->global_work_size = global_work_size;
    gen->local_work_size = local_work_size;
    
    // Calculer nombre de threads EU
    gen->num_threads = (global_work_size + local_work_size - 1) / local_work_size;
    printf("[AUTO]   - Threads EU: %u\n", gen->num_threads);
    
    // Copier kernel ISA dans buffer KERNEL
    auto_buffer_t *kernel_buf = &gen->buffers[BUFFER_TYPE_KERNEL];
    if (kernel_size > kernel_buf->size) {
        fprintf(stderr, "[AUTO-ERROR] Kernel trop grand (%u > %lu)\n", 
                kernel_size, kernel_buf->size);
        return -1;
    }
    
    memcpy(kernel_buf->cpu_addr, kernel_isa, kernel_size);
    printf("[AUTO] ✓ Kernel ISA copié dans buffer KERNEL\n");
    
    return 0;
}

/**
 * Dump batch en hexadécimal pour debug
 */
void dump_batch_hex(uint32_t *batch, uint32_t dwords) {
    printf("[AUTO] Batch buffer (%u DWords = %u bytes):\n", dwords, dwords * 4);
    for (uint32_t i = 0; i < dwords; i++) {
        if (i % 4 == 0) {
            printf("[AUTO]   [%3u] ", i);
        }
        printf("0x%08x ", batch[i]);
        if (i % 4 == 3 || i == dwords - 1) {
            printf("\n");
        }
    }
}

/**
 * Dump configuration EXECBUFFER2 pour debug
 */
void dump_execbuffer2_config(batch_generator_t *gen) {
    printf("\n[AUTO] ========================================\n");
    printf("[AUTO] CONFIGURATION EXECBUFFER2 AUTOMATIQUE\n");
    printf("[AUTO] ========================================\n");
    printf("[AUTO] buffer_count: %u (comme OpenCL)\n", gen->buffer_count);
    printf("[AUTO] batch_len: %u bytes (comme OpenCL: 320)\n", gen->batch_dwords * 4);
    printf("[AUTO] flags: 0x%lx (I915_EXEC_NO_RELOC)\n", gen->exec_flags);
    printf("[AUTO] context: %u\n", gen->ctx_id);
    printf("[AUTO] \n");
    printf("[AUTO] Buffers (12 avec soft-pinning):\n");
    for (uint32_t i = 0; i < gen->buffer_count; i++) {
        printf("[AUTO]   [%2u] %-15s handle=%u offset=0x%016lx flags=0x%x\n",
               i, gen->buffers[i].name, gen->buffers[i].handle,
               gen->buffers[i].offset, gen->buffers[i].flags);
    }
    printf("[AUTO] ========================================\n\n");
}

/**
 * Nettoyer générateur
 */
void batch_generator_cleanup(batch_generator_t *gen) {
    printf("[AUTO] Nettoyage générateur automatique...\n");
    
    // Unmapper et fermer buffers
    for (uint32_t i = 0; i < gen->buffer_count; i++) {
        if (gen->buffers[i].cpu_addr) {
            munmap(gen->buffers[i].cpu_addr, gen->buffers[i].size);
        }
        if (gen->buffers[i].handle) {
            struct drm_gem_close close_arg = {
                .handle = gen->buffers[i].handle
            };
            ioctl(gen->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        }
    }
    
    // Détruire context
    if (gen->ctx_id) {
        struct drm_i915_gem_context_destroy ctx_destroy = {
            .ctx_id = gen->ctx_id
        };
        ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    }
    
    // Libérer batch data
    if (gen->batch_data) {
        free(gen->batch_data);
    }
    
    printf("[AUTO] ✓ Nettoyage terminé\n");
}
/**
 * CYCLE C597 - GÉNÉRATION AUTOMATIQUE COMMANDES BATCH
 * Basé sur reverse engineering OpenCL (C545, C585)
 */

// Génération STATE_BASE_ADDRESS (basé sur capture OpenCL C545)
// OpenCL utilise 6 instances, la 5ème @ 0x00A0 est complète (19 DWords)
int generate_state_base_address(batch_generator_t *gen, uint32_t *batch, uint32_t *idx) {
    if (!gen || !batch || !idx) return -1;
    
    // STATE_BASE_ADDRESS complet (19 DWords) - Instance 5 OpenCL
    batch[(*idx)++] = 0x61010011;  // Opcode + length=19
    
    // DW1: General State Base Address Modify Enable
    batch[(*idx)++] = 0x00000001;
    
    // DW2-3: General State Base Address (LOW/HIGH)
    batch[(*idx)++] = 0x00000000;
    batch[(*idx)++] = (uint32_t)(gen->buffers[BUFFER_TYPE_GENERAL_STATE].offset >> 32);
    
    // DW4-5: Surface State Base Address (LOW/HIGH) - CRITIQUE!
    uint64_t surface_gtt = gen->buffers[BUFFER_TYPE_SURFACE_STATE].offset;
    batch[(*idx)++] = (uint32_t)(surface_gtt & 0xFFFFFFFF);
    batch[(*idx)++] = (uint32_t)(surface_gtt >> 32);
    
    // DW6-7: Dynamic State Base Address (LOW/HIGH)
    uint64_t dynamic_gtt = gen->buffers[BUFFER_TYPE_DYNAMIC_STATE].offset;
    batch[(*idx)++] = (uint32_t)(dynamic_gtt & 0xFFFFFFFF);
    batch[(*idx)++] = (uint32_t)(dynamic_gtt >> 32);
    
    // DW8: Indirect Object Base Address Modify Enable
    batch[(*idx)++] = 0x00000001;
    
    // DW9: Indirect Object Base Address LOW
    uint64_t indirect_gtt = gen->buffers[BUFFER_TYPE_INDIRECT].offset;
    batch[(*idx)++] = (uint32_t)(indirect_gtt & 0xFFFFFFFF);
    
    // DW10: Indirect Object Upper Bound
    batch[(*idx)++] = 0x00000041;
    
    // DW11: Instruction Base Address Modify Enable
    batch[(*idx)++] = 0x00008001;
    
    // DW12-13: Instruction Base Address (LOW/HIGH) - KERNEL!
    uint64_t kernel_gtt = gen->buffers[BUFFER_TYPE_KERNEL].offset;
    batch[(*idx)++] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);
    batch[(*idx)++] = (uint32_t)(kernel_gtt >> 32);
    
    // DW14: General State Buffer Size
    batch[(*idx)++] = 0xfffff001;
    
    // DW15: Dynamic State Buffer Size
    batch[(*idx)++] = 0xfffff001;
    
    // DW16-17: Indirect Object Buffer Size (LOW/HIGH)
    batch[(*idx)++] = (uint32_t)(indirect_gtt & 0xFFFFFFFF);
    batch[(*idx)++] = (uint32_t)(indirect_gtt >> 32);
    
    // DW18: Instruction Buffer Size
    batch[(*idx)++] = 0x003bf000;
    
    return 0;
}

// Génération MEDIA_VFE_STATE (basé sur capture OpenCL C545)
// Configuration: 167 threads max, CURBE 1922 bytes
int generate_media_vfe_state(batch_generator_t *gen, uint32_t *batch, uint32_t *idx) {
    if (!gen || !batch || !idx) return -1;
    
    // MEDIA_VFE_STATE (9 DWords) @ 0x0058 dans batch OpenCL
    batch[(*idx)++] = 0x70000007;  // Opcode + length=9
    
    // DW1-2: Scratch Space Base Pointer (LOW/HIGH)
    uint64_t scratch_gtt = gen->buffers[BUFFER_TYPE_SCRATCH].offset;
    batch[(*idx)++] = (uint32_t)(scratch_gtt & 0xFFFFFFFF);
    batch[(*idx)++] = (uint32_t)(scratch_gtt >> 32);
    
    // DW3: Max Threads + URB Entries
    // OpenCL: 0x00a70100 = Max Threads=167 (0xa7), URB Entries=1 (0x01)
    // Calculer max threads basé sur hardware (24 EUs Gen9 UHD 620)
    uint32_t max_threads = 167;  // Valeur OpenCL validée
    uint32_t urb_entries = 1;
    batch[(*idx)++] = (max_threads << 16) | (urb_entries << 8);
    
    // DW4: URB Entry Allocation Size
    batch[(*idx)++] = 0x00000000;
    
    // DW5: CURBE Allocation Size
    // OpenCL: 0x07820000 = CURBE Size=1922 (0x782)
    uint32_t curbe_size = 1922;  // Valeur OpenCL validée
    batch[(*idx)++] = (curbe_size << 16);
    
    // DW6-8: Reserved
    batch[(*idx)++] = 0x00000000;
    batch[(*idx)++] = 0x00000000;
    batch[(*idx)++] = 0x00000000;
    
    return 0;
}


// Génération MEDIA_INTERFACE_DESCRIPTOR_LOAD (basé sur capture OpenCL C545 @ 0x00EC)
// Configure Interface Descriptor Table (IDRT) pour le kernel
int generate_media_interface_descriptor_load(batch_generator_t *gen, uint32_t *batch, uint32_t *idx) {
    if (!gen || !batch || !idx) return -1;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWords) @ 0x00EC dans batch OpenCL
    batch[(*idx)++] = 0x61020001;  // Opcode: MEDIA_INTERFACE_DESCRIPTOR_LOAD, length=3
    
    // DW1: Interface Descriptor Total Length
    // OpenCL: 0xfffee000 (valeur relocalisée)
    // Pour nous: utiliser offset IDRT buffer
    uint64_t idrt_gtt = gen->buffers[BUFFER_TYPE_IDRT].offset;
    batch[(*idx)++] = (uint32_t)(idrt_gtt & 0xFFFFFFFF);
    
    // DW2: Interface Descriptor Data Start Address
    // OpenCL: 0x00000000 (offset relatif dans IDRT)
    batch[(*idx)++] = 0x00000000;
    
    return 0;
}

// Génération GPGPU_WALKER (basé sur capture OpenCL C545 @ 0x0110)
// Dispatch compute threads sur GPU Gen9
int generate_gpgpu_walker(batch_generator_t *gen,
                          uint32_t *batch,
                          uint32_t *idx,
                          uint32_t thread_x,
                          uint32_t thread_y,
                          uint32_t thread_z) {
    if (!gen || !batch || !idx) return -1;
    
    // GPGPU_WALKER (15 DWords) @ 0x0110 dans batch OpenCL
    batch[(*idx)++] = 0x18800101;  // Opcode: GPGPU_WALKER, length=15
    
    // DW1-2: Interface Descriptor Offset (LOW/HIGH)
    // ROOT CAUSE #153 FIX: Utiliser OFFSET dans buffer IDRT, pas adresse GTT absolue
    // Documentation Intel Gen9: "Offset from Interface Descriptor Base Address"
    // Interface Descriptor Base = buffer IDRT (défini dans MEDIA_IDL)
    // Notre Interface Descriptor est au DÉBUT du buffer IDRT → offset = 0
    uint32_t idrt_offset = 0x00000000;  // Offset dans buffer IDRT (début)
    batch[(*idx)++] = idrt_offset;      // DW1: Offset LOW (0x00000000)
    batch[(*idx)++] = 0x00000000;       // DW2: Offset HIGH (0x00000000)
    
    // DW3: Indirect Data Length
    // OpenCL: 0x00000000 (pas de données indirectes)
    batch[(*idx)++] = 0x00000000;
    
    // DW4-6: Thread Group ID Starting X/Y/Z
    // OpenCL: 0x00000000 (démarrer à 0,0,0)
    batch[(*idx)++] = 0x00000000;  // Starting X
    batch[(*idx)++] = 0x00000000;  // Starting Y
    batch[(*idx)++] = 0x00000000;  // Starting Z
    
    // DW7: Reserved
    batch[(*idx)++] = 0x00000000;
    
    // DW8-10: Thread Group ID X/Y/Z Dimension
    // OpenCL: 0x00000000 (= 1 thread group dans chaque dimension)
    // Note: 0 signifie 1 thread group (valeur encodée - 1)
    batch[(*idx)++] = thread_x > 0 ? (thread_x - 1) : 0;  // X Dimension
    batch[(*idx)++] = thread_y > 0 ? (thread_y - 1) : 0;  // Y Dimension
    batch[(*idx)++] = thread_z > 0 ? (thread_z - 1) : 0;  // Z Dimension
    
    // DW11-12: Right Execution Mask, Bottom Execution Mask
    // OpenCL: 0x00000000 (tous les threads actifs)
    batch[(*idx)++] = 0x00000000;  // Right Execution Mask
    batch[(*idx)++] = 0x00000000;  // Bottom Execution Mask
    
    // DW13-14: Reserved
    batch[(*idx)++] = 0x00000000;
    batch[(*idx)++] = 0x00000000;
    
    return 0;
}

/**
 * CYCLE C604 - ROOT CAUSE #154: PIPE_CONTROL MANQUANT
 *
 * Génération PIPE_CONTROL pour synchronisation cache GPU
 *
 * PROBLÈME IDENTIFIÉ:
 * - OpenCL utilise PIPE_CONTROL après GPGPU_WALKER
 * - Notre batch natif n'a PAS de PIPE_CONTROL
 * - Sans PIPE_CONTROL, résultats GPU restent dans cache
 * - CPU lit 0xDEADBEEF au lieu de 0x12345678
 *
 * SOLUTION:
 * - Ajouter PIPE_CONTROL après GPGPU_WALKER
 * - Flags: CS Stall + DC Flush + Cache Invalidations
 * - Structure: 6 DWords
 *
 * Référence: Analyse comparative OpenCL vs DRM natif C604
 * Référence: Intel Gen9 Command Reference - PIPE_CONTROL
 */
int generate_pipe_control(batch_generator_t *gen,
                          uint32_t *batch,
                          uint32_t *idx) {
    if (!gen || !batch || !idx) return -1;
    
    // PIPE_CONTROL (6 DWords)
    // Opcode: 0x7A000004 = PIPE_CONTROL, length=6
    batch[(*idx)++] = 0x7A000004;
    
    // DW1: Control Flags
    // Bit 21: CS Stall (Command Streamer Stall) - Attendre fin dispatch
    // Bit 18: DC Flush Enable (Data Cache Flush) - Flush cache données
    // Bit 12: Render Target Cache Flush - Flush cache render target
    // Bit 11: Instruction Cache Invalidate - Invalider cache instructions
    // Bit 10: Texture Cache Invalidate - Invalider cache textures
    // Bit 6: Post-Sync Operation - Opération post-synchronisation
    batch[(*idx)++] = (1 << 21) |  // CS Stall
                      (1 << 18) |  // DC Flush
                      (1 << 12) |  // RT Cache Flush
                      (1 << 11) |  // Instruction Cache Invalidate
                      (1 << 10) |  // Texture Cache Invalidate
                      (1 << 6);    // Post-Sync Write Immediate
    
    // DW2-3: Address (LOW/HIGH) pour Post-Sync Write
    // Pour un simple flush, adresse = 0 (pas d'écriture)
    batch[(*idx)++] = 0x00000000;  // Address LOW
    batch[(*idx)++] = 0x00000000;  // Address HIGH
    
    // DW4-5: Immediate Data (LOW/HIGH)
    // Données à écrire si Post-Sync Write actif
    // Pour un simple flush, données = 0
    batch[(*idx)++] = 0x00000000;  // Immediate Data LOW
    batch[(*idx)++] = 0x00000000;  // Immediate Data HIGH
    
    return 0;
}


/**
 * CYCLE C599 - GÉNÉRATION BATCH COMPLET AUTOMATIQUE
 * Wrapper qui génère toutes les commandes validées (C597-C598)
 */

/**
 * Générer batch buffer complet automatiquement
 * Basé sur structure OpenCL C545 (13 commandes identifiées)
 *
 * Structure batch générée:
 * 1. PIPELINE_SELECT GPGPU
 * 2. STATE_BASE_ADDRESS (19 DWords) - C597
 * 3. MEDIA_VFE_STATE (9 DWords) - C597
 * 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWords) - C598
 * 5. GPGPU_WALKER (15 DWords) - C598
 * 6. MI_BATCH_BUFFER_END
 *
 * Total: ~47 DWords (188 bytes)
 * Note: OpenCL utilise 6 STATE_BASE_ADDRESS + 3 BINDING_TABLE_POOL (80 DWords)
 *       Pour C599, on génère version simplifiée avec commandes validées
 */
int batch_generator_generate(batch_generator_t *gen) {
    if (!gen || !gen->initialized) {
        fprintf(stderr, "[AUTO-ERROR] Générateur non initialisé\n");
        return -1;
    }
    
    printf("\n[AUTO] ========================================\n");
    printf("[AUTO] GÉNÉRATION BATCH AUTOMATIQUE C599\n");
    printf("[AUTO] ========================================\n\n");
    
    uint32_t *batch = gen->batch_data;
    uint32_t idx = 0;
    
    // 1. PIPELINE_SELECT GPGPU (1 DWord)
    printf("[AUTO] [1/6] Génération PIPELINE_SELECT GPGPU...\n");
    batch[idx++] = 0x69041312;  // PIPELINE_SELECT GPGPU (valeur OpenCL C545)
    printf("[AUTO]   ✓ PIPELINE_SELECT: 1 DWord @ offset %u\n", idx - 1);
    
    // 2. STATE_BASE_ADDRESS (19 DWords) - C597
    printf("[AUTO] [2/6] Génération STATE_BASE_ADDRESS...\n");
    uint32_t sba_start = idx;
    if (generate_state_base_address(gen, batch, &idx) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec génération STATE_BASE_ADDRESS\n");
        return -1;
    }
    printf("[AUTO]   ✓ STATE_BASE_ADDRESS: %u DWords @ offset %u\n",
           idx - sba_start, sba_start);
    
    // 3. MEDIA_VFE_STATE (9 DWords) - C597
    printf("[AUTO] [3/6] Génération MEDIA_VFE_STATE...\n");
    uint32_t vfe_start = idx;
    if (generate_media_vfe_state(gen, batch, &idx) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec génération MEDIA_VFE_STATE\n");
        return -1;
    }
    printf("[AUTO]   ✓ MEDIA_VFE_STATE: %u DWords @ offset %u\n",
           idx - vfe_start, vfe_start);
    
    // 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWords) - C598
    printf("[AUTO] [4/6] Génération MEDIA_INTERFACE_DESCRIPTOR_LOAD...\n");
    uint32_t idl_start = idx;
    if (generate_media_interface_descriptor_load(gen, batch, &idx) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec génération MEDIA_INTERFACE_DESCRIPTOR_LOAD\n");
        return -1;
    }
    printf("[AUTO]   ✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD: %u DWords @ offset %u\n",
           idx - idl_start, idl_start);
    
    // 5. GPGPU_WALKER (15 DWords) - C598
    printf("[AUTO] [5/7] Génération GPGPU_WALKER...\n");
    uint32_t walker_start = idx;
    // Dimensions par défaut: 1x1x1 thread group (comme OpenCL C545)
    if (generate_gpgpu_walker(gen, batch, &idx, 1, 1, 1) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec génération GPGPU_WALKER\n");
        return -1;
    }
    printf("[AUTO]   ✓ GPGPU_WALKER: %u DWords @ offset %u\n",
           idx - walker_start, walker_start);
    
    // 6. PIPE_CONTROL (6 DWords) - C604 ROOT CAUSE #154 FIX
    printf("[AUTO] [6/7] Génération PIPE_CONTROL (ROOT CAUSE #154 FIX)...\n");
    uint32_t pipe_start = idx;
    if (generate_pipe_control(gen, batch, &idx) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec génération PIPE_CONTROL\n");
        return -1;
    }
    printf("[AUTO]   ✓ PIPE_CONTROL: %u DWords @ offset %u\n",
           idx - pipe_start, pipe_start);
    printf("[AUTO]   ℹ️  ROOT CAUSE #154: Synchronisation cache GPU ajoutée\n");
    
    // 7. MI_BATCH_BUFFER_END (1 DWord)
    printf("[AUTO] [7/7] Ajout MI_BATCH_BUFFER_END...\n");
    batch[idx++] = MI_BATCH_BUFFER_END;
    printf("[AUTO]   ✓ MI_BATCH_BUFFER_END: 1 DWord @ offset %u\n", idx - 1);
    
    // Sauvegarder nombre de DWords
    gen->batch_dwords = idx;
    gen->batch_generated = true;
    
    printf("\n[AUTO] ========================================\n");
    printf("[AUTO] BATCH GÉNÉRÉ AVEC SUCCÈS (C604)\n");
    printf("[AUTO] ========================================\n");
    printf("[AUTO] Total: %u DWords (%u bytes)\n", gen->batch_dwords, gen->batch_dwords * 4);
    printf("[AUTO] \n");
    printf("[AUTO] Détail commandes:\n");
    printf("[AUTO]   1. PIPELINE_SELECT:     1 DWord\n");
    printf("[AUTO]   2. STATE_BASE_ADDRESS: 19 DWords\n");
    printf("[AUTO]   3. MEDIA_VFE_STATE:     9 DWords\n");
    printf("[AUTO]   4. MEDIA_IDL:           3 DWords\n");
    printf("[AUTO]   5. GPGPU_WALKER:       15 DWords\n");
    printf("[AUTO]   6. PIPE_CONTROL:        6 DWords ← ROOT CAUSE #154 FIX\n");
    printf("[AUTO]   7. MI_BATCH_END:        1 DWord\n");
    printf("[AUTO]   --------------------------------\n");
    printf("[AUTO]   TOTAL:                 54 DWords\n");
    printf("[AUTO] \n");
    printf("[AUTO] 🔧 ROOT CAUSE #154 RÉSOLU:\n");
    printf("[AUTO]    PIPE_CONTROL ajouté pour synchronisation cache GPU\n");
    printf("[AUTO]    Sans PIPE_CONTROL, résultats restent dans cache GPU\n");
    printf("[AUTO] ========================================\n\n");
    
    // Dump batch pour debug
    dump_batch_hex(batch, gen->batch_dwords);
    
    return 0;
}

/**
 * Soumettre batch au GPU avec configuration automatique
 * Configure automatiquement EXECBUFFER2 comme OpenCL (C585)
 */
int batch_generator_submit(batch_generator_t *gen) {
    if (!gen || !gen->batch_generated) {
        fprintf(stderr, "[AUTO-ERROR] Batch non généré\n");
        return -1;
    }
    
    printf("\n[AUTO] ========================================\n");
    printf("[AUTO] SOUMISSION BATCH AU GPU\n");
    printf("[AUTO] ========================================\n\n");
    
    // Copier batch dans buffer BATCH
    auto_buffer_t *batch_buf = &gen->buffers[BUFFER_TYPE_BATCH];
    memcpy(batch_buf->cpu_addr, gen->batch_data, gen->batch_dwords * 4);
    printf("[AUTO] ✓ Batch copié dans buffer BATCH (%u bytes)\n", gen->batch_dwords * 4);
    
    // Configurer exec_objects (12 buffers avec soft-pinning)
    struct drm_i915_gem_exec_object2 exec_objects[OPENCL_BUFFER_COUNT];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    for (uint32_t i = 0; i < gen->buffer_count; i++) {
        exec_objects[i].handle = gen->buffers[i].handle;
        exec_objects[i].offset = gen->buffers[i].offset;
        exec_objects[i].flags = gen->buffers[i].flags;  // 0x18 = PINNED + 48B
    }
    
    // Configurer EXECBUFFER2 (comme OpenCL C585)
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = gen->buffer_count,
        .batch_start_offset = 0,
        .batch_len = gen->batch_dwords * 4,
        .flags = gen->exec_flags,  // 0x800 = I915_EXEC_NO_RELOC
        .rsvd1 = gen->ctx_id,
        .rsvd2 = 0
    };
    
    // Dump configuration pour debug
    dump_execbuffer2_config(gen);
    
    // Soumettre au GPU
    printf("[AUTO] Soumission EXECBUFFER2 au GPU...\n");
    if (ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec EXECBUFFER2: %s\n", strerror(errno));
        return -1;
    }
    
    printf("[AUTO] ✓ EXECBUFFER2 accepté par le GPU!\n");
    printf("[AUTO] ========================================\n\n");
    
    return 0;
}

/**
 * Attendre fin exécution GPU
 */
int batch_generator_wait(batch_generator_t *gen) {
    if (!gen) return -1;
    
    printf("[AUTO] Attente fin exécution GPU...\n");
    
    // Attendre batch buffer (handle 0)
    struct drm_i915_gem_wait wait_arg = {
        .bo_handle = gen->buffers[BUFFER_TYPE_BATCH].handle,
        .timeout_ns = 10000000000ULL,  // 10 secondes
        .flags = 0
    };
    
    if (ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg) < 0) {
        fprintf(stderr, "[AUTO-ERROR] Échec attente GPU: %s\n", strerror(errno));
        return -1;
    }
    
    printf("[AUTO] ✓ GPU terminé\n");
    return 0;
}

/**
 * Lire résultats depuis buffer output
 *
 * ROOT CAUSE #150 FIX (C601): Cache Coherency GPU→CPU
 * PROBLÈME: Données restent dans cache GPU, pas visibles CPU
 * SOLUTION: I915_GEM_SET_DOMAIN force flush cache GPU avant lecture CPU
 *
 * Référence: PROTOCOLE_TEST_CUMULATIF_DRM.md TEST_009 (lignes 189-202)
 * Référence: Analyse strace OpenCL C601 (clEnqueueReadBuffer fait automatiquement)
 */
int batch_generator_read_results(batch_generator_t *gen, void *output, size_t size) {
    if (!gen || !output) return -1;
    
    auto_buffer_t *output_buf = &gen->buffers[BUFFER_TYPE_OUTPUT];
    
    if (size > output_buf->size) {
        fprintf(stderr, "[AUTO-ERROR] Taille output trop grande\n");
        return -1;
    }
    
    // ROOT CAUSE #150 FIX: Forcer transition GPU→CPU domain
    // Ceci flush les caches GPU et rend les données visibles au CPU
    printf("[C601-FIX] ROOT CAUSE #150: Flush cache GPU→CPU...\n");
    struct drm_i915_gem_set_domain set_domain = {
        .handle = output_buf->handle,
        .read_domains = I915_GEM_DOMAIN_CPU,  // Données doivent être en domaine CPU
        .write_domain = 0                      // Pas d'écriture CPU
    };
    
    if (ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) < 0) {
        fprintf(stderr, "[C601-ERROR] Échec I915_GEM_SET_DOMAIN: %s\n", strerror(errno));
        fprintf(stderr, "[C601-ERROR] Les données GPU peuvent être dans le cache!\n");
        return -1;
    }
    
    printf("[C601-FIX] ✓ Cache GPU flushé, données visibles CPU\n");
    
    // MAINTENANT lire résultats (caches GPU flushés)
    memcpy(output, output_buf->cpu_addr, size);
    return 0;
}

/**
 * ============================================================================
 * CYCLE C600 - CONFIGURATION GPU STRUCTURES
 * ============================================================================
 * Implémentation des fonctions de configuration IDRT, Surface State, Binding Table
 * pour permettre l'exécution réelle de kernels GPU
 */

/**
 * Configurer Interface Descriptor Table (IDRT)
 * 
 * Structure IDRT (32 bytes = 8 DWords):
 * DW0: Kernel Start Pointer [31:6] (bits 5:0 = 0)
 * DW1: Kernel Start Pointer [63:32]
 * DW2: [31:16] = 0, [15:11] = Sampler Count, [10:6] = Binding Table Entry Count, [5:0] = CURBE Read Length
 * DW3: Sampler State Pointer [31:5] (bits 4:0 = 0)
 * DW4: Binding Table Pointer [15:5] (bits 4:0 = 0)
 * DW5: CURBE Read Offset [15:0], Number of Threads [25:16]
 * DW6: Shared Local Memory Size, Barrier Enable, etc.
 * DW7: Cross-Thread Constant Data Read Length
 */
int configure_idrt(batch_generator_t *gen) {
    if (!gen || !gen->initialized) {
        fprintf(stderr, "[C600-ERROR] Générateur non initialisé\n");
        return -1;
    }
    
    printf("\n[C600] ========================================\n");
    printf("[C600] CONFIGURATION IDRT\n");
    printf("[C600] ========================================\n\n");
    
    // Récupérer buffer IDRT
    auto_buffer_t *idrt_buf = &gen->buffers[BUFFER_TYPE_IDRT];
    uint32_t *idrt = (uint32_t *)idrt_buf->cpu_addr;
    memset(idrt, 0, 32);  // 8 DWords = 32 bytes
    
    // Récupérer offsets GTT
    uint64_t kernel_gtt = gen->buffers[BUFFER_TYPE_KERNEL].offset;
    
    // DW0-1: Kernel Start Pointer (64-bit, aligné 64 bytes)
    idrt[0] = (uint32_t)(kernel_gtt & 0xFFFFFFC0);  // Bits 31:6 seulement
    idrt[1] = (uint32_t)(kernel_gtt >> 32);
    
    // DW2: Configuration
    // [15:11] = Sampler Count = 0 (pas de sampler)
    // [10:6]  = Binding Table Entry Count = 1 (1 surface)
    // [5:0]   = CURBE Read Length = 2 (2 DWords = 8 bytes pour adresse 64-bit)
    // ROOT CAUSE #149 FIX: Le kernel OpenCL attend l'adresse du buffer en argument!
    idrt[2] = (1 << 6) | 2;  // 1 binding table entry + CURBE read length 2
    
    // DW3: Sampler State Pointer = 0 (pas de sampler)
    idrt[3] = 0;
    
    // DW4: Binding Table Pointer (offset relatif depuis Surface State Base)
    // CORRECTION C601: Offset EN DWORDS, pas en bytes!
    // ROOT CAUSE #146: Binding Table DOIT être dans les 8KB suivant Surface State!
    // Bits 5-15 = offset en DWords (max 2047 DWords = 8188 bytes)
    // La Binding Table doit être JUSTE APRÈS Surface State, pas à +1MB!
    
    // SOLUTION: Placer Binding Table immédiatement après Surface State
    // Surface State = 64 bytes = 16 DWords
    // Binding Table commence à offset 16 DWords (64 bytes)
    uint32_t bt_offset_dwords = 16;  // 64 bytes après Surface State Base
    idrt[4] = (bt_offset_dwords << 5) & 0x0000FFE0;  // Bits 5-15, aligné 32 bytes
    
    // DW5: Number of Threads
    // [25:16] = Number of Threads = 1 (pour test simple)
    // [15:0]  = CURBE Read Offset = 0
    idrt[5] = (1 << 16);  // 1 thread
    
    // DW6: Shared Local Memory Size = 0, Barrier Enable = 0
    idrt[6] = 0;
    
    // DW7: Cross-Thread Constant Data Read Length = 0
    // (Les arguments sont dans CURBE per-thread, pas cross-thread)
    idrt[7] = 0;
    
    // ROOT CAUSE #149 FIX: Configurer CURBE avec adresse du buffer OUTPUT
    auto_buffer_t *curbe_buf = &gen->buffers[BUFFER_TYPE_CURBE];
    uint64_t *curbe = (uint64_t *)curbe_buf->cpu_addr;
    uint64_t output_gtt = gen->buffers[BUFFER_TYPE_OUTPUT].offset;
    curbe[0] = output_gtt;  // Argument 0: adresse du buffer output
    
    printf("[C600] ✓ CURBE configuré:\n");
    printf("[C600]   Argument 0 (output*): 0x%016lx\n", output_gtt);
    
    printf("[C600] ✓ IDRT configuré (32 bytes):\n");
    printf("[C600]   Kernel GTT:        0x%016lx\n", kernel_gtt);
    printf("[C600]   Binding Table:     offset 0x%08x DWords (64 bytes depuis Surface State Base)\n", bt_offset_dwords);
    printf("[C600]   Num Threads:       1\n");
    printf("[C600]   Binding Entries:   1\n");
    
    // Dump IDRT pour debug
    printf("[C600] \n[C600] IDRT Dump (8 DWords):\n");
    for (int i = 0; i < 8; i++) {
        printf("[C600]   DW%d: 0x%08x\n", i, idrt[i]);
    }
    printf("[C600] ========================================\n\n");
    
    return 0;
}

/**
 * Configurer Surface State pour buffer output
 * 
 * Structure Surface State (64 bytes = 16 DWords):
 * DW0: Surface Type, Surface Format
 * DW1: Surface Base Address [31:0]
 * DW2: Surface Base Address [63:32]
 * DW3: Width [13:0]
 * DW4: Height [13:0]
 * DW5: Depth [10:0]
 * DW6: Surface Pitch [17:0]
 * DW7-15: Autres paramètres
 */
int configure_surface_state(batch_generator_t *gen, uint32_t surface_index) {
    if (!gen || !gen->initialized) {
        fprintf(stderr, "[C600-ERROR] Générateur non initialisé\n");
        return -1;
    }
    
    printf("\n[C600] ========================================\n");
    printf("[C600] CONFIGURATION SURFACE STATE #%u\n", surface_index);
    printf("[C600] ========================================\n\n");
    
    // Récupérer buffer SURFACE_STATE
    auto_buffer_t *ss_buf = &gen->buffers[BUFFER_TYPE_SURFACE_STATE];
    uint32_t *surface_state = (uint32_t *)ss_buf->cpu_addr;
    
    // Offset pour cette surface (64 bytes par surface)
    uint32_t *ss = &surface_state[surface_index * 16];
    memset(ss, 0, 64);
    
    // Récupérer buffer OUTPUT
    auto_buffer_t *output_buf = &gen->buffers[BUFFER_TYPE_OUTPUT];
    uint64_t output_gtt = output_buf->offset;
    uint32_t output_size = output_buf->size;
    
    // DW0: Surface Type = BUFFER (0x0), Surface Format = RAW (0x1FF)
    // CORRECTION C601: Activer Surface Array Enable (bit 31) pour UNTYPED_SURFACE_WRITE
    // [31] = Surface Array Enable = 1 (requis pour UNTYPED operations)
    // [30:29] = Surface Type = 0 (BUFFER)
    // [27:18] = Surface Format = 0x1FF (RAW)
    ss[0] = (0x1 << 31) | (0x0 << 29) | (0x1FF << 18);
    
    // DW1-2: Surface Base Address (64-bit GTT)
    ss[1] = (uint32_t)(output_gtt & 0xFFFFFFFF);
    ss[2] = (uint32_t)(output_gtt >> 32);
    
    // DW3: Width = output_size - 1 (en bytes)
    ss[3] = output_size - 1;
    
    // DW4: Height = 0 (buffer 1D)
    ss[4] = 0;
    
    // DW5: Depth = 0, Surface Pitch = 0 (calculé automatiquement)
    ss[5] = 0;
    
    // DW6: Surface Pitch = 0 (pour RAW buffer)
    ss[6] = 0;
    
    // DW7-15: Réservés / autres paramètres
    for (int i = 7; i < 16; i++) {
        ss[i] = 0;
    }
    
    printf("[C600] ✓ Surface State #%u configuré (64 bytes):\n", surface_index);
    printf("[C600]   Type:              BUFFER\n");
    printf("[C600]   Format:            RAW\n");
    printf("[C600]   Base Address:      0x%016lx\n", output_gtt);
    printf("[C600]   Size:              %u bytes\n", output_size);
    
    // Dump Surface State pour debug
    printf("[C600] \n[C600] Surface State Dump (16 DWords):\n");
    for (int i = 0; i < 16; i++) {
        printf("[C600]   DW%d: 0x%08x\n", i, ss[i]);
    }
    printf("[C600] ========================================\n\n");
    
    return 0;
}

/**
 * Configurer Binding Table
 *
 * ROOT CAUSE #146 FIX: Binding Table DOIT être < 8KB depuis Surface State Base
 * Solution: Placer Binding Table DANS buffer Surface State, juste après Surface State #0
 *
 * Layout buffer SURFACE_STATE:
 * - Offset 0-63:    Surface State #0 (64 bytes = 16 DWords)
 * - Offset 64-67:   Binding Table Entry 0 (4 bytes) <- ICI!
 *
 * Chaque entrée = offset relatif (en DWords) depuis Surface State Base
 */
int configure_binding_table(batch_generator_t *gen) {
    if (!gen || !gen->initialized) {
        fprintf(stderr, "[C600-ERROR] Générateur non initialisé\n");
        return -1;
    }
    
    printf("\n[C600] ========================================\n");
    printf("[C600] CONFIGURATION BINDING TABLE\n");
    printf("[C600] ========================================\n\n");
    
    // ROOT CAUSE #146 FIX: Binding Table DANS buffer Surface State!
    // Placer à offset 64 bytes (16 DWords) après le début
    auto_buffer_t *ss_buf = &gen->buffers[BUFFER_TYPE_SURFACE_STATE];
    uint32_t *surface_state_buffer = (uint32_t *)ss_buf->cpu_addr;
    uint32_t *binding_table = surface_state_buffer + 16;  // +16 DWords = +64 bytes
    
    // Entry 0: Offset vers Surface State #0 (en DWords)
    // Offset = 0 DWords (première surface à offset 0)
    binding_table[0] = 0x00000000;
    
    printf("[C600] ✓ Binding Table configurée:\n");
    printf("[C600]   Entry 0: offset 0x%08x DWords (Surface State #0)\n", binding_table[0]);
    printf("[C600]   Placée à offset 64 bytes dans buffer Surface State\n");
    printf("[C600]   (Respecte contrainte: < 8KB depuis Surface State Base)\n");
    printf("[C600] ========================================\n\n");
    
    return 0;
}

// Made with Bob
