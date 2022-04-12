//
// Created by kingdo on 2022/4/8.
//

#ifndef SHARE_MEMORY_OBJECT_PSHM_UCASE_H
#define SHARE_MEMORY_OBJECT_PSHM_UCASE_H
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define errExit(msg)        \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

#define BUF_SIZE 1024 /* Maximum size for exchanged string */

/* Define a structure that will be imposed on the shared
   memory object */

typedef struct Shmbuf {
    sem_t sem1;         /* POSIX unnamed semaphore */
    sem_t sem2;         /* POSIX unnamed semaphore */
    size_t cnt;         /* Number of bytes used in 'buf' */
    char buf[BUF_SIZE]; /* Data being transferred */
} Schbuf;
#endif//SHARE_MEMORY_OBJECT_PSHM_UCASE_H
