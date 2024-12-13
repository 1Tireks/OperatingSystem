#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

const int MAX_LENGTH = 255;

int main(int argc, char* argv[]) {
    char file_name[MAX_LENGTH];
    char mmapped_file_name[MAX_LENGTH];
    strcpy(file_name, argv[1]);
    strcpy(mmapped_file_name, argv[2]);

    char semaphore_name[MAX_LENGTH];
    strcpy(semaphore_name, argv[3]);
    char semaphoreForParent_name[MAX_LENGTH];
    strcpy(semaphoreForParent_name, argv[4]);

    int file_descriptor = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if (file_descriptor == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int mmapped_file_descriptor = shm_open(mmapped_file_name, O_RDWR, 0777);
    if (mmapped_file_descriptor == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    char* mmapped_file_pointer = mmap(NULL, MAX_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, mmapped_file_descriptor, 0);
    if (mmapped_file_pointer == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t* semaphore = sem_open(semaphore_name, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_t* semaphoreForParent = sem_open(semaphoreForParent_name, 0);
    if (semaphoreForParent == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    char vowels[] = {'a', 'e', 'i', 'o', 'u', 'y', 'A', 'E', 'I', 'O', 'U', 'Y'};
    char string[MAX_LENGTH];
    while (1) {
        sem_wait(semaphore);

        strcpy(string, mmapped_file_pointer);

        sem_post(semaphoreForParent);

        if (strlen(string) == 0) {
            break;
        }

        for (int index = 0; index < strlen(string); ++index) {
            if (memchr(vowels, string[index], sizeof(vowels)) == NULL) {
                write(file_descriptor, &string[index], 1);
            }
        }
    }

    munmap(mmapped_file_pointer, MAX_LENGTH);
    sem_close(semaphore);
    sem_close(semaphoreForParent);
    close(file_descriptor);

    return 0;
}