/* envdup - wrapper script to create duplicate environment entries */

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

int main(int argc, char *argv[]) {
    char **newenv;
    int envcount = 0;

    if (argc < 2) errx(64, "Usage: envdup command [args ..]");

    newenv = environ;
    while (*newenv++ != NULL) envcount++;

    newenv = malloc(sizeof(char *) * (envcount + 3));
    if (newenv == NULL) err(1, "malloc failed");
    if (envcount > 0)
        memcpy(newenv, environ, sizeof(char *) * envcount);
    newenv[envcount]     = "FOO=one";
    newenv[envcount + 1] = "FOO=two";
    newenv[envcount + 2] = NULL;

    environ = newenv;
    argv++;
    execvp(*argv, argv);
    err(1, "exec failed '%s'", *argv);
}
