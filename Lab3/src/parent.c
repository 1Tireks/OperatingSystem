#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <sys/wait.h>

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

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s file1 file2\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(SharedMemory)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    SharedMemory *shm = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_init(&shm->sem_parent, 1, 1);
    sem_init(&shm->sem_child1, 1, 0);
    sem_init(&shm->sem_child2, 1, 0);
    shm->exit_flag = false;

    pid_t child1 = fork();
    if (child1 == 0) {
        execlp("./child", "./child", "child1", argv[1], NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    pid_t child2 = fork();
    if (child2 == 0) {
        execlp("./child", "./child", "child2", argv[2], NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    char input[BUFFER_SIZE];
    while (true) {
        sem_wait(&shm->sem_parent);

        printf("Input strings (press ENTER to exit): ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("fgets");
            continue;
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (input[0] == '\0') {
            shm->exit_flag = true;
            sem_post(&shm->sem_child1);
            sem_post(&shm->sem_child2);
            break;
        }

        if (rand() % 100 > 80) {
            strncpy(shm->buffer1, input, BUFFER_SIZE - 1);
            shm->buffer1[BUFFER_SIZE - 1] = '\0';
            sem_post(&shm->sem_child1);
        } else {
            strncpy(shm->buffer2, input, BUFFER_SIZE - 1);
            shm->buffer2[BUFFER_SIZE - 1] = '\0';
            sem_post(&shm->sem_child2);
        }
    }

    wait(NULL);
    wait(NULL);

    sem_destroy(&shm->sem_parent);
    sem_destroy(&shm->sem_child1);
    sem_destroy(&shm->sem_child2);
    munmap(shm, sizeof(SharedMemory));
    shm_unlink(SHM_NAME);

    return 0;
}