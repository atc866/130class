#include "asgn2_helper_funcs.h"
#include "helper.h"

#define BUFFSIZE     4096
#define regexrequest "^([a-zA-Z]{1,8}) /([a-zA-Z0-9.-]{1,63}) (HTTP/[0-9]\\.[0-9])"
#define headerregex  "([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})\r\n\r\n"
int parse(char *buff, int bytesread, requestinfo *rinfo) {
    regex_t re;
    regmatch_t matches[4];
    int rc;
    char *token = strstr(buff, "\r\n");
    char requestinfo[BUFFSIZE + 1] = { '\0' };
    int pos = token - buff;
    //printf("pos:%i",pos);
    if (token == NULL) {
        dprintf(
            rinfo->fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return -1;
    }
    for (int i = 0; i < pos; i++) {
        requestinfo[i] = buff[i];
    }
    rc = regcomp(&re, regexrequest, REG_EXTENDED);
    rc = regexec(&re, requestinfo, 4, matches, 0);
    //printf("bytesread:%i", bytesread);
    //printf("fd:%i",rinfo->fd);
    if (rc == 0) {
        //printf("match0:%s",requestinfo);
        if (matches[1].rm_eo - matches[1].rm_so > 8) {
            dprintf(rinfo->fd,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
            return -1;
        }
        rinfo->method = buff + matches[1].rm_so;
        rinfo->method[matches[1].rm_eo] = '\0';
        //check for valid command
        if ((strcmp(rinfo->method, "GET") != 0) && (strcmp(rinfo->method, "PUT") != 0)) {
            dprintf(rinfo->fd,
                "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\nNot Implemented\n", 16);
            return -1;
        }
        //printf("\ncommand:%s",rinfo->method);
        rinfo->uri = buff + matches[2].rm_so;
        rinfo->uri[matches[2].rm_eo - matches[2].rm_so] = '\0';
        //printf("uri:%s",rinfo->uri);
        rinfo->version = requestinfo + matches[3].rm_so;
        if (buff[matches[3].rm_eo] != '\r' || buff[matches[3].rm_eo + 1] != '\n') {
            dprintf(rinfo->fd,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
            return -1;
        }
        rinfo->version[matches[3].rm_eo - matches[3].rm_so] = '\0';
        if (strcmp(rinfo->version, "HTTP/1.1") != 0) {
            dprintf(rinfo->fd,
                "HTTP/1.1 505 Version Not Supported\r\nContent-Length: %d\r\n\r\nVersion Not "
                "Supported\n",
                22);
            return -1;
        }
        //check for right version and if get, then check only two remaining bits->\r\n
        if (strcmp(rinfo->method, "PUT") == 0) {
            //parse headerfiled
            buff += matches[3].rm_eo;
            rc = regcomp(&re, headerregex, REG_EXTENDED);
            rc = regexec(&re, buff, 3, matches, 0);

            if (rc == 0) {
                //printf("match1??%s??",buff+matches[1].rm_so);
                char *hfield = buff + matches[1].rm_so;
                int r = matches[1].rm_eo - matches[1].rm_so;
                hfield[r] = '\0';
                //printf("hfield%s",hfield);
                //printf("match2??%s??",buff+matches[2].rm_so);
                //printf("match2end??%s??",buff+matches[2].rm_eo);
                r = matches[2].rm_eo - matches[2].rm_so;
                char *clength = buff + matches[2].rm_so;
                clength[r] = '\0';
                rinfo->contentlength = atoi(clength);
                printf("contentlength::%i::", rinfo->contentlength);
                rinfo->messagebody = buff + (matches[2].rm_eo + 4);
                //printf("messagebody::%s::",rinfo->messagebody);
                rinfo->leftoverbytes = bytesread - (matches[3].rm_eo + matches[2].rm_eo + 4);
                //read leftover bytes from messagebody
            }
        }
    } else {
        dprintf(
            rinfo->fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return -1;
    }
    return 0;
}

int get(requestinfo *rinfo) {
    //need to add catching errors here!
    int fd = open(rinfo->uri, O_RDONLY);
    if (fd < 0) {
        if (errno == EACCES) {
            dprintf(
                rinfo->fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        } else if (errno == ENOENT) {
            dprintf(
                rinfo->fd, "HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\n\r\nNot Found\n", 10);
        } else {
            dprintf(rinfo->fd,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
                "Error\n",
                22);
        }
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        dprintf(rinfo->fd,
            "HTTP/1.1 500 Internal Server Error\r\n Content-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        close(fd);
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        dprintf(rinfo->fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        close(fd);
        return -1;
    }
    stat(rinfo->uri, &st);
    size_t size = st.st_size;
    printf("size:%i", (int) size);
    dprintf(rinfo->fd, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", size);
    int byteswritten = pass_n_bytes(fd, rinfo->fd, size);
    if (byteswritten < 0) {
        dprintf(rinfo->fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

int put(requestinfo *rinfo) {
    int fd = open(rinfo->uri, O_RDONLY);
    int fileExists = access(rinfo->uri, F_OK) == 0;
    fd = open(rinfo->uri, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) {
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            dprintf(
                rinfo->fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);

        } else {
            dprintf(
                rinfo->fd, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\n", 22);
        }
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        dprintf(rinfo->fd, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\n", 5);
        return -1;
    }
    //write to socket
    //printf("MESSAGEBODY:%s:", rinfo->messagebody);
    //printf("LEFTOVERBYTES%i:", rinfo->leftoverbytes);
    //write left over bytes in buff
    int writtenbytes = write_n_bytes(fd, rinfo->messagebody, rinfo->leftoverbytes);
    if (writtenbytes == -1) {
        dprintf(rinfo->fd,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\nNot Implemented\n", 16);
        close(fd);
        return -1;
    }
    //write content-length-leftover bytes in buff->stuff not read yet.
    int passnbytes = pass_n_bytes(rinfo->fd, fd, (rinfo->contentlength - rinfo->leftoverbytes));
    if (passnbytes == -1) {
        dprintf(rinfo->fd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        close(fd);
        return -1;
    }
    if (fileExists) {
        dprintf(rinfo->fd, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nOK\n", 3);
    } else {
        dprintf(rinfo->fd, "HTTP/1.1 201 Created\r\nContent-Length: %d\r\n\r\nCreated\n", 8);
    }
    close(fd);
    return 0;
}
