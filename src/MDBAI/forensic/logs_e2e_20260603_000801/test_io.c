#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    const char *filename = "/tmp/mdbai_test_io.txt";
    const char *data = "MDBAI Forensic Test Data - Bit-level tracking\n";
    
    // Test write
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open write");
        return 1;
    }
    
    ssize_t written = write(fd, data, strlen(data));
    if (written < 0) {
        perror("write");
        close(fd);
        return 1;
    }
    printf("Written %zd bytes\n", written);
    close(fd);
    
    // Test read
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open read");
        return 1;
    }
    
    char buffer[256];
    ssize_t nread = read(fd, buffer, sizeof(buffer) - 1);
    if (nread < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    buffer[nread] = '\0';
    printf("Read %zd bytes: %s", nread, buffer);
    close(fd);
    
    // Cleanup
    unlink(filename);
    
    return 0;
}
