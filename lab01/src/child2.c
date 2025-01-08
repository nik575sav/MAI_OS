#include <unistd.h>
#include <string.h>

int main() {
    char input[1024];
    ssize_t bytes_read;
    char output[1024];

    while ((bytes_read = read(STDIN_FILENO, input, sizeof(input))) > 0) {
        ssize_t output_index = 0;
        int space_flag = 0;

        for (ssize_t i = 0; i < bytes_read; i++) {
            if (input[i] == ' ') {
                if (!space_flag) {
                    output[output_index++] = ' ';
                    space_flag = 1;
                }
            } else {
                output[output_index++] = input[i];
                space_flag = 0;
            }
        }

        write(STDOUT_FILENO, output, output_index);
    }

    return 0;
}