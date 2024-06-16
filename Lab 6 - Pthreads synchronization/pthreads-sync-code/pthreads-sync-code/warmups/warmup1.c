#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 10
#define NUM_INCREMENTS 1000

int counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *incrementCounterWithoutLock(void *arg) {
    for (int i = 0; i < NUM_INCREMENTS; ++i) {
        ++counter;
    }
    pthread_exit(NULL);
}

void *incrementCounterWithLock(void *arg) {
    for (int i = 0; i < NUM_INCREMENTS; ++i) {
        pthread_mutex_lock(&mutex);
        ++counter;
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];

    // Without using locks
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, &incrementCounterWithoutLock, NULL); // returns 0 if successfully created otherwise can add error statement
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
    printf("Counter without lock: %d\n", counter);

    // Using locks
    counter = 0;
    pthread_t threads2[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads2[i], NULL, &incrementCounterWithLock, NULL);
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads2[i], NULL);
    }
    printf("Counter with lock: %d\n", counter);

    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
    return 0;
}
