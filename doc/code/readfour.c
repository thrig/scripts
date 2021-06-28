#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
char buf[5];
int main(int argc, char *argv[]) {
    fgets(buf, 5, stdin);
    printf("%s >%s<\n", *argv, buf);

    /* avoid infinite loop of execs... */
    if (strncmp(*argv, "./replace", 10) == 0) return 0;

    /* replace self under a new name */
    execl("./readfour", "./replace", (char *) 0);
    abort();
}
