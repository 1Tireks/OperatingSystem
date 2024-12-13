#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_NAME "/shared_mem"
#define SEM_NAME "/semaphore"

void remove_vowels(char *str) {
    char *ptr_read = str, *ptr_write = str;
    while (*ptr_read) {
        if (!strchr("AEIOUaeiou", *ptr_read)) {
            *ptr_write++ = *ptr_read;
        }
        ptr_read++;
    }
    *ptr_write = '\\0';
}

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    char *shared_mem = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    sem_t *sem = sem_open(SEM_NAME, 0);

    while (1) {
        sem_wait(sem);
        if (1) {
            remove_vowels(shared_mem);
            printf("Результат: %s\\n", shared_mem);
        }
    }

    munmap(shared_mem, 1024);
    sem_close(sem);

    return 0;
}