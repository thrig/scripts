#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
char buf[8];
int main(int argc, char *argv[]) {
    int ch, fd[2], rv;

    signal(SIGPIPE, SIG_IGN);
    pipe(fd);

    switch (fork()) {
    case -1: err(1, "could not fork");
    case 0:                             /* child */
        rv = lseek(STDIN_FILENO, -7, SEEK_END);
        if (rv == -1) err(1, "could not seek");
        sleep(3);
        close(fd[1]);
        break;
    default:                            /* parent */
        close(fd[1]);
        read(fd[0], &ch, 1);
        read(STDIN_FILENO, buf, 7);
        printf("read '%s'\n", buf);
    }
    return 0;
}
