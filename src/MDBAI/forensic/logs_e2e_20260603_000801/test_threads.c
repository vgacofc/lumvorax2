#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *thread_func(void *arg) {
    int id = *(int*)arg;
    printf("Thread %d started\n", id);
    usleep(100000); // 100ms
    printf("Thread %d finished\n", id);
    return NULL;
}

int main() {
    pthread_t threads[4];
    int ids[4] = {1, 2, 3, 4};
    
    // Créer 4 threads
    for (int i = 0; i < 4; i++) {
        if (pthread_create(&threads[i], NULL, thread_func, &ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    
    // Attendre tous les threads
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("All threads completed\n");
    return 0;
}
