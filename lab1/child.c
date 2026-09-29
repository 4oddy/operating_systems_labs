#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>

void write_string(int fd, const char *str) {
    size_t len = 0;
    while (str[len] != '\0') len++;
    write(fd, str, len);
}

int read_line(int fd, char *buf, int max_size) {
    int b_read = 0;
    char ch;
    ssize_t res;

    while (b_read < max_size - 1) {
        res = read(fd, &ch, 1);
        if (res <= 0) {
            break;
        }
        if (ch == '\r') continue; 
        if (ch == '\n') break;   

        buf[b_read++] = ch;
    }

    buf[b_read] = '\0';
    if (res <= 0 && b_read == 0) {
        return 0; 
    }
    return 1;
}

int main(void) {
    char buf[1024];

    while (read_line(STDIN_FILENO, buf, sizeof(buf))) {
        int numbers[3];
        int count = 0;
        int i = 0;

        while (buf[i] != '\0') {
            while (buf[i] == ' ' || buf[i] == '\t') {
                i++;
            }
            if (buf[i] == '\0') break;

            int sign = 1;
            if (buf[i] == '-') {
                sign = -1;
                i++;
            } else if (buf[i] == '+') {
                i++;
            }

            if (buf[i] < '0' || buf[i] > '9') {
                while (buf[i] != '\0' && buf[i] != ' ' && buf[i] != '\t') {
                    i++;
                }
                continue;
            }

            int num = 0;
            while (buf[i] >= '0' && buf[i] <= '9') {
                num = num * 10 + (buf[i] - '0');
                i++;
            }
            
            numbers[count++] = num * sign;
        }

        if (count == 0) continue;

        int result = numbers[0];
        bool zero_division_err = 0;

        for (int i = 1; i < count; i++) {
            if (numbers[i] == 0) {
                zero_division_err = 1;
                break;
            }
            result /= numbers[i];
        }

        if (zero_division_err) {
            write_string(STDOUT_FILENO, "Ошибка: деление на ноль\n");
            _exit(1); 
        }

        char out_buf[32];
        snprintf(out_buf, sizeof(out_buf), "%d", result);
        write_string(STDOUT_FILENO, out_buf);
        write_string(STDOUT_FILENO, "\n");
    }

    _exit(0);
}
