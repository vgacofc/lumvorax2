/*
 * LUM Cache Layer — Implémentation cache haute performance
 *
 * Conformité : STANDARD_NAMES.md v4.2
 * Cycle : C142
 * Date : 2026-04-30
 */

#define _GNU_SOURCE
#include "lum_cache.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

/* CRC32C pour intégrité */
static uint32_t crc32c_table[256];
static bool crc32c_initialized = false;

static void init_crc32c_table(void) {
    if (crc32c_initialized) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0x82F63B78 : 0);
        }
        crc32c_table[i] = crc;
    }
    crc32c_initialized = true;
}

static uint32_t compute_crc32c(const void* data, size_t len) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = crc32c_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Fonction de hash simple (djb2) */
static uint32_t hash_key(const char* key, uint32_t table_size) {
    uint32_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % table_size;
}

/* Crée une nouvelle entrée */
static lum_cache_entry_t* create_entry(const char* key, const void* value, 
                                       size_t value_size, uint64_t ttl_ns) {
    lum_cache_entry_t* entry = calloc(1, sizeof(lum_cache_entry_t));
    if (!entry) return NULL;
    
    strncpy(entry->key, key, sizeof(entry->key) - 1);
    
    entry->value = malloc(value_size);
    if (!entry->value) {
        free(entry);
        return NULL;
    }
    memcpy(entry->value, value, value_size);
    entry->value_size = value_size;
    
    entry->access_count = 1;
    entry->last_access_ns = get_timestamp_ns();
    entry->creation_ns = entry->last_access_ns;
    entry->ttl_ns = ttl_ns;
    entry->nx48_score = 0.5f;
    
    entry->crc32c = compute_crc32c(entry, sizeof(lum_cache_entry_t) - sizeof(uint32_t));
    
    return entry;
}

/* Libère une entrée */
static void free_entry(lum_cache_entry_t* entry) {
    if (!entry) return;
    if (entry->value) free(entry->value);
    free(entry);
}

/* Vérifie si une entrée est expirée */
static bool is_expired(const lum_cache_entry_t* entry) {
    if (entry->ttl_ns == 0) return false;
    uint64_t now = get_timestamp_ns();
    return (now - entry->creation_ns) > entry->ttl_ns;
}

/* Retire une entrée de la liste LRU */
static void lru_remove(lum_cache_t* cache, lum_cache_entry_t* entry) {
    if (entry->lru_prev) entry->lru_prev->lru_next = entry->lru_next;
    if (entry->lru_next) entry->lru_next->lru_prev = entry->lru_prev;
    if (cache->lru_head == entry) cache->lru_head = entry->lru_next;
    if (cache->lru_tail == entry) cache->lru_tail = entry->lru_prev;
    entry->lru_prev = entry->lru_next = NULL;
}

/* Ajoute une entrée en tête de liste LRU */
static void lru_add_head(lum_cache_t* cache, lum_cache_entry_t* entry) {
    entry->lru_next = cache->lru_head;
    entry->lru_prev = NULL;
    if (cache->lru_head) cache->lru_head->lru_prev = entry;
    cache->lru_head = entry;
    if (!cache->lru_tail) cache->lru_tail = entry;
}

/* Éviction selon politique */
static int evict_entry(lum_cache_t* cache) {
    if (cache->num_entries == 0) return -1;
    
    lum_cache_entry_t* victim = NULL;
    
    switch (cache->config.eviction_policy) {
        case LUM_CACHE_EVICTION_LRU:
            victim = cache->lru_tail;
            break;
            
        case LUM_CACHE_EVICTION_LFU: {
            uint64_t min_count = UINT64_MAX;
            for (uint32_t i = 0; i < cache->config.hash_table_size; i++) {
                lum_cache_entry_t* e = cache->hash_table[i];
                while (e) {
                    if (e->access_count < min_count) {
                        min_count = e->access_count;
                        victim = e;
                    }
                    e = e->next;
                }
            }
            break;
        }
        
        case LUM_CACHE_EVICTION_NX48: {
            float min_score = 1.0f;
            for (uint32_t i = 0; i < cache->config.hash_table_size; i++) {
                lum_cache_entry_t* e = cache->hash_table[i];
                while (e) {
                    if (e->nx48_score < min_score) {
                        min_score = e->nx48_score;
                        victim = e;
                    }
                    e = e->next;
                }
            }
            break;
        }
    }
    
    if (!victim) return -1;
    
    lum_cache_delete(cache, victim->key);
    cache->stats.evictions++;
    
    return 0;
}

lum_cache_t* lum_cache_init(const lum_cache_config_t* config) {
    if (!config) return NULL;
    
    init_crc32c_table();
    
    lum_cache_t* cache = calloc(1, sizeof(lum_cache_t));
    if (!cache) return NULL;
    
    cache->config = *config;
    if (cache->config.max_entries == 0) cache->config.max_entries = 1000000;
    if (cache->config.hash_table_size == 0) cache->config.hash_table_size = 65536;
    
    cache->hash_table = calloc(cache->config.hash_table_size, sizeof(lum_cache_entry_t*));
    if (!cache->hash_table) {
        free(cache);
        return NULL;
    }
    
    pthread_mutex_init(&cache->lock, NULL);
    
    return cache;
}

int lum_cache_set(lum_cache_t* cache, const char* key, const void* value, 
                  size_t value_size, uint64_t ttl_ns) {
    if (!cache || !key || !value) return -1;
    
    pthread_mutex_lock(&cache->lock);
    
    uint32_t index = hash_key(key, cache->config.hash_table_size);
    
    /* Cherche si la clé existe déjà */
    lum_cache_entry_t* entry = cache->hash_table[index];
    lum_cache_entry_t* prev = NULL;
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            /* Mise à jour */
            free(entry->value);
            entry->value = malloc(value_size);
            if (!entry->value) {
                pthread_mutex_unlock(&cache->lock);
                return -1;
            }
            memcpy(entry->value, value, value_size);
            entry->value_size = value_size;
            entry->last_access_ns = get_timestamp_ns();
            entry->access_count++;
            
            lru_remove(cache, entry);
            lru_add_head(cache, entry);
            
            cache->stats.total_sets++;
            pthread_mutex_unlock(&cache->lock);
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }
    
    /* Éviction si nécessaire */
    if (cache->num_entries >= cache->config.max_entries) {
        evict_entry(cache);
    }
    
    /* Nouvelle entrée */
    lum_cache_entry_t* new_entry = create_entry(key, value, value_size, 
                                                 ttl_ns ? ttl_ns : cache->config.default_ttl_ns);
    if (!new_entry) {
        pthread_mutex_unlock(&cache->lock);
        return -1;
    }
    
    /* Ajoute à la table de hash */
    if (prev) {
        prev->next = new_entry;
    } else {
        cache->hash_table[index] = new_entry;
    }
    
    /* Ajoute à la liste LRU */
    lru_add_head(cache, new_entry);
    
    cache->num_entries++;
    cache->stats.total_sets++;
    cache->stats.memory_used_bytes += sizeof(lum_cache_entry_t) + value_size;
    
    pthread_mutex_unlock(&cache->lock);
    return 0;
}

int lum_cache_get(lum_cache_t* cache, const char* key, void** value_out, size_t* value_size_out) {
    if (!cache || !key) return -1;
    
    pthread_mutex_lock(&cache->lock);
    
    uint32_t index = hash_key(key, cache->config.hash_table_size);
    lum_cache_entry_t* entry = cache->hash_table[index];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (is_expired(entry)) {
                lum_cache_delete(cache, key);
                cache->stats.expired_entries++;
                cache->stats.total_gets++;
                cache->stats.cache_misses++;
                pthread_mutex_unlock(&cache->lock);
                return -1;
            }
            
            entry->last_access_ns = get_timestamp_ns();
            entry->access_count++;
            
            lru_remove(cache, entry);
            lru_add_head(cache, entry);
            
            if (value_out) *value_out = entry->value;
            if (value_size_out) *value_size_out = entry->value_size;
            
            cache->stats.total_gets++;
            cache->stats.cache_hits++;
            cache->stats.hit_rate = (double)cache->stats.cache_hits / (double)cache->stats.total_gets * 100.0;
            
            pthread_mutex_unlock(&cache->lock);
            return 0;
        }
        entry = entry->next;
    }
    
    cache->stats.total_gets++;
    cache->stats.cache_misses++;
    cache->stats.hit_rate = (double)cache->stats.cache_hits / (double)cache->stats.total_gets * 100.0;
    
    pthread_mutex_unlock(&cache->lock);
    return -1;
}

int lum_cache_delete(lum_cache_t* cache, const char* key) {
    if (!cache || !key) return -1;
    
    pthread_mutex_lock(&cache->lock);
    
    uint32_t index = hash_key(key, cache->config.hash_table_size);
    lum_cache_entry_t* entry = cache->hash_table[index];
    lum_cache_entry_t* prev = NULL;
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                cache->hash_table[index] = entry->next;
            }
            
            lru_remove(cache, entry);
            cache->stats.memory_used_bytes -= (sizeof(lum_cache_entry_t) + entry->value_size);
            free_entry(entry);
            cache->num_entries--;
            cache->stats.total_deletes++;
            
            pthread_mutex_unlock(&cache->lock);
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&cache->lock);
    return -1;
}

bool lum_cache_exists(lum_cache_t* cache, const char* key) {
    void* dummy;
    return lum_cache_get(cache, key, &dummy, NULL) == 0;
}

int lum_cache_clear(lum_cache_t* cache) {
    if (!cache) return -1;
    
    pthread_mutex_lock(&cache->lock);
    
    int count = 0;
    for (uint32_t i = 0; i < cache->config.hash_table_size; i++) {
        lum_cache_entry_t* entry = cache->hash_table[i];
        while (entry) {
            lum_cache_entry_t* next = entry->next;
            free_entry(entry);
            count++;
            entry = next;
        }
        cache->hash_table[i] = NULL;
    }
    
    cache->lru_head = cache->lru_tail = NULL;
    cache->num_entries = 0;
    cache->stats.memory_used_bytes = 0;
    
    pthread_mutex_unlock(&cache->lock);
    return count;
}

int lum_cache_cleanup_expired(lum_cache_t* cache) {
    if (!cache) return -1;
    
    pthread_mutex_lock(&cache->lock);
    
    int count = 0;
    for (uint32_t i = 0; i < cache->config.hash_table_size; i++) {
        lum_cache_entry_t* entry = cache->hash_table[i];
        lum_cache_entry_t* prev = NULL;
        
        while (entry) {
            lum_cache_entry_t* next = entry->next;
            
            if (is_expired(entry)) {
                if (prev) {
                    prev->next = next;
                } else {
                    cache->hash_table[i] = next;
                }
                
                lru_remove(cache, entry);
                cache->stats.memory_used_bytes -= (sizeof(lum_cache_entry_t) + entry->value_size);
                free_entry(entry);
                cache->num_entries--;
                count++;
            } else {
                prev = entry;
            }
            
            entry = next;
        }
    }
    
    cache->stats.expired_entries += count;
    
    pthread_mutex_unlock(&cache->lock);
    return count;
}

int lum_cache_get_stats(lum_cache_t* cache, lum_cache_stats_t* stats) {
    if (!cache || !stats) return -1;
    
    pthread_mutex_lock(&cache->lock);
    *stats = cache->stats;
    pthread_mutex_unlock(&cache->lock);
    
    return 0;
}

int lum_cache_reset_stats(lum_cache_t* cache) {
    if (!cache) return -1;
    
    pthread_mutex_lock(&cache->lock);
    memset(&cache->stats, 0, sizeof(lum_cache_stats_t));
    pthread_mutex_unlock(&cache->lock);
    
    return 0;
}

void lum_cache_destroy(lum_cache_t* cache) {
    if (!cache) return;
    
    pthread_mutex_lock(&cache->lock);
    
    /* Libérer toutes les entrées individuellement */
    for (uint32_t i = 0; i < cache->config.hash_table_size; i++) {
        lum_cache_entry_t* entry = cache->hash_table[i];
        while (entry) {
            lum_cache_entry_t* next = entry->next;
            free_entry(entry);  /* Libère entry->value et entry */
            entry = next;
        }
        cache->hash_table[i] = NULL;
    }
    
    pthread_mutex_unlock(&cache->lock);
    
    if (cache->hash_table) {
        free(cache->hash_table);
    }
    
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

int lum_cache_save(lum_cache_t* cache, const char* filepath) {
    if (!cache || !filepath) return -1;
    
    FILE* f = fopen(filepath, "wb");
    if (!f) return -1;
    
    pthread_mutex_lock(&cache->lock);
    
    /* Écrit le nombre d'entrées */
    fwrite(&cache->num_entries, sizeof(uint32_t), 1, f);
    
    /* Écrit chaque entrée */
    for (uint32_t i = 0; i < cache->config.hash_table_size; i++) {
        lum_cache_entry_t* entry = cache->hash_table[i];
        while (entry) {
            fwrite(entry->key, sizeof(entry->key), 1, f);
            fwrite(&entry->value_size, sizeof(size_t), 1, f);
            fwrite(entry->value, entry->value_size, 1, f);
            fwrite(&entry->ttl_ns, sizeof(uint64_t), 1, f);
            entry = entry->next;
        }
    }
    
    pthread_mutex_unlock(&cache->lock);
    fclose(f);
    
    return 0;
}

int lum_cache_load(lum_cache_t* cache, const char* filepath) {
    if (!cache || !filepath) return -1;
    
    FILE* f = fopen(filepath, "rb");
    if (!f) return -1;
    
    uint32_t num_entries;
    if (fread(&num_entries, sizeof(uint32_t), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    
    int loaded = 0;
    for (uint32_t i = 0; i < num_entries; i++) {
        char key[256];
        size_t value_size;
        uint64_t ttl_ns;
        
        if (fread(key, sizeof(key), 1, f) != 1) break;
        if (fread(&value_size, sizeof(size_t), 1, f) != 1) break;
        
        void* value = malloc(value_size);
        if (!value) break;
        
        if (fread(value, value_size, 1, f) != 1) {
            free(value);
            break;
        }
        
        if (fread(&ttl_ns, sizeof(uint64_t), 1, f) != 1) {
            free(value);
            break;
        }
        
        if (lum_cache_set(cache, key, value, value_size, ttl_ns) == 0) {
            loaded++;
        }
        
        free(value);
    }
    
    fclose(f);
    return loaded;
}

int lum_cache_warmup_nx48(lum_cache_t* cache, const char** keys, uint32_t num_keys) {
    if (!cache || !keys) return -1;
    
    /* Implémentation simplifiée - précharge les clés avec score NX48 élevé */
    int warmed = 0;
    for (uint32_t i = 0; i < num_keys; i++) {
        void* value;
        size_t size;
        if (lum_cache_get(cache, keys[i], &value, &size) == 0) {
            warmed++;
        }
    }
    
    return warmed;
}

// Made with Bob
