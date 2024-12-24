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
#include <errno.h>

#define SHM_NAME "/shared_memory111"
#define BUFFER_SIZE 4096

void print_message(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

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
        print_message("Usage: path file1 file2\n");
        exit(EXIT_FAILURE);
    }

    srand(getpid());

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

        print_message("Input strings (press ENTER to exit): ");
        int bytes_read = read(STDIN_FILENO, input, BUFFER_SIZE);
        if (bytes_read <= 0) {
            perror("read");
            continue;
        }
        input[bytes_read - 1] = '\0';

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