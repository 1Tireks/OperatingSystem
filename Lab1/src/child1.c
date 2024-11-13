#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define VOWELS "AEIOUaeiou"

void remove_vowels(char *str) {
    char *ptr = str;
    while (*str) {
        if (!strchr(VOWELS, *str)) {
            *ptr++ = *str;
        }
        str++;
    }
    *ptr = '\0';
}

int main() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        buffer[bytes_read] = '\0';
        remove_vowels(buffer);
        write(STDOUT_FILENO, buffer, strlen(buffer)); // Запись результата в файл после dup2
    }

    return 0;
}