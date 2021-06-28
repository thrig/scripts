/* buffork - buffer across a fork (bad) */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void) {
    pid_t pid;
    printf("buffered in %d\n", getpid());
    pid = fork();
    if (pid == -1) abort();
    printf("after in %d\n", getpid());
    exit(0);
}
