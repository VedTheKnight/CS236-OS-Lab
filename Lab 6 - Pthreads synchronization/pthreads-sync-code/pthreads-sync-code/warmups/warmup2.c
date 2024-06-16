#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* printThread(void *arg){
    // pthread_mutex_lock(&mutex); 
    int i = *(int *)arg;
    printf("I am thread %d\n", i);
    // pthread_mutex_unlock(&mutex); 
    pthread_exit(NULL);
}

int main(){

    pthread_t threads[NUM_THREADS];
    int ret;
    int args[NUM_THREADS];

    for(int i=0; i < NUM_THREADS; i++){
        args[i] = i;
        ret = pthread_create(&threads[i], NULL, &printThread, &args[i]);
        if(ret != 0) {
            printf("Error: pthread_create() failed\n");
            exit(1);
        }
    }

    // without this the main function could print before the other threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("I am the main thread\n");
    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
}