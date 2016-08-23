/*
 * Writes the command line arguments (or standard input) to a tty using
 * the TIOCSTI ioctl. This will typically require root access, the
 * setuid bit set on this binary (danger!), or on Linux the appropriate
 * capability granted. The bytes will be written as given, except on the
 * command line a space will be written between each argument, and a
 * final newline written if the -N option is set.
 *
 * Example usage:
 *
 *   echo echo hi | sudo ttywrite $(tty) -
 *   echo echo hi | sudo ttywrite $(tty)
 *   sudo ttywrite              $(tty) echo hi
 *   sudo ttywrite -N           $(tty) echo hi
 *   sudo ttywrite -N -d 100000 $(tty) echo hi
 *
 * Though more likely some other terminal besides the one running this
 * code should be the target...
 *
 * On Linux, see also 'uinput'. To automate things under X11, xdotool.
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#if defined(__DARWIN__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/ttycom.h>
#endif

#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

// https://github.com/thrig/goptfoo
#include <goptfoo.h>

void emit_help(void);
void tty_write(int fd, int argc, char *argv[]);

unsigned long Flag_Delay = 0;   // -d
bool Flag_Newline = false;      // -N

int main(int argc, char *argv[])
{
    int ch, fd;

    while ((ch = getopt(argc, argv, "d:h?N")) != -1) {
        switch (ch) {
        case 'd':
            /* KLUGE useconds_t is "unsigned int" on *BSD at time of
             * writing so assume that as max for usleep(3) */
            Flag_Delay = flagtoul(ch, optarg, 0UL, (unsigned long) UINT_MAX);
            break;
        case 'N':
            Flag_Newline = true;
            break;
        case 'h':
        case '?':
        default:
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1)
        emit_help();

    if ((fd = open(*argv, O_WRONLY)) < 0)
        err(EX_IOERR, "could not open '%s'", *argv);
    tty_write(fd, --argc, ++argv);
    close(fd);

    exit(EXIT_SUCCESS);
}

void emit_help(void)
{
    fprintf(stderr,
            "Usage: ttywrite [-d microseconds] [-N] dev [command...|-]\n");
    exit(EX_USAGE);
}

void tty_write(int fd, int argc, char *argv[])
{
    int c;

    if (argc == 0 || ((*argv)[0] == '-' && (*argv)[1] == '\0')) {
        while ((c = getchar()) != EOF) {
            ioctl(fd, TIOCSTI, &c);
            if (Flag_Delay)
                usleep(Flag_Delay);
        }
        if (ferror(stdin)) {
            err(EX_IOERR, "error reading from standard input");
        }
        return;
    }

    while (*argv) {
        while (**argv != '\0') {
            ioctl(fd, TIOCSTI, (*argv)++);
            if (Flag_Delay)
                usleep(Flag_Delay);
        }
        ioctl(fd, TIOCSTI, " ");
        argv++;
    }

    if (Flag_Newline)
        ioctl(fd, TIOCSTI, "\n");
}
