/* n-of - select N lines from standard input */

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <goptfoo.h>

void emit_help(void);

int main(int argc, char *argv[]) {
#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");
#endif
    if (argc != 2) emit_help();

    // arc4random() goes up to UINT32_MAX but set celing lower than that
    // as never expect to select so many items...
    size_t count = argtoul("count", argv[1], 1, INT32_MAX);

    char **samples;
    // TODO overflow risk on sizeof * math?
    if ((samples = malloc(count * sizeof(char *))) == NULL)
        err(EX_OSERR, "malloc failed");

    char *line        = NULL;
    size_t linebuflen = 0, linenum = 0;
    ssize_t numchars;
    while ((numchars = getline(&line, &linebuflen, stdin)) > 0) {
        if (linenum++ < count) {
            samples[linenum - 1] = strndup(line, numchars);
        } else {
            size_t i = arc4random_uniform(linenum);
            if (i < count) {
                free(samples[i]);
                samples[i] = strndup(line, numchars);
            }
        }
    }
    // some may not want this to be a non-zero exit? see how the script
    // gets used in practice to see if will be a problem or needs a flag
    if (linenum == 0) exit(1);

    if (linenum < count) count = linenum;
    for (size_t i = 0; i < count; i++)
        fputs(samples[i], stdout);
    // POSIX ultimate newline compliance
    size_t len = strlen(samples[count-1]);
    if (samples[count-1][len-1] != '\n') putchar('\n');

    exit(EXIT_SUCCESS);
}

void emit_help(void) {
    fputs("Usage: n-of count < ...\n", stderr);
    exit(EX_USAGE);
}
