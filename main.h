#ifndef MAIN_HEADER
#define MAIN_HEADER

#define _GNU_SOURCE                                                             //macro and headers needed for the clone() syscall
#include <sched.h>
#include <signal.h>

#include <strings.h>                                                            //strlen() function
#include <stdint.h>                                                             //types by size definitions
#include <stdlib.h>                                                             //atoi() function
#include <unistd.h>                                                             //read(), write() syscalls
#include <fcntl.h>                                                              //write() syscall
#include <stdio.h>                                                              //printf()

#define STACK_SIZE 8192                                                         //a default stack size, sufficient for what we're doing
#define SET_STACK(arg)  sbrk(arg) + arg                                         //returns the address of the top of the newly allocated stack space

#define OPEN_FILE_ERR  0x01
#define READ_FILE_ERR  0x02
#define WRITE_FILE_ERR 0x03

#define MAIN_ERR_MSG "usage :\nftp -TYPE ADDRESS PORT\n     [-s] or [-c]\n"

typedef uint8_t errno;

int error(char *message, errno err_type);

#endif
