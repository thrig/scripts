/*
 * Snips any ultimate linefeed chars (\r and \n) from the named files.
 *
 * This is mostly just silly programming practice; files on Unix often
 * need that ultimate newline, as otherwise shell 'while' loops might
 * loose that last line, and so forth. (Hence certain editors warning if
 * the file lacks a trailing newline.)
 */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

void trim_file(const int fd, const char *file);
void usage(void);

int main(int argc, char *argv[])
{
    int ch, fd;

    while ((ch = getopt(argc, argv, "h?")) != -1) {
        switch (ch) {
        case 'h':
        case '?':
            usage();
            /* NOTREACHED */
        default:
            ;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1) {
        usage();
        /* NOTREACHED */
    }

    while (*argv) {
        if ((fd = open(*argv, O_RDWR)) == -1)
            err(EX_IOERR, "could not open '%s'", *argv);
        trim_file(fd, *argv);
        if (close(fd) == -1)
            err(EX_IOERR, "could not close '%s'", *argv);
        argv++;
    }

    exit(EXIT_SUCCESS);
}

void trim_file(const int fd, const char *file)
{
    int only_newlines;
    long int start_size, trunc_size, i;
    char buf[2];

    if ((start_size = lseek(fd, 0, SEEK_END)) < 0)
        err(EX_IOERR, "could not seek '%s'", file);
    if (start_size == 0)
        return;                 /* empty file, whatevs */
    trunc_size = start_size;
    only_newlines = 1;

    for (i = 1; i <= start_size; i++) {
        /*
         * Gist of logic is to step backwards through the file, ending
         * when a non-newline character is detected, and then truncating
         * the file to that point. This will be quick for normal files,
         * but less so for pathological cases of large files consisting
         * entirely of newlines.
         */
        if (lseek(fd, i * -1, SEEK_END) < 0)
            err(EX_IOERR, "could not seek '%s'", file);
        if (read(fd, &buf, 1) < 0)
            err(EX_IOERR, "could not read '%s'", file);

        switch (buf[0]) {
        case '\n':
        case '\r':
            break;
        default:
            only_newlines = 0;
            trunc_size = lseek(fd, 0, SEEK_CUR);
            i = start_size;
        }
    }

    /*
     * Edge case: a file with only one or more newlines will not cause
     * trunc_size to be updated, and thus would not be truncated.
     */
    if (only_newlines == 1)
        trunc_size = 0;

    /*
     * Another edge case is if the file is changed between the "find the
     * offset code" and this here truncate, but that gets into locking,
     * or more properly the file workflow and how to avoid locking.
     */
    if (trunc_size < start_size)
        if (ftruncate(fd, trunc_size) != 0)
            err(EX_IOERR, "could not truncate '%s'", file);
}

void usage(void)
{
    errx(EX_USAGE, "need files to snip ultimate newlines from");
}
