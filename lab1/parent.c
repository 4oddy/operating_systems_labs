#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void write_string(int fd, char *s) {
    size_t len = 0;
    while (s[len] != '\0') len++;
    write(fd, s, len);
}

int main() {
    int status;
    char filename[256];

    write_string(STDOUT_FILENO, "Введите имя файла (max 255 символов): ");

    size_t b_read = 0;
    char ch; 

    while(b_read < sizeof(filename) - 1) {
        int res = read(STDIN_FILENO, &ch, 1);
        if (res <= 0 || ch == '\n' || ch == '\r') break;
        filename[b_read++] = ch;
    }

    filename[b_read] = '\0';

    if (b_read == 0) {
        write_string(STDOUT_FILENO, "пустой файл");
        _exit(1);
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        write_string(STDOUT_FILENO, "не открывается файл");
        _exit(1);
    }

    int pipe_[2];
    if (pipe(pipe_) < 0) {
        write_string(STDOUT_FILENO, "не удалось создать pipe");
        close(file_fd);
        _exit(1);
    }

    pid_t pid = fork();
    
    if (pid < 0) {
        write_string(STDOUT_FILENO, "Не удалось сделать fork");
        close(file_fd);
        close(pipe_[0]);
        close(pipe_[1]);
        _exit(1);
    }

    if (pid == 0) {
        dup2(file_fd, STDIN_FILENO);
        dup2(pipe_[1], STDOUT_FILENO);

        close(file_fd);
        close(pipe_[1]);
        close(pipe_[0]);

        execl("./child", "child", NULL);

        write_string(STDOUT_FILENO, "Exec неуспешен");
        _exit(1);
    }

    else {
        close(file_fd);
        close(pipe_[1]);
        
        char buffer[512];
        size_t count ;

        while ((count = read(pipe_[0], buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, count);
        }

        close(pipe_[0]);
        wait(&status);
    }

    _exit(0);
}
