/* blocksig - blocks signals then replaces self with the specified
 * command. only SIGINT is masked by default */

#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

/* TODO might use _NSIG - 1 but that would need portability research */
#define MAX_SIG 32

bool Seen_Sig[MAX_SIG];

void emit_help(void);

int main(int argc, char *argv[])
{
    int advance = 0;
    int ch, sig_num;
    sigset_t block;

#ifdef __OpenBSD__
    if (pledge("exec stdio", NULL) == -1)
        err(1, "pledge failed");
#endif

    setlocale(LC_ALL, "C");

    /* C-c by default, unless -s flag */
    sigemptyset(&block);
    sigaddset(&block, SIGINT);

    while ((ch = getopt(argc, argv, "h?s:")) != -1) {
        switch (ch) {
        case 's':
            sigemptyset(&block);

            if (!isdigit(*optarg))
                errx(EX_DATAERR, "signals must be plain integers");

            while (sscanf(optarg, "%2d%n", &sig_num, &advance) == 1) {
                if (sig_num < 1)
                    errx(EX_DATAERR, "signal number must be positive integer");
                else if (sig_num > MAX_SIG)
                    errx(EX_DATAERR, "signal number exceeds max of %d",
                         MAX_SIG);

                if (!Seen_Sig[sig_num - 1]) {
                    sigaddset(&block, sig_num);
                    Seen_Sig[sig_num - 1] = true;
                }
                optarg += advance;

                if (*optarg == '\0')
                    break;

                if (!(*optarg == ' ' || *optarg == ','))
                    errx(EX_DATAERR,
                         "signals must be space or comma separated");

                optarg++;
                if (!isdigit(*optarg))
                    errx(EX_DATAERR, "signals must be plain integers");
            }
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

    if (sigprocmask(SIG_BLOCK, &block, NULL) == -1)
        err(EX_OSERR, "could not set sigprocmask of %d", block);

    execvp(*argv, argv);
    err(EX_OSERR, "could not exec %s", *argv);
}

void emit_help(void)
{
    fputs("Usage: blocksig [-s 'signum ..'] command [args ..]\n", stderr);
    exit(EX_USAGE);
}
