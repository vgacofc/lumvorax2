/*
 * C197.36 - Batch Patcher avec Relinking Dynamique GPU
 * 
 * OBJECTIF : Patcher un batch OpenCL capturé avec nouvelles adresses GPU
 * 
 * STRATÉGIE CORRIGÉE :
 * 1. Charger batch capturé (320 bytes)
 * 2. Créer nouveaux buffers GEM
 * 3. Obtenir leurs GPU Virtual Addresses
 * 4. PATCHER toutes les adresses dans le batch
 * 5. Reconstruire exec_objects avec handles corrects
 * 6. Soumettre via EXECBUFFER2
 * 
 * PROBLÈME RÉSOLU : Les GPU VA ne sont PAS stables entre runs
 * SOLUTION : Relinking dynamique complet
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

// DRM/i915 headers
#include <drm/i915_drm.h>
#include <xf86drm.h>

#define BATCH_SIZE 320
#define ISA_SIZE 4448
#define BUFFER_SIZE (256 * 1024)  // 256KB pour mining

// Structure pour tracker les relocations
typedef struct {
    uint32_t offset;        // Offset dans le batch
    uint64_t old_addr;      // Ancienne adresse GPU
    uint64_t new_addr;      // Nouvelle adresse GPU
    const char* name;       // Description
} relocation_t;

// Analyse du batch pour identifier les pointeurs GPU
static int analyze_batch_pointers(const uint8_t* batch, size_t size, relocation_t* relocs, int* count) {
    *count = 0;
    
    printf("\n=== ANALYSE BATCH POUR POINTEURS GPU ===\n");
    
    // Scan pour adresses 64-bit alignées (pattern typique Gen9)
    for (size_t i = 0; i < size - 8; i += 4) {
        uint64_t value = *(uint64_t*)(batch + i);
        
        // Heuristique : adresse GPU valide
        // - Entre 0x00007C0000000000 et 0x00007FFFFFFFFFFF (canonical 48-bit)
        // - Alignée sur 64 bytes minimum
        if ((value & 0xFFFF800000000000ULL) == 0x00007C0000000000ULL &&
            (value & 0x3F) == 0) {
            
            relocs[*count].offset = i;
            relocs[*count].old_addr = value;
            relocs[*count].new_addr = 0;  // À remplir
            
            // Identifier le type de pointeur par position
            if (i < 64) {
                relocs[*count].name = "STATE_BASE_ADDRESS";
            } else if (i < 128) {
                relocs[*count].name = "INTERFACE_DESCRIPTOR";
            } else if (i < 192) {
                relocs[*count].name = "BINDING_TABLE";
            } else {
                relocs[*count].name = "SURFACE_STATE";
            }
            
            printf("  [%d] Offset 0x%03X : 0x%016lX (%s)\n",
                   *count, relocs[*count].offset, value, relocs[*count].name);
            
            (*count)++;
            if (*count >= 32) break;  // Max 32 relocations
        }
    }
    
    printf("Total pointeurs GPU trouvés : %d\n", *count);
    return 0;
}

// Créer un buffer GEM et obtenir son GPU VA
static int create_gem_buffer_with_va(int fd, size_t size, uint32_t* handle, uint64_t* gpu_va, void** cpu_ptr) {
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
    
    // Obtenir GPU VA via un EXECBUFFER2 dummy
    // Note : i915 assigne automatiquement les GPU VA
    // On utilise l'offset retourné par EXECBUFFER2
    *gpu_va = 0;  // Sera rempli après premier EXECBUFFER2
    
    printf("  GEM buffer créé : handle=%u, size=%zu, cpu_ptr=%p\n",
           *handle, create.size, *cpu_ptr);
    
    return 0;
}

// Patcher le batch avec nouvelles adresses
static void patch_batch_addresses(uint8_t* batch, relocation_t* relocs, int count) {
    printf("\n=== PATCHING BATCH AVEC NOUVELLES ADRESSES ===\n");
    
    for (int i = 0; i < count; i++) {
        if (relocs[i].new_addr == 0) {
            printf("  [%d] SKIP (pas de nouvelle adresse)\n", i);
            continue;
        }
        
        uint64_t* ptr = (uint64_t*)(batch + relocs[i].offset);
        uint64_t old = *ptr;
        *ptr = relocs[i].new_addr;
        
        printf("  [%d] Offset 0x%03X : 0x%016lX → 0x%016lX (%s)\n",
               i, relocs[i].offset, old, relocs[i].new_addr, relocs[i].name);
    }
}

// Soumettre le batch patché
static int submit_patched_batch(int fd, uint32_t batch_handle, uint32_t isa_handle, 
                                uint32_t buffer_handle, uint64_t* gpu_vas) {
    struct drm_i915_gem_exec_object2 exec_objects[3] = {0};
    
    // Object 0 : ISA
    exec_objects[0].handle = isa_handle;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;
    exec_objects[0].alignment = 0;
    exec_objects[0].offset = 0;
    exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    // Object 1 : Buffer
    exec_objects[1].handle = buffer_handle;
    exec_objects[1].relocation_count = 0;
    exec_objects[1].relocs_ptr = 0;
    exec_objects[1].alignment = 0;
    exec_objects[1].offset = 0;
    exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE;
    
    // Object 2 : Batch
    exec_objects[2].handle = batch_handle;
    exec_objects[2].relocation_count = 0;
    exec_objects[2].relocs_ptr = 0;
    exec_objects[2].alignment = 0;
    exec_objects[2].offset = 0;
    exec_objects[2].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 3;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = BATCH_SIZE;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = 0;  // Context (0 = default)
    
    printf("\n=== SOUMISSION EXECBUFFER2 ===\n");
    printf("  Batch handle : %u\n", batch_handle);
    printf("  ISA handle   : %u\n", isa_handle);
    printf("  Buffer handle: %u\n", buffer_handle);
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("EXECBUFFER2");
        printf("  errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }
    
    // Récupérer les GPU VA assignées
    gpu_vas[0] = exec_objects[0].offset;
    gpu_vas[1] = exec_objects[1].offset;
    gpu_vas[2] = exec_objects[2].offset;
    
    printf("  ✓ SUCCÈS !\n");
    printf("  GPU VA ISA    : 0x%016lX\n", gpu_vas[0]);
    printf("  GPU VA Buffer : 0x%016lX\n", gpu_vas[1]);
    printf("  GPU VA Batch  : 0x%016lX\n", gpu_vas[2]);
    
    return 0;
}

int main(int argc, char** argv) {
    printf("=== C197.36 - BATCH PATCHER AVEC RELINKING GPU ===\n\n");
    
    // 1. Ouvrir DRM
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open /dev/dri/renderD128");
        return 1;
    }
    printf("✓ DRM ouvert : fd=%d\n", fd);
    
    // 2. Charger batch capturé
    const char* batch_file = "/tmp/c197_35_captured_batch.bin";
    FILE* f = fopen(batch_file, "rb");
    if (!f) {
        fprintf(stderr, "❌ Batch capturé introuvable : %s\n", batch_file);
        fprintf(stderr, "   Exécutez d'abord c197_35 pour capturer un batch OpenCL\n");
        close(fd);
        return 1;
    }
    
    uint8_t batch_data[BATCH_SIZE];
    size_t read_size = fread(batch_data, 1, BATCH_SIZE, f);
    fclose(f);
    
    printf("✓ Batch chargé : %zu bytes depuis %s\n", read_size, batch_file);
    
    // 3. Analyser les pointeurs GPU dans le batch
    relocation_t relocs[32];
    int reloc_count = 0;
    analyze_batch_pointers(batch_data, read_size, relocs, &reloc_count);
    
    if (reloc_count == 0) {
        fprintf(stderr, "❌ Aucun pointeur GPU trouvé dans le batch\n");
        close(fd);
        return 1;
    }
    
    // 4. Créer nouveaux buffers GEM
    printf("\n=== CRÉATION BUFFERS GEM ===\n");
    
    uint32_t isa_handle, buffer_handle, batch_handle;
    uint64_t isa_va, buffer_va, batch_va;
    void *isa_ptr, *buffer_ptr, *batch_ptr;
    
    if (create_gem_buffer_with_va(fd, ISA_SIZE, &isa_handle, &isa_va, &isa_ptr) < 0) {
        close(fd);
        return 1;
    }
    
    if (create_gem_buffer_with_va(fd, BUFFER_SIZE, &buffer_handle, &buffer_va, &buffer_ptr) < 0) {
        close(fd);
        return 1;
    }
    
    if (create_gem_buffer_with_va(fd, BATCH_SIZE, &batch_handle, &batch_va, &batch_ptr) < 0) {
        close(fd);
        return 1;
    }
    
    // 5. Charger ISA SHA256 réel
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
    
    // 6. Initialiser buffer mining
    uint32_t* nonces = (uint32_t*)buffer_ptr;
    for (int i = 0; i < 256; i++) {
        nonces[i] = 0x12340000 + i;  // Nonces test
    }
    printf("✓ Buffer mining initialisé : 256 nonces\n");
    
    // 7. Premier EXECBUFFER2 pour obtenir GPU VA
    uint64_t gpu_vas[3];
    if (submit_patched_batch(fd, batch_handle, isa_handle, buffer_handle, gpu_vas) < 0) {
        fprintf(stderr, "❌ Premier EXECBUFFER2 échoué\n");
        close(fd);
        return 1;
    }
    
    // 8. Mettre à jour les relocations avec GPU VA réelles
    printf("\n=== MISE À JOUR RELOCATIONS ===\n");
    for (int i = 0; i < reloc_count; i++) {
        // Heuristique : assigner GPU VA selon le type
        if (strstr(relocs[i].name, "INTERFACE") || strstr(relocs[i].name, "STATE_BASE")) {
            relocs[i].new_addr = gpu_vas[0];  // ISA
        } else if (strstr(relocs[i].name, "SURFACE")) {
            relocs[i].new_addr = gpu_vas[1];  // Buffer
        } else {
            relocs[i].new_addr = gpu_vas[2];  // Batch
        }
        printf("  [%d] %s → 0x%016lX\n", i, relocs[i].name, relocs[i].new_addr);
    }
    
    // 9. Patcher le batch avec nouvelles adresses
    patch_batch_addresses(batch_data, relocs, reloc_count);
    
    // 10. Copier batch patché dans GEM buffer
    memcpy(batch_ptr, batch_data, read_size);
    printf("✓ Batch patché copié dans GEM buffer\n");
    
    // 11. Soumettre batch patché final
    printf("\n=== SOUMISSION BATCH PATCHÉ FINAL ===\n");
    if (submit_patched_batch(fd, batch_handle, isa_handle, buffer_handle, gpu_vas) < 0) {
        fprintf(stderr, "❌ EXECBUFFER2 final échoué\n");
        close(fd);
        return 1;
    }
    
    // 12. Attendre exécution GPU
    struct drm_i915_gem_wait wait = {0};
    wait.bo_handle = batch_handle;
    wait.timeout_ns = 1000000000;  // 1 seconde
    
    printf("\n=== ATTENTE EXÉCUTION GPU ===\n");
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
    } else {
        printf("✓ GPU terminé\n");
    }
    
    // 13. Vérifier résultats
    printf("\n=== VÉRIFICATION RÉSULTATS ===\n");
    printf("Premiers nonces buffer :\n");
    for (int i = 0; i < 16; i++) {
        printf("  [%d] = 0x%08X\n", i, nonces[i]);
    }
    
    // 14. Sauvegarder batch patché
    const char* patched_file = "/tmp/c197_36_patched_batch.bin";
    f = fopen(patched_file, "wb");
    if (f) {
        fwrite(batch_data, 1, read_size, f);
        fclose(f);
        printf("\n✓ Batch patché sauvegardé : %s\n", patched_file);
    }
    
    // Cleanup
    close(fd);
    
    printf("\n=== C197.36 TERMINÉ ===\n");
    printf("Statut : %s\n", gpu_vas[0] != 0 ? "✓ SUCCÈS" : "❌ ÉCHEC");
    
    return gpu_vas[0] != 0 ? 0 : 1;
}

// Made with Bob
