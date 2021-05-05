// log-update - git post-commit or post-merge hook that writes the path
// of the repository to a file; this file is consumed by another script
// that push repositories elsewhere, and so forth. a minimum of work is
// done here to not unduly delay the commit command

#include <sys/file.h>
#include <sys/param.h>

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define UPDATE_FILE ".gupdates"

// NOTE +1 is to include the '\0' record separator in the output
char pwd[PATH_MAX + 1];

int main(void) {
    char *home, *update_file;
    int fd;
    sigset_t block;
    size_t len;
    ssize_t ret;

#ifdef __OpenBSD__
    if (pledge("cpath flock rpath stdio unveil wpath", NULL) == -1)
        err(1, "pledge failed");
#endif

    if ((home = getenv("HOME")) == NULL) err(EX_OSERR, "no HOME");
    if (asprintf(&update_file, "%s/%s", home, UPDATE_FILE) == -1)
        err(EX_OSERR, "asprintf failed");

#ifdef __OpenBSD__
    if (unveil(update_file, "cw") == -1) err(1, "unveil failed");
    if (unveil(NULL, NULL) == -1) err(1, "unveil failed");
#endif

    sigemptyset(&block);
    sigaddset(&block, SIGINT);
    if (sigprocmask(SIG_BLOCK, &block, NULL) == -1)
        err(EX_OSERR, "sigprocmask failed %d", block);

    if ((fd = open(update_file, O_APPEND | O_CREAT | O_EXLOCK | O_WRONLY,
                   0660)) == -1)
        err(EX_IOERR, "open failed '%s'", update_file);

    if (getcwd((char *) &pwd, PATH_MAX) == NULL) err(EX_IOERR, "getcwd failed");

    // NOTE +1 is to include the '\0' record separator in the output
    len = strnlen((const char *) &pwd, PATH_MAX) + 1;

    // NOTE may want to here remove $HOME from the repository path; this
    // may be necessary should $HOME change (and no forwarding symlink
    // is put into place to keep the old paths valid) between when this
    // command runs and subsequent code parses the log file

    ret = write(fd, pwd, len);
    if (ret == -1) {
        err(EX_IOERR, "write failed");
    } else if (ret != len) {
        err(EX_IOERR, "incomplete write?? want %lu got %ld", len, ret);
    }

    close(fd);
    exit(EXIT_SUCCESS);
}
