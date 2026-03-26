#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "mmap_wal.h"

void mmap_wal_init(const char* path, size_t size) {
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    ftruncate(fd, size);
    void* map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memset(map, 0, size);
    close(fd);
    // Log forensic : WAL Initialisé en MMap
}
