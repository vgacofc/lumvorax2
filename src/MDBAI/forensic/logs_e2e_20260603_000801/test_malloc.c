#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main() {
    // Test allocations diverses tailles
    void *ptr1 = malloc(1024);        // 1 KB
    void *ptr2 = malloc(1024 * 1024); // 1 MB
    void *ptr3 = calloc(100, sizeof(int)); // 400 bytes
    
    if (ptr1 && ptr2 && ptr3) {
        memset(ptr1, 0xAA, 1024);
        memset(ptr2, 0xBB, 1024 * 1024);
        printf("Allocations OK\n");
    }
    
    free(ptr1);
    free(ptr2);
    free(ptr3);
    
    return 0;
}
