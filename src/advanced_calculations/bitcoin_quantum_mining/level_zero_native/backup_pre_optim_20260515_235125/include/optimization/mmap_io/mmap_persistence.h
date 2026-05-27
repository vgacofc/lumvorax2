#ifndef MMAP_PERSISTENCE_H
#define MMAP_PERSISTENCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MMAP_VERSION_MAJOR 1
#define MMAP_VERSION_MINOR 0
#define MMAP_VERSION_PATCH 0

#define MMAP_DEFAULT_SIZE (1024 * 1024 * 16)
#define MMAP_HEADER_MAGIC 0x4D4D4150494F5631ULL
#define MMAP_PAGE_SIZE 4096

typedef enum {
    MMAP_MODE_READ = 0x01,
    MMAP_MODE_WRITE = 0x02,
    MMAP_MODE_READWRITE = 0x03,
    MMAP_MODE_PRIVATE = 0x04,
    MMAP_MODE_SHARED = 0x08
} mmap_mode_t;

typedef struct {
    uint64_t magic;
    uint32_t version;
    uint32_t flags;
    uint64_t data_size;
    uint64_t checksum;
    uint64_t created_timestamp;
    uint64_t modified_timestamp;
    uint8_t reserved[32];
} mmap_header_t;

typedef struct {
    int fd;
    void* base_addr;
    size_t mapped_size;
    size_t file_size;
    mmap_mode_t mode;
    mmap_header_t* header;
    void* data_start;
    size_t data_capacity;
    bool is_valid;
    uint64_t read_bytes;
    uint64_t write_bytes;
    uint64_t sync_count;
} mmap_file_t;

typedef struct {
    mmap_file_t** files;
    size_t file_count;
    size_t file_capacity;
    uint64_t total_mapped;
    uint64_t total_synced;
    bool initialized;
} mmap_manager_t;

mmap_manager_t* mmap_manager_create(void);
void mmap_manager_destroy(mmap_manager_t* manager);
mmap_file_t* mmap_file_open(mmap_manager_t* manager, const char* path, 
                            size_t size, mmap_mode_t mode);
void mmap_file_close(mmap_file_t* file);
int mmap_file_write(mmap_file_t* file, size_t offset, const void* data, size_t size);
int mmap_file_read(mmap_file_t* file, size_t offset, void* data, size_t size);
int mmap_file_sync(mmap_file_t* file);
int mmap_file_resize(mmap_file_t* file, size_t new_size);
void* mmap_file_get_ptr(mmap_file_t* file, size_t offset);
size_t mmap_file_get_size(mmap_file_t* file);
uint64_t mmap_manager_get_total_mapped(mmap_manager_t* manager);

#endif
