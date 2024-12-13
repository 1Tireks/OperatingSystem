
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

const int MAX_LENGTH = 255;

// Функция для создания ребёнка
int create_process() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

int main() {
    const int number_processes = 2;
    const char *mmapped_file_names[number_processes] = {"mmaped_file_1", "mmaped_file_2"};
    const char *semaphores_names[number_processes] = {"/semaphoreOne", "/semaphoreTwo"};
    const char *semaphoresForParent_names[number_processes] = {"/semaphoresForParentOne", "/semaphoresForParentTwo"};

    // Считываем имена выходных файлов для дочерних процессов
    char file_names[number_processes][MAX_LENGTH];
    for (int i = 0; i < number_processes; ++i) {
        printf("Enter filename for child%d: ", i + 1);
        if (fgets(file_names[i], MAX_LENGTH, stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        // Убираем перенос строки
        size_t len = strlen(file_names[i]);
        if (file_names[i][len - 1] == '\n') {
            file_names[i][len - 1] = '\0';
        }
    }

    // Создаем memory-mapped файлы и получаем их дескрипторы
    int mmapped_file_descriptors[number_processes];
    char *mmapped_file_pointers[number_processes];
    for (int i = 0; i < number_processes; ++i) {
        // Удаляем существующую разделяемую память
        shm_unlink(mmapped_file_names[i]);

        // Открываем разделяемую память
        mmapped_file_descriptors[i] = shm_open(mmapped_file_names[i], O_RDWR | O_CREAT | O_TRUNC, 0777);
        if (mmapped_file_descriptors[i] == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }

        // Устанавливаем размер
        if (ftruncate(mmapped_file_descriptors[i], MAX_LENGTH) == -1) {
            perror("ftruncate");
            exit(EXIT_FAILURE);
        }

        // Проецируем в память
        mmapped_file_pointers[i] = mmap(NULL, MAX_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, mmapped_file_descriptors[i], 0);
        if (mmapped_file_pointers[i] == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    // создаем семафоры
    sem_t *semaphores[number_processes][2];
    for (int i = 0; i < number_processes; ++i) {
        semaphores[i][0] = sem_open(semaphores_names[i], O_CREAT, 0777, 0);
        if (semaphores[i][0] == SEM_FAILED) {
            perror("sem_open for child semaphores");
            exit(EXIT_FAILURE);
        }

        semaphores[i][1] = sem_open(semaphoresForParent_names[i], O_CREAT, 0777, 1);
        if (semaphores[i][1] == SEM_FAILED) {
            perror("sem_open for parent semaphores");
            exit(EXIT_FAILURE);
        }
    }

    // Создаем дочерние процессы
    for (int index = 0; index < number_processes; ++index) {
        pid_t process_id = create_process();
        if (process_id == 0) {
            // Дочерний процесс
            execl("./child", "child", file_names[index], mmapped_file_names[index], semaphores_names[index], semaphoresForParent_names[index], NULL);
            perror("exec");
            exit(EXIT_FAILURE);
        }
    }

    // Считываем вводные данные из консоли и передаем детям
    char string[MAX_LENGTH];
    int counter = 0;
    while (fgets(string, MAX_LENGTH, stdin)) {
        int child_index = counter % 2;
        sem_wait(semaphores[child_index][1]);

        strcpy(mmapped_file_pointers[child_index], string);
        printf("Main process: sending string '%s' to child %d\n", string, child_index);

        sem_post(semaphores[child_index][0]);
        counter++;
    }

    // Оповещаем процессы о завершении
    for (int i = 0; i < number_processes; ++i) {
        sem_wait(semaphores[i][1]);
        mmapped_file_pointers[i][0] = '\0';  // Посылаем пустую строку как сигнал завершения
        sem_post(semaphores[i][0]);
    }

    wait(NULL);  // Ожидаем завершения всех дочерних процессов

    // Очистка ресурсов
    for (int i = 0; i < number_processes; ++i) {
        munmap(mmapped_file_pointers[i], MAX_LENGTH);
        shm_unlink(mmapped_file_names[i]);
        close(mmapped_file_descriptors[i]);
        sem_close(semaphores[i][0]);
        sem_close(semaphores[i][1]);
        sem_unlink(semaphores_names[i]);
        sem_unlink(semaphoresForParent_names[i]);
    }

    return 0;
}