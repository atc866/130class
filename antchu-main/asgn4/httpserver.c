#include "asgn2_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "queue.h"
#include "request.h"
#include "response.h"
#include "rwlock.h"
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
typedef struct node {
    char *uri;
    rwlock_t *rwlock;
    struct node *next;
} node;

typedef struct list {
    node *head;
    int length;
} list;

void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void *worker();
node *createnode(char *nuri, rwlock_t *nrwlock);
node *createnode(char *nuri, rwlock_t *nrwlock) {
    node *newNode = malloc(sizeof(node));
    if (!newNode) {
        return NULL;
    }
    char *uricopy = malloc((strlen(nuri) + 1));
    strcpy(uricopy, nuri);
    newNode->uri = uricopy;
    newNode->rwlock = nrwlock;
    newNode->next = NULL;
    return newNode;
}
list *makelist();
list *makelist() {
    list *list = malloc(sizeof(list));
    list->length = 0;
    if (!list) {
        return NULL;
    }
    list->head = NULL;
    //fprintf(stderr,"????");
    return list;
}
void addtolist(char *uri, list *list);
void addtolist(char *uri, list *list) {
    node *current = NULL;
    rwlock_t *nrwlock = rwlock_new(N_WAY, 1);
    if (list->head == NULL) {
        list->head = createnode(uri, nrwlock);
    } else {
        current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = createnode(uri, nrwlock);
    }
    list->length++;
}
//-------------------------------------
rwlock_t *find(char *nuri, list *nlist);
rwlock_t *find(char *nuri, list *nlist) {
    //fprintf(stderr, "%i ", nlist->length);
    node *current = NULL;
    if (nlist->head == NULL) {
        return NULL;
    } else {
        current = nlist->head;
        while (current != NULL) {
            //fprintf(stderr,"%s,%s\n",current->uri,nuri);
            if (strcmp(current->uri, nuri) == 0) {
                rwlock_t *rv = current->rwlock;
                //fprintf(stderr,"returned!\n");
                return rv;
            }
            current = current->next;
        }
    }
    return NULL;
}

//-------------------------------------
volatile sig_atomic_t done = 0;
void signaldetect(int signum) {
    (void) signum;
    //(int)signum;
    fprintf(stderr, "CAUGHT");
    done = 1;
}

queue_t *rqueue;
pthread_mutex_t mutex;
list *locklist;
int main(int argc, char **argv) {
    // COMMAND LINE INPUT
    int opt = 0;
    int nt = 4;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't':
            nt = atoi(optarg);
            if (nt < 0) {
                fprintf(stderr, "Invalid thread size.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default: break;
        }
    }

    char *end = NULL;
    size_t port = (size_t) strtoull(argv[optind], &end, 10);
    if (end && *end != '\0') {
        warnx("invalid port number: %s", argv[optind]);
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[optind]);
        fprintf(stderr, "usage: %s <port>\n", argv[optind]);
        return EXIT_FAILURE;
    }
    // INITIALIZE SOCKET
    signal(SIGPIPE, SIG_IGN);
    Listener_Socket socket;
    listener_init(&socket, port);

    // INITIALIZE QUEUE
    rqueue = queue_new(nt);

    // INITIALIZE MUTEX LOCK
    pthread_mutex_init(&mutex, NULL);

    locklist = makelist();
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = signaldetect;
    sigaction(SIGKILL, &action, NULL);

    // INITIALIZE THREADS
    pthread_t threads[nt];
    for (int i = 0; i < nt; i++) {
        pthread_create(&(threads[i]), NULL, worker, NULL);
    }

    // DISPATCHER MAIN THREAD
    while (true || !done) {
        uintptr_t connfd = listener_accept(&socket);
        queue_push(rqueue, (void *) connfd);
    }
    return EXIT_SUCCESS;
}
void *worker() {
    while (true) {
        uintptr_t fd = -1;
        queue_pop(rqueue, (void **) &fd);
        handle_connection(fd);
        close(fd);
    }
}
void handle_connection(int connfd) {
    conn_t *conn = conn_new(connfd);

    const Response_t *res = conn_parse(conn);

    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        //debug("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);
}

void handle_get(conn_t *conn) {
    //debug("handling get request for %s", uri);
    pthread_mutex_lock(&mutex);
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;
    // Open the file..
    //pthread_mutex_unlock(&mutex);
    rwlock_t *urwlock;
    if (find(uri, locklist) == NULL) {
        addtolist(uri, locklist);
        urwlock = find(uri, locklist);
        //reader_lock(urwlock);
    } else {
        urwlock = find(uri, locklist);
        //reader_lock(urwlock);
    }
    //pthread_mutex_unlock(&mutex);
    reader_lock(urwlock);
    pthread_mutex_unlock(&mutex);
    int fd = open(uri, O_RDONLY);
    //fprintf(stderr,"1%i",fd);
    //fprintf(stderr,"%i",fd);
    //pthread_mutex_unlock(&mutex);
    if (fd < 0) {
        //debug("%s: %d", uri, errno);
        if (errno == EACCES) {
            res = &RESPONSE_FORBIDDEN;
            conn_send_response(conn, res);
            char *h = conn_get_header(conn, "Request-Id");
            if (h == NULL) {
                h = "0";
            }
            fprintf(stderr, "GET,/%s,403,%s\n", uri, h);
        } else if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
            conn_send_response(conn, res);
            char *h = conn_get_header(conn, "Request-Id");
            if (h == NULL) {
                h = "0";
            }
            fprintf(stderr, "GET,/%s,404,%s\n", uri, h);
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            conn_send_response(conn, res);
            char *h = conn_get_header(conn, "Request-Id");
            if (h == NULL) {
                h = "0";
            }
            fprintf(stderr, "GET,/%s,500,%s\n", uri, h);
        }
        reader_unlock(urwlock);
        return;
    }
    //fprintf(stderr,"2%i",fd);
    struct stat st;
    fstat(fd, &st);
    uint64_t size = st.st_size;
    if (S_ISDIR(st.st_mode)) {
        res = &RESPONSE_FORBIDDEN;
        conn_send_response(conn, res);
        char *h = conn_get_header(conn, "Request-Id");
        if (h == NULL) {
            h = "0";
        }
        fprintf(stderr, "GET,/%s,403,%s\n", uri, h);
        reader_unlock(urwlock);
        return;
    }
    //fprintf(stderr,"3%i",fd);
    res = conn_send_file(conn, fd, size);
    if (res == NULL) {
        res = &RESPONSE_OK;
        char *h = conn_get_header(conn, "Request-Id");
        if (h == NULL) {
            h = "0";
        }
        fprintf(stderr, "GET,/%s,200,%s\n", uri, h);
    } else {
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        char *h = conn_get_header(conn, "Request-Id");
        if (h == NULL) {
            h = "0";
        }
        fprintf(stderr, "GET,/%s,500,%s\n", uri, h);
    }
    //conn_send_response(conn,res);
    reader_unlock(urwlock);
    close(fd);
    //pthread_mutex_unlock(&mutex);
    //out:
    //conn_send_response(conn, res);
}

void handle_unsupported(conn_t *conn) {
    debug("handling unsupported request");

    // send responses
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
    return;
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;
    pthread_mutex_lock(&mutex);
    //debug("handling put request for %s", uri);
    // Check if file already exists before opening it.
    //debug("%s existed? %d", uri, existed);
    // Open the file..
    rwlock_t *urwlock;
    if (find(uri, locklist) == NULL) {
        addtolist(uri, locklist);
        urwlock = find(uri, locklist);
        //writer_lock(urwlock);
    } else {
        urwlock = find(uri, locklist);
        //writer_lock(urwlock);
    }
    //pthread_mutex_unlock(&mutex);
    writer_lock(urwlock);
    pthread_mutex_unlock(&mutex);
    bool existed = access(uri, F_OK) == 0;
    int fd = open(uri, O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (fd < 0) {
        //debug("%s: %d", uri, errno);
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            res = &RESPONSE_FORBIDDEN;
            conn_send_response(conn, res);
            char *h = conn_get_header(conn, "Request-Id");
            if (h == NULL) {
                h = "0";
            }
            fprintf(stderr, "PUT,/%s,403,%s\n", uri, h);
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            conn_send_response(conn, res);
            char *h = conn_get_header(conn, "Request-Id");
            if (h == NULL) {
                h = "0";
            }
            fprintf(stderr, "PUT,/%s,500,%s\n", uri, h);
            //pthread_mutex_unlock(&mutex);
        }
        //writer_unlock(urwlock);
        return;
    }
    res = conn_recv_file(conn, fd);
    if (res == NULL && existed) {
        res = &RESPONSE_OK;
        char *h = conn_get_header(conn, "Request-Id");
        if (h == NULL) {
            h = "0";
        }
        fprintf(stderr, "PUT,/%s,200,%s\n", uri, h);
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
        char *h = conn_get_header(conn, "Request-Id");
        if (h == NULL) {
            h = "0";
        }
        fprintf(stderr, "PUT,/%s,201,%s\n", uri, h);
    } else {
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        char *h = conn_get_header(conn, "Request-Id");
        if (h == NULL) {
            h = "0";
        }
        fprintf(stderr, "PUT,/%s,500,%s\n", uri, h);
    }
    conn_send_response(conn, res);
    writer_unlock(urwlock);
    close(fd);
}
