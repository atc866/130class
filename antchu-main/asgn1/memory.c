#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include <unistd.h>
#include <fcntl.h>
#define BUFF_SIZE 4102
char buff[BUFF_SIZE];
//read file function
int readfile(char *filename) {
    ssize_t bytesWritten = 0;
    ssize_t fd = open(filename, O_RDWR);
    ssize_t bytesRead = read(fd, buff, BUFF_SIZE);
    //use code given in practicum
    if (fd == -1) {
        fprintf(stderr, "Invalid Command\n");
        return 1;
    }
    while (bytesRead > 0) {
        if (bytesRead < 0) {
            fprintf(stderr, "Operation Failed\n");
            return 1;
        } else if (bytesRead > 0) {
            bytesWritten = 0;
            while (bytesWritten < bytesRead) {
                size_t bytes = write(STDOUT_FILENO, buff + bytesWritten, bytesRead - bytesWritten);
                if (bytes <= 0) {
                    fprintf(stderr, "Operation Failed\n");
                    close(fd);
                    return 1;
                }
                bytesWritten += bytes;
            }
        }
        bytesRead = read(fd, buff, BUFF_SIZE);
    }
    close(fd);
    return 0;
}
int main() {
    ssize_t bytesRead = 0;
    ssize_t readtotal = bytesRead;
    //get intial read/used for when entering input from keyboard
    do {
        bytesRead = read(STDIN_FILENO, buff + readtotal, BUFF_SIZE - readtotal);
        if (bytesRead == -1) {
            printf("?????");
            return 1;
        }
        readtotal += bytesRead;

    } while (bytesRead > 0 && readtotal < BUFF_SIZE);
    if (bytesRead == -1) {
        printf("????");
        return 1;
    }
    const char delimit[2] = "\n";
    char *token;
    char *filename;
    int count = 0;
    char inputcopy[BUFF_SIZE];
    //copy buffer to inputcopy for future use
    for (int i = 0; i < readtotal; i++) {
        inputcopy[i] = buff[i];
    }
    //delimit first newline to get actual command
    token = strtok(buff, delimit);
    if (strcmp(token, "get") == 0) {
        //check for valid command
        while (token != NULL) {
            token = strtok(NULL, delimit);
            if (count == 0) {
                filename = token;
            }
            count += 1;
        }
        if (count < 2) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        if (count > 2) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        if (inputcopy[readtotal - 1] != '\n'
            || (int) readtotal > (int) (4 + strlen(filename) + 1)) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        if (strlen(filename) > 4096) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        //valid command then use readfile function
        if (readfile(filename) == 1) {
            return 1;
        }
        return 0;
    }
    //if set command
    else if (strcmp(token, "set") == 0) {
        filename = strtok(NULL, delimit);
        char *length = strtok(NULL, delimit);
        //check for valid command
        if (filename == NULL || length == NULL) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        int len = atoi(length);
        char message[BUFF_SIZE];
        if (strlen(filename) > 4096) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        int i = 0;
        //get message from buffer using inputcopy of buffer
        while ((i < len)
               && (i < (int) (readtotal - (strlen(filename) + 1 + strlen(length) + 1 + 4)))) {
            message[i] = inputcopy[i + (strlen(filename) + 1 + strlen(length) + 1 + 4)];
            i += 1;
        }
        ssize_t bytesWritten = 0;
        ssize_t fd;
        //open file for writing and use practicum code to write to file
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (fd == -1) {
            fprintf(stderr, "Invalid Command\n");
            close(fd);
            return 1;
        }
        //change bytesread to how many bytes in message
        bytesRead = readtotal - (strlen(filename) + 1 + strlen(length) + 1 + 4);
        if (len < bytesRead) {
            bytesRead = len;
        }
        //practicum code
        while (bytesRead > 0) {
            if (bytesRead < 0) {
                fprintf(stderr, "Operation Failed\n");
                close(fd);
                return 1;
            } else {
                while (bytesWritten < bytesRead) {
                    ssize_t bytes = write(fd, message, bytesRead);
                    if (bytes <= 0) {
                        fprintf(stderr, "cannot write to stdout\n");
                        close(fd);
                        return 1;
                    }
                    bytesWritten += bytes;
                }
            }
            //if readtotal from first read of command was less than buffsize than dont need to read again and end
            if (readtotal < BUFF_SIZE) {
                close(fd);
                fprintf(stdout, "OK\n");
                return 0;
            }
            //read again for bigger message
            else {
                bytesRead = read(STDIN_FILENO, buff, BUFF_SIZE);
                for (int i = 0; i < bytesRead; i++) {
                    message[i] = buff[i];
                }
            }
            bytesWritten = 0;
        }
        close(fd);
        fprintf(stdout, "OK\n");
        return 0;
    } else {
        fprintf(stderr, "Invalid Command\n");
        return 1;
    }
    return 0;
}
