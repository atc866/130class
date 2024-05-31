#pragma once

#include "asgn2_helper_funcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#define BUFSIZE 4096

typedef struct requestinfo {
    char *method;
    char *uri;
    char *version;
    char *messagebody;
    int contentlength;
    int fd;
    int leftoverbytes;
    int pos;
} requestinfo;

int parse(char *buff, int bytesread, requestinfo *rinfo);

int get(requestinfo *rinfo);

int put(requestinfo *rinfo);
