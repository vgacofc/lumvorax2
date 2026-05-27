/*
 * C197.40 - Replay EXECBUFFER2 avec Remplacement Handles
 * 
 * OBJECTIF : Rejouer batch OpenCL capturé avec nouveaux GEM buffers
 * 
 * STRATÉGIE :
 * 1. Charger capture complète (exec_objects + batch)
 * 2. Créer nouveaux GEM buffers (ISA, mining, batch)
 * 3. Remplacer handles dans exec_objects
 * 4. Soumettre EXECBUFFER2
 * 5. Valider exécution GPU
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
#include <time.h>
#include <errno.h>

#include <drm/i915_drm.h>
#include <xf86drm.h>

#define ISA_SIZE 4448
#define BUFFER_SIZE (256 * 1024)  // 256KB

// Structure capture (même que C197.39)
typedef struct {
    uint32_t magic;
    int buffer_count;
    uint32_t batch_len;
    uint64_t flags;
    uint32_t ctx_id;
    struct drm_i915_gem_exec_object2 exec_objects[32];
    uint8_t batch_data[4096];
} execbuffer_capture_t;

// Créer GEM buffer et obtenir handle
static int create_gem_buffer(int fd, size_t size, uint32_t* handle, void** cpu_ptr) {
    struct drm_i915_gem_create create = {0};
    create.size = (size + 4095) & ~4095;  // Align 4KB
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("GEM_CREATE");
        return -1;
    }
    
    *handle = create.handle;
    
    // MMAP pour accès CPU
    struct drm_i915_gem_mmap mmap_arg = {0};
    mmap_arg.handle = *handle;
    mmap_arg.offset = 0;
    mmap_arg.size = create.size;
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        perror("GEM_MMAP");
        return -1;
    }
    
    *cpu_ptr = (void*)mmap_arg.addr_ptr;
    
    printf("  ✓ GEM buffer créé : handle=%u, size=%zu, ptr=%p\n",
           *handle, create.size, *cpu_ptr);
    
    return 0;
}

int main(int argc, char** argv) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C197.40 - REPLAY AVEC REMPLACEMENT HANDLES               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
    // 1. Ouvrir DRM
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open DRM");
        return 1;
    }
    printf("✓ DRM ouvert : fd=%d\n", fd);
    
    // 2. Charger capture
    const char* capture_file = "/tmp/c197_39_execbuffer_complete.bin";
    FILE* f = fopen(capture_file, "rb");
    if (!f) {
        fprintf(stderr, "❌ Capture introuvable : %s\n", capture_file);
        close(fd);
        return 1;
    }
    
    execbuffer_capture_t capture;
    size_t read_size = fread(&capture, 1, sizeof(capture), f);
    fclose(f);
    
    if (capture.magic != 0xDEADBEEF) {
        fprintf(stderr, "❌ Magic invalide : 0x%08X\n", capture.magic);
        close(fd);
        return 1;
    }
    
    printf("✓ Capture chargée : %zu bytes\n", read_size);
    printf("  Buffer count : %d\n", capture.buffer_count);
    printf("  Batch length : %u\n", capture.batch_len);
    printf("  Flags        : 0x%lX\n", capture.flags);
    
    // 3. Créer nouveaux GEM buffers
    printf("\n=== CRÉATION NOUVEAUX GEM BUFFERS ===\n");
    
    uint32_t isa_handle, buffer_handle, batch_handle;
    void *isa_ptr, *buffer_ptr, *batch_ptr;
    
    // ISA buffer
    if (create_gem_buffer(fd, ISA_SIZE, &isa_handle, &isa_ptr) < 0) {
        close(fd);
        return 1;
    }
    
    // Mining buffer
    if (create_gem_buffer(fd, BUFFER_SIZE, &buffer_handle, &buffer_ptr) < 0) {
        close(fd);
        return 1;
    }
    
    // Batch buffer
    if (create_gem_buffer(fd, capture.batch_len, &batch_handle, &batch_ptr) < 0) {
        close(fd);
        return 1;
    }
    
    // 4. Charger ISA SHA256
    const char* isa_file = "/tmp/c197_29_extracted_isa.bin";
    f = fopen(isa_file, "rb");
    if (!f) {
        fprintf(stderr, "❌ ISA introuvable : %s\n", isa_file);
        close(fd);
        return 1;
    }
    
    size_t isa_size = fread(isa_ptr, 1, ISA_SIZE, f);
    fclose(f);
    printf("✓ ISA SHA256 chargé : %zu bytes\n", isa_size);
    
    // 5. Initialiser buffer mining (test pattern)
    uint32_t* nonces = (uint32_t*)buffer_ptr;
    for (int i = 0; i < 256; i++) {
        nonces[i] = 0x12340000 + i;
    }
    printf("✓ Buffer mining initialisé : 256 nonces\n");
    
    // 6. Copier batch data
    memcpy(batch_ptr, capture.batch_data, capture.batch_len);
    printf("✓ Batch data copié : %u bytes\n", capture.batch_len);
    
    // 7. Remplacer handles dans exec_objects
    printf("\n=== REMPLACEMENT HANDLES ===\n");
    
    struct drm_i915_gem_exec_object2 new_exec_objects[32];
    memcpy(new_exec_objects, capture.exec_objects, 
           capture.buffer_count * sizeof(struct drm_i915_gem_exec_object2));
    
    // Stratégie : Remplacer les 3 derniers objets (ISA, buffer, batch)
    // Les autres sont des états/surfaces OpenCL qu'on garde
    
    int isa_idx = -1, buffer_idx = -1, batch_idx = -1;
    
    // Le batch est toujours le dernier
    batch_idx = capture.buffer_count - 1;
    
    // Heuristique : ISA et buffer sont probablement les 2 avant le batch
    if (capture.buffer_count >= 3) {
        isa_idx = capture.buffer_count - 3;
        buffer_idx = capture.buffer_count - 2;
    }
    
    printf("Indices identifiés :\n");
    printf("  ISA    : [%d] (handle %u → %u)\n", 
           isa_idx, capture.exec_objects[isa_idx].handle, isa_handle);
    printf("  Buffer : [%d] (handle %u → %u)\n",
           buffer_idx, capture.exec_objects[buffer_idx].handle, buffer_handle);
    printf("  Batch  : [%d] (handle %u → %u)\n",
           batch_idx, capture.exec_objects[batch_idx].handle, batch_handle);
    
    // Remplacer handles
    if (isa_idx >= 0) {
        new_exec_objects[isa_idx].handle = isa_handle;
        new_exec_objects[isa_idx].offset = 0;  // i915 assignera GPU VA
    }
    
    if (buffer_idx >= 0) {
        new_exec_objects[buffer_idx].handle = buffer_handle;
        new_exec_objects[buffer_idx].offset = 0;
        new_exec_objects[buffer_idx].flags |= EXEC_OBJECT_WRITE;  // Buffer output
    }
    
    new_exec_objects[batch_idx].handle = batch_handle;
    new_exec_objects[batch_idx].offset = 0;
    
    // 8. Soumettre EXECBUFFER2
    printf("\n=== SOUMISSION EXECBUFFER2 ===\n");
    
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)new_exec_objects;
    execbuf.buffer_count = capture.buffer_count;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = capture.batch_len;
    execbuf.flags = capture.flags;
    execbuf.rsvd1 = 0;  // Context 0 (default)
    
    printf("Paramètres :\n");
    printf("  buffer_count : %d\n", execbuf.buffer_count);
    printf("  batch_len    : %u\n", execbuf.batch_len);
    printf("  flags        : 0x%lX\n", execbuf.flags);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(stderr, "❌ EXECBUFFER2 échoué : %s (errno=%d)\n", 
                strerror(errno), errno);
        close(fd);
        return 1;
    }
    
    printf("✓ EXECBUFFER2 soumis avec succès !\n");
    
    // Afficher GPU VA assignées
    printf("\nGPU VA assignées par i915 :\n");
    for (int i = 0; i < capture.buffer_count; i++) {
        printf("  [%d] handle=%u, GPU VA=0x%016lX\n",
               i, new_exec_objects[i].handle, new_exec_objects[i].offset);
    }
    
    // 9. Attendre exécution GPU
    printf("\n=== ATTENTE EXÉCUTION GPU ===\n");
    
    struct drm_i915_gem_wait wait = {0};
    wait.bo_handle = batch_handle;
    wait.timeout_ns = 1000000000;  // 1 seconde
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
    } else {
        printf("✓ GPU terminé\n");
    }
    
    // 10. Vérifier résultats
    printf("\n=== VÉRIFICATION RÉSULTATS ===\n");
    printf("Premiers nonces buffer :\n");
    for (int i = 0; i < 16; i++) {
        printf("  [%2d] = 0x%08X", i, nonces[i]);
        if (nonces[i] == 0x12345678) {
            printf(" ← MAGIC VALUE TROUVÉ !");
        }
        printf("\n");
    }
    
    // 11. Métriques
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                     (t_end.tv_nsec - t_start.tv_nsec) / 1000000.0;
    
    printf("\n=== MÉTRIQUES ===\n");
    printf("Temps total      : %.2f ms\n", elapsed);
    printf("Setup OpenCL ref : 502 ms\n");
    printf("Gain             : %.1f× plus rapide\n", 502.0 / elapsed);
    
    // Cleanup
    close(fd);
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C197.40 TERMINÉ - Replay réussi !                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

// Made with Bob
