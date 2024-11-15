#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <complex.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 32
#define MAX_SIZE 20

typedef struct {
    int row;
    int col;
    int size;
    double complex (*aPtr)[MAX_SIZE];
    double complex (*bPtr)[MAX_SIZE];
    double complex (*cPtr)[MAX_SIZE];
    pthread_mutex_t *mutex;
} __dataThread;

void print_error(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
    write(STDERR_FILENO, "\n", 1);
    exit(EXIT_FAILURE);
}

void print_message(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

void* matrix_multiply(void* matrix) {
    __dataThread* m = (__dataThread*)matrix;

    double complex result = 0.0 + 0.0*I;

    for (int index = 0; index < m->size; index++) {
        result += m->aPtr[m->row][index] * m->bPtr[index][m->col];
    }

    pthread_mutex_lock(m->mutex);
    m->cPtr[m->row][m->col] = result;
    pthread_mutex_unlock(m->mutex);

    return NULL;
}

void convert_to_base(const double num, char* result, int base) {

    int num_abs = num;
    int i_num = 0;

    do {
        int digit =  num_abs % base;

        result[i_num++] = digit + '0';

        num_abs /= base;

    } while (num_abs > 0.1);

    result[i_num] = '\0';

    for (int i = 0; i < i_num / 2; i++) {
        char temp = result[i];
        result[i] = result[i_num - i - 1];
        result[i_num - i - 1] = temp;
    }

}
 
int main() {

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Ввод макс потоков
    print_message("Введи максимальное количество потоков работающие в одно время: ");
    bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
    if (bytes_read <= 0) {
        print_error("Ошибка при чтении.\n");
    }
    buffer[bytes_read - 1] = '\0';

    const int maxThreads = atoi(buffer);

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    pthread_t threads[MAX_SIZE * MAX_SIZE];
    __dataThread threadData[MAX_SIZE * MAX_SIZE];

    double complex A[MAX_SIZE][MAX_SIZE], B[MAX_SIZE][MAX_SIZE], C[MAX_SIZE][MAX_SIZE];
    int N = 20; // Размер матрицы

    // Инициализация матриц A, B и C
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = i + j * I;
            B[i][j] = (i - j) + (i + j) * I;
            C[i][j] = 0;
        }
    }

    int thrCount = 0;
    struct timespec start, end;

    // Получаем время
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Пробегаемся по всем ячейкам матриц
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            // Если создали столько потоков сколько нужно (maxThreads)
            // Даем им работать и обнуляем счетчик
            if (thrCount == maxThreads) {
                for (int t = 0; t < maxThreads; t++) {
                    pthread_join(threads[t], NULL);
                }
                thrCount = 0;
            }

            threadData[thrCount] = (__dataThread){i, j, N, A, B, C, &mutex};

            if (pthread_create(&threads[thrCount], NULL, matrix_multiply, &threadData[thrCount])) {
                print_message("Ошибка при создании потока.\n");
            }

            thrCount++;
        }
    }

    // Выполняем то что осталось
    for (int t = 0; t < thrCount; t++) {
        pthread_join(threads[t], NULL);
    }

    // Получаем время
    clock_gettime(CLOCK_MONOTONIC, &end);

    double timeResult = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1.0E3;

    print_message("Количество потоков: ");
    convert_to_base((double)maxThreads, buffer, 10);
    print_message(buffer);
    print_message("\n");
    print_message("Затраченное время в мкс: ");
    convert_to_base(timeResult, buffer, 10);
    print_message(buffer);
    print_message("\n");

    pthread_mutex_destroy(&mutex);

    return 0;
}