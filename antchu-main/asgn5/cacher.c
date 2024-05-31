#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>

//each node represents something in cache
typedef struct nodeobj {
    char *uri;
    int bit;
    struct nodeobj *next;
} nodeobj;

typedef nodeobj *node;

//linked list
typedef struct listobj {
    node head;
    int length;
} listobj;

typedef listobj *list;

node createnode(char *nuri);
node createnode(char *nuri) {
    node newNode = malloc(sizeof(nodeobj));
    if (!newNode) {
        return NULL;
    }
    newNode->uri = malloc((strlen(nuri) + 1));
    strcpy(newNode->uri, nuri);
    newNode->bit = 0;
    newNode->next = NULL;
    return newNode;
}
void freeNode(node *n);
void freeNode(node *n) {
    if (n != NULL && *n != NULL) {
        free((*n)->uri);
        free(*n);
        *n = NULL;
    }
}
list makelist();
list makelist() {
    list l = malloc(sizeof(listobj));
    l->length = 0;
    if (!l) {
        return NULL;
    }
    l->head = NULL;
    return l;
}
void pushlist(char *uri, list l);
void pushlist(char *uri, list l) {
    node current = NULL;
    if (l->head == NULL) {
        l->head = createnode(uri);
    } else {
        current = l->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = createnode(uri);
    }
    l->length++;
}
void poplist(list l);
void poplist(list l) {
    node temp = NULL;
    if (l->head == NULL) {
        return;
    } else {
        temp = l->head;
        if (l->length > 1) {
            l->head = l->head->next;
            freeNode(&temp);
        } else {
            l->head = NULL;
            freeNode(&temp);
        }
    }
    l->length--;
}

int contains(char *nuri, list nlist);
int contains(char *nuri, list nlist) {
    //fprintf(stderr, "%i ", nlist->length);
    node current = NULL;
    if (nlist->head == NULL) {
        return 0;
    } else {
        current = nlist->head;
        while (current != NULL) {
            if (strcmp(current->uri, nuri) == 0) {
                //found that shit!
                return 1;
            }
            current = current->next;
        }
    }
    return 0;
}

node find(char *nuri, list nlist);
node find(char *nuri, list nlist) {
    //fprintf(stderr, "%i ", nlist->length);
    node current = NULL;
    if (nlist->head == NULL) {
        return NULL;
    } else {
        current = nlist->head;
        while (current != NULL) {
            if (strcmp(current->uri, nuri) == 0) {
                node rv = current;
                return rv;
            }
            current = current->next;
        }
    }
    return NULL;
}
node findindex(int index, list nlist);
node findindex(int index, list nlist) {
    //fprintf(stderr, "%i ", nlist->length);
    node current = NULL;
    if (nlist->head == NULL) {
        return NULL;
    } else if (nlist->length < index) {
        printf("something wrong with index inputted for sure");
        return NULL;
    } else {
        current = nlist->head;
        int i = 0;
        while (current != NULL && i < index) {
            current = current->next;
            i += 1;
        }
        return current;
    }
    return NULL;
}
void delete (char *uri, list l);
void delete (char *uri, list l) {
    node current = l->head;
    node previous = current;
    while (current != NULL) {
        if (strcmp(current->uri, uri) == 0) {
            previous->next = current->next;
            if (current == l->head)
                l->head = current->next;
            freeNode(&current);
            l->length--;
            return;
        }
        previous = current;
        current = current->next;
    }
}
void display(list l);
void display(list l) {
    node current = l->head;
    if (l->head == NULL)
        return;
    for (int i = 0; i < l->length; i++) {
        printf("%s\n", current->uri);
        current = current->next;
    }
}
void freelist(list *l);
void freelist(list *l) {
    if (l != NULL & *l != NULL) {
        while ((*l)->length != 0) {
            poplist(*l);
        }
        free(*l);
        *l = NULL;
    }
}

void set_index(list l, int p, char *element);
void set_index(list l, int p, char *element) {
    //fprintf(stderr, "%i ", nlist->length);
    node current = NULL;
    if (l->head == NULL) {
        fprintf(stderr, "??? error");
        return;
    } else if (l->length < p) {
        printf("something wrong with index inputted for sure");
        return;
    } else {
        current = l->head;
        int i = 0;
        while (current != NULL && i < p) {
            current = current->next;
            i += 1;
        }
        free(current->uri);
        current->uri = malloc((strlen(element) + 1));
        strcpy(current->uri, element);
        return;
    }
    return;
}
typedef enum { FIFO, LRU, CLOCK } POLICY;
typedef struct cachestruct {
    POLICY policy;
    list cachelist;
    list history;
    int clockpointer;
    int maxsize;
    int num_compulsory_misses;
    int num_capacity_misses;
} cachestruct;
int insert_fifo(cachestruct *cache, char *element);
int insert_fifo(cachestruct *cache, char *element) {
    if (cache == NULL || element == NULL) {
        fprintf(stderr, "bad inputs");
        return EXIT_FAILURE;
    }
    //if in cache then return 1->hit!
    if (contains(element, cache->cachelist)) {
        return 1; //hit
    }
    //pop item at front of list if cache is full, need to keep track of this
    if (cache->cachelist->length == cache->maxsize) {
        poplist(cache->cachelist); //remove item at front of list
        if (!contains(element, cache->history)) {
            cache->num_compulsory_misses++;
            pushlist(element, cache->history);
        } else {
            cache->num_capacity_misses++;
        }
    } else {
        if (!contains(element, cache->history)) {
            cache->num_compulsory_misses++;
            pushlist(element, cache->history);
        }
    }
    pushlist(element, cache->cachelist);
    return 0;
}
int insert_lru(cachestruct *cache, char *element);
int insert_lru(cachestruct *cache, char *element) {
    if (cache == NULL || element == NULL) {
        fprintf(stderr, "bad inputs");
        return EXIT_FAILURE;
    }
    //if in cache then return 1->hit!
    if (contains(element, cache->cachelist)) {
        delete (element, cache->cachelist);
        pushlist(element, cache->cachelist);
        return 1; //hit
    }
    //pop item at front of list if cache is full, need to keep track of this
    if (cache->cachelist->length == cache->maxsize) {
        poplist(cache->cachelist); //remove item at front of list
        if (!contains(element, cache->history)) {
            cache->num_compulsory_misses++;
            pushlist(element, cache->history);
        } else {
            cache->num_capacity_misses++;
        }
    } else {
        if (!contains(element, cache->history)) {
            cache->num_compulsory_misses++;
            pushlist(element, cache->history);
        }
    }
    pushlist(element, cache->cachelist);
    return 0;
}
int insert_clock(cachestruct *cache, char *element);
int insert_clock(cachestruct *cache, char *element) {
    if (cache == NULL || element == NULL) {
        fprintf(stderr, "bad inputs");
        return EXIT_FAILURE;
    }

    if (contains(element, cache->cachelist)) {
        node item = find(element, cache->cachelist);
        //set reference bit to 1
        if (item == NULL) {
            fprintf(stderr, "something bad happened");
            return EXIT_FAILURE;
        }
        item->bit = 1;
        return 1;
    }
    if (cache->cachelist->length == cache->maxsize) {
        node item
            = findindex(cache->clockpointer, cache->cachelist); //get element at index/clock pointer
        while (item->bit == 1) {
            item->bit = 0;
            cache->clockpointer = (cache->clockpointer + 1) % (cache->maxsize);
            item = findindex(cache->clockpointer, cache->cachelist);
        }
        set_index(cache->cachelist, cache->clockpointer, element);
        //cache_increment_clockpointer(cache);

        cache->clockpointer = (cache->clockpointer + 1) % (cache->maxsize);

        if (!contains(element, cache->history)) {
            cache->num_compulsory_misses++;
            pushlist(element, cache->history);
        } else {
            cache->num_capacity_misses++; //capacity full and have already seen element
        }
        return 0;
    } else {
        if (!contains(element, cache->history)) {
            cache->num_compulsory_misses++;
            pushlist(element, cache->history);
        }
        pushlist(element, cache->cachelist);
        return 0;
    }
}
int main(int argc, char **argv) {
    // Default values
    int capacity = 3; // Default cache size
    int opt = 0;
    POLICY policy;
    policy = FIFO;

    // Parse command line arguments
    while (optind < argc) {
        if ((opt = getopt(argc, argv, "N:FLC")) != -1) {
            switch (opt) {
            default: fprintf(stderr, "bad arg"); return 0;
            case 'N':
                capacity = atoi(optarg);
                if (capacity <= 0) {
                    fprintf(stderr, "Invalid thread size.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'L': policy = LRU; break;
            case 'F': policy = FIFO; break;
            case 'C': policy = CLOCK; break;
            }
        }
    }
    if (argc != 4) {
        fprintf(stderr, "????");
        return EXIT_FAILURE;
    }
    struct cachestruct cache;
    cache.policy = policy;
    cache.clockpointer = 0;
    cache.maxsize = capacity;
    cache.num_capacity_misses = 0;
    cache.num_compulsory_misses = 0;
    cache.history = makelist();
    cache.cachelist = makelist();
    // Process input until stdin is closed
    //printf("capacity: %i",capacity);
    //printf("policy: %u",policy);
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        char *t = buffer;
        if (policy == FIFO) {
            if (insert_fifo(&cache, t) == 0) {
                printf("MISS\n");
            } else {
                printf("HIT\n");
            }
        }
        if (policy == LRU) {
            if (insert_lru(&cache, t) == 0) {
                printf("MISS\n");
            } else {
                printf("HIT\n");
            }
        }
        if (policy == CLOCK) {
            if (insert_clock(&cache, t) == 0) {
                printf("MISS\n");
            } else {
                printf("HIT\n");
            }
            //display(cache.cachelist);
        }
    }
    printf("%i %i\n", cache.num_compulsory_misses, cache.num_capacity_misses);
    //display(cache.cachelist);
    freelist(&cache.history);
    freelist(&cache.cachelist);
    return 0;
}
