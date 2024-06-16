#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_THREADS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int next_thread_id = 1;

void* printThread(void *arg){

    int thread_id = *(int *)arg;

    int sleep_time = rand()%10;
    sleep(sleep_time);

    pthread_mutex_lock(&mutex); 
    // we wait until its not this threads turn to print
    while (thread_id != next_thread_id) {
        pthread_cond_wait(&cond, &mutex);
    }

    ++next_thread_id;
    printf("I am thread %d\n", thread_id);

    // signal the next thread to print
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(){

    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];

    int ret;
    srand(time(NULL));

    for(int i=0; i < NUM_THREADS; i++){
        thread_args[i] = i + 1;
        
        ret = pthread_create(&threads[i], NULL, &printThread,&thread_args[i]);
        if(ret != 0) {
            printf("Error: pthread_create() failed\n");
            exit(1);
        }
    }

    // without this the ordering could get messed up
    for (int i = 1; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("I am the main thread\n");
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    pthread_exit(NULL);
}