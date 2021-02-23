/* is-mounted - list mount points or check if a directory is a mount point */

#include <sys/types.h>

#include <sys/mount.h>
#include <sys/param.h>
#include <sys/ucred.h>

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

void emit_help(void);

int Flag_Mntcheck;
int Flag_Nullsep;
int Flag_Quiet;

char Sep = '\n';

// see getfsstat(2)
int Stat_Flag = MNT_WAIT;

int main(int argc, char *argv[]) {
#ifdef __OpenBSD__
    if (pledge("rpath stdio", NULL) == -1) err(1, "pledge failed");
#endif

    int ch;
    while ((ch = getopt(argc, argv, "h?0Nmq")) != -1) {
        switch (ch) {
        case '0': Sep = '\0'; break;
        case 'N': Stat_Flag = MNT_NOWAIT; break;
        case 'm': Flag_Mntcheck = 1; break;
        case 'q': Flag_Quiet = 1; break;
        case 'h':
        case '?':
        default:
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;
    if (argc > 1) emit_help();
    if (Flag_Mntcheck && argc == 0) emit_help();

    int exit_status = 0;
    if (argc == 1) {
        if (**argv == '\0') emit_help();
        exit_status = 1;
    }

    int count;
    struct statfs *mntbufp;
    if ((count = getmntinfo(&mntbufp, Stat_Flag)) < 0)
        err(1, "getmntinfo failed");
    if (count == 0) errx(1, "no file systems mounted??");

    for (int i = 0; i < count; i++) {
        if (argc == 1) {
            if (strncmp(*argv, mntbufp[i].f_mntonname, MAXPATHLEN) == 0) {
                if (!Flag_Quiet)
                    printf("%.*s%c", MAXPATHLEN, mntbufp[i].f_mntonname, Sep);
                exit_status = 0;
                break;
            }
        } else {
            printf("%.*s%c", MAXPATHLEN, mntbufp[i].f_mntonname, Sep);
        }
    }

    exit(exit_status);
}

void emit_help(void) {
    fputs("Usage: is-mounted [-0Nmq] [mount-point]\n", stderr);
    exit(EX_USAGE);
}
