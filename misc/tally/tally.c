/* tally - tallies up duplicate lines in input */

#include <getopt.h>

#include "redblack-bst.h"

void emit_help(void);
void read_lines(const char *file);

int Flag_Total; // -t

int main(int argc, char *argv[]) {
    int ch;

#ifdef __OpenBSD__
    if (pledge("rpath stdio", NULL) == -1) err(1, "pledge failed");
#endif

    while ((ch = getopt(argc, argv, "h?t")) != -1) {
        switch (ch) {
        case 't': Flag_Total = 1; break;
        case 'h':
        case '?':
        default:
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;
    if (argc == 0 || strncmp(*argv, "-", (size_t) 2) == 0) {
        read_lines(NULL);
    } else {
        while (*argv) {
            read_lines(*argv);
            argv++;
        }
    }
    rbbst_tally(Flag_Total);
    exit(EXIT_SUCCESS);
}

void emit_help(void) {
    fputs("Usage: tally [-t] [files|-]\n", stderr);
    exit(EX_USAGE);
}

void read_lines(const char *file) {
    FILE *fh;
    char *line        = NULL;
    size_t linebuflen = 0;
    ssize_t linelen;
    if (file) {
        if ((fh = fopen(file, "r")) == NULL)
            err(EX_IOERR, "could not open '%s'", file);
    } else {
        fh = stdin;
    }
    while ((linelen = getline(&line, &linebuflen, fh)) > 0)
        rbbst_add(line, linelen);
    if (file) fclose(fh);
}
