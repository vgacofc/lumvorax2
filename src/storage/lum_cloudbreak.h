/**
 * @file lum_cloudbreak.h
 * @brief LumVorax Cloudbreak - Stockage Horizontal Scalable
 * 
 * Cloudbreak distribue les comptes sur plusieurs disques/shards
 * pour permettre un scaling horizontal du stockage.
 * 
 * Performance cible : 1M+ comptes/sec en lecture/écriture
 * 
 * @version 1.0.0
 * @date 2026-05-02
 */

#ifndef LUM_CLOUDBREAK_H
#define LUM_CLOUDBREAK_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LUM_CLOUDBREAK_MAX_SHARDS 256
#define LUM_CLOUDBREAK_MAX_ACCOUNT_SIZE (10*1024*1024)  // 10MB

typedef struct {
    uint32_t shard_id;
    char path[256];
    int fd;
    uint64_t size;
    uint64_t num_accounts;
    pthread_rwlock_t lock;
} lum_cloudbreak_shard_t;

typedef struct {
    uint32_t num_shards;
    lum_cloudbreak_shard_t** shards;
    
    // Métriques
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t total_bytes_read;
    uint64_t total_bytes_written;
} lum_cloudbreak_t;

lum_cloudbreak_t* lum_cloudbreak_init(uint32_t num_shards, const char* base_path);
void lum_cloudbreak_free(lum_cloudbreak_t* cb);

bool lum_cloudbreak_store(lum_cloudbreak_t* cb, const uint8_t* account_id, 
                          const uint8_t* data, uint32_t size);
bool lum_cloudbreak_load(lum_cloudbreak_t* cb, const uint8_t* account_id,
                         uint8_t* data, uint32_t* size);
bool lum_cloudbreak_delete(lum_cloudbreak_t* cb, const uint8_t* account_id);

void lum_cloudbreak_get_metrics(lum_cloudbreak_t* cb, uint64_t* reads, uint64_t* writes,
                                uint64_t* bytes_read, uint64_t* bytes_written);

#ifdef __cplusplus
}
#endif

#endif /* LUM_CLOUDBREAK_H */

// Made with Bob
