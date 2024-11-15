#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define PROBABILITY 0.8

void print_error(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
    write(STDERR_FILENO, "\n", 1);
    exit(EXIT_FAILURE);
}

void print_message(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

int main() {
    int pipe1[2], pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        print_error("Ошибка при создании pipe.\n");
    }

    srand(getpid());

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Ввод имени файла для child1
    print_message("Введи имя файла для child1: ");
    bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
    if (bytes_read <= 0) {
        print_error("Ошибка при чтении имени файла для child1.\n");
    }
    buffer[bytes_read - 1] = '\0';

    int file1_fd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
    if (file1_fd == -1) {
        print_error("Ошибка при открытии файла для child1.\n");
    }

    // Ввод имени файла для child2
    print_message("Введи имя файла для child2: ");
    bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
    if (bytes_read <= 0) {
        print_error("Ошибка при чтении имени файла для child2.\n");
    }
    buffer[bytes_read - 1] = '\0';

    int file2_fd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
    if (file2_fd == -1) {
        print_error("Ошибка при открытии файла для child2.\n");
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        print_error("Ошибка при создании дочернего процесса child1");
    } else if (pid1 == 0) { // Первый дочерний процесс (child1)
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(file2_fd);
        dup2(pipe1[0], STDIN_FILENO); // Читаем из pipe1
        dup2(file1_fd, STDOUT_FILENO); // Пишем в файл для child1
        close(pipe1[0]);
        close(file1_fd);
        execl("./child1", "./child1", NULL);
        print_error("Ошибка при запуске программы первого дочернего процесса.\n");
    } 

    pid_t pid2 = fork();
    if (pid2 == -1) {
        print_error("Ошибка при создании дочернего процесса child2");
    } else if (pid2 == 0) { // Второй дочерний процесс (child2)
        close(pipe2[1]);
        close(pipe1[0]);
        close(pipe1[1]);
        close(file1_fd);
        dup2(pipe2[0], STDIN_FILENO); // Читаем из pipe2
        dup2(file2_fd, STDOUT_FILENO); // Пишем в файл для child2
        close(pipe2[0]);
        close(file2_fd);
        execl("./child2", "./child2", NULL);
        print_error("Ошибка при запуске программы второго дочернего процесса.\n");
    } 

    // Родительский процесс
    close(pipe1[0]);
    close(pipe2[0]);
    close(file1_fd);
    close(file2_fd);

    print_message("Вводите строчечки:\n");

    for (int i = 0; i < 10; i++) {
        bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (((double)rand() / RAND_MAX) < PROBABILITY) {
            if (write(pipe1[1], buffer, bytes_read) != bytes_read) {
                print_error("Ошибка при записи в pipe1.\n");
            }
        } else {
            if (write(pipe2[1], buffer, bytes_read) != bytes_read) {
                print_error("Ошибка при записи в pipe2.\n");
            }
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    if (wait(NULL) == -1) {
        print_error("Ошибка при ожидании завершения дочернего процесса.\\n");
    }
    if (wait(NULL) == -1) {
        print_error("Ошибка при ожидании завершения дочернего процесса.\\n");
    }

    return 0;
}

// output1.txt
// output2.txt