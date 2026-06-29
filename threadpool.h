#ifndef THREADPOOL_H
#define THREADPOOL_H

typedef void (*task_fn)(void*);

#define QUEUE_SIZE 256 
#define MAX_THREADS 16

typedef struct{
    int nthreads;
    pthread_t threads[MAX_THREADS];
    int queue[QUEUE_SIZE]; //queue of connection socket fds
    pthread_cond_t space_available;
    pthread_cond_t task_available;
    pthread_mutex_t mylock;
    int head, tail, count;
    int shutdown;
} Threadpool;

Threadpool* pool_create(int nthreads);
void pool_submit(Threadpool* pool, int conn_sock);
void pool_destroy(Threadpool* pool);

#endif