/* torc - terminal orchestration, perhaps for something like controlling
 * multiple gdb sessions in lock-step. uses the unsavory TIOCSTI ioctl
 * call, so thus requires root or otherwise suitable permissions to
 * operate (OpenBSD dropped support for TIOCSTI)
 *
 * be sure to have some way to exit this program, as it by design passes
 * most everything seen off to the controlled terminals */

#include <sys/ioctl.h>
#include <sys/stat.h>
#if defined(__FreeBSD__)
#include <sys/ttycom.h>
#endif

#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

#define READ_BUF_LEN 32

enum { TORC_TERM_NORM, TORC_TERM_RAW } Terminal_Mode = TORC_TERM_NORM;
struct termios Original_Termios;

const char *Flag_Outputfile;    /* -o file */
int Flag_Quiet;                 /* -q */

void cleanup(void);
void emit_help(void);
void handle_sig(int signo);
void raw_terminal(int fd);
void reset_term(int fd);

int main(int argc, char *argv[])
{
    char *device = NULL;
    char buf[READ_BUF_LEN], *mytty, **tty_list;
    int ch, *fd_list;
    int output_fd = 0;
    sigset_t blockthese;
    ssize_t readret, rri;
    unsigned int dead_terms, fd_list_len, fdi;

    while ((ch = getopt(argc, argv, "h?o:Pq")) != -1) {
        switch (ch) {
        case 'o':
            Flag_Outputfile = optarg;
            break;
        case 'P':
            warnx("PID %d", getpid());
            break;
        case 'q':
            Flag_Quiet = 1;
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

    if (isatty(STDIN_FILENO) == 0)
        errx(EX_USAGE, "standard input is not a terminal");
    if ((mytty = ttyname(STDIN_FILENO)) == NULL)
        errx(EX_IOERR, "could not get ttyname of stdin??");

    if ((fd_list = malloc(sizeof(int) * argc)) == NULL)
        err(EX_OSERR, "could not malloc() fd list");
    fd_list_len = 0;
    dead_terms = 0;

    tty_list = argv;

    while (*argv) {
        if (strchr(*argv, '/') == NULL) {
            if (asprintf(&device, "%s%s", _PATH_DEV, *argv) == -1)
                err(EX_OSERR, "could not asprintf() device path");
        } else {
            device = *argv;
        }
        if (strcmp(mytty, device) == 0)
            errx(EX_USAGE, "will not write to my tty: %s", mytty);
        if ((fd_list[fd_list_len++] = open(device, O_WRONLY)) == -1)
            err(EX_IOERR, "could not open '%s'", device);
        argv++;
    }

    if (Flag_Outputfile) {
        if ((output_fd =
             open(Flag_Outputfile, O_CREAT | O_WRONLY | O_APPEND, 0666)) < 0)
            err(EX_IOERR, "could not open '%s'", Flag_Outputfile);
    }

    sigemptyset(&blockthese);
    sigaddset(&blockthese, SIGALRM);
    sigaddset(&blockthese, SIGPIPE);
    sigaddset(&blockthese, SIGQUIT);
    sigaddset(&blockthese, SIGTSTP);
    sigaddset(&blockthese, SIGUSR1);
    sigaddset(&blockthese, SIGUSR2);
    sigprocmask(SIG_BLOCK, &blockthese, NULL);
    if (signal(SIGHUP, handle_sig) == SIG_ERR)
        err(EX_OSERR, "could not signal(SIGHUP)");
    if (signal(SIGINT, handle_sig) == SIG_ERR)
        err(EX_OSERR, "could not signal(SIGINT)");
    if (signal(SIGTERM, handle_sig) == SIG_ERR)
        err(EX_OSERR, "could not signal(SIGTERM)");

    atexit(cleanup);
    raw_terminal(STDIN_FILENO);

    while (1) {
        if ((readret = read(STDIN_FILENO, buf, (size_t) READ_BUF_LEN)) == -1) {
            err(EX_IOERR, "read failed");
        }
        if (readret == 0) {
            /* EOF on standard input, nothing more to do */
            exit(EXIT_SUCCESS);
        } else if (readret < 0) {
            errx(1, "unexpected negative return from read: %ld", readret);
        }
        for (rri = 0; rri < readret; rri++) {
            for (fdi = 0; fdi < fd_list_len; fdi++) {
                if (fd_list[fdi] != -1) {
                    if (ioctl(fd_list[fdi], TIOCSTI, buf + rri) == -1) {
                        if (!Flag_Quiet) {
                            warn("could not write to %s", tty_list[fdi]);
                            /* avoid dangling off end of error message */
                            if (write(STDOUT_FILENO, "\x1b[1G", 4) < 0)
                                err(EX_IOERR, "write to stdout failed");
                        }
                        close(fd_list[fdi]);
                        fd_list[fdi] = -1;
                        dead_terms++;
                    }
                }
            }
            /* modest controlling terminal display rectification */
            switch (buf[rri]) {
            case 127:
                /* http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
                 * cursor backward, erase to right */
                if (write(STDOUT_FILENO, "\x1b[D\x1b[K", 6) < 0)
                    err(EX_IOERR, "write to stdout failed");
                break;
            case 13:
                /* CR/NL kluge, may not be portable */
                if (write(STDOUT_FILENO, "\n", 1) < 0)
                    err(EX_IOERR, "write to stdout failed");
            default:
                if (write(STDOUT_FILENO, (buf + rri), 1) < 0)
                    err(EX_IOERR, "write to stdout failed");
            }
        }
        if (dead_terms == fd_list_len) {
            /* could false positive, in which case will need to check if
             * ioctl sets errno and then extra logic on that value... */
            if (!Flag_Quiet)
                warnx("no error-free terminals remain");
            exit(1);
        }
        if (output_fd != 0) {
            if (write(output_fd, buf, (size_t) readret) < 0)
                err(EX_IOERR, "write to output '%s' failed", Flag_Outputfile);
        }
    }

    exit(1);                    /* NOTREACHED */
}

void cleanup(void)
{
    reset_term(STDIN_FILENO);
    write(STDOUT_FILENO, "\n", 1);
}

void emit_help(void)
{
    fputs("Usage: torc [-o file] [-P] [-q] dev [dev2 ..]\n", stderr);
    exit(EX_USAGE);
}

void handle_sig(int signo)
{
    cleanup();
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void raw_terminal(int fd)
{
    struct termios terminfo;
    if (tcgetattr(fd, &terminfo) < 0)
        err(EX_OSERR, "could not tcgetattr()");
    Original_Termios = terminfo;
    cfmakeraw(&terminfo);
    if (tcsetattr(fd, TCSAFLUSH, &terminfo) < 0)
        err(EX_OSERR, "could not tcsetattr()");
    Terminal_Mode = TORC_TERM_RAW;
}

void reset_term(int fd)
{
    if (Terminal_Mode == TORC_TERM_NORM)
        return;
    if (tcsetattr(fd, TCSANOW, &Original_Termios) < 0)
        err(EX_OSERR, "could not reset terminal with tcsetattr()");
    Terminal_Mode = TORC_TERM_NORM;
}
