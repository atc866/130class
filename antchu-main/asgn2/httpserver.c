#include "asgn2_helper_funcs.h"
#include "helper.h"
#define BUFFSIZE 4096
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("?????");
        return 1;
    }
    Listener_Socket socket;
    int port = strtol(argv[1], NULL, 10);
    int r = listener_init(&socket, port);
    //printf("%i", port);
    //printf("%i", r);
    if (r == -1) {
        //error
        return 1;
    }
    char buff[4096] = { '\0' };
    while (true) {
        int fd = listener_accept(&socket);
        if (fd == -1) {
            return 1;
        }
        requestinfo rinfo;
        rinfo.fd = fd;
        int bytesRead = 0;
        int readtotal = 0;
        char *token = "\r\n\r\n";
        do {
            bytesRead = read(rinfo.fd, buff + readtotal, BUFFSIZE - readtotal);
            if (bytesRead != -1) {
                readtotal += bytesRead;
            }
        } while (bytesRead > 0 && readtotal < BUFFSIZE && strstr(buff, token) == NULL);
        printf("bytesRead:%i", bytesRead);
        printf("%s", buff);
        //printf("%s",token);
        //printf("???");
        int rv = parse(buff, readtotal, &rinfo);
        if (rv != -1) {
            if (strcmp(rinfo.method, "GET") == 0) {
                get(&rinfo);
            } else if (strcmp(rinfo.method, "PUT") == 0) {
                put(&rinfo);
            } else {
                dprintf(rinfo.fd,
                    "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\nNot Implemented\n",
                    16);
            }
        }
        //parse needs request variable to store, buff, and bytes written.

        //printf("token:%s",token);
        //printf("bytes:%i",bytesRead);
        close(rinfo.fd);
        memset(buff, '\0', sizeof(buff));
    }

    return 0;
}
