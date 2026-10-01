#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#define BUF_SIZE 2048
#define CHUNK_SIZE 1024

int main(void) {
    char buf[BUF_SIZE];
    int head = 0, tail = 0, bytes = 0;
    bool eof = false, last = false, num_ends = false;
    int num = 0, res = 0;
    int sign = 1, cnt = 0;

    while (!last) {
        if (!eof && BUF_SIZE - bytes >= CHUNK_SIZE) {
            int n = BUF_SIZE - tail;
            if (n > CHUNK_SIZE) n = CHUNK_SIZE;
            int r = read(STDIN_FILENO, buf + tail, n);
            if (r <= 0) {
                eof = true;
            } else {
                tail = (tail + r) % BUF_SIZE;
                bytes += r;
            }
        }

        char c;
        if (bytes > 0) {
            c = buf[head];
            head = (head + 1) % BUF_SIZE;
            bytes--;
        } else if (eof) {
            c = '\n';
            last = true;
        } else {
            continue;
        }

        if (c >= '0' && c <= '9') {
            num = num * 10 + (c - '0');
            num_ends = true;
            continue;
        }

        if (num_ends) {
            if (cnt > 0 && num == 0) {
                write(STDOUT_FILENO, "Ошибка: деление на ноль\n", 44);
                _exit(1);
            }
            res = cnt > 0 ? res / num * sign : num * sign;
            cnt++;
            num = 0;
            num_ends = false;
            sign = 1;
        }

        if (c == '-') sign = -1;

        if (c == '\n') {
            if (cnt > 0) {
                char out[32];
                int len = snprintf(out, sizeof(out), "%d\n", res);
                write(STDOUT_FILENO, out, len);
            }
            cnt = 0;
            res = 0;
            sign = 1;
        }
    }

    _exit(0);
}
