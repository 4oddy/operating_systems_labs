#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define BUF_SIZE 2048
#define CHUNK_SIZE 1024
#define SHM_SIZE 4096

char *shm_buf;
sem_t *sem;

void send_msg(const char *data, uint32_t len) {
    uint32_t *length = (uint32_t *)shm_buf;
    char *text = shm_buf + sizeof(uint32_t);

    while (1) {
        sem_wait(sem);
        if (*length == 0) {

            if (len != UINT32_MAX) {
                memcpy(text, data, len);
            } 

            *length = len;
            sem_post(sem);
            return;
        }
        sem_post(sem);
    }
}

int main(void) {
    char shm_name[64], sem_name[64];

    snprintf(shm_name, sizeof(shm_name), "/lab-shm-%d", getppid());
    snprintf(sem_name, sizeof(sem_name), "/lab-sem-%d", getppid());

    int shm = shm_open(shm_name, O_RDWR, 0);
    if (shm == -1) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(1);
    }

    shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(1);
    }

    sem = sem_open(sem_name, O_RDWR);
    if (sem == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(1);
    }

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
                const char msg[] = "error: dividing by zero\n";
                send_msg(msg, sizeof(msg) - 1);
                send_msg(NULL, UINT32_MAX);
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
                send_msg(out, len);
            }
            cnt = 0;
            res = 0;
            sign = 1;
        }
    }

    send_msg(NULL, UINT32_MAX);
    _exit(0);
}
