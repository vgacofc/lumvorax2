#ifndef SLAB_ALLOCATOR_H
#define SLAB_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#define SLAB_VERSION_MAJOR 1
#define SLAB_VERSION_MINOR 0
#define SLAB_VERSION_PATCH 0

#define SLAB_MIN_SIZE 8
#define SLAB_MAX_SIZE 4096
#define SLAB_ALIGNMENT 16
#define SLAB_PAGE_SIZE 65536
#define SLAB_MAX_CACHES 32

typedef struct slab_object {
    struct slab_object* next;
} slab_object_t;

typedef struct slab_page {
    struct slab_page* next;
    struct slab_page* prev;
    void* memory;
    size_t object_size;
    size_t objects_per_page;
    size_t free_count;
    slab_object_t* free_list;
    uint64_t allocation_bitmap[64];
} slab_page_t;

typedef struct {
    size_t object_size;
    size_t alignment;
    slab_page_t* partial_pages;
    slab_page_t* full_pages;
    slab_page_t* empty_pages;
    pthread_mutex_t lock;
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t active_objects;
    uint64_t pages_allocated;
} slab_cache_t;

typedef struct {
    slab_cache_t* caches[SLAB_MAX_CACHES];
    size_t cache_count;
    pthread_mutex_t global_lock;
    uint64_t total_memory_used;
    uint64_t peak_memory_used;
    bool initialized;
} slab_allocator_t;

slab_allocator_t* slab_allocator_create(void);
void slab_allocator_destroy(slab_allocator_t* allocator);
slab_cache_t* slab_cache_create(slab_allocator_t* allocator, size_t object_size, size_t alignment);
void slab_cache_destroy(slab_cache_t* cache);
void* slab_alloc(slab_cache_t* cache);
void slab_free(slab_cache_t* cache, void* ptr);
void* slab_alloc_size(slab_allocator_t* allocator, size_t size);
void slab_free_size(slab_allocator_t* allocator, void* ptr, size_t size);
void slab_allocator_stats(slab_allocator_t* allocator, uint64_t* total_allocs, 
                          uint64_t* total_frees, uint64_t* memory_used);
void slab_cache_shrink(slab_cache_t* cache);

#endif
