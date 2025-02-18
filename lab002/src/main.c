#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/mman.h>

#define MAX_THREADS 8 // Максимальное количество потоков

// Структура для передачи параметров в потоки
typedef struct {
    int *arr;
    int low;
    int count;
    int direction;
    int *active_threads;
    int max_threads;
} thread_args;

// Функция для обмена элементов
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Функция для преобразования числа в строку и записи в буфер
int int_to_str(int num, char *buffer, int buffer_size) {
    int len = 0;
    int temp = num;
    do {
        if (len >= buffer_size - 1) break;
        buffer[len++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);

    // Реверс строки
    for (int i = 0; i < len / 2; i++) {
        char tmp = buffer[i];
        buffer[i] = buffer[len - i - 1];
        buffer[len - i - 1] = tmp;
    }
    buffer[len] = '\0'; // Добавляем нулевой символ
    return len;
}

// Функция для выполнения битонического слияния
void bitonic_merge(int arr[], int low, int count, int direction) {
    if (count > 1) {
        int k = count / 2;
        for (int i = low; i < low + k; i++) {
            if (direction == (arr[i] > arr[i + k])) {
                swap(&arr[i], &arr[i + k]);
            }
        }
        bitonic_merge(arr, low, k, direction);
        bitonic_merge(arr, low + k, k, direction);
    }
}

// Функция для выполнения битонической сортировки
void *bitonic_sort_thread(void *arg) {
    thread_args *args = (thread_args *)arg;
    int *arr = args->arr;
    int low = args->low;
    int count = args->count;
    int direction = args->direction;
    int *active_threads = args->active_threads;
    int max_threads = args->max_threads;

    if (count > 1) {
        int k = count / 2;
        pthread_t thread1 = 0, thread2 = 0;

        if (*active_threads < max_threads) {
            thread_args args1 = {arr, low, k, 1, active_threads, max_threads};
            pthread_create(&thread1, NULL, bitonic_sort_thread, &args1);
            __sync_fetch_and_add(active_threads, 1);
        } else {
            bitonic_sort_thread(&(thread_args){arr, low, k, 1, active_threads, max_threads});
        }

        if (*active_threads < max_threads) {
            thread_args args2 = {arr, low + k, k, 0, active_threads, max_threads};
            pthread_create(&thread2, NULL, bitonic_sort_thread, &args2);
            __sync_fetch_and_add(active_threads, 1);
        } else {
            bitonic_sort_thread(&(thread_args){arr, low + k, k, 0, active_threads, max_threads});
        }

        if (thread1) {
            pthread_join(thread1, NULL);
            __sync_fetch_and_sub(active_threads, 1);
        }

        if (thread2) {
            pthread_join(thread2, NULL);
            __sync_fetch_and_sub(active_threads, 1);
        }

        bitonic_merge(arr, low, count, direction);
    } else if (count == 1) {
        // Обработка одиночного элемента для гарантии корректности
        bitonic_merge(arr, low, count, direction);
    }

    return NULL;
}

// Функция для печати массива
void print_array(int arr[], int size) {
    char buffer[128];
    for (int i = 0; i < size; i++) {
        int len = int_to_str(arr[i], buffer, sizeof(buffer));
        if (write(1, buffer, len) == -1) {
            write(2, "Error: write failed\n", 20);
        }
        if (write(1, " ", 1) == -1) {
            write(2, "Error: write failed\n", 20);
        }
    }
    write(1, "\n", 1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        const char *usage = "Usage: ./bitonic_sort <array size> <max threads>\n";
        write(1, usage, strlen(usage));
        return 1;
    }

    int n = atoi(argv[1]);
    int max_threads = atoi(argv[2]);
    if (max_threads <= 0) {
        max_threads = MAX_THREADS;
    }

    int *arr = mmap(NULL, n * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (arr == MAP_FAILED) {
        write(2, "Error: mmap failed\n", 18);
        return 1;
    }

    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 100;
    }

    write(1, "Initial array:\n", 15);
    print_array(arr, n);

    int active_threads = 1;
    thread_args args = {arr, 0, n, 1, &active_threads, max_threads};
    bitonic_sort_thread(&args);

    write(1, "\nSorted array:\n", 15);
    print_array(arr, n);

    if (munmap(arr, n * sizeof(int)) == -1) {
        write(2, "Error: munmap failed\n", 20);
    }

    return 0;
}
