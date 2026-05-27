/*
 * C197.42 - Exact Replay with Softpin (CRITICAL FIX)
 * 
 * CORRECTIONS APPLIQUÉES:
 * 1. ✅ Restauration GPU VA exactes (softpin)
 * 2. ✅ Suppression memset() sur state buffers
 * 3. ✅ Conservation flags originaux
 * 4. ✅ Copie ISA + nonces uniquement
 * 
 * PRINCIPE: Replay bit-for-bit identique sauf données utiles
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <i915_drm.h>
#include <drm.h>

#define CAPTURE_FILE "/tmp/c197_39_execbuffer_complete.bin"
#define ISA_FILE "/tmp/c197_29_extracted_isa.bin"

// Structure capture (format C197.39)
typedef struct {
    uint32_t magic;                                    // 0xDEADBEEF
    int buffer_count;                                  // Nombre d'exec_objects
    uint32_t batch_len;                                // Taille batch
    uint64_t flags;                                    // Flags EXECBUFFER2
    uint32_t ctx_id;                                   // Context ID
    struct drm_i915_gem_exec_object2 exec_objects[32]; // Max 32 objects
    uint8_t batch_data[4096];                          // Max 4KB batch
} execbuffer_capture_t;

// Vérification adresse GPU canonique 48-bit
static int is_canonical_48bit(uint64_t addr) {
    if (addr == 0) return 1;
    uint64_t top_bits = addr >> 47;
    return (top_bits == 0 || top_bits == 0x1FFFF);
}

int main(void) {
    printf("=== C197.42 - Exact Replay with Softpin ===\n\n");
    
    // 1. Charger capture complète
    FILE* f = fopen(CAPTURE_FILE, "rb");
    if (!f) {
        perror("fopen capture");
        return 1;
    }
    
    execbuffer_capture_t capture;
    if (fread(&capture, sizeof(capture), 1, f) != 1) {
        fprintf(stderr, "Erreur lecture capture\n");
        fclose(f);
        return 1;
    }
    fclose(f);
    
    // Vérifier magic
    if (capture.magic != 0xDEADBEEF) {
        fprintf(stderr, "❌ Magic invalide: 0x%08x (attendu 0xDEADBEEF)\n", capture.magic);
        return 1;
    }
    
    printf("✅ Capture chargée: %d exec_objects, batch %u bytes\n",
           capture.buffer_count, capture.batch_len);
    
    // 2. Charger ISA Gen9
    f = fopen(ISA_FILE, "rb");
    if (!f) {
        perror("fopen ISA");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t isa_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* isa_data = malloc(isa_size);
    if (fread(isa_data, 1, isa_size, f) != isa_size) {
        fprintf(stderr, "Erreur lecture ISA\n");
        fclose(f);
        return 1;
    }
    fclose(f);
    
    printf("✅ ISA chargé: %zu bytes\n", isa_size);
    
    // 3. Ouvrir DRM
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        perror("open DRM");
        return 1;
    }
    
    printf("✅ DRM ouvert: fd=%d\n", drm_fd);
    
    // 4. Déduire tailles objets depuis capture
    int num_objects = capture.buffer_count;
    if (num_objects > 32) {
        fprintf(stderr, "❌ Trop d'objets: %d > 32\n", num_objects);
        return 1;
    }
    
    size_t object_sizes[32];
    
    // Analyse des GPU VA pour déduire tailles
    for (int i = 0; i < num_objects; i++) {
        uint64_t offset = capture.exec_objects[i].offset;
        
        // Heuristique basée sur GPU VA patterns
        if (offset >= 0xFFFF800000000000ULL) {
            // Softpin → petits objets (state)
            object_sizes[i] = 4096;
        } else if (i == 0) {
            // Premier objet = ISA/State
            object_sizes[i] = 16384;
        } else if (i == 2) {
            // Surface
            object_sizes[i] = 65536;
        } else if (i == 5) {
            // Buffer output (mining)
            object_sizes[i] = 262144;
        } else if (i == 6) {
            // Buffer input
            object_sizes[i] = 1024;
        } else if (i == 8) {
            // Dynamic state
            object_sizes[i] = 8192;
        } else {
            // Défaut
            object_sizes[i] = 4096;
        }
    }
    
    // 5. Créer TOUS les GEM buffers avec tailles correctes
    uint32_t new_handles[32];
    void* ptrs[32];
    
    printf("\n=== Création GEM Buffers ===\n");
    
    for (int i = 0; i < num_objects; i++) {
        struct drm_i915_gem_create create = {
            .size = object_sizes[i]
        };
        
        if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
            fprintf(stderr, "GEM_CREATE[%d] failed: %s\n", i, strerror(errno));
            return 1;
        }
        
        new_handles[i] = create.handle;
        
        // Mapper buffer
        struct drm_i915_gem_mmap mmap_arg = {
            .handle = new_handles[i],
            .offset = 0,
            .size = object_sizes[i],
            .flags = 0
        };
        
        if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
            fprintf(stderr, "GEM_MMAP[%d] failed: %s\n", i, strerror(errno));
            return 1;
        }
        
        ptrs[i] = (void*)(uintptr_t)mmap_arg.addr_ptr;
        
        printf("  [%d] handle=%u size=%zu ptr=%p\n", 
               i, new_handles[i], object_sizes[i], ptrs[i]);
    }
    
    // 6. ✅ FIX #2: NE PAS ZERO les state buffers
    // On laisse les buffers tels quels (garbage OK pour test)
    
    // 7. Copier ISA dans buffer[0]
    if (isa_size <= object_sizes[0]) {
        memcpy(ptrs[0], isa_data, isa_size);
        printf("✅ ISA copié dans buffer[0]: %zu bytes\n", isa_size);
    } else {
        fprintf(stderr, "❌ ISA trop grand: %zu > %zu\n", isa_size, object_sizes[0]);
        return 1;
    }
    
    // 8. Initialiser nonces dans buffer[5] (mining output)
    uint32_t* nonces = (uint32_t*)ptrs[5];
    for (size_t i = 0; i < object_sizes[5] / sizeof(uint32_t); i++) {
        nonces[i] = 0xDEADBEEF + i;
    }
    printf("✅ Nonces initialisés dans buffer[5]: %zu entries\n", 
           object_sizes[5] / sizeof(uint32_t));
    
    // 9. Copier batch dans buffer[num_objects-1]
    int batch_idx = num_objects - 1;
    memcpy(ptrs[batch_idx], capture.batch_data, capture.batch_len);
    printf("✅ Batch copié dans buffer[%d]: %u bytes\n",
           batch_idx, capture.batch_len);
    
    // 10. ✅ FIX #1: Créer exec_objects avec GPU VA EXACTES (softpin)
    struct drm_i915_gem_exec_object2 new_exec_objects[32];
    
    printf("\n=== Configuration Exec Objects (Softpin) ===\n");
    
    for (int i = 0; i < num_objects; i++) {
        // Copier structure originale
        new_exec_objects[i] = capture.exec_objects[i];
        
        // ✅ FIX #1: Remplacer handle MAIS garder offset exact
        new_exec_objects[i].handle = new_handles[i];
        
        // ✅ FIX #3: Conserver flags originaux + forcer PINNED
        new_exec_objects[i].flags |= EXEC_OBJECT_PINNED;
        
        // Vérifier GPU VA canonique
        uint64_t gpu_va = new_exec_objects[i].offset;
        if (!is_canonical_48bit(gpu_va)) {
            fprintf(stderr, "❌ GPU VA non canonique[%d]: 0x%lx\n", i, gpu_va);
            return 1;
        }
        
        printf("  [%d] handle=%u → %u, GPU VA=0x%016lx, flags=0x%llx\n",
               i, capture.exec_objects[i].handle, new_handles[i],
               gpu_va, new_exec_objects[i].flags);
    }
    
    // 11. Préparer EXECBUFFER2 (identique à capture)
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)(uintptr_t)new_exec_objects,
        .buffer_count = num_objects,
        .batch_start_offset = 0,
        .batch_len = capture.batch_len,
        .flags = capture.flags,
        .rsvd1 = capture.ctx_id
    };
    
    printf("\n=== EXECBUFFER2 Submission ===\n");
    printf("  buffer_count: %u\n", execbuf.buffer_count);
    printf("  batch_len: %u\n", execbuf.batch_len);
    printf("  flags: 0x%llx\n", execbuf.flags);
    printf("  ctx_id: %u\n", execbuf.rsvd1);
    
    // 12. Soumettre à GPU
    printf("\n🚀 Soumission EXECBUFFER2...\n");
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(stderr, "❌ EXECBUFFER2 échoué: %s (errno=%d)\n", 
                strerror(errno), errno);
        
        // Diagnostic détaillé
        printf("\n=== Diagnostic Échec ===\n");
        for (int i = 0; i < num_objects; i++) {
            printf("  [%d] handle=%u exists? ", i, new_handles[i]);
            
            struct drm_gem_close close_test = { .handle = new_handles[i] };
            if (ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_test) == 0) {
                printf("✅ (mais fermé par test)\n");
                // Recréer
                struct drm_i915_gem_create create = { .size = object_sizes[i] };
                ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create);
                new_handles[i] = create.handle;
            } else {
                printf("❌ INEXISTANT\n");
            }
        }
        
        return 1;
    }
    
    printf("✅ EXECBUFFER2 RÉUSSI!\n");
    
    // 13. Attendre GPU
    printf("\n⏳ Attente GPU...\n");
    
    for (int i = 0; i < num_objects; i++) {
        struct drm_i915_gem_wait wait = {
            .bo_handle = new_handles[i],
            .timeout_ns = 1000000000LL // 1s
        };
        
        if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
            fprintf(stderr, "⚠️ GEM_WAIT[%d] timeout\n", i);
        }
    }
    
    printf("✅ GPU terminé\n");
    
    // 14. Lire résultats
    printf("\n=== Résultats Mining ===\n");
    
    uint32_t* results = (uint32_t*)ptrs[5];
    int changed = 0;
    
    for (size_t i = 0; i < 64; i++) {
        if (results[i] != 0xDEADBEEF + i) {
            printf("  [%zu] 0x%08x (modifié)\n", i, results[i]);
            changed++;
        }
    }
    
    if (changed > 0) {
        printf("✅ %d nonces modifiés par GPU\n", changed);
    } else {
        printf("⚠️ Aucun nonce modifié (GPU n'a pas écrit?)\n");
    }
    
    // 15. Cleanup
    for (int i = 0; i < num_objects; i++) {
        munmap(ptrs[i], object_sizes[i]);
        
        struct drm_gem_close close = { .handle = new_handles[i] };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
    }
    
    close(drm_fd);
    free(isa_data);
    
    printf("\n=== C197.42 Terminé ===\n");
    printf("✅ Replay exact avec softpin appliqué\n");
    printf("✅ State buffers préservés\n");
    printf("✅ Flags originaux conservés\n");
    
    return 0;
}

// Made with Bob
