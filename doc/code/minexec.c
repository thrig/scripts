#include <err.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
    argv++;
    execvp(*argv, argv);
    err(1, "exec failed");
}
