#include "queue.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
struct queue {
    int size;
    int head;
    int tail;
    void **buffer;
    int count;
    //locks
    //one lock for empty queue
    //one lock for full queue
    //one multiplex lock
    pthread_mutex_t mutex;
    pthread_cond_t full;
    pthread_cond_t empty;
};

//constructor malloc
queue_t *queue_new(int size) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (q != NULL) {
        q->size = size;
        q->head = 0;
        q->tail = 0;
        q->buffer = (void **) malloc(size * sizeof(void *));
        q->count = 0;
        pthread_mutex_init(&(q)->mutex, NULL);
        pthread_cond_init(&(q)->full, NULL);
        pthread_cond_init(&(q)->empty, NULL);
    }
    //malloc and intialize struct variables
    //intilize size to size, head to 0, tail to 0
    return q;
}

//delete queue/free memory
void queue_delete(queue_t **q) {
    if (((*q) != NULL) && ((*q)->buffer) != NULL) {
        int rv = pthread_mutex_destroy(&((*q)->mutex));
        assert(rv == 0);
        rv = pthread_cond_destroy(&((*q)->full));
        assert(rv == 0);
        rv = pthread_cond_destroy(&((*q)->empty));
        assert(rv == 0);
        free((*q)->buffer);
        (*q)->buffer = NULL;
        free(*q);
        *q = NULL;
    }
}

//push item onto queue
bool queue_push(queue_t *q, void *elem) {
    //push element into buffer
    //basically set buffer[in]=elemenet
    //change head using the in=(in+1)%size for the next push element
    //check for null stuff
    if (q == NULL) {
        return false;
    }
    pthread_mutex_lock(&(q)->mutex);
    while (q->count == q->size) {
        pthread_cond_wait(&q->full, &q->mutex);
    }
    q->buffer[q->head] = elem;
    q->head = ((q->head) + 1) % (q->size);
    q->count += 1;
    pthread_cond_signal(&q->empty);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

//queue item from queue
bool queue_pop(queue_t *q, void **elem) {
    //push buffer[out]
    //return this?
    //do we need to delete? idk
    //maybe set the pointer to null
    //remove element at the head if we are thinking of head as the first in
    //keep track of tail
    //use trail to decide which element to remove, incremebet using out=(out+1)%size
    //check for null stuff;
    if (q == NULL) {
        return false;
    }
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0) {
        pthread_cond_wait(&q->empty, &q->mutex);
    }
    *elem = q->buffer[q->tail];
    q->tail = ((q->tail) + 1) % (q->size);
    q->count -= 1;
    pthread_cond_signal(&q->full);
    pthread_mutex_unlock(&q->mutex);
    return true;
}
