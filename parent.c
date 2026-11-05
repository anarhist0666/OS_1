#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

#define BUFFER_SIZE 4096
#define FILENAME_SIZE 256
#define WRITE_STR(fd, str) write(fd, str, sizeof(str) - 1)

ssize_t read_line(int fd, char *buffer, size_t max_capacity) {
    size_t pos = 0;
    char c;
    ssize_t n;
    
    while (pos < max_capacity - 1) {
        n = read(fd, &c, 1);
        if (n == -1) return -1;
        if (n == 0) {
            if (pos == 0) return 0;
            break;
        }
        buffer[pos++] = c;
        if (c == '\n') break;
    }
    buffer[pos] = '\0';
    return pos;
}

int write_all(int fd, const char *data, size_t size) {
    size_t written = 0;
    while (written < size) {
        ssize_t n = write(fd, data + written, size - written);
        if (n == -1) return -1;
        written += n;
    }
    return 0;
}

int main() {
    int pipe1[2], pipe2[2];
    pid_t child1_pid, child2_pid;
    char filename1[FILENAME_SIZE];
    char filename2[FILENAME_SIZE];
    char buffer[BUFFER_SIZE];
    ssize_t line_len;
    int line_number = 0;
    int status;
    
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);
    
    if (pipe(pipe1) == -1) {
        WRITE_STR(STDERR_FILENO, "Ошибка: не удалось создать pipe1\n");
        _exit(1);
    }
    
    if (pipe(pipe2) == -1) {
        close(pipe1[0]);
        close(pipe1[1]);
        WRITE_STR(STDERR_FILENO, "Ошибка: не удалось создать pipe2\n");
        _exit(1);
    }
    
    WRITE_STR(STDOUT_FILENO, "Введите имя файла для child1: ");
    line_len = read_line(STDIN_FILENO, filename1, FILENAME_SIZE);
    if (line_len <= 0) {
        WRITE_STR(STDERR_FILENO, "Ошибка: не удалось прочитать имя файла 1\n");
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        _exit(1);
    }
    
    if (filename1[line_len - 1] == '\n') filename1[line_len - 1] = '\0';
    
    WRITE_STR(STDOUT_FILENO, "Введите имя файла для child2: ");
    line_len = read_line(STDIN_FILENO, filename2, FILENAME_SIZE);
    if (line_len <= 0) {
        WRITE_STR(STDERR_FILENO, "Ошибка: не удалось прочитать имя файла 2\n");
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        _exit(1);
    }
    
    if (filename2[line_len - 1] == '\n') filename2[line_len - 1] = '\0';
    
    int fd1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd1 == -1) {
        WRITE_STR(STDERR_FILENO, "Ошибка: не удалось открыть файл 1\n");
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        _exit(1);
    }
    
    int fd2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd2 == -1) {
        WRITE_STR(STDERR_FILENO, "Ошибка: не удалось открыть файл 2\n");
        close(fd1);
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        _exit(1);
    }
    
    child1_pid = fork();
    if (child1_pid == -1) {
        WRITE_STR(STDERR_FILENO, "Ошибка: fork завершился неудачно\n");
        close(fd1); close(fd2);
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        _exit(1);
    }
    
    if (child1_pid == 0) {
        close(pipe1[1]); close(pipe2[0]); close(pipe2[1]);
        if (dup2(pipe1[0], STDIN_FILENO) == -1) _exit(1);
        close(pipe1[0]);
        if (dup2(fd1, STDOUT_FILENO) == -1) _exit(1);
        close(fd1); close(fd2);
        
        char *const argv[] = {"./child", NULL};
        execv("./child", argv);
        WRITE_STR(STDERR_FILENO, "Ошибка: exec ./child завершился неудачно\n");
        _exit(1);
    }
    
    child2_pid = fork();
    if (child2_pid == -1) {
        WRITE_STR(STDERR_FILENO, "Ошибка: fork завершился неудачно\n");
        close(fd1); close(fd2);
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        waitpid(child1_pid, NULL, 0);
        _exit(1);
    }
    
    if (child2_pid == 0) {
        close(pipe1[0]); close(pipe1[1]); close(pipe2[1]);
        if (dup2(pipe2[0], STDIN_FILENO) == -1) _exit(1);
        close(pipe2[0]);
        if (dup2(fd2, STDOUT_FILENO) == -1) _exit(1);
        close(fd1); close(fd2);
        
        char *const argv[] = {"./child", NULL};
        execv("./child", argv);
        WRITE_STR(STDERR_FILENO, "Ошибка: exec ./child завершился неудачно\n");
        _exit(1);
    }
    
    close(pipe1[0]); close(pipe2[0]);
    close(fd1); close(fd2);
    
    WRITE_STR(STDOUT_FILENO, "Введите строки (Ctrl+D для завершения):\n");
    
    while (1) {
        line_len = read_line(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (line_len == -1) break;
        if (line_len == 0) break;
        
        line_number++;
        
        if (line_number % 2 == 1) {
            if (write_all(pipe1[1], buffer, line_len) == -1) break;
        } else {
            if (write_all(pipe2[1], buffer, line_len) == -1) break;
        }
    }
    
    close(pipe1[1]);
    close(pipe2[1]);
    
    int child1_exit_status = 1;
    int child2_exit_status = 1;
    
    if (waitpid(child1_pid, &status, 0) != -1 && WIFEXITED(status)) {
        child1_exit_status = WEXITSTATUS(status);
    }
    
    if (waitpid(child2_pid, &status, 0) != -1 && WIFEXITED(status)) {
        child2_exit_status = WEXITSTATUS(status);
    }
    
    if (child1_exit_status == 0 && child2_exit_status == 0) {
        WRITE_STR(STDOUT_FILENO, "Родительский процесс успешно завершен\n");
        return 0;
    } else {
        WRITE_STR(STDERR_FILENO, "Родительский процесс завершен с ошибками\n");
        return 1;
    }
}