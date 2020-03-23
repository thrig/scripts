// n-of - select N lines from standard input

#include <err.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <goptfoo.h>

int Flag_Shuffle; // -s

void emit_help(void);

int main(int argc, char *argv[]) {
#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");
#endif

    int ch;
    while ((ch = getopt(argc, argv, "h?s")) != -1) {
        switch (ch) {
        case 's': Flag_Shuffle = 1; break;
        case 'h':
        case '?':
        default: emit_help();
        }
    }
    argc -= optind;
    argv += optind;
    if (argc != 1) emit_help();

    // arc4random() goes up to UINT32_MAX but set celing lower than that
    // as never expect to select so many items...
    size_t count = argtoul("count", argv[0], 1, INT32_MAX);

    char **samples;
    if ((samples = malloc(sizeof(char *) * count)) == NULL)
        err(EX_OSERR, "malloc failed");

    char *line        = NULL;
    size_t linebuflen = 0, linenum = 0, fin;
    ssize_t numchars;
    while ((numchars = getline(&line, &linebuflen, stdin)) > 0) {
        if (linenum++ < count) {
            fin          = linenum - 1;
            samples[fin] = strndup(line, numchars);
        } else {
            size_t i = arc4random_uniform(linenum);
            if (i < count) {
                fin = i;
                free(samples[fin]);
                samples[fin] = strndup(line, numchars);
            }
        }
    }
    // some may not want this to be a non-zero exit? see how the script
    // gets used in practice to see if will be a problem or needs a flag
    if (linenum == 0) exit(1);

    if (linenum < count) count = linenum;

    // POSIX ultimate newline compliance (also for when the last line
    // gets assigned to some other slot)
    size_t len = strlen(samples[fin]);
    if (samples[fin][len - 1] != '\n') {
        char *new;
        asprintf(&new, "%s\n", samples[fin]);
        free(samples[fin]);
        samples[fin] = new;
    }

    if (count > 1 && Flag_Shuffle) {
        char *tmp;
        for (size_t i = count; --i;) {
            size_t j = arc4random_uniform(i + 1);
            if (i != j) {
                tmp        = samples[i];
                samples[i] = samples[j];
                samples[j] = tmp;
            }
        }
    }
    for (size_t i = 0; i < count; i++)
        fputs(samples[i], stdout);

    // could free the samples but we're about to
    exit(EXIT_SUCCESS);
}

void emit_help(void) {
    fputs("Usage: n-of count < ...\n", stderr);
    exit(EX_USAGE);
}
