#include "slab_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static slab_page_t* slab_page_create(size_t object_size, size_t alignment) {
    slab_page_t* page = (slab_page_t*)calloc(1, sizeof(slab_page_t));
    if (!page) return NULL;
    
    size_t aligned_size = align_up(object_size, alignment);
    page->objects_per_page = (SLAB_PAGE_SIZE - sizeof(slab_page_t)) / aligned_size;
    if (page->objects_per_page == 0) page->objects_per_page = 1;
    
    size_t total_size = page->objects_per_page * aligned_size;
    if (posix_memalign(&page->memory, alignment, total_size) != 0) {
        free(page);
        return NULL;
    }
    memset(page->memory, 0, total_size);
    
    page->object_size = aligned_size;
    page->free_count = page->objects_per_page;
    page->free_list = NULL;
    
    uint8_t* ptr = (uint8_t*)page->memory;
    for (size_t i = 0; i < page->objects_per_page; i++) {
        slab_object_t* obj = (slab_object_t*)(ptr + i * aligned_size);
        obj->next = page->free_list;
        page->free_list = obj;
    }
    
    return page;
}

static void slab_page_destroy(slab_page_t* page) {
    if (page) {
        free(page->memory);
        free(page);
    }
}

slab_allocator_t* slab_allocator_create(void) {
    slab_allocator_t* allocator = (slab_allocator_t*)calloc(1, sizeof(slab_allocator_t));
    if (!allocator) return NULL;
    
    pthread_mutex_init(&allocator->global_lock, NULL);
    allocator->initialized = true;
    
    return allocator;
}

void slab_allocator_destroy(slab_allocator_t* allocator) {
    if (!allocator) return;
    
    for (size_t i = 0; i < allocator->cache_count; i++) {
        slab_cache_destroy(allocator->caches[i]);
    }
    
    pthread_mutex_destroy(&allocator->global_lock);
    free(allocator);
}

slab_cache_t* slab_cache_create(slab_allocator_t* allocator, size_t object_size, size_t alignment) {
    if (!allocator || allocator->cache_count >= SLAB_MAX_CACHES) return NULL;
    
    if (object_size < SLAB_MIN_SIZE) object_size = SLAB_MIN_SIZE;
    if (alignment == 0) alignment = SLAB_ALIGNMENT;
    
    slab_cache_t* cache = (slab_cache_t*)calloc(1, sizeof(slab_cache_t));
    if (!cache) return NULL;
    
    cache->object_size = align_up(object_size, alignment);
    cache->alignment = alignment;
    pthread_mutex_init(&cache->lock, NULL);
    
    cache->partial_pages = slab_page_create(cache->object_size, alignment);
    if (!cache->partial_pages) {
        pthread_mutex_destroy(&cache->lock);
        free(cache);
        return NULL;
    }
    cache->pages_allocated = 1;
    
    pthread_mutex_lock(&allocator->global_lock);
    allocator->caches[allocator->cache_count++] = cache;
    allocator->total_memory_used += SLAB_PAGE_SIZE;
    pthread_mutex_unlock(&allocator->global_lock);
    
    return cache;
}

void slab_cache_destroy(slab_cache_t* cache) {
    if (!cache) return;
    
    slab_page_t* page = cache->partial_pages;
    while (page) {
        slab_page_t* next = page->next;
        slab_page_destroy(page);
        page = next;
    }
    
    page = cache->full_pages;
    while (page) {
        slab_page_t* next = page->next;
        slab_page_destroy(page);
        page = next;
    }
    
    page = cache->empty_pages;
    while (page) {
        slab_page_t* next = page->next;
        slab_page_destroy(page);
        page = next;
    }
    
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

void* slab_alloc(slab_cache_t* cache) {
    if (!cache) return NULL;
    
    pthread_mutex_lock(&cache->lock);
    
    if (!cache->partial_pages) {
        if (cache->empty_pages) {
            cache->partial_pages = cache->empty_pages;
            cache->empty_pages = cache->empty_pages->next;
            if (cache->partial_pages) cache->partial_pages->next = NULL;
        } else {
            cache->partial_pages = slab_page_create(cache->object_size, cache->alignment);
            if (!cache->partial_pages) {
                pthread_mutex_unlock(&cache->lock);
                return NULL;
            }
            cache->pages_allocated++;
        }
    }
    
    slab_page_t* page = cache->partial_pages;
    slab_object_t* obj = page->free_list;
    
    if (!obj) {
        pthread_mutex_unlock(&cache->lock);
        return NULL;
    }
    
    page->free_list = obj->next;
    page->free_count--;
    
    if (page->free_count == 0) {
        cache->partial_pages = page->next;
        page->next = cache->full_pages;
        if (cache->full_pages) cache->full_pages->prev = page;
        cache->full_pages = page;
    }
    
    cache->total_allocations++;
    cache->active_objects++;
    
    pthread_mutex_unlock(&cache->lock);
    
    return (void*)obj;
}

void slab_free(slab_cache_t* cache, void* ptr) {
    if (!cache || !ptr) return;
    
    pthread_mutex_lock(&cache->lock);
    
    slab_object_t* obj = (slab_object_t*)ptr;
    
    slab_page_t* page = cache->partial_pages;
    while (page) {
        uint8_t* start = (uint8_t*)page->memory;
        uint8_t* end = start + page->objects_per_page * page->object_size;
        if ((uint8_t*)ptr >= start && (uint8_t*)ptr < end) {
            obj->next = page->free_list;
            page->free_list = obj;
            page->free_count++;
            cache->total_frees++;
            cache->active_objects--;
            pthread_mutex_unlock(&cache->lock);
            return;
        }
        page = page->next;
    }
    
    page = cache->full_pages;
    while (page) {
        uint8_t* start = (uint8_t*)page->memory;
        uint8_t* end = start + page->objects_per_page * page->object_size;
        if ((uint8_t*)ptr >= start && (uint8_t*)ptr < end) {
            obj->next = page->free_list;
            page->free_list = obj;
            page->free_count++;
            
            if (page->prev) page->prev->next = page->next;
            else cache->full_pages = page->next;
            if (page->next) page->next->prev = page->prev;
            
            page->next = cache->partial_pages;
            page->prev = NULL;
            if (cache->partial_pages) cache->partial_pages->prev = page;
            cache->partial_pages = page;
            
            cache->total_frees++;
            cache->active_objects--;
            pthread_mutex_unlock(&cache->lock);
            return;
        }
        page = page->next;
    }
    
    pthread_mutex_unlock(&cache->lock);
}

void* slab_alloc_size(slab_allocator_t* allocator, size_t size) {
    if (!allocator) return NULL;
    
    pthread_mutex_lock(&allocator->global_lock);
    
    for (size_t i = 0; i < allocator->cache_count; i++) {
        if (allocator->caches[i]->object_size >= size) {
            pthread_mutex_unlock(&allocator->global_lock);
            return slab_alloc(allocator->caches[i]);
        }
    }
    
    pthread_mutex_unlock(&allocator->global_lock);
    
    slab_cache_t* cache = slab_cache_create(allocator, size, SLAB_ALIGNMENT);
    if (!cache) return NULL;
    
    return slab_alloc(cache);
}

void slab_free_size(slab_allocator_t* allocator, void* ptr, size_t size) {
    if (!allocator || !ptr) return;
    
    pthread_mutex_lock(&allocator->global_lock);
    
    for (size_t i = 0; i < allocator->cache_count; i++) {
        if (allocator->caches[i]->object_size >= size) {
            pthread_mutex_unlock(&allocator->global_lock);
            slab_free(allocator->caches[i], ptr);
            return;
        }
    }
    
    pthread_mutex_unlock(&allocator->global_lock);
}

void slab_allocator_stats(slab_allocator_t* allocator, uint64_t* total_allocs,
                          uint64_t* total_frees, uint64_t* memory_used) {
    if (!allocator) return;
    
    uint64_t allocs = 0, frees = 0, mem = 0;
    
    pthread_mutex_lock(&allocator->global_lock);
    
    for (size_t i = 0; i < allocator->cache_count; i++) {
        slab_cache_t* cache = allocator->caches[i];
        allocs += cache->total_allocations;
        frees += cache->total_frees;
        mem += cache->pages_allocated * SLAB_PAGE_SIZE;
    }
    
    pthread_mutex_unlock(&allocator->global_lock);
    
    if (total_allocs) *total_allocs = allocs;
    if (total_frees) *total_frees = frees;
    if (memory_used) *memory_used = mem;
}

void slab_cache_shrink(slab_cache_t* cache) {
    if (!cache) return;
    
    pthread_mutex_lock(&cache->lock);
    
    slab_page_t* page = cache->empty_pages;
    while (page) {
        slab_page_t* next = page->next;
        slab_page_destroy(page);
        cache->pages_allocated--;
        page = next;
    }
    cache->empty_pages = NULL;
    
    pthread_mutex_unlock(&cache->lock);
}
