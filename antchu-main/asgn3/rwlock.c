#include "rwlock.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
struct rwlock {
    int readers;
    int writers;
    int readersT;
    int write_wait;
    int reader_wait;
    int n;
    PRIORITY priority;
    pthread_mutex_t mutex;
    pthread_cond_t read_cond;
    pthread_cond_t write_cond;
};

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rw = (rwlock_t *) malloc(sizeof(rwlock_t));
    if (rw != NULL) {
        pthread_mutex_init(&rw->mutex, NULL);
        pthread_cond_init(&rw->read_cond, NULL);
        pthread_cond_init(&rw->write_cond, NULL);
        rw->readers = 0;
        rw->writers = 0;
        rw->readersT = 0;
        rw->priority = p;
        rw->write_wait = 0;
        rw->reader_wait = 0;
        rw->n = n;
    }

    return rw;
}

void rwlock_delete(rwlock_t **rw) {
    if (*rw != NULL) {
        pthread_mutex_destroy(&(*rw)->mutex);
        pthread_cond_destroy(&(*rw)->read_cond);
        pthread_cond_destroy(&(*rw)->write_cond);
        free(*rw);
        *rw = NULL;
    }
}

//reader wants to acquire a lock
void reader_lock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw)->mutex);
    rw->reader_wait += 1;
    //readers cant acquire lock if priority of writers and there is a writer writing or there are writers waiting
    while ((rw->writers > 0) || (rw->priority == WRITERS && (rw->writers > 0 || rw->write_wait > 0))
           || (rw->priority == N_WAY && rw->readersT >= rw->n && (rw->write_wait > 0))) {
        pthread_cond_wait(&rw->read_cond, &rw->mutex);
    }
    rw->reader_wait -= 1;
    rw->readers += 1;
    rw->readersT += 1;
    pthread_mutex_unlock(&rw->mutex);
}

void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    //what if someone calls unlock before lock?? do we need to account for this? not sure.
    rw->readers--;
    if (rw->readers == 0 && rw->write_wait > 0) {
        pthread_cond_signal(&rw->write_cond);
    }
    pthread_mutex_unlock(&rw->mutex);
}
//writer wants to acquire lock
void writer_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->write_wait += 1;
    //regardless of priority, writers can only acquire lock when readers and writers are 0
    while ((rw->readers > 0) || (rw->writers > 0)
           || (rw->priority == N_WAY && rw->reader_wait > 0 && rw->readersT < rw->n)
           || (rw->priority == READERS && rw->reader_wait > 0)) {
        pthread_cond_wait(&rw->write_cond, &rw->mutex);
    }
    rw->readersT = 0; //readers before a writer accesses becomes 0 here?
    rw->write_wait -= 1;
    rw->writers++;

    pthread_mutex_unlock(&rw->mutex);
}

//writer gives lock away basically one writer is done
void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);

    rw->writers--;
    //depending on prioity who to signal???

    //priority is for readers.
    //if both waiting are 0 than do I still need to signal? not sure.
    if (rw->priority == READERS && rw->reader_wait != 0) {
        pthread_cond_broadcast(&(rw)->read_cond);
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
    if (rw->priority == READERS && rw->write_wait != 0 && rw->reader_wait == 0) {
        pthread_cond_signal(&(rw)->write_cond);
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
    //priority is for writers.
    //if there are writers waiting, then signal its okay to write otherwise can broadcast a read signal
    if (rw->priority == WRITERS && rw->write_wait > 0) {
        pthread_cond_signal(&(rw)->write_cond);
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
    if (rw->priority == WRITERS && rw->reader_wait > 0 && rw->write_wait == 0) {
        pthread_cond_broadcast(&(rw)->read_cond);
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
    //NWAY
    //writer calls unlock->more than n readers waiting, than only signal to n readers.
    if (rw->priority == N_WAY && rw->write_wait > 0 && rw->reader_wait >= rw->n) {
        for (int i = 0; i < (rw->n); i++) {
            pthread_cond_signal(&(rw)->read_cond);
        }
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
    //if writer calls unlock and writers waiting and readers waiting less than 0 than can broad cast
    //actually doesnt matter how many writers waiting if there are readers waiting less than n can still broad cast?
    if (rw->priority == N_WAY && (rw->reader_wait > 0 && rw->reader_wait < rw->n)
        && rw->write_wait > 0) {
        pthread_cond_broadcast(&(rw)->read_cond);
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
    //writers calls unlock->and no readers waiting, than can just signal to writer.
    if (rw->priority == N_WAY && (rw->reader_wait == 0) && (rw->write_wait > 0)) {
        pthread_cond_signal(&(rw)->write_cond);
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
    if (rw->priority == N_WAY && (rw->write_wait == 0 && rw->reader_wait > 0)) {
        pthread_cond_broadcast(&(rw)->read_cond);
        pthread_mutex_unlock(&rw->mutex);
        return;
    }
}
