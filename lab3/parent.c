#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_SIZE 4096

void write_string(int fd, char *s) {
    size_t len = 0;
    while (s[len] != '\0') len++;
    write(fd, s, len);
}

int main() {
    int status;
    char filename[256];

    write_string(STDOUT_FILENO, "enter file's name (max 255 symbols): ");

    size_t b_read = 0;
    char ch;

    while(b_read < sizeof(filename) - 1) {
        int res = read(STDIN_FILENO, &ch, 1);
        if (res <= 0 || ch == '\n' || ch == '\r') break;
        filename[b_read++] = ch;
    }

    filename[b_read] = '\0';

    if (b_read == 0) {
        write_string(STDOUT_FILENO, "empty file");
        _exit(1);
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        write_string(STDOUT_FILENO, "can't open the file");
        _exit(1);
    }

    char shm_name[64], sem_name[64];
    snprintf(shm_name, sizeof(shm_name), "/lab-shm-%d", getpid());
    snprintf(sem_name, sizeof(sem_name), "/lab-sem-%d", getpid());

    int shm = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm == -1) {
        write_string(STDOUT_FILENO, "can't open shared memory");
        perror(shm_name);
        _exit(1);
    }

    if (ftruncate(shm, SHM_SIZE) == -1) {
        write_string(STDOUT_FILENO, "can't resize shared memory");
        _exit(1);
    }

    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        write_string(STDOUT_FILENO, "can't map shared memory");
        _exit(1);
    }

    sem_t *sem = sem_open(sem_name, O_RDWR | O_CREAT | O_EXCL, 0600, 1);
    if (sem == SEM_FAILED) {
        write_string(STDOUT_FILENO, "failed to create semaphore");
        _exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        write_string(STDOUT_FILENO, "failed to fork");
        _exit(1);
    }

    if (pid == 0) {
        dup2(file_fd, STDIN_FILENO);
        close(file_fd);

        execl("./child", "child", NULL);

        write_string(STDOUT_FILENO, "failed exec");
        _exit(1);
    }

    else {
        close(file_fd);

        uint32_t *length = (uint32_t *)shm_buf;
        char *text = shm_buf + sizeof(uint32_t);

        int running = 1;
        while (running) {
            sem_wait(sem);

            if (*length == UINT32_MAX) {
                running = 0;
            } else if (*length > 0) {
                write(STDOUT_FILENO, text, *length);
                *length = 0;
            }

            sem_post(sem);
        }

        wait(&status);

        sem_close(sem);
        sem_unlink(sem_name);
        munmap(shm_buf, SHM_SIZE);
        close(shm);
        shm_unlink(shm_name);
    }

    _exit(0);
}
