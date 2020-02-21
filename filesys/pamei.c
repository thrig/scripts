/* pamei - run only one instance of something (via a filesystem cache) */

#include <sys/stat.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

enum { NOPE = -1, CHILD };

void emit_help(void);
/* some flavor of POSIX apparently has wordexp.h; that might be a better
 * option for OS that have that */
char *texpand(const char *s);

int main(int argc, char *argv[]) {
#ifdef __OpenBSD__
    if (pledge("cpath exec flock proc rpath stdio unveil wpath", NULL) == NOPE)
        err(EX_OSERR, "pledge failed");
#endif
    if (argc < 3) emit_help();

    char *lockdir = texpand(getenv("PAMEI_DIR"));
    struct stat sb;
    if (stat(lockdir, &sb) == NOPE)
        err(EX_IOERR, "could not stat '%s'", lockdir);
#ifdef __OpenBSD__
    if (pledge("cpath exec flock proc rpath stdio unveil wpath", NULL) == NOPE)
        err(EX_OSERR, "pledge failed");
    if (unveil("/", "rx") == NOPE) err(EX_OSERR, "unveil failed");
    if (unveil(lockdir, "crw") == NOPE) err(EX_OSERR, "unveil failed");
    if (unveil(NULL, NULL) == NOPE) err(EX_OSERR, "unveil failed");
#endif

    char *label = argv[1];
    size_t len  = strlen(label);
    for (int i = 0; i < len; i++)
        if (label[i] == '/') label[i] = '_';
    char *path;
    int ret = asprintf(&path, "%s/%s", lockdir, label);
    if (ret >= PATH_MAX) err(EX_IOERR, "path is too long");
    free(lockdir);

    // TODO may want to setsid and control various signals and try to
    // kill the child if the parent is whacked but that's probably lots
    // of effort for minor edge cases rare under the intended use

    const int fd = open(path, O_CREAT | O_RDONLY, 0666);
    if (fd == NOPE) err(EX_IOERR, "open failed '%s'", path);
    if (flock(fd, LOCK_EX | LOCK_NB) == NOPE) {
        if (errno == EWOULDBLOCK)
            errx(2, "resource is locked");
        else
            err(EX_IOERR, "flock failed");
    }

#ifdef __OpenBSD__
    if (pledge("exec proc rpath stdio", NULL) == NOPE)
        err(EX_OSERR, "pledge failed");
#endif
    const pid_t pid = fork();
    if (pid == NOPE) err(EX_OSERR, "fork failed");
    if (pid == CHILD) {
        close(fd);
        argv += 2;
        execvp(*argv, argv);
        err(EX_OSERR, "could not exec '%s'", *argv);
    }

    int status, exit_status = 0;
    if (waitpid(pid, &status, 0) == NOPE) err(EX_OSERR, "waitpid failed");
    if (status != EXIT_SUCCESS) exit_status = 1;
    // flock(fd, LOCK_UN);
    // close(fd);
    exit(exit_status);
}

void emit_help(void) {
    fputs("Usage: pamei label command [args ..]\n", stderr);
    exit(EX_USAGE);
}

char *texpand(const char *s) {
    if (s == NULL || s[0] == '\0') {
        return texpand("~/var/cache/pamei");
    } else if (s[0] == '~') {
        char *out;
        struct passwd *p;
        if (s[1] == '/' || s[1] == '\0') {
            char *home;
            if ((home = getenv("HOME")) == NULL) {
                if ((p = getpwuid(getuid())) == NULL)
                    err(EX_IOERR, "cannot find HOME directory");
                home = p->pw_dir;
            }
            asprintf(&out, "%s%s", home, &s[1]);
        } else {
            char *user = strdup(&s[1]);
            char *end  = strchr(user, '/');
            if (end != NULL) *end = '\0';
            if ((p = getpwnam(user)) == NULL)
                err(EX_OSERR, "cannot lookup user %s", user);
            free(user);
            asprintf(&out, "%s%s", p->pw_dir,
                     end == NULL ? "" : s + (end - user + 1));
        }
        return out;
    } else {
        return strdup(s);
    }
    /* NOTREACHED (I hope) */
}
