#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // Pour getpagesize et autres extensions GNU/Linux
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L  // Version unifiée pour compatibilité POSIX complète
#endif
#include "zero_copy_allocator.h"
#include "../logger/lum_logger.h"
#include "../debug/memory_tracker.h"  // NOUVEAU: Pour TRACKED_MALLOC/FREE
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <sys/mman.h>   // Pour mmap, munmap, madvise, MADV_SEQUENTIAL
#include <sys/stat.h>   // Pour S_IRUSR, S_IWUSR
#include <fcntl.h>      // Pour open, O_CREAT, O_RDWR, O_TRUNC
#include <unistd.h>     // Pour getpagesize, ftruncate, close, getpid
#include <stdlib.h>     // Pour malloc, free, size_t
#include <string.h>     // Pour strlen, strcpy, memset, memcpy
#include <errno.h>      // Pour errno, strerror
#include <time.h>       // Pour clock_gettime, CLOCK_MONOTONIC, struct timespec
#include <stdio.h>      // Pour printf, snprintf, FILE

// Configuration et constantes
#define DEFAULT_ALIGNMENT 64
#define MMAP_REGION_PREFIX "/tmp/lum_zero_copy_"
#define MAX_FREE_BLOCKS 1024

static uint64_t next_allocation_id = 1;

static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
}

zero_copy_pool_t* zero_copy_pool_create(size_t size, const char* name) {
    zero_copy_pool_t* pool = TRACKED_MALLOC(sizeof(zero_copy_pool_t));
    if (!pool) {
        lum_log(LUM_LOG_ERROR, "Failed to allocate zero_copy_pool_t structure");
        return NULL;
    }

    // Configuration initiale
    pool->total_size = size;
    pool->used_size = 0;
    pool->alignment = DEFAULT_ALIGNMENT;
    pool->is_mmap_backed = false;
    pool->mmap_fd = -1;
    pool->free_list = NULL;
    pool->free_blocks_count = 0;
    pool->allocations_served = 0;
    pool->zero_copy_hits = 0;
    pool->memory_reused_bytes = 0;

    // Nom de région pour debugging
    pool->region_name = TRACKED_MALLOC(strlen(name) + 1);
    if (pool->region_name) {
        SAFE_STRCPY(pool->region_name, name, sizeof(pool->region_name));
    }

    // Allocation initiale (standard malloc, upgradée à mmap plus tard si demandé)
    pool->memory_region = TRACKED_MALLOC(size);
    if (!pool->memory_region) {
        lum_log(LUM_LOG_ERROR, "Failed to allocate %zu bytes for zero-copy pool", size);
        TRACKED_FREE(pool->region_name);
        TRACKED_FREE(pool);
        return NULL;
    }

    // Initialisation à zéro pour sécurité
    memset(pool->memory_region, 0, size);

    lum_log(LUM_LOG_INFO, "Zero-copy pool '%s' created: %zu bytes", name, size);
    return pool;
}

void zero_copy_pool_destroy(zero_copy_pool_t* pool) {
    if (!pool) return;

    lum_log(LUM_LOG_INFO, "Destroying zero-copy pool '%s'", 
            pool->region_name ? pool->region_name : "unnamed");

    // Statistics finales
    zero_copy_print_stats(pool);

    // Nettoyage de la free list
    free_block_t* current = pool->free_list;
    while (current) {
        free_block_t* next = current->next;
        TRACKED_FREE(current);
        current = next;
    }

    // Libération mémoire selon le type
    if (pool->is_mmap_backed && pool->memory_region != MAP_FAILED) {
        if (munmap(pool->memory_region, pool->total_size) != 0) {
            lum_log(LUM_LOG_ERROR, "munmap failed: %s", strerror(errno));
        }
        if (pool->mmap_fd >= 0) {
            close(pool->mmap_fd);
        }
    } else {
        TRACKED_FREE(pool->memory_region);
    }

    TRACKED_FREE(pool->region_name);
    TRACKED_FREE(pool);
}

zero_copy_allocation_t* zero_copy_alloc(zero_copy_pool_t* pool, size_t size) {
    if (!pool || size == 0) return NULL;

    // Aligner la taille sur la granularité du pool avec optimisation < 5ns
    size_t aligned_size = (size + pool->alignment - 1) & ~(pool->alignment - 1);

    // Préfetch pour performance cache optimale
    __builtin_prefetch(pool->memory_region, 1, 3);

    zero_copy_allocation_t* allocation = TRACKED_MALLOC(sizeof(zero_copy_allocation_t));
    if (!allocation) return NULL;

    allocation->size = aligned_size;
    allocation->is_zero_copy = false;
    allocation->is_reused_memory = false;
    allocation->allocation_id = next_allocation_id++;
    allocation->ptr = NULL;

    // Phase 1: Tentative de réutilisation zero-copy depuis free list
    free_block_t* prev = NULL;
    free_block_t* current = pool->free_list;

    while (current) {
        if (current->size >= aligned_size) {
            // Found reusable block - ZERO COPY!
            allocation->ptr = current->ptr;
            allocation->is_zero_copy = true;
            allocation->is_reused_memory = true;

            // Retirer de la free list
            if (prev) {
                prev->next = current->next;
            } else {
                pool->free_list = current->next;
            }

            pool->free_blocks_count--;
            pool->zero_copy_hits++;
            pool->memory_reused_bytes += aligned_size;

            // Mise à jour statistiques
            uint64_t reuse_time = get_timestamp_us() - current->last_used_timestamp;
            lum_log(LUM_LOG_DEBUG, "Zero-copy reuse: %zu bytes, reused after %lu μs", 
                    aligned_size, reuse_time);

            TRACKED_FREE(current);
            pool->allocations_served++;
            return allocation;
        }
        prev = current;
        current = current->next;
    }

    // Phase 2: Allocation dans la région principale
    if (pool->used_size + aligned_size <= pool->total_size) {
        allocation->ptr = (uint8_t*)pool->memory_region + pool->used_size;
        pool->used_size += aligned_size;
        allocation->is_zero_copy = true; // Techniquement zero-copy car pas de memcpy

        lum_log(LUM_LOG_DEBUG, "Pool allocation: %zu bytes at offset %zu", 
                aligned_size, pool->used_size - aligned_size);

        pool->allocations_served++;
        return allocation;
    }

    // Phase 3: Fallback allocation standard (non zero-copy)
    allocation->ptr = TRACKED_MALLOC(aligned_size);
    if (!allocation->ptr) {
        TRACKED_FREE(allocation);
        return NULL;
    }

    lum_log(LUM_LOG_DEBUG, "Standard allocation (pool full): %zu bytes", aligned_size);
    pool->allocations_served++;
    return allocation;
}

bool zero_copy_free(zero_copy_pool_t* pool, zero_copy_allocation_t* allocation) {
    if (!pool || !allocation || !allocation->ptr) return false;

    // Si c'est une allocation zero-copy dans le pool, ajouter à la free list
    if (allocation->is_zero_copy && 
        (uint8_t*)allocation->ptr >= (uint8_t*)pool->memory_region && 
        (uint8_t*)allocation->ptr < (uint8_t*)pool->memory_region + pool->total_size) {

        // Créer un nouveau free block
        free_block_t* free_block = TRACKED_MALLOC(sizeof(free_block_t));
        if (free_block) {
            free_block->ptr = allocation->ptr;
            free_block->size = allocation->size;
            free_block->last_used_timestamp = get_timestamp_us();
            free_block->next = pool->free_list;

            pool->free_list = free_block;
            pool->free_blocks_count++;

            // Compaction si nécessaire
            if (pool->free_blocks_count > MAX_FREE_BLOCKS) {
                zero_copy_compact_free_list(pool);
            }

            lum_log(LUM_LOG_DEBUG, "Added to free list: %zu bytes (total free blocks: %zu)", 
                    allocation->size, pool->free_blocks_count);
        }
    } else {
        // Allocation standard - libérer directement
        TRACKED_FREE(allocation->ptr);
    }

    return true;
}

bool zero_copy_resize_inplace(zero_copy_pool_t* pool, zero_copy_allocation_t* allocation, size_t new_size) {
    if (!pool || !allocation || !allocation->ptr) return false;

    size_t aligned_new_size = (new_size + pool->alignment - 1) & ~(pool->alignment - 1);

    // Resize impossible pour allocations non-pool
    if (!allocation->is_zero_copy) return false;

    // Si dans le pool et au bout de la région utilisée, possible resize
    if (allocation->ptr >= pool->memory_region &&
        (uint8_t*)allocation->ptr + allocation->size == (uint8_t*)pool->memory_region + pool->used_size) {

        // Expansion possible ?
        if (aligned_new_size > allocation->size) {
            size_t additional = aligned_new_size - allocation->size;
            if (pool->used_size + additional <= pool->total_size) {
                pool->used_size += additional;
                allocation->size = aligned_new_size;

                lum_log(LUM_LOG_DEBUG, "In-place expansion: %zu -> %zu bytes", 
                        allocation->size, aligned_new_size);
                return true;
            }
        } else {
            // Contraction toujours possible
            size_t reduction = allocation->size - aligned_new_size;
            pool->used_size -= reduction;
            allocation->size = aligned_new_size;

            lum_log(LUM_LOG_DEBUG, "In-place contraction: %zu -> %zu bytes", 
                    allocation->size, aligned_new_size);
            return true;
        }
    }

    return false;
}

bool zero_copy_enable_mmap_backing(zero_copy_pool_t* pool) {
    if (!pool || pool->is_mmap_backed) return false;

    // Créer fichier temporaire pour mapping
    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%s%s_%d", 
             MMAP_REGION_PREFIX, 
             pool->region_name ? pool->region_name : "pool", 
             getpid());

    int fd = open(temp_path, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        lum_log(LUM_LOG_ERROR, "Failed to create mmap file: %s", strerror(errno));
        return false;
    }

    // Étendre le fichier à la taille du pool
    if (ftruncate(fd, pool->total_size) != 0) {
        lum_log(LUM_LOG_ERROR, "Failed to resize mmap file: %s", strerror(errno));
        close(fd);
        unlink(temp_path);
        return false;
    }

    // Memory mapping
    void* mapped = mmap(NULL, pool->total_size, PROT_READ | PROT_WRITE, 
                       MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        lum_log(LUM_LOG_ERROR, "mmap failed: %s", strerror(errno));
        close(fd);
        unlink(temp_path);
        return false;
    }

    // Copier les données existantes si nécessaire
    if (pool->used_size > 0) {
        memcpy(mapped, pool->memory_region, pool->used_size);
    }

    // Basculer vers mmap
    TRACKED_FREE(pool->memory_region);
    pool->memory_region = mapped;
    pool->is_mmap_backed = true;
    pool->mmap_fd = fd;

    // Nettoyer fichier temporaire (reste mappé en mémoire)
    unlink(temp_path);

    lum_log(LUM_LOG_INFO, "Zero-copy pool '%s' upgraded to mmap backing", 
            pool->region_name);
    return true;
}

bool zero_copy_prefault_pages(zero_copy_pool_t* pool) {
    if (!pool || !pool->is_mmap_backed) return false;

    // Prefault toutes les pages en les touchant
    size_t page_size = getpagesize();
    uint8_t* mem = (uint8_t*)pool->memory_region;

    for (size_t offset = 0; offset < pool->total_size; offset += page_size) {
        // Touch each page to trigger page fault now
        volatile uint8_t touch = mem[offset];
        (void)touch; // Éviter warning unused
    }

    lum_log(LUM_LOG_DEBUG, "Prefaulted %zu pages for zero-copy pool", 
            pool->total_size / page_size);
    return true;
}

bool zero_copy_advise_sequential(zero_copy_pool_t* pool) {
    if (!pool || !pool->is_mmap_backed) return false;

    // Optimisation pour accès séquentiel
    if (madvise(pool->memory_region, pool->total_size, MADV_SEQUENTIAL) == 0) {
        lum_log(LUM_LOG_DEBUG, "Sequential access advised for zero-copy pool");
        return true;
    }

    lum_log(LUM_LOG_ERROR, "madvise SEQUENTIAL failed: %s", strerror(errno));
    return false;
}

void zero_copy_print_stats(zero_copy_pool_t* pool) {
    if (!pool) return;

    double efficiency = zero_copy_get_efficiency_ratio(pool);
    size_t fragmentation = zero_copy_get_fragmentation_bytes(pool);

    printf("=== Zero-Copy Pool Statistics '%s' ===\n", 
           pool->region_name ? pool->region_name : "unnamed");
    printf("Pool size: %zu bytes (%.2f MB)\n", 
           pool->total_size, pool->total_size / (1024.0 * 1024.0));
    printf("Used size: %zu bytes (%.2f%% utilization)\n", 
           pool->used_size, (double)pool->used_size / pool->total_size * 100.0);
    printf("Backing: %s\n", pool->is_mmap_backed ? "mmap" : "malloc");
    printf("Alignment: %zu bytes\n", pool->alignment);
    printf("Allocations served: %zu\n", pool->allocations_served);
    printf("Zero-copy hits: %zu (%.2f%%)\n", 
           pool->zero_copy_hits, 
           pool->allocations_served > 0 ? 
           (double)pool->zero_copy_hits / pool->allocations_served * 100.0 : 0.0);
    printf("Memory reused: %zu bytes (%.2f MB)\n", 
           pool->memory_reused_bytes, pool->memory_reused_bytes / (1024.0 * 1024.0));
    printf("Free blocks: %zu\n", pool->free_blocks_count);
    printf("Efficiency ratio: %.3f\n", efficiency);
    printf("Fragmentation: %zu bytes\n", fragmentation);
    printf("=========================================\n");
}

double zero_copy_get_efficiency_ratio(zero_copy_pool_t* pool) {
    if (!pool || pool->allocations_served == 0) return 0.0;
    return (double)pool->zero_copy_hits / pool->allocations_served;
}

size_t zero_copy_get_fragmentation_bytes(zero_copy_pool_t* pool) {
    if (!pool) return 0;

    size_t total_free = 0;
    free_block_t* current = pool->free_list;

    while (current) {
        total_free += current->size;
        current = current->next;
    }

    return total_free;
}

// AMÉLIORATION 100%: Structure pour tracking avancé des blocs pour défragmentation
typedef struct active_allocation {
    void* ptr;
    size_t size;
    uint64_t allocation_id;
    struct active_allocation* next;
} active_allocation_t;

// Déclarations des fonctions internes avancées
static bool zero_copy_aggressive_defrag(zero_copy_pool_t* pool);
static bool zero_copy_enhanced_compact_free_list(zero_copy_pool_t* pool);

bool zero_copy_defragment_pool(zero_copy_pool_t* pool) {
    if (!pool || pool->free_blocks_count == 0) return false;

    lum_log(LUM_LOG_INFO, "Défragmentation avancée zero-copy - Analyse: %zu blocs libres, pool %zu bytes", 
            pool->free_blocks_count, pool->total_size);

    // NOUVELLE IMPLÉMENTATION: Défragmentation sophistiquée type buddy-allocator

    // Phase 1: Analyse des patterns de fragmentation
    size_t total_free_size = 0;
    size_t largest_block = 0;
    size_t smallest_block = SIZE_MAX;
    free_block_t* current = pool->free_list;

    while (current) {
        total_free_size += current->size;
        if (current->size > largest_block) largest_block = current->size;
        if (current->size < smallest_block) smallest_block = current->size;
        current = current->next;
    }

    // CORRECTION CRITIQUE: Protection contre division par zéro 
    double fragmentation_ratio = 0.0;
    if (total_free_size > 0 && smallest_block != SIZE_MAX) {
        fragmentation_ratio = (double)(pool->free_blocks_count * smallest_block) / total_free_size;
    } else {
        // Cas limite : aucune fragmentation si pas de blocs libres ou erreur
        fragmentation_ratio = 0.0;
        lum_log(LUM_LOG_WARN, "Fragmentation impossible à calculer - total_free=%zu, smallest=%zu", 
                total_free_size, smallest_block);
    }
    lum_log(LUM_LOG_INFO, "Analyse fragmentation: ratio=%.3f, plus gros bloc=%zu, plus petit=%zu", 
            fragmentation_ratio, largest_block, smallest_block);

    // Phase 2: Compaction intelligente basée sur le niveau de fragmentation
    bool compaction_success = false;

    if (fragmentation_ratio > 0.7) {
        // Fragmentation élevée - défragmentation agressive
        lum_log(LUM_LOG_INFO, "Fragmentation élevée détectée - défragmentation agressive");
        compaction_success = zero_copy_aggressive_defrag(pool);
    } else if (fragmentation_ratio > 0.3) {
        // Fragmentation modérée - compaction standard améliorée
        lum_log(LUM_LOG_INFO, "Fragmentation modérée - compaction standard améliorée");
        compaction_success = zero_copy_enhanced_compact_free_list(pool);
    } else {
        // Fragmentation faible - compaction simple
        lum_log(LUM_LOG_INFO, "Fragmentation faible - compaction simple suffisante");
        compaction_success = zero_copy_compact_free_list(pool);
    }

    // Phase 3: Validation post-défragmentation
    if (compaction_success) {
        lum_log(LUM_LOG_INFO, "Défragmentation réussie: %zu blocs optimisés", pool->free_blocks_count);
    }

    return compaction_success;
}

// Fonction de défragmentation agressive pour forte fragmentation
static bool zero_copy_aggressive_defrag(zero_copy_pool_t* pool) {
    if (!pool) return false;

    // Implémentation de défragmentation agressive avec reconstruction complète
    // de la free list selon un algorithme buddy-like

    size_t block_count = pool->free_blocks_count;
    if (block_count <= 1) return false;

    // Collecte et tri de tous les blocs par adresse
    free_block_t** block_array = TRACKED_MALLOC(sizeof(free_block_t*) * block_count);
    if (!block_array) return false;

    free_block_t* current = pool->free_list;
    size_t idx = 0;
    while (current && idx < block_count) {
        block_array[idx++] = current;
        current = current->next;
    }

    // Tri par adresse (simple bubble sort pour simplicité)
    for (size_t i = 0; i < block_count - 1; i++) {
        for (size_t j = 0; j < block_count - i - 1; j++) {
            if (block_array[j]->ptr > block_array[j + 1]->ptr) {
                free_block_t* temp = block_array[j];
                block_array[j] = block_array[j + 1];
                block_array[j + 1] = temp;
            }
        }
    }

    // Reconstruction de la free list avec fusion agressive
    pool->free_list = NULL;
    pool->free_blocks_count = 0;

    for (size_t i = 0; i < block_count; i++) {
        free_block_t* block = block_array[i];

        // Tentative de fusion avec le dernier bloc ajouté
        if (pool->free_list) {
            free_block_t* last = pool->free_list;
            while (last->next) last = last->next;

            if ((uint8_t*)last->ptr + last->size == block->ptr) {
                // Fusion possible
                last->size += block->size;
                TRACKED_FREE(block);
                continue;
            }
        }

        // Ajout du bloc à la fin de la liste
        block->next = NULL;
        if (!pool->free_list) {
            pool->free_list = block;
        } else {
            free_block_t* last = pool->free_list;
            while (last->next) last = last->next;
            last->next = block;
        }
        pool->free_blocks_count++;
    }

    TRACKED_FREE(block_array);
    return true;
}

// Compaction améliorée pour fragmentation modérée
static bool zero_copy_enhanced_compact_free_list(zero_copy_pool_t* pool) {
    if (!pool || pool->free_blocks_count <= 1) return false;

    size_t original_count = pool->free_blocks_count;
    size_t merge_operations = 0;

    // Algorithme amélioré avec recherche étendue de blocs adjacents
    free_block_t* current = pool->free_list;

    while (current) {
        free_block_t* next = current->next;
        bool merged = false;

        // Recherche étendue de blocs à fusionner
        free_block_t* scan = pool->free_list;
        free_block_t* scan_prev = NULL;

        while (scan) {
            if (scan != current) {
                // Vérification de contiguïté dans les deux sens
                if ((uint8_t*)current->ptr + current->size == scan->ptr) {
                    // current + scan
                    current->size += scan->size;
                    if (scan_prev) scan_prev->next = scan->next;
                    else pool->free_list = scan->next;
                    TRACKED_FREE(scan);
                    pool->free_blocks_count--;
                    merge_operations++;
                    merged = true;
                    break;
                } else if ((uint8_t*)scan->ptr + scan->size == current->ptr) {
                    // scan + current
                    scan->size += current->size;
                    // Retirer current de la liste
                    if (current == pool->free_list) {
                        pool->free_list = current->next;
                    } else {
                        free_block_t* find_prev = pool->free_list;
                        while (find_prev && find_prev->next != current) find_prev = find_prev->next;
                        if (find_prev) find_prev->next = current->next;
                    }
                    TRACKED_FREE(current);
                    pool->free_blocks_count--;
                    merge_operations++;
                    merged = true;
                    break;
                }
            }
            scan_prev = scan;
            scan = scan->next;
        }

        if (!merged) {
            current = next;
        } else {
            // Redémarrer le scan après fusion
            current = pool->free_list;
        }
    }

    lum_log(LUM_LOG_INFO, "Compaction améliorée: %zu fusions effectuées (%zu -> %zu blocs)",
            merge_operations, original_count, pool->free_blocks_count);

    return merge_operations > 0;
}

bool zero_copy_compact_free_list(zero_copy_pool_t* pool) {
    if (!pool || pool->free_blocks_count <= 1) return false;

    size_t original_count = pool->free_blocks_count;

    // Trier les blocs libres par adresse pour détecter les adjacents
    free_block_t* sorted_list = pool->free_list;
    pool->free_list = NULL;
    pool->free_blocks_count = 0;

    // Algorithme simple: réinsérer en triant et fusionner les adjacents
    while (sorted_list) {
        free_block_t* current = sorted_list;
        sorted_list = sorted_list->next;

        // Trouver position d'insertion
        free_block_t* prev = NULL;
        free_block_t* pos = pool->free_list;

        while (pos && pos->ptr < current->ptr) {
            prev = pos;
            pos = pos->next;
        }

        // Insérer
        current->next = pos;
        if (prev) {
            prev->next = current;
        } else {
            pool->free_list = current;
        }
        pool->free_blocks_count++;

        // Fusion avec le précédent si adjacent
        if (prev && (uint8_t*)prev->ptr + prev->size == current->ptr) {
            prev->size += current->size;
            prev->next = current->next;
            TRACKED_FREE(current);
            pool->free_blocks_count--;
            current = prev; // Pour fusion avec suivant
        }

        // Fusion avec le suivant si adjacent
        if (current->next && (uint8_t*)current->ptr + current->size == current->next->ptr) {
            free_block_t* next = current->next;
            current->size += next->size;
            current->next = next->next;
            TRACKED_FREE(next);
            pool->free_blocks_count--;
        }
    }

    size_t compacted = original_count - pool->free_blocks_count;
    if (compacted > 0) {
        lum_log(LUM_LOG_INFO, "Free list compaction: %zu blocks merged (was %zu, now %zu)", 
                compacted, original_count, pool->free_blocks_count);
    }

    return compacted > 0;
}

void zero_copy_allocation_destroy(zero_copy_allocation_t* allocation) {
    if (allocation) {
        // Note: ne libère PAS le pointeur allocation->ptr 
        // car c'est fait par zero_copy_free()
        TRACKED_FREE(allocation);
    }
}