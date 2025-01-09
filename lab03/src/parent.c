#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define SHM_SIZE 1024

// Вывод ошибки и завершение программы
void handle_error(const char *msg) {
    const char *error_suffix = ": Ошибка\n";
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, error_suffix, strlen(error_suffix));
    _exit(EXIT_FAILURE);
}

// Вывод сообщения в стандартный вывод
void write_message(const char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

// Чтение строки из ввода
void read_message(char *buffer, size_t size) {
    ssize_t bytes_read = read(STDIN_FILENO, buffer, size - 1);
    if (bytes_read <= 0) handle_error("Ошибка чтения");
    buffer[bytes_read - 1] = '\0'; // Удаляем символ новой строки
}

int main() {
    // Создание файла для ключа
    int fd = open("shared_memory", O_CREAT | O_RDWR, 0666);
    if (fd == -1) handle_error("Ошибка создания файла");
    close(fd);

    // Создание ключа для разделяемой памяти
    key_t key = ftok("shared_memory", 65);
    if (key == -1) handle_error("Ошибка ftok");

    // Создание сегмента разделяемой памяти
    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) handle_error("Ошибка shmget");

    // Подключение к разделяемой памяти
    char *shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) handle_error("Ошибка shmat");

    write_message("Введите строку (или пустую строку для выхода): ");
    char input_buffer[SHM_SIZE];

    // Основной цикл обработки ввода
    while (1) {
        read_message(input_buffer, SHM_SIZE);

        // Проверка на завершение
        if (strcmp(input_buffer, "") == 0) break;

        // Копирование введенной строки в разделяемую память
        strcpy(shared_memory, input_buffer);

        // Запуск первого дочернего процесса
        pid_t pid1 = fork();
        if (pid1 == -1) handle_error("Ошибка fork (child1)");

        if (pid1 == 0) {
            execl("./child1", "./child1", NULL);
            handle_error("Ошибка execl (child1)");
        }

        wait(NULL); // Ожидание завершения первого процесса

        // Запуск второго дочернего процесса
        pid_t pid2 = fork();
        if (pid2 == -1) handle_error("Ошибка fork (child2)");

        if (pid2 == 0) {
            execl("./child2", "./child2", NULL);
            handle_error("Ошибка execl (child2)");
        }

        wait(NULL); // Ожидание завершения второго процесса

        // Вывод результата
        write_message("Результат обработки: ");
        write_message(shared_memory);
        write_message("\nВведите строку (или пустую строку для выхода): ");
    }

    // Отключение от разделяемой памяти и удаление сегмента
    if (shmdt(shared_memory) == -1) handle_error("Ошибка shmdt");
    if (shmctl(shmid, IPC_RMID, NULL) == -1) handle_error("Ошибка shmctl");

    return 0;
}
