/*
 * Opens potentially a lot of file descriptors on the specified file. Handy for
 * who knows what purpose, perhaps checking how write speeds change with this
 * many FD then open on the file... (Note: assumes only in,out,err open when
 * launched.)
 *
 * See also fuser(1) or lsof(8) to help diagnose situations where many open
 * file descriptors are suspected.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

long Flag_FD_Max;               /* -f */

void emit_usage(void);

int main(int argc, char *argv[])
{
    int ch;
    char *ep;
    long i;

    while ((ch = getopt(argc, argv, "f:h?")) != -1) {
        switch (ch) {
        case 'f':
            errno = 0;
            Flag_FD_Max = strtol(optarg, &ep, 10);
            if (optarg[0] == '\0' || *ep != '\0')
                errx(EX_DATAERR, "could not parse -f count flag");
            if (errno == ERANGE
                && (Flag_FD_Max == LONG_MIN || Flag_FD_Max == LONG_MAX))
                errx(EX_DATAERR, "value for -f outside bounds for long type");
            /* in,out,err implicit, so 4 would be those plus one new */
            if (Flag_FD_Max < 4)
                errx(EX_DATAERR, "must open >3 file handles via -f count");
            break;

        case 'h':
        case '?':
        default:
            emit_usage();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
        emit_usage();

    if (Flag_FD_Max == 0)
        Flag_FD_Max = INT_MAX;

    if (Flag_FD_Max >= sysconf(_SC_OPEN_MAX))
        warnx("notice: fd count %ld in excess of _SC_OPEN_MAX %ld",
              Flag_FD_Max, sysconf(_SC_OPEN_MAX));

    /* start at three because of std{in,out,err} */
    for (i = 3; i < Flag_FD_Max; i++) {
        if (open(*argv, O_RDONLY) < 0) {
            warn("open() failed at %ld of %ld, ceasing opens on '%s'", i,
                 Flag_FD_Max, *argv);
            break;
        }
    }

    /* Presumably this is where file I/O or other things in other processes
     * are done, C-c or otherwise signal to abort this program. */
    while (1)
        sleep(UINT_MAX);

    exit(EXIT_SUCCESS);         /* NOTREACHED */
}

void emit_usage(void)
{
    fprintf(stderr, "Usage: nomfd [-f fdcount] file\n");
    exit(EX_USAGE);
}
