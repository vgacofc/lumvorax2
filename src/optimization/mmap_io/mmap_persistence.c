#include "mmap_persistence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static uint64_t compute_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    return checksum;
}

mmap_manager_t* mmap_manager_create(void) {
    mmap_manager_t* manager = (mmap_manager_t*)calloc(1, sizeof(mmap_manager_t));
    if (!manager) return NULL;
    
    manager->file_capacity = 16;
    manager->files = (mmap_file_t**)calloc(manager->file_capacity, sizeof(mmap_file_t*));
    if (!manager->files) {
        free(manager);
        return NULL;
    }
    
    manager->initialized = true;
    return manager;
}

void mmap_manager_destroy(mmap_manager_t* manager) {
    if (!manager) return;
    
    for (size_t i = 0; i < manager->file_count; i++) {
        mmap_file_close(manager->files[i]);
    }
    
    free(manager->files);
    free(manager);
}

mmap_file_t* mmap_file_open(mmap_manager_t* manager, const char* path,
                            size_t size, mmap_mode_t mode) {
    if (!manager || !path) return NULL;
    
    mmap_file_t* file = (mmap_file_t*)calloc(1, sizeof(mmap_file_t));
    if (!file) return NULL;
    
    int flags = O_CREAT;
    int prot = 0;
    int mmap_flags = 0;
    
    if (mode & MMAP_MODE_READ) {
        flags |= O_RDONLY;
        prot |= PROT_READ;
    }
    if (mode & MMAP_MODE_WRITE) {
        flags = O_RDWR | O_CREAT;
        prot |= PROT_WRITE;
    }
    if (mode & MMAP_MODE_READWRITE) {
        flags = O_RDWR | O_CREAT;
        prot = PROT_READ | PROT_WRITE;
    }
    
    if (mode & MMAP_MODE_PRIVATE) {
        mmap_flags = MAP_PRIVATE;
    } else {
        mmap_flags = MAP_SHARED;
    }
    
    file->fd = open(path, flags, 0644);
    if (file->fd < 0) {
        free(file);
        return NULL;
    }
    
    if (size == 0) size = MMAP_DEFAULT_SIZE;
    
    size_t total_size = sizeof(mmap_header_t) + size;
    total_size = ((total_size + MMAP_PAGE_SIZE - 1) / MMAP_PAGE_SIZE) * MMAP_PAGE_SIZE;
    
    if (ftruncate(file->fd, total_size) != 0) {
        close(file->fd);
        free(file);
        return NULL;
    }
    
    file->base_addr = mmap(NULL, total_size, prot, mmap_flags, file->fd, 0);
    if (file->base_addr == MAP_FAILED) {
        close(file->fd);
        free(file);
        return NULL;
    }
    
    file->mapped_size = total_size;
    file->file_size = total_size;
    file->mode = mode;
    file->header = (mmap_header_t*)file->base_addr;
    file->data_start = (uint8_t*)file->base_addr + sizeof(mmap_header_t);
    file->data_capacity = size;
    
    if (file->header->magic != MMAP_HEADER_MAGIC) {
        file->header->magic = MMAP_HEADER_MAGIC;
        file->header->version = (MMAP_VERSION_MAJOR << 16) | (MMAP_VERSION_MINOR << 8) | MMAP_VERSION_PATCH;
        file->header->flags = 0;
        file->header->data_size = 0;
        file->header->checksum = 0;
        file->header->created_timestamp = get_timestamp_ns();
        file->header->modified_timestamp = file->header->created_timestamp;
    }
    
    file->is_valid = true;
    
    if (manager->file_count >= manager->file_capacity) {
        size_t new_cap = manager->file_capacity * 2;
        mmap_file_t** new_files = (mmap_file_t**)realloc(manager->files, new_cap * sizeof(mmap_file_t*));
        if (!new_files) {
            mmap_file_close(file);
            return NULL;
        }
        manager->files = new_files;
        manager->file_capacity = new_cap;
    }
    
    manager->files[manager->file_count++] = file;
    manager->total_mapped += total_size;
    
    return file;
}

void mmap_file_close(mmap_file_t* file) {
    if (!file) return;
    
    if (file->base_addr && file->base_addr != MAP_FAILED) {
        mmap_file_sync(file);
        munmap(file->base_addr, file->mapped_size);
    }
    
    if (file->fd >= 0) {
        close(file->fd);
    }
    
    free(file);
}

int mmap_file_write(mmap_file_t* file, size_t offset, const void* data, size_t size) {
    if (!file || !data || !file->is_valid) return -1;
    if (offset + size > file->data_capacity) return -2;
    
    memcpy((uint8_t*)file->data_start + offset, data, size);
    
    if (offset + size > file->header->data_size) {
        file->header->data_size = offset + size;
    }
    
    file->header->modified_timestamp = get_timestamp_ns();
    file->write_bytes += size;
    
    return 0;
}

int mmap_file_read(mmap_file_t* file, size_t offset, void* data, size_t size) {
    if (!file || !data || !file->is_valid) return -1;
    if (offset + size > file->header->data_size) return -2;
    
    memcpy(data, (uint8_t*)file->data_start + offset, size);
    file->read_bytes += size;
    
    return 0;
}

int mmap_file_sync(mmap_file_t* file) {
    if (!file || !file->base_addr || !file->is_valid) return -1;
    
    file->header->checksum = compute_checksum(file->data_start, file->header->data_size);
    
    int result = msync(file->base_addr, file->mapped_size, MS_SYNC);
    if (result == 0) {
        file->sync_count++;
    }
    
    return result;
}

int mmap_file_resize(mmap_file_t* file, size_t new_size) {
    if (!file || !file->is_valid) return -1;
    
    size_t new_total = sizeof(mmap_header_t) + new_size;
    new_total = ((new_total + MMAP_PAGE_SIZE - 1) / MMAP_PAGE_SIZE) * MMAP_PAGE_SIZE;
    
    mmap_file_sync(file);
    munmap(file->base_addr, file->mapped_size);
    
    if (ftruncate(file->fd, new_total) != 0) {
        file->is_valid = false;
        return -2;
    }
    
    int prot = 0;
    if (file->mode & MMAP_MODE_READ) prot |= PROT_READ;
    if (file->mode & MMAP_MODE_WRITE) prot |= PROT_WRITE;
    if (file->mode & MMAP_MODE_READWRITE) prot = PROT_READ | PROT_WRITE;
    
    int flags = (file->mode & MMAP_MODE_PRIVATE) ? MAP_PRIVATE : MAP_SHARED;
    
    file->base_addr = mmap(NULL, new_total, prot, flags, file->fd, 0);
    if (file->base_addr == MAP_FAILED) {
        file->is_valid = false;
        return -3;
    }
    
    file->mapped_size = new_total;
    file->file_size = new_total;
    file->header = (mmap_header_t*)file->base_addr;
    file->data_start = (uint8_t*)file->base_addr + sizeof(mmap_header_t);
    file->data_capacity = new_size;
    
    return 0;
}

void* mmap_file_get_ptr(mmap_file_t* file, size_t offset) {
    if (!file || !file->is_valid || offset >= file->data_capacity) return NULL;
    return (uint8_t*)file->data_start + offset;
}

size_t mmap_file_get_size(mmap_file_t* file) {
    return file ? file->header->data_size : 0;
}

uint64_t mmap_manager_get_total_mapped(mmap_manager_t* manager) {
    return manager ? manager->total_mapped : 0;
}
