#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>

#define SHM_NAME "/shared_mem"
#define SEM_NAME "/semaphore"

int main() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 1024);

    char *shared_mem = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);

    while (1) {
        char input[256];
        printf("Введите строку: ");
        fgets(input, sizeof(input), stdin);

        if (rand() % 100 < 80) {
            strcpy(shared_mem, input);
        } else {
            strcpy(shared_mem + 512, input);
        }

        sem_post(sem);
    }

    munmap(shared_mem, 1024);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}