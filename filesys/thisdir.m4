/*
divert(-1)
thisdir - wrapper for utilities that do something involving the CWD

define(`DEFAULT_TDIR', `"'esyscmd(`printf "%s" "$HOME"')`/libexec/thisdir"')dnl
divert(0)dnl
*/

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

void emit_help(void);

int main(int argc, char *argv[]) {
#ifdef __OpenBSD__
    if (pledge("exec rpath stdio", NULL) == -1) err(1, "pledge failed");
#endif

    int ch;
    while ((ch = getopt(argc, argv, "h?")) != -1) {
        switch (ch) {
        case 'h':
        case '?':
        default:
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1 || *argv[0] == '\0') emit_help();

    /* make mode name less likely to be something naughty */
    char *ap               = argv[0];
    int maybedir           = 1;
    unsigned long dotcount = 0;
    while (*ap != '\0') {
        switch (*ap) {
        case '.':
            if (maybedir) dotcount++;
            break;
        case '/':
            if (maybedir && dotcount == 2) {
                errx(1, "mode name must not contain ../");
            } else {
                dotcount = 0;
                maybedir = 1;
            }
            break;
        default: maybedir = 0;
        }
        ap++;
    }

    /* pass custom arguments down to the mode */
    char **targs;
    if ((targs = calloc(argc + 2, sizeof(char **))) == NULL)
        err(1, "malloc failed");
    for (int i = 1; i < argc; i++)
        targs[i + 1] = argv[i];

    /* mode name is the program name, CWD becomes first argument,
     * standardize ENV values */
    targs[0] = argv[0];
    if ((targs[1] = getcwd(NULL, 0)) == NULL) err(1, "getcwd failed");
    char *tdir;
    if ((tdir = getenv("THISDIR")) == NULL) {
        tdir = DEFAULT_TDIR;
        setenv("THISDIR", DEFAULT_TDIR, 1);
    }
    setenv("PWD", targs[1], 1);

    char *path;
    if (asprintf(&path, "%s/%s", tdir, targs[0]) < 0) err(1, "asprintf failed");
    execv(path, targs);
    err(1, "execv failed '%s'", path);

    /* NOTREACHED */
    exit(42);
}

void emit_help(void) {
    fputs("Usage: thisdir -- mode [mode-args]\n", stderr);
    exit(EX_USAGE);
}
