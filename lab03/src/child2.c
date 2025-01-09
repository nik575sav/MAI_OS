#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>

#define SHM_SIZE 1024

void handle_error(const char *msg) {
    write(2, msg, strlen(msg));
    _exit(EXIT_FAILURE);
}

int main() {
    // Получение ключа для разделяемой памяти
    key_t key = ftok("shared_memory", 65);
    if (key == -1) handle_error("Error: ftok failed\n");

    // Получение ID разделяемой памяти
    int shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid == -1) handle_error("Error: shmget failed\n");

    // Присоединение к разделяемой памяти
    char *shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) handle_error("Error: shmat failed\n");

    // Удаление подряд идущих пробелов
    char *read_ptr = shared_memory;
    char *write_ptr = shared_memory;
    int space_flag = 0;

    while (*read_ptr != '\0') {
        if (*read_ptr == ' ') {
            if (!space_flag) {
                *write_ptr++ = ' ';
                space_flag = 1; // Запоминаем, что пробел уже записан
            }
        } else {
            *write_ptr++ = *read_ptr;
            space_flag = 0; // Обнуляем флаг, так как встретился не пробел
        }
        read_ptr++;
    }
    *write_ptr = '\0'; // Завершаем строку

    // Отсоединение от разделяемой памяти
    if (shmdt(shared_memory) == -1) handle_error("Error: shmdt failed\n");

    return 0;
}
