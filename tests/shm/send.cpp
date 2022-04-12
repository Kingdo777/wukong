/** pshm_ucase_send.c
 *  Licensed under GNU General Public License v2 or later
 */
#include "pshm_ucase.h"
#include <cstring>
#include <wukong/utils/shm/ShareMemoryObject.h>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s /shm-path string\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* shmpath = argv[1];
    char* string        = argv[2];
    size_t len          = strlen(string);

    if (len > BUF_SIZE)
    {
        fprintf(stderr, "String is too long\n");
        exit(EXIT_FAILURE);
    }

    /* Open the existing shared memory object and map it
      into the caller's address space. */

    Shmbuf* shmp;
    WK_CHECK_FUNC_RET_WITH_EXIT(ShareMemoryObject::open(shmpath, sizeof(Schbuf), (void**)&shmp, true));

    /* Copy data into the shared memory object. */
    shmp->cnt = len;
    memcpy(&shmp->buf, string, len);

    /* Tell peer that it can now access shared memory. */

    if (sem_post(&shmp->sem1) == -1)
        errExit("sem_post");

    /* Wait until peer says that it has finished accessing
      the shared memory. */

    if (sem_wait(&shmp->sem2) == -1)
        errExit("sem_wait");

    /* Write modified data in shared memory to standard output. */

    write(STDOUT_FILENO, &shmp->buf, len);
    write(STDOUT_FILENO, "\n", 1);

    exit(EXIT_SUCCESS);
}