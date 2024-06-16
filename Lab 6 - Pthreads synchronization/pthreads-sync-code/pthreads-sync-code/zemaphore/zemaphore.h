#include <pthread.h>

typedef struct zemaphore {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int counter;
} zem_t;

void zem_init(zem_t *, int);
void zem_up(zem_t *);
void zem_down(zem_t *);
