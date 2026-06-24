/**
 * @file lum_cloudbreak.c
 * @brief Implémentation Cloudbreak - Stockage Horizontal
 */

#include "lum_cloudbreak.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h>

static uint32_t hash_account(const uint8_t* account_id) {
    uint32_t hash = 0;
    for (int i = 0; i < 32; i++) {
        hash = hash * 31 + account_id[i];
    }
    return hash;
}

lum_cloudbreak_t* lum_cloudbreak_init(uint32_t num_shards, const char* base_path) {
    if (num_shards == 0 || num_shards > LUM_CLOUDBREAK_MAX_SHARDS || !base_path) {
        return NULL;
    }
    
    lum_cloudbreak_t* cb = (lum_cloudbreak_t*)calloc(1, sizeof(lum_cloudbreak_t));
    if (!cb) return NULL;
    
    cb->num_shards = num_shards;
    cb->shards = (lum_cloudbreak_shard_t**)calloc(num_shards, sizeof(lum_cloudbreak_shard_t*));
    
    // Créer shards
    for (uint32_t i = 0; i < num_shards; i++) {
        lum_cloudbreak_shard_t* shard = (lum_cloudbreak_shard_t*)calloc(1, sizeof(lum_cloudbreak_shard_t));
        shard->shard_id = i;
        snprintf(shard->path, sizeof(shard->path), "%s/shard_%04u.db", base_path, i);
        
        // Créer fichier shard
        shard->fd = open(shard->path, O_RDWR | O_CREAT, 0644);
        if (shard->fd < 0) {
            // Cleanup et retour erreur
            for (uint32_t j = 0; j < i; j++) {
                close(cb->shards[j]->fd);
                free(cb->shards[j]);
            }
            free(shard);
            free(cb->shards);
            free(cb);
            return NULL;
        }
        
        pthread_rwlock_init(&shard->lock, NULL);
        cb->shards[i] = shard;
    }
    
    return cb;
}

void lum_cloudbreak_free(lum_cloudbreak_t* cb) {
    if (!cb) return;
    
    for (uint32_t i = 0; i < cb->num_shards; i++) {
        if (cb->shards[i]) {
            close(cb->shards[i]->fd);
            pthread_rwlock_destroy(&cb->shards[i]->lock);
            free(cb->shards[i]);
        }
    }
    
    free(cb->shards);
    free(cb);
}

bool lum_cloudbreak_store(lum_cloudbreak_t* cb, const uint8_t* account_id,
                          const uint8_t* data, uint32_t size) {
    if (!cb || !account_id || !data || size > LUM_CLOUDBREAK_MAX_ACCOUNT_SIZE) {
        return false;
    }
    
    // Déterminer shard
    uint32_t shard_id = hash_account(account_id) % cb->num_shards;
    lum_cloudbreak_shard_t* shard = cb->shards[shard_id];
    
    pthread_rwlock_wrlock(&shard->lock);
    
    // Écrire données (simplifié - append only)
    ssize_t written = write(shard->fd, account_id, 32);
    written += write(shard->fd, &size, sizeof(uint32_t));
    written += write(shard->fd, data, size);
    
    if (written == (ssize_t)(32 + sizeof(uint32_t) + size)) {
        shard->num_accounts++;
        shard->size += written;
        cb->total_writes++;
        cb->total_bytes_written += size;
    }
    
    pthread_rwlock_unlock(&shard->lock);
    
    return written > 0;
}

bool lum_cloudbreak_load(lum_cloudbreak_t* cb, const uint8_t* account_id,
                         uint8_t* data, uint32_t* size) {
    if (!cb || !account_id || !data || !size) return false;
    
    uint32_t shard_id = hash_account(account_id) % cb->num_shards;
    lum_cloudbreak_shard_t* shard = cb->shards[shard_id];
    
    pthread_rwlock_rdlock(&shard->lock);
    
    // Recherche linéaire (simplifié - devrait utiliser index)
    lseek(shard->fd, 0, SEEK_SET);
    
    uint8_t stored_id[32];
    uint32_t stored_size;
    bool found = false;
    
    while (read(shard->fd, stored_id, 32) == 32) {
        if (read(shard->fd, &stored_size, sizeof(uint32_t)) != sizeof(uint32_t)) break;
        
        if (memcmp(stored_id, account_id, 32) == 0) {
            if (read(shard->fd, data, stored_size) == (ssize_t)stored_size) {
                *size = stored_size;
                found = true;
                cb->total_reads++;
                cb->total_bytes_read += stored_size;
            }
            break;
        } else {
            lseek(shard->fd, stored_size, SEEK_CUR);
        }
    }
    
    pthread_rwlock_unlock(&shard->lock);
    
    return found;
}

bool lum_cloudbreak_delete(lum_cloudbreak_t* cb, const uint8_t* account_id) {
    // TODO: Implémenter suppression (marquer comme deleted)
    (void)cb;          // Suppress unused parameter warning
    (void)account_id;  // Suppress unused parameter warning
    return false;
}

void lum_cloudbreak_get_metrics(lum_cloudbreak_t* cb, uint64_t* reads, uint64_t* writes,
                                uint64_t* bytes_read, uint64_t* bytes_written) {
    if (!cb) return;
    if (reads) *reads = cb->total_reads;
    if (writes) *writes = cb->total_writes;
    if (bytes_read) *bytes_read = cb->total_bytes_read;
    if (bytes_written) *bytes_written = cb->total_bytes_written;
}

// Made with Bob
