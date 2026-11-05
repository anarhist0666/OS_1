#include <unistd.h>

#define BUFFER_SIZE 4096

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

void invert_string(char *str, ssize_t len) {
    ssize_t str_len = len;
    if (str_len > 0 && str[str_len - 1] == '\n') {
        str_len--;
    }

    char temp[BUFFER_SIZE];
    ssize_t pos = 0;
    ssize_t i = str_len;

    while (i > 0) {
        ssize_t start = i - 1;
        while (start > 0 && ((unsigned char)str[start] & 0xC0) == 0x80) {
            start--;
        }
        for (ssize_t k = start; k < i; k++) {
            temp[pos++] = str[k];
        }
        i = start;
    }

    for (ssize_t k = 0; k < pos; k++) {
        str[k] = temp[k];
    }
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
    char buffer[BUFFER_SIZE];
    ssize_t line_len;
    
    while (1) {
        line_len = read_line(STDIN_FILENO, buffer, BUFFER_SIZE);
        
        if (line_len == -1) _exit(1);
        if (line_len == 0) break;
        
        invert_string(buffer, line_len);
        
        if (write_all(STDOUT_FILENO, buffer, line_len) == -1) {
            _exit(1);
        }
    }
    
    return 0;
}