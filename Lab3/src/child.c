#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

#define SHM_NAME "/shared_memory"
#define BUFFER_SIZE 4096

typedef struct {
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    sem_t sem_parent;
    sem_t sem_child1;
    sem_t sem_child2;
    bool exit_flag;
} SharedMemory;

void delete_vowels(char *str) {
    if (!str) return;

    const char *vowels = "AEIOUYaeiouy";
    size_t j = 0;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (!strchr(vowels, str[i])) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <child_id> <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0600);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    SharedMemory *shm = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    int output_file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (output_file == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    sem_t *my_sem;
    char *my_buffer;

    if (strcmp(argv[1], "child1") == 0) {
        my_sem = &shm->sem_child1;
        my_buffer = shm->buffer1;
    } else {
        my_sem = &shm->sem_child2;
        my_buffer = shm->buffer2;
    }

    while (true) {
        sem_wait(my_sem);

        if (shm->exit_flag) break;

        delete_vowels(my_buffer);

        size_t len = strlen(my_buffer);
        if (write(output_file, my_buffer, len) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        write(output_file, "\n", 1);

        sem_post(&shm->sem_parent);
    }

    close(output_file);
    munmap(shm, sizeof(SharedMemory));
    return 0;
}