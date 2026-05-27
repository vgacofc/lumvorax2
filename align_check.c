#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main() {
    for (int i = 0; i < 10; i++) {
        uint8_t *p = malloc(64);

        printf("alloc %d: ptr=%p mod32=%ld\n",
               i, p, ((uintptr_t)p) % 32);

        free(p);
    }
}
