/** pshm_ucase_bounce.c
 *   Licensed under GNU General Public License v2 or later.
 */
#include "pshm_ucase.h"
#include <cctype>
#include <cstring>
#include <wukong/utils/shm/ShareMemoryObject.h>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s /shm-path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* shmpath = argv[1];

    /* Create shared memory object and set its size to the size
       of our structure. */

    ShareMemoryObject shm(sizeof(Shmbuf), shmpath);
    WK_CHECK_FUNC_RET_WITH_EXIT(shm.create());

    Shmbuf* shmp;
    WK_CHECK_FUNC_RET_WITH_EXIT(shm.open(reinterpret_cast<void**>(&shmp), true));

    /* Initialize semaphores as process-shared, with value 0. */

    if (sem_init(&shmp->sem1, 1, 0) == -1)
        errExit("sem_init-sem1");
    if (sem_init(&shmp->sem2, 1, 0) == -1)
        errExit("sem_init-sem2");

    /* Wait for 'sem1' to be posted by peer before touching
       shared memory. */

    if (sem_wait(&shmp->sem1) == -1)
        errExit("sem_wait");

    /* Convert data in shared memory into upper case. */

    for (int j = 0; j < shmp->cnt; j++)
        shmp->buf[j] = (char)toupper(shmp->buf[j]);

    /* Post 'sem2' to tell the peer that it can now
       access the modified data in shared memory. */

    if (sem_post(&shmp->sem2) == -1)
        errExit("sem_post");

    /* Unlink the shared memory object. Even if the peer process
       is still using the object, this is okay. The object will
       be removed only after all open references are closed. */
}