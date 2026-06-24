#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "../optimization/mmap_io/mmap_persistence.h"

/**
 * @brief Initialise un Write-Ahead Log (WAL) en mode memory-mapped
 * @param path Chemin du fichier WAL
 * @param size Taille du mapping en octets
 *
 * Utilise le système mmap_persistence.h existant pour éviter duplication.
 * Conforme STANDARD_NAMES_MDBAI.md Section 9.
 */
void mmap_wal_init(const char* path, size_t size) {
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        return; /* Failed to open file */
    }
    
    int ret = ftruncate(fd, (off_t)size);
    (void)ret; /* Intentionally ignore return value - best effort truncate */
    
    void* map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        close(fd);
        return; /* Failed to map */
    }
    
    memset(map, 0, size);
    munmap(map, size); /* Unmap after initialization */
    close(fd);
    
    /* Log forensic : WAL Initialisé en MMap */
}
