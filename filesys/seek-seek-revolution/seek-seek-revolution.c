#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

enum { NOPE = -1, CHILD = 0, FILE_START = 0 };

struct timespec naptime = {0, 640000000};

void dance(int fd);

int main(int argc, char *argv[]) {
    int block[2], fd;
    pid_t pid;

    if (argc != 2) fputs("Usage: seek-seek-revolution writable-file\n", stderr);

    if ((fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0600)) == NOPE)
        err(1, "could not write %s", argv[1]);
    pipe(block);
    pid = fork();
    if (pid == NOPE) {
        err(1, "fork failed");
    } else if (pid == CHILD) {
        char ch;
        close(block[1]);
        close(STDIN_FILENO);
        dup(fd);
        read(block[0], &ch, 1);
        close(block[0]);
        execl("/bin/sh", "sh", (char *) 0);
    } else {
        dance(fd);
        close(block[0]);
        write(block[1], "g", (size_t) 1);
        close(block[1]);
        while (1) {
            lseek(fd, FILE_START, SEEK_SET);
            dance(fd);
            nanosleep(&naptime, NULL);
        }
    }
}

inline void dance(int fd) {
    static int x = 0;
    dprintf(fd, "printf '\\r%s'\nsleep 1\n", x++ & 1 ? "\\\\\\\\o" : "o//");
    lseek(fd, FILE_START, SEEK_SET);
}
